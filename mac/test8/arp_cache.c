/**
 * libdnet ARP 缓存操作示例 - test8
 */

#include <stdio.h>
#include <dnet.h>
#include <dnet/arp.h>

void demo_arp_pack(void) {
    u_char packet[ETH_HDR_LEN + ARP_HDR_LEN];
    printf("\n=== ARP 数据包构建 ===\n");
    printf("ARP Request: Who has 192.168.1.1? Tell 192.168.1.100\n");
}

void demo_arp_types(void) {
    printf("\n=== ARP 操作类型 ===\n");
    printf("  1 - ARP Request\n");
    printf("  2 - ARP Reply\n");
    printf("  3 - RARP Request\n");
    printf("  4 - RARP Reply\n");
}

int main(void) {
    printf("libdnet ARP 缓存操作示例\n");
    demo_arp_pack();
    demo_arp_types();
    printf("\n所有演示完成！\n");
    return 0;
}
