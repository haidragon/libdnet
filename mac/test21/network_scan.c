#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

int route_callback(const struct route_entry *entry, void *arg) {
    int *count = (int *)arg;
    if (*count >= 5) return 1;  // 只显示前5条

    if (entry->route_dst.addr_type == ADDR_TYPE_IP) {
        char dst_str[32];
        addr_ntop(&entry->route_dst, dst_str, sizeof(dst_str));

        printf("   - 网络: %s", dst_str);
        if (entry->route_gw.addr_type == ADDR_TYPE_IP) {
            char gw_str[32];
            addr_ntop(&entry->route_gw, gw_str, sizeof(gw_str));
            printf(" 网关: %s", gw_str);
        }
        printf(" 接口: %s\n", entry->intf_name);
        (*count)++;
    }
    return 0;
}

int intf_callback(const struct intf_entry *entry, void *arg) {
    int *count = (int *)arg;
    if (*count >= 5) return 1;  // 只显示前5个

    int is_up = (entry->intf_flags & INTF_FLAG_UP) != 0;
    int is_loopback = (entry->intf_flags & INTF_FLAG_LOOPBACK) != 0;

    if (is_up && !is_loopback && entry->intf_addr.addr_type == ADDR_TYPE_IP) {
        char ip_str[32];
        addr_ntop(&entry->intf_addr, ip_str, sizeof(ip_str));

        printf("   ✓ %s: %s (类型: %d)\n",
               entry->intf_name,
               ip_str,
               entry->intf_type);
        (*count)++;
    }
    return 0;
}

int main() {
    printf("=== libdnet 网络扫描示例 ===\n\n");

    // 1. ARP 扫描
    printf("1. ARP 缓存查询:\n");
    printf("   尝试查询已知主机的 MAC:\n");

    arp_t *arp = arp_open();
    if (arp != NULL) {
        struct arp_entry entry;
        int found = 0;

        // 查询本地主机
        if (addr_pton("127.0.0.1", &entry.arp_pa) == 0) {
            if (arp_get(arp, &entry) == 0) {
                printf("   ✓ 127.0.0.1 -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                       entry.arp_ha.addr_eth.data[0],
                       entry.arp_ha.addr_eth.data[1],
                       entry.arp_ha.addr_eth.data[2],
                       entry.arp_ha.addr_eth.data[3],
                       entry.arp_ha.addr_eth.data[4],
                       entry.arp_ha.addr_eth.data[5]);
                found++;
            }
        }

        if (found == 0) {
            printf("   未找到ARP条目\n");
        }

        arp_close(arp);
    } else {
        printf("   无法打开 ARP 缓存\n");
    }

    // 2. 路由表扫描
    printf("\n2. 路由表扫描:\n");
    printf("   查找可达的网络 (前5条):\n");

    route_t *route = route_open();
    if (route != NULL) {
        int route_count = 0;
        route_loop(route, route_callback, &route_count);
        route_close(route);
    }

    // 3. 接口扫描
    printf("\n3. 接口扫描:\n");
    printf("   查找活动网络接口 (前5个):\n");

    intf_t *iop = intf_open();
    if (iop != NULL) {
        int intf_count = 0;
        intf_loop(iop, intf_callback, &intf_count);
        intf_close(iop);
    }

    // 4. 地址转换测试
    printf("\n4. 地址转换测试:\n");
    const char *test_addrs[] = {"192.168.1.1", "8.8.8.8", "255.255.255.255", NULL};

    for (int i = 0; test_addrs[i] != NULL; i++) {
        struct addr addr;
        if (addr_pton(test_addrs[i], &addr) == 0) {
            char buf[32];
            addr_ntop(&addr, buf, sizeof(buf));
            printf("   %s -> %s (类型: %d)\n", test_addrs[i], buf, addr.addr_type);
        }
    }

    // 5. 扫描建议
    printf("\n5. 网络扫描建议:\n");
    printf("   - 仅在授权网络中使用\n");
    printf("   - 控制扫描速率，避免过载\n");
    printf("   - 记录扫描结果\n");
    printf("   - 使用专业工具如 nmap 进行完整扫描\n");

    printf("\n=== 网络扫描示例完成 ===\n");
    return 0;
}
