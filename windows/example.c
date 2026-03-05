/*
 * libdnet Windows 使用案例
 *
 * 功能：获取本机网络接口信息，包括IP地址、MAC地址、MTU等
 *
 * 编译方式：
 * gcc -I../include -L../src/.libs -o example example.c -ldnet -lws2_32 -liphlpapi
 *
 * 运行方式：
 * example.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

/*
 * 打印网络接口标志
 */
static void
print_flags(u_short flags)
{
    if (flags & INTF_FLAG_UP)
        printf(" UP");
    if (flags & INTF_FLAG_LOOPBACK)
        printf(" LOOPBACK");
    if (flags & INTF_FLAG_POINTOPOINT)
        printf(" POINTOPOINT");
    if (flags & INTF_FLAG_NOARP)
        printf(" NOARP");
    if (flags & INTF_FLAG_BROADCAST)
        printf(" BROADCAST");
    if (flags & INTF_FLAG_MULTICAST)
        printf(" MULTICAST");
}

/*
 * 回调函数：打印单个网络接口信息
 */
static int
print_interface(const struct intf_entry *entry, void *arg)
{
    int i;

    printf("\n[%s]\n", entry->intf_name);
    printf("  Flags: 0x%04x", entry->intf_flags);
    print_flags(entry->intf_flags);
    printf("\n");

    if (entry->intf_mtu != 0)
        printf("  MTU: %d\n", entry->intf_mtu);

    /* 打印IP地址 */
    if (entry->intf_addr.addr_type == ADDR_TYPE_IP) {
        if (entry->intf_dst_addr.addr_type == ADDR_TYPE_IP) {
            printf("  Inet: %s -> %s\n",
                addr_ntoa(&entry->intf_addr),
                addr_ntoa(&entry->intf_dst_addr));
        } else {
            printf("  Inet: %s\n", addr_ntoa(&entry->intf_addr));
        }
    }

    /* 打印MAC地址 */
    if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
        printf("  MAC:  %s\n", addr_ntoa(&entry->intf_link_addr));
    }

    /* 打印IP别名 */
    for (i = 0; i < entry->intf_alias_num; i++) {
        printf("  Alias: %s\n", addr_ntoa(&entry->intf_alias_addrs[i]));
    }

    return 0;
}

/*
 * 主函数：获取并显示所有网络接口信息
 */
int main(void)
{
    intf_t *intf;

    printf("=== libdnet Windows 使用案例 ===\n");
    printf("获取本机网络接口信息\n\n");

    /* 打开网络接口 */
    if ((intf = intf_open()) == NULL) {
        fprintf(stderr, "错误：无法打开网络接口\n");
        return 1;
    }

    /* 遍历所有网络接口 */
    if (intf_loop(intf, print_interface, NULL) < 0) {
        fprintf(stderr, "错误：无法获取网络接口信息\n");
        intf_close(intf);
        return 1;
    }

    printf("\n=== 完成 ===\n");

    /* 关闭网络接口 */
    intf_close(intf);

    return 0;
}
