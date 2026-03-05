#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dnet.h>
#include <signal.h>

static int running = 1;

void sigint_handler(int sig) {
    running = 0;
    printf("\n\n收到中断信号，停止捕获...\n");
}

void print_packet_info(const uint8_t *buf, size_t len) {
    struct eth_hdr *eth;
    struct ip_hdr *ip;
    char src[32], dst[32];
    struct addr addr;

    printf("  数据包长度: %zu 字节\n", len);

    if (len >= sizeof(struct eth_hdr)) {
        eth = (struct eth_hdr *)buf;
        printf("  以太网类型: 0x%04X\n", ntohs(eth->eth_type));

        if (ntohs(eth->eth_type) == ETH_TYPE_IP && len >= sizeof(struct eth_hdr) + sizeof(struct ip_hdr)) {
            ip = (struct ip_hdr *)(buf + sizeof(struct eth_hdr));

            addr.addr_type = ADDR_TYPE_IP;
            memcpy(&addr.addr_ip, &ip->ip_src, 4);
            addr_ntop(&addr, src, sizeof(src));

            memcpy(&addr.addr_ip, &ip->ip_dst, 4);
            addr_ntop(&addr, dst, sizeof(dst));

            printf("  IP 协议: %d (%s)\n", ip->ip_p,
                   ip->ip_p == IPPROTO_TCP ? "TCP" :
                   ip->ip_p == IPPROTO_UDP ? "UDP" :
                   ip->ip_p == IPPROTO_ICMP ? "ICMP" : "其他");

            printf("  IP 地址: %s -> %s\n", src, dst);
            printf("  TTL: %d, 总长度: %d\n", ip->ip_ttl, ntohs(ip->ip_len));
        }
    }

    printf("  数据 (前 %zu 字节):\n", len < 64 ? len : 64);
    for (size_t i = 0; i < len && i < 64; i++) {
        if (i % 16 == 0) printf("    %04zx: ", i);
        printf("%02x ", buf[i]);
        if (i % 16 == 15) printf("\n");
    }
    if (len > 0 && len % 16 != 0) printf("\n");
}

// 回调函数用于捕获数据包（libdnet的eth_recv不支持回调，这里简化处理）
int packet_callback(const uint8_t *buf, size_t len, void *arg) {
    int *count = (int *)arg;
    (*count)++;
    printf("\n[数据包 #%d]\n", *count);
    print_packet_info(buf, len);
    printf("\n");
    return 0;
}

int main() {
    eth_t *eth;
    uint8_t buf[65536];
    char ifname[16] = "en0";
    int packet_count = 0;
    int max_packets = 5;

    printf("=== libdnet 数据包捕获示例 ===\n");
    printf("注意: 此示例演示数据包捕获概念\n");
    printf("实际数据包捕获建议使用 libpcap\n\n");

    signal(SIGINT, sigint_handler);

    printf("1. 打开接口 %s...\n", ifname);
    printf("   ================================================\n\n");

    eth = eth_open(ifname);
    if (eth == NULL) {
        fprintf(stderr, "错误: 无法打开接口 %s\n", ifname);
        fprintf(stderr, "注意: libdnet的eth_open不支持完整的数据包捕获\n");
        fprintf(stderr, "建议使用 libpcap 进行数据包捕获\n\n");
        printf("2. 数据包捕获说明:\n");
        printf("   ================================================\n");
        printf("   - libdnet 专注于发送原始数据包\n");
        printf("   - 数据包捕获建议使用 libpcap\n");
        printf("   - libpcap 示例代码:\n\n");
        printf("     #include <pcap.h>\n");
        printf("     pcap_t *handle = pcap_open_live(ifname, 65535, 1, 1000, errbuf);\n");
        printf("     pcap_loop(handle, -1, packet_handler, NULL);\n\n");
        printf("3. 提示:\n");
        printf("   - 使用 'tcpdump -i %s' 捕获数据包\n", ifname);
        printf("   - libdnet 更适合数据包注入和构造\n");
        return 1;
    }

    printf("   成功打开接口 %s\n", ifname);
    printf("   将尝试捕获 %d 个数据包\n\n", max_packets);

    printf("2. 尝试接收数据包:\n");
    printf("   ================================================\n\n");
    printf("   (libdnet的eth_recv功能有限)\n\n");

    // libdnet 的 eth_open/eth_recv 主要用于发送，接收功能有限
    eth_close(eth);

    printf("3. 数据包捕获建议:\n");
    printf("   ================================================\n");
    printf("   - 使用 libpcap 进行完整的数据包捕获\n");
    printf("   - libdnet 专注于数据包构造和发送\n");
    printf("   - 结合两者使用效果最佳:\n");
    printf("     * libpcap: 数据包捕获和分析\n");
    printf("     * libdnet: 数据包构造和发送\n");

    printf("\n=== 数据包捕获示例完成 ===\n");
    return 0;
}
