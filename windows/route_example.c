/*
 * libdnet Windows 路由表使用案例
 *
 * 功能：查询系统路由表
 *
 * 编译方式：
 * gcc -I../include -L../src/.libs -o route_example.exe route_example.c -ldnet -lws2_32 -liphlpapi
 */

#include <stdio.h>
#include <stdlib.h>
#include "dnet.h"

/*
 * 打印路由条目
 */
static int
print_route_entry(const struct route_entry *entry, void *arg)
{
    printf("  %s/%d -> %s",
        addr_ntoa(&entry->route_dst),
        entry->route_dst.addr_bits,
        addr_ntoa(&entry->route_gw));

    if (entry->intf_name[0] != '\0')
        printf(" (via %s)", entry->intf_name);

    printf("\n");
    return 0;
}

/*
 * 主函数：查询路由表
 */
int main(void)
{
    route_t *route;

    printf("=== libdnet 路由表使用案例 ===\n\n");

    /* 打开路由表 */
    if ((route = route_open()) == NULL) {
        fprintf(stderr, "错误：无法打开路由表\n");
        return 1;
    }

    /* 显示所有路由 */
    printf("系统路由表：\n");
    if (route_loop(route, print_route_entry, NULL) < 0) {
        fprintf(stderr, "错误：无法读取路由表\n");
        route_close(route);
        return 1;
    }

    printf("\n=== 完成 ===\n");

    /* 关闭路由表 */
    route_close(route);

    return 0;
}
