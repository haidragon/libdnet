/*
 * libdnet Windows 最简示例
 *
 * 功能：获取本机默认 IP 地址
 *
 * 编译方式：
 * gcc -I../include -L../src/.libs -o simple.exe simple.c -ldnet -lws2_32 -liphlpapi
 */

#include <stdio.h>
#include <stdlib.h>
#include "dnet.h"

int main(void)
{
    intf_t *intf;
    struct intf_entry entry;
    struct addr addr;
    char buf[1024];

    printf("获取本机默认 IP 地址...\n");

    /* 打开网络接口 */
    if ((intf = intf_open()) == NULL) {
        fprintf(stderr, "错误：无法打开网络接口\n");
        return 1;
    }

    /* 查询本地回环地址对应的接口 */
    addr_aton("127.0.0.1", &addr);

    memset(&entry, 0, sizeof(entry));
    entry.intf_len = sizeof(buf);

    /* 获取接口信息 */
    if (intf_get_src(intf, &entry, &addr) == 0) {
        printf("接口名称: %s\n", entry.intf_name);
        printf("IP 地址:  %s\n", addr_ntoa(&entry.intf_addr));
        printf("MAC 地址: %s\n", addr_ntoa(&entry.intf_link_addr));
    } else {
        printf("无法获取接口信息\n");
    }

    intf_close(intf);
    return 0;
}
