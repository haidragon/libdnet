/**
 * libdnet TCP 数据包操作示例 - test5
 *
 * 功能：
 * 1. TCP 数据包构建
 * 2. TCP 头部解析
 * 3. TCP 标志位处理
 * 4. TCP 校验和计算
 * 5. TCP 选项处理
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>
#include <dnet/ip.h>
#include <dnet/tcp.h>

/* TCP 标志 */
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20

/* 演示构建 TCP 数据包 */
void demo_tcp_pack(void) {
    u_char ip_packet[IP_HDR_LEN + TCP_HDR_LEN + 32];
    u_char *tcp_packet = ip_packet + IP_HDR_LEN;

    printf("\n=== TCP 数据包构建 ===\n");

    /* 先构建 IP 头部 */
    ip_pack_hdr(ip_packet, 0, IP_HDR_LEN + TCP_HDR_LEN + 32,
                12345, 0, 64, IP_PROTO_TCP,
                inet_addr("192.168.1.100"), inet_addr("10.0.0.1"));

    /* 构建 TCP 头部 */
    tcp_pack_hdr(tcp_packet,
                12345,          /* 源端口 */
                80,             /* 目标端口 */
                0x10000000,     /* 序列号 */
                0,              /* 确认号 */
                TH_SYN,         /* 标志: SYN */
                8192,           /* 窗口大小 */
                0);             /* 紧急指针 */

    printf("构建 TCP 数据包 (SYN):\n");
    printf("  源端口: %d\n", 12345);
    printf("  目标端口: %d\n", 80);
    printf("  序列号: %u\n", 0x10000000);
    printf("  标志: SYN\n");
    printf("  窗口大小: %d\n", 8192);
}

/* 演示 TCP 标志位 */
void demo_tcp_flags(void) {
    printf("\n=== TCP 标志位 ===\n");

    printf("TCP 标志说明:\n");
    printf("  FIN (0x01) - 结束连接\n");
    printf("  SYN (0x02) - 同步序列号\n");
    printf("  RST (0x04) - 重置连接\n");
    printf("  PSH (0x08) - 推送数据\n");
    printf("  ACK (0x10) - 确认号有效\n");
    printf("  URG (0x20) - 紧急指针有效\n");

    printf("\n常见组合:\n");
    printf("  SYN     - 连接请求\n");
    printf("  SYN+ACK - 连接接受\n");
    printf("  ACK     - 确认\n");
    printf("  FIN+ACK - 主动关闭\n");
    printf("  RST     - 重置\n");
}

/* 演示 TCP 三次握手 */
void demo_tcp_handshake(void) {
    printf("\n=== TCP 三次握手 ===\n");

    printf("1. 客户端 → 服务器: SYN\n");
    printf("   序列号: 1000\n");
    printf("   标志: SYN\n");

    printf("\n2. 服务器 → 客户端: SYN+ACK\n");
    printf("   序列号: 5000\n");
    printf("   确认号: 1001\n");
    printf("   标志: SYN+ACK\n");

    printf("\n3. 客户端 → 服务器: ACK\n");
    printf("   序列号: 1001\n");
    printf("   确认号: 5001\n");
    printf("   标志: ACK\n");

    printf("\n连接建立！\n");
}

/* 演示 TCP 四次挥手 */
void demo_tcp_teardown(void) {
    printf("\n=== TCP 四次挥手 ===\n");

    printf("1. 客户端 → 服务器: FIN+ACK\n");
    printf("   序列号: 2000\n");
    printf("   标志: FIN+ACK\n");

    printf("\n2. 服务器 → 客户端: ACK\n");
    printf("   序列号: 6000\n");
    printf("   确认号: 2001\n");
    printf("   标志: ACK\n");

    printf("\n3. 服务器 → 客户端: FIN+ACK\n");
    printf("   序列号: 6001\n");
    printf("   标志: FIN+ACK\n");

    printf("\n4. 客户端 → 服务器: ACK\n");
    printf("   序列号: 2001\n");
    printf("   确认号: 6002\n");
    printf("   标志: ACK\n");

    printf("\n连接关闭！\n");
}

/* 演示常见端口 */
void demo_tcp_ports(void) {
    printf("\n=== 常见 TCP 端口 ===\n");

    printf("知名端口:\n");
    printf("  21  - FTP\n");
    printf("  22  - SSH\n");
    printf("  23  - Telnet\n");
    printf("  25  - SMTP\n");
    printf("  53  - DNS\n");
    printf("  80  - HTTP\n");
    printf("  110 - POP3\n");
    printf("  143 - IMAP\n");
    printf("  443 - HTTPS\n");
    printf("  3389 - RDP\n");
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("libdnet TCP 数据包操作示例\n");
    printf("======================\n");

    demo_tcp_pack();
    demo_tcp_flags();
    demo_tcp_handshake();
    demo_tcp_teardown();
    demo_tcp_ports();

    printf("\n所有演示完成！\n");
    return 0;
}
