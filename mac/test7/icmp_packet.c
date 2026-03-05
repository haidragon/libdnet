/**
 * libdnet ICMP 数据包操作示例 - test7
 */

#include <stdio.h>
#include <dnet.h>
#include <dnet/ip.h>
#include <dnet/icmp.h>

void demo_icmp_echo(void) {
    printf("\n=== ICMP Echo (Ping) ===\n");
    printf("类型: %d (Echo Request)\n", ICMP_ECHO);
    printf("类型: %d (Echo Reply)\n", ICMP_ECHOREPLY);
}

void demo_icmp_types(void) {
    printf("\n=== ICMP 类型 ===\n");
    printf("  0  - Echo Reply\n");
    printf("  3  - Destination Unreachable\n");
    printf("  8  - Echo Request\n");
    printf("  11 - Time Exceeded\n");
}

int main(void) {
    printf("libdnet ICMP 数据包操作示例\n");
    demo_icmp_echo();
    demo_icmp_types();
    printf("\n所有演示完成！\n");
    return 0;
}
