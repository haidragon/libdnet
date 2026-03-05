/*
 * packet_builder.c - 数据包构建工具
 *
 * 基于 libdnet 的 macOS 数据包构建示例
 * 演示如何构建和发送 TCP SYN、ICMP Echo 等数据包
 *
 * 编译: gcc -o packet_builder packet_builder.c -I../include -L../.libs -ldnet
 * 运行: sudo ./packet_builder
 *
 * 注意: 需要管理员权限才能发送原始数据包
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dnet.h>
#include <dnet/eth.h>
#include <dnet/ip.h>
#include <dnet/tcp.h>
#include <dnet/udp.h>
#include <dnet/icmp.h>
#include <dnet/arp.h>

/* 打印十六进制数据 */
static void print_hex(const uint8_t *data, size_t len, const char *prefix)
{
    printf("%s", prefix);
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0 && i > 0)
            printf("\n%s", prefix);
        printf("%02x ", data[i]);
        if (i % 8 == 7)
            printf(" ");
    }
    printf("\n");
}

/* 构建 ICMP Echo Request 数据包 */
static void build_icmp_ping(uint8_t *packet, size_t *packet_len,
                           ip_addr_t src_ip, ip_addr_t dst_ip,
                           uint16_t icmp_id, uint16_t icmp_seq,
                           const char *data, size_t data_len)
{
    size_t icmp_len = ICMP_HDR_LEN + 2 + data_len;  // header + id/seq + data
    size_t total_len = IP_HDR_LEN + icmp_len;

    /* 构建 IP 头部 */
    ip_pack_hdr(packet,
                IP_TOS_DEFAULT,
                total_len,
                1234,            // IP ID
                0,               // 分片偏移
                64,              // TTL
                IP_PROTO_ICMP,   // 协议
                src_ip,
                dst_ip);

    /* 构建 ICMP 头部 */
    struct icmp_hdr *icmp = (struct icmp_hdr *)(packet + IP_HDR_LEN);
    icmp_pack_hdr_echo(icmp,
                       ICMP_ECHO,
                       ICMP_CODE_NONE,
                       icmp_id,
                       icmp_seq,
                       data,
                       data_len);

    /* 计算 IP 校验和 */
    ip_checksum(packet, IP_HDR_LEN, 0);

    /* 计算 ICMP 校验和 */
    icmp_checksum(icmp, icmp_len);

    *packet_len = total_len;
}

/* 构建 TCP SYN 数据包 */
static void build_tcp_syn(uint8_t *packet, size_t *packet_len,
                          ip_addr_t src_ip, uint16_t src_port,
                          ip_addr_t dst_ip, uint16_t dst_port,
                          uint32_t seq, uint32_t ack,
                          uint8_t flags, uint16_t win)
{
    size_t total_len = IP_HDR_LEN + TCP_HDR_LEN;

    /* 构建 IP 头部 */
    ip_pack_hdr(packet,
                IP_TOS_DEFAULT,
                total_len,
                1234,
                0,
                64,
                IP_PROTO_TCP,
                src_ip,
                dst_ip);

    /* 构建 TCP 头部 */
    tcp_pack_hdr(packet + IP_HDR_LEN,
                 src_port,
                 dst_port,
                 seq,
                 ack,
                 flags,
                 win,
                 0);  // 紧急指针

    struct ip_hdr *ip = (struct ip_hdr *)packet;
    struct tcp_hdr *tcp = (struct tcp_hdr *)(packet + IP_HDR_LEN);

    /* 计算 IP 校验和 */
    ip_checksum(packet, IP_HDR_LEN, 0);

    /* 计算 TCP 校验和 */
    tcp_checksum(ip, tcp, TCP_HDR_LEN);

    *packet_len = total_len;
}

/* 构建 UDP 数据包 */
static void build_udp_packet(uint8_t *packet, size_t *packet_len,
                            ip_addr_t src_ip, uint16_t src_port,
                            ip_addr_t dst_ip, uint16_t dst_port,
                            const char *data, size_t data_len)
{
    size_t udp_len = UDP_HDR_LEN + data_len;
    size_t total_len = IP_HDR_LEN + udp_len;

    /* 构建 IP 头部 */
    ip_pack_hdr(packet,
                IP_TOS_DEFAULT,
                total_len,
                1234,
                0,
                64,
                IP_PROTO_UDP,
                src_ip,
                dst_ip);

    /* 构建 UDP 头部 */
    struct udp_hdr *udp = (struct udp_hdr *)(packet + IP_HDR_LEN);
    udp_pack_hdr(udp, src_port, dst_port, udp_len);

    /* 复制数据 */
    if (data_len > 0) {
        memcpy(packet + IP_HDR_LEN + UDP_HDR_LEN, data, data_len);
    }

    /* 计算 IP 校验和 */
    ip_checksum(packet, IP_HDR_LEN, 0);

    *packet_len = total_len;
}

/* 构建 ARP 请求数据包 */
static void build_arp_request(uint8_t *packet, size_t *packet_len,
                             eth_addr_t src_mac, ip_addr_t src_ip,
                             ip_addr_t dst_ip)
{
    eth_addr_t broadcast_mac;
    eth_pton(ETH_ADDR_BROADCAST, &broadcast_mac);

    /* 构建以太网头部 */
    eth_pack_hdr(packet, broadcast_mac, src_mac, ETH_TYPE_ARP);

    /* 构建 ARP 头部 */
    arp_pack_hdr_ethip(packet + ETH_HDR_LEN,
                       ARP_OP_REQUEST,
                       src_mac,
                       src_ip,
                       broadcast_mac,
                       dst_ip);

    *packet_len = ETH_HDR_LEN + ARP_HDR_LEN + ARP_ETHIP_LEN;
}

/* 构建 ARP 响应数据包 */
static void build_arp_reply(uint8_t *packet, size_t *packet_len,
                            eth_addr_t src_mac, ip_addr_t src_ip,
                            eth_addr_t dst_mac, ip_addr_t dst_ip)
{
    /* 构建以太网头部 */
    eth_pack_hdr(packet, dst_mac, src_mac, ETH_TYPE_ARP);

    /* 构建 ARP 头部 */
    arp_pack_hdr_ethip(packet + ETH_HDR_LEN,
                       ARP_OP_REPLY,
                       src_mac,
                       src_ip,
                       dst_mac,
                       dst_ip);

    *packet_len = ETH_HDR_LEN + ARP_HDR_LEN + ARP_ETHIP_LEN;
}

/* 构建 TCP SYN/ACK 数据包 */
static void build_tcp_syn_ack(uint8_t *packet, size_t *packet_len,
                              ip_addr_t src_ip, uint16_t src_port,
                              ip_addr_t dst_ip, uint16_t dst_port,
                              uint32_t seq, uint32_t ack)
{
    build_tcp_syn(packet, packet_len,
                  src_ip, src_port,
                  dst_ip, dst_port,
                  seq, ack,
                  TH_SYN | TH_ACK,
                  65535);
}

/* 构建 TCP RST 数据包 */
static void build_tcp_rst(uint8_t *packet, size_t *packet_len,
                         ip_addr_t src_ip, uint16_t src_port,
                         ip_addr_t dst_ip, uint16_t dst_port,
                         uint32_t seq, uint32_t ack)
{
    build_tcp_syn(packet, packet_len,
                  src_ip, src_port,
                  dst_ip, dst_port,
                  seq, ack,
                  TH_RST | TH_ACK,
                  65535);
}

/* 演示 ICMP Ping 数据包构建 */
static void demo_icmp_ping(void)
{
    printf("\n========== ICMP Echo Request 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    ip_addr_t src_ip = htonl(0x0a000001);  // 10.0.0.1
    ip_addr_t dst_ip = htonl(0x0a000002);  // 10.0.0.2

    build_icmp_ping(packet, &packet_len,
                    src_ip, dst_ip,
                    1, 0, "hello", 5);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_str[16], dst_str[16];
    ip_ntop(&src_ip, src_str, sizeof(src_str));
    ip_ntop(&dst_ip, dst_str, sizeof(dst_str));

    printf("\n数据包结构:\n");
    printf("  IP: %s → %s\n", src_str, dst_str);
    printf("  ICMP Echo Request (id=1, seq=0)\n");
    printf("  数据: \"hello\"\n");
}

/* 演示 TCP SYN 数据包构建 */
static void demo_tcp_syn(void)
{
    printf("\n========== TCP SYN 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    ip_addr_t src_ip = htonl(0x0a000001);  // 10.0.0.1
    ip_addr_t dst_ip = htonl(0x0a000002);  // 10.0.0.2
    uint16_t src_port = 12345;
    uint16_t dst_port = 80;

    build_tcp_syn(packet, &packet_len,
                  src_ip, src_port,
                  dst_ip, dst_port,
                  1000, 0,  // seq=1000, ack=0
                  TH_SYN,
                  65535);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_str[16], dst_str[16];
    ip_ntop(&src_ip, src_str, sizeof(src_str));
    ip_ntop(&dst_ip, dst_str, sizeof(dst_str));

    printf("\n数据包结构:\n");
    printf("  IP: %s → %s\n", src_str, dst_str);
    printf("  TCP: %s:%u → %s:%u\n", src_str, src_port, dst_str, dst_port);
    printf("  Flags: SYN\n");
    printf("  Seq: 1000, Ack: 0, Window: 65535\n");
}

/* 演示 TCP SYN/ACK 数据包构建 */
static void demo_tcp_syn_ack(void)
{
    printf("\n========== TCP SYN/ACK 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    ip_addr_t src_ip = htonl(0x0a000002);  // 10.0.0.2
    ip_addr_t dst_ip = htonl(0x0a000001);  // 10.0.0.1
    uint16_t src_port = 80;
    uint16_t dst_port = 12345;

    build_tcp_syn_ack(packet, &packet_len,
                      src_ip, src_port,
                      dst_ip, dst_port,
                      2000, 1001);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_str[16], dst_str[16];
    ip_ntop(&src_ip, src_str, sizeof(src_str));
    ip_ntop(&dst_ip, dst_str, sizeof(dst_str));

    printf("\n数据包结构:\n");
    printf("  IP: %s → %s\n", src_str, dst_str);
    printf("  TCP: %s:%u → %s:%u\n", src_str, src_port, dst_str, dst_port);
    printf("  Flags: SYN | ACK\n");
    printf("  Seq: 2000, Ack: 1001, Window: 65535\n");
}

/* 演示 TCP RST 数据包构建 */
static void demo_tcp_rst(void)
{
    printf("\n========== TCP RST 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    ip_addr_t src_ip = htonl(0x0a000002);  // 10.0.0.2
    ip_addr_t dst_ip = htonl(0x0a000001);  // 10.0.0.1
    uint16_t src_port = 80;
    uint16_t dst_port = 12345;

    build_tcp_rst(packet, &packet_len,
                 src_ip, src_port,
                 dst_ip, dst_port,
                 2000, 1001);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_str[16], dst_str[16];
    ip_ntop(&src_ip, src_str, sizeof(src_str));
    ip_ntop(&dst_ip, dst_str, sizeof(dst_str));

    printf("\n数据包结构:\n");
    printf("  IP: %s → %s\n", src_str, dst_str);
    printf("  TCP: %s:%u → %s:%u\n", src_str, src_port, dst_str, dst_port);
    printf("  Flags: RST | ACK\n");
    printf("  Seq: 2000, Ack: 1001, Window: 65535\n");
}

/* 演示 UDP 数据包构建 */
static void demo_udp(void)
{
    printf("\n========== UDP 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    ip_addr_t src_ip = htonl(0x0a000001);  // 10.0.0.1
    ip_addr_t dst_ip = htonl(0x0a000002);  // 10.0.0.2
    uint16_t src_port = 54321;
    uint16_t dst_port = 53;  // DNS

    build_udp_packet(packet, &packet_len,
                     src_ip, src_port,
                     dst_ip, dst_port,
                     "test", 4);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_str[16], dst_str[16];
    ip_ntop(&src_ip, src_str, sizeof(src_str));
    ip_ntop(&dst_ip, dst_str, sizeof(dst_str));

    printf("\n数据包结构:\n");
    printf("  IP: %s → %s\n", src_str, dst_str);
    printf("  UDP: %s:%u → %s:%u\n", src_str, src_port, dst_str, dst_port);
    printf("  数据: \"test\"\n");
}

/* 演示 ARP 请求数据包构建 */
static void demo_arp_request(void)
{
    printf("\n========== ARP Request 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    eth_addr_t src_mac;
    ip_addr_t src_ip = htonl(0x0a000001);  // 10.0.0.1
    ip_addr_t dst_ip = htonl(0x0a000002);  // 10.0.0.2

    eth_pton("00:11:22:33:44:55", &src_mac);

    build_arp_request(packet, &packet_len, src_mac, src_ip, dst_ip);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_ip_str[16], dst_ip_str[16];
    ip_ntop(&src_ip, src_ip_str, sizeof(src_ip_str));
    ip_ntop(&dst_ip, dst_ip_str, sizeof(dst_ip_str));
    char src_mac_str[18];
    eth_ntop(&src_mac, src_mac_str, sizeof(src_mac_str));

    printf("\n数据包结构:\n");
    printf("  Ethernet: %s → ff:ff:ff:ff:ff:ff (Broadcast)\n", src_mac_str);
    printf("  ARP: Who has %s? Tell %s\n", dst_ip_str, src_ip_str);
}

/* 演示 ARP 响应数据包构建 */
static void demo_arp_reply(void)
{
    printf("\n========== ARP Reply 数据包构建 ==========\n");

    uint8_t packet[128];
    size_t packet_len;

    eth_addr_t src_mac, dst_mac;
    ip_addr_t src_ip = htonl(0x0a000002);  // 10.0.0.2
    ip_addr_t dst_ip = htonl(0x0a000001);  // 10.0.0.1

    eth_pton("00:11:22:33:44:55", &src_mac);
    eth_pton("aa:bb:cc:dd:ee:ff", &dst_mac);

    build_arp_reply(packet, &packet_len, src_mac, src_ip, dst_mac, dst_ip);

    printf("数据包长度: %zu 字节\n", packet_len);
    print_hex(packet, packet_len, "  ");

    char src_ip_str[16], dst_ip_str[16];
    ip_ntop(&src_ip, src_ip_str, sizeof(src_ip_str));
    ip_ntop(&dst_ip, dst_ip_str, sizeof(dst_ip_str));
    char src_mac_str[18], dst_mac_str[18];
    eth_ntop(&src_mac, src_mac_str, sizeof(src_mac_str));
    eth_ntop(&dst_mac, dst_mac_str, sizeof(dst_mac_str));

    printf("\n数据包结构:\n");
    printf("  Ethernet: %s → %s\n", src_mac_str, dst_mac_str);
    printf("  ARP: %s is at %s\n", src_ip_str, src_mac_str);
}

/* 发送 IP 数据包 */
static void send_ip_packet(const uint8_t *packet, size_t len)
{
    ip_t *i = ip_open();
    if (i == NULL) {
        perror("ip_open");
        fprintf(stderr, "提示: 需要 sudo 权限才能发送原始数据包\n");
        return;
    }

    printf("\n发送 IP 数据包...\n");
    ssize_t sent = ip_send(i, packet, len);
    if (sent < 0) {
        perror("ip_send");
    } else {
        printf("成功发送 %zd 字节\n", sent);
    }

    ip_close(i);
}

/* 演示发送数据包 */
static void demo_send_packets(void)
{
    printf("\n========== 发送数据包演示 ==========\n");
    printf("注意: 实际发送需要有效的网络配置\n");

    uint8_t packet[128];
    size_t packet_len;

    /* 发送 ICMP Ping */
    ip_addr_t src_ip = htonl(0x7f000001);  // 127.0.0.1
    ip_addr_t dst_ip = htonl(0x7f000001);  // 127.0.0.1

    build_icmp_ping(packet, &packet_len,
                    src_ip, dst_ip,
                    1, 0, "hello", 5);

    printf("\n发送 ICMP Echo 到 localhost...");
    send_ip_packet(packet, packet_len);
}

int main(int argc, char *argv[])
{
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     libdnet 数据包构建工具 (macOS 示例)                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    /* ========== ICMP 数据包演示 ========== */
    demo_icmp_ping();

    /* ========== TCP 数据包演示 ========== */
    demo_tcp_syn();
    demo_tcp_syn_ack();
    demo_tcp_rst();

    /* ========== UDP 数据包演示 ========== */
    demo_udp();

    /* ========== ARP 数据包演示 ========== */
    demo_arp_request();
    demo_arp_reply();

    /* ========== 发送数据包演示 ========== */
    demo_send_packets();

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     数据包构建演示完成                                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    return 0;
}
