/**
 * libdnet UDP 数据包操作示例 - test6
 */

#include <stdio.h>
#include <dnet.h>
#include <dnet/ip.h>
#include <dnet/udp.h>

void demo_udp_pack(void) {
    u_char ip_packet[IP_HDR_LEN + UDP_HDR_LEN + 32];
    printf("\n=== UDP 数据包构建 ===\n");

    ip_pack_hdr(ip_packet, 0, IP_HDR_LEN + UDP_HDR_LEN + 32,
                12345, 0, 64, IP_PROTO_UDP,
                inet_addr("192.168.1.100"), inet_addr("8.8.8.8"));

    printf("UDP: 192.168.1.100:12345 -> 8.8.8.8:53\n");
}

void demo_udp_ports(void) {
    printf("\n=== 常见 UDP 端口 ===\n");
    printf("  53  - DNS\n");
    printf("  67  - DHCP Server\n");
    printf("  68  - DHCP Client\n");
    printf("  123 - NTP\n");
    printf("  161 - SNMP\n");
}

int main(void) {
    printf("libdnet UDP 数据包操作示例\n");
    demo_udp_pack();
    demo_udp_ports();
    printf("\n所有演示完成！\n");
    return 0;
}
