/*
 * libdnet Windows ARP 使用案例
 *
 * 功能：操作 ARP 表
 *
 * 编译方式：
 * gcc -I../include -L../src/.libs -o arp_example.exe arp_example.c -ldnet -lws2_32 -liphlpapi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dnet.h"

/*
 * 打印 ARP 条目
 */
static int
print_arp_entry(const struct arp_entry *entry, void *arg)
{
    printf("  %s -> %s\n",
        addr_ntoa(&entry->arp_pa),
        addr_ntoa(&entry->arp_ha));
    return 0;
}

/*
 * 主函数：操作 ARP 表
 */
int main(void)
{
    arp_t *arp;
    struct arp_entry entry;

    printf("=== libdnet ARP 使用案例 ===\n\n");

    /* 打开 ARP 表 */
    if ((arp = arp_open()) == NULL) {
        fprintf(stderr, "错误：无法打开 ARP 表\n");
        return 1;
    }

    /* 显示当前 ARP 表 */
    printf("当前 ARP 表：\n");
    if (arp_loop(arp, print_arp_entry, NULL) < 0) {
        fprintf(stderr, "错误：无法读取 ARP 表\n");
        arp_close(arp);
        return 1;
    }

    printf("\n");

    /* 获取指定主机的 ARP 条目 */
    printf("查询网关 ARP 信息（192.168.1.1）：\n");
    addr_aton("192.168.1.1", &entry.arp_pa);

    if (arp_get(arp, &entry) == 0) {
        printf("  %s -> %s\n",
            addr_ntoa(&entry.arp_pa),
            addr_ntoa(&entry.arp_ha));
    } else {
        printf("  未找到 ARP 条目\n");
    }

    printf("\n=== 完成 ===\n");

    /* 关闭 ARP 表 */
    arp_close(arp);

    return 0;
}
