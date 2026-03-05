/*
 * interface_scanner.c - 网络接口扫描工具
 *
 * 基于 libdnet 的 macOS 网络接口扫描示例
 *
 * 编译: gcc -o interface_scanner interface_scanner.c -I../include -L../.libs -ldnet -lcheck
 * 运行: ./interface_scanner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>
#include <dnet/addr.h>
#include <dnet/eth.h>
#include <dnet/intf.h>
#include <dnet/route.h>
#include <dnet/arp.h>

/* 打印接口信息 */
static int print_interface(const struct intf_entry *entry, void *arg)
{
    char addr_buf[256];
    char mac_buf[256];
    int *count = (int *)arg;

    (*count)++;

    printf("\n[接口 #%d]\n", *count);
    printf("名称: %s\n", entry->intf_name);
    printf("索引: %u\n", entry->intf_index);
    printf("类型: %u (", entry->intf_type);

    switch (entry->intf_type) {
        case INTF_TYPE_ETH:
            printf("Ethernet");
            break;
        case INTF_TYPE_LOOPBACK:
            printf("Loopback");
            break;
        case INTF_TYPE_PPP:
            printf("PPP");
            break;
        case INTF_TYPE_TUN:
            printf("Tunnel");
            break;
        default:
            printf("Other");
            break;
    }
    printf(")\n");

    printf("标志: 0x%04x (", entry->intf_flags);
    if (entry->intf_flags & INTF_FLAG_UP)
        printf("UP ");
    if (entry->intf_flags & INTF_FLAG_LOOPBACK)
        printf("LOOPBACK ");
    if (entry->intf_flags & INTF_FLAG_POINTOPOINT)
        printf("POINTOPOINT ");
    if (entry->intf_flags & INTF_FLAG_BROADCAST)
        printf("BROADCAST ");
    if (entry->intf_flags & INTF_FLAG_MULTICAST)
        printf("MULTICAST ");
    if (entry->intf_flags & INTF_FLAG_NOARP)
        printf("NOARP ");
    printf(")\n");

    printf("MTU: %u 字节\n", entry->intf_mtu);

    /* 打印 IP 地址 */
    if (entry->intf_addr.addr_type != ADDR_TYPE_NONE) {
        addr_ntop(&entry->intf_addr, addr_buf, sizeof(addr_buf));
        printf("IP 地址: %s/%d\n", addr_buf, entry->intf_addr.addr_bits);
    }

    /* 打印链路层地址 (MAC) */
    if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
        eth_ntop(&entry->intf_link_addr.addr_eth, mac_buf, sizeof(mac_buf));
        printf("MAC 地址: %s\n", mac_buf);
    }

    /* 打印点对点目标地址 */
    if (entry->intf_flags & INTF_FLAG_POINTOPOINT &&
        entry->intf_dst_addr.addr_type != ADDR_TYPE_NONE) {
        addr_ntop(&entry->intf_dst_addr, addr_buf, sizeof(addr_buf));
        printf("点对点目标: %s\n", addr_buf);
    }

    /* 打印别名地址 */
    if (entry->intf_alias_num > 0) {
        printf("别名地址 (%u 个):\n", entry->intf_alias_num);
        for (uint32_t i = 0; i < entry->intf_alias_num; i++) {
            addr_ntop(&entry->intf_alias_addrs[i], addr_buf, sizeof(addr_buf));
            printf("  [%u] %s/%d\n", i + 1, addr_buf,
                   entry->intf_alias_addrs[i].addr_bits);
        }
    }

    /* 打印驱动信息 */
    if (strlen(entry->driver_name) > 0) {
        printf("驱动: %s\n", entry->driver_name);
    }
    if (strlen(entry->driver_vers) > 0) {
        printf("驱动版本: %s\n", entry->driver_vers);
    }

    return 0;
}

/* 打印路由条目 */
static int print_route(const struct route_entry *entry, void *arg)
{
    char dst_buf[256];
    char gw_buf[256];
    int *count = (int *)arg;

    (*count)++;

    addr_ntop(&entry->route_dst, dst_buf, sizeof(dst_buf));

    if (entry->route_gw.addr_type == ADDR_TYPE_NONE) {
        printf("[路由 #%d] %s dev %s metric %d\n",
               *count, dst_buf, entry->intf_name, entry->metric);
    } else {
        addr_ntop(&entry->route_gw, gw_buf, sizeof(gw_buf));
        printf("[路由 #%d] %s via %s dev %s metric %d\n",
               *count, dst_buf, gw_buf, entry->intf_name, entry->metric);
    }

    return 0;
}

/* 打印 ARP 条目 */
static int print_arp_entry(const struct arp_entry *entry, void *arg)
{
    char ip_buf[256];
    char mac_buf[256];
    int *count = (int *)arg;

    (*count)++;

    addr_ntop(&entry->arp_pa, ip_buf, sizeof(ip_buf));
    eth_ntop(&entry->arp_ha.addr_eth, mac_buf, sizeof(mac_buf));

    printf("[ARP #%d] %s → %s\n", *count, ip_buf, mac_buf);

    return 0;
}

/* 演示地址操作 */
static void demo_address_operations(void)
{
    printf("\n========== 地址操作演示 ==========\n");

    struct addr addr1, addr2, net, bcast;
    char buf[256];

    /* 演示 IPv4 地址 */
    printf("\n1. IPv4 地址操作:\n");
    addr_pton("192.168.1.100/24", &addr1);
    printf("   原始地址: ");
    addr_ntop(&addr1, buf, sizeof(buf));
    printf("%s/%d\n", buf, addr1.addr_bits);

    /* 计算网络地址 */
    addr_net(&addr1, &net);
    printf("   网络地址: ");
    addr_ntop(&net, buf, sizeof(buf));
    printf("%s\n", buf);

    /* 计算广播地址 */
    addr_bcast(&addr1, &bcast);
    printf("   广播地址: ");
    addr_ntop(&bcast, buf, sizeof(buf));
    printf("%s\n", buf);

    /* 演示地址比较 */
    addr_pton("192.168.1.100/24", &addr1);
    addr_pton("192.168.1.101/24", &addr2);
    printf("   比较相同网络的不同地址: %d\n", addr_cmp(&addr1, &addr2));

    addr_pton("192.168.1.100/24", &addr1);
    addr_pton("192.168.1.100/24", &addr2);
    printf("   比较相同地址: %d\n", addr_cmp(&addr1, &addr2));

    addr_pton("192.168.1.100/24", &addr1);
    addr_pton("10.0.0.0/8", &addr2);
    printf("   比较不同网络: %d\n", addr_cmp(&addr1, &addr2));

    /* 演示以太网地址 */
    printf("\n2. 以太网地址操作:\n");
    eth_addr_t eth1, eth2;
    eth_pton("00:11:22:33:44:55", &eth1);
    printf("   MAC 地址: ");
    eth_ntop(&eth1, buf, sizeof(buf));
    printf("%s\n", buf);

    /* 判断是否为多播地址 */
    printf("   是否为多播: %s\n",
           ETH_IS_MULTICAST(eth1.data) ? "是" : "否");

    /* 判断是否为广播地址 */
    printf("   是否为广播: %s\n",
           memcmp(eth1.data, ETH_ADDR_BROADCAST, ETH_ADDR_LEN) == 0 ? "是" : "否");

    eth_pton("01:00:5e:00:00:01", &eth2);  // IPv4 多播 MAC
    printf("   %s 是否为多播: %s\n", buf,
           ETH_IS_MULTICAST(eth2.data) ? "是" : "否");

    /* 演示 IPv6 地址 */
    printf("\n3. IPv6 地址操作:\n");
    ip6_addr_t ip6;
    ip6_pton("2001:db8::1", &ip6);
    printf("   IPv6 地址: ");
    ip6_ntop(&ip6, buf, sizeof(buf));
    printf("%s\n", buf);

    ip6_pton("::1", &ip6);
    printf("   Loopback: ");
    ip6_ntop(&ip6, buf, sizeof(buf));
    printf("%s\n", buf);

    ip6_pton("fe80::1", &ip6);
    printf("   Link-local: ");
    ip6_ntop(&ip6, buf, sizeof(buf));
    printf("%s\n", buf);
}

/* 演示位宽掩码转换 */
static void demo_bitmask_conversion(void)
{
    printf("\n========== 位宽掩码转换演示 ==========\n");

    uint32_t mask;
    char buf[32];
    int bits;

    /* IPv4 位宽 → 掩码 */
    printf("\nIPv4 位宽 → 掩码:\n");
    for (bits = 8; bits <= 32; bits += 8) {
        addr_btom(bits, &mask, IP_ADDR_LEN);
        printf("  /%d → 0x%08x (", bits, ntohl(mask));
        ip_addr_t ip = mask;
        ip_ntop(&ip, buf, sizeof(buf));
        printf("%s)\n", buf);
    }

    /* 掩码 → IPv4 位宽 */
    printf("\nIPv4 掩码 → 位宽:\n");
    struct addr mask_addr;
    addr_pton("255.0.0.0", &mask_addr);
    addr_mtob(&mask_addr.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.0.0.0 → /%d\n", bits);

    addr_pton("255.255.0.0", &mask_addr);
    addr_mtob(&mask_addr.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.255.0.0 → /%d\n", bits);

    addr_pton("255.255.255.0", &mask_addr);
    addr_mtob(&mask_addr.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.255.255.0 → /%d\n", bits);
}

/* 查找包含指定地址的接口 */
static void find_interface_for_address(const char *addr_str)
{
    printf("\n========== 查找包含地址 %s 的接口 ==========\n", addr_str);

    intf_t *i = intf_open();
    if (i == NULL) {
        perror("intf_open");
        return;
    }

    struct intf_entry *entry;
    size_t len = sizeof(*entry) + 16 * sizeof(struct addr);
    entry = calloc(1, len);
    if (entry == NULL) {
        perror("calloc");
        intf_close(i);
        return;
    }

    entry->intf_len = len;

    struct addr target_addr;
    if (addr_pton(addr_str, &target_addr) < 0) {
        fprintf(stderr, "无效的地址: %s\n", addr_str);
        free(entry);
        intf_close(i);
        return;
    }

    /* 遍历所有接口 */
    if (intf_get_src(i, entry, &target_addr) >= 0) {
        char buf[256];
        printf("找到接口: %s\n", entry->intf_name);
        printf("  接口地址: ");
        addr_ntop(&entry->intf_addr, buf, sizeof(buf));
        printf("%s/%d\n", buf, entry->intf_addr.addr_bits);
    } else {
        printf("未找到包含该地址的接口\n");
    }

    free(entry);
    intf_close(i);
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     libdnet 网络接口扫描工具 (macOS 示例)                ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    /* ========== 扫描网络接口 ========== */
    printf("\n========== 网络接口扫描 ==========\n");

    intf_t *i = intf_open();
    if (i == NULL) {
        perror("intf_open");
        fprintf(stderr, "提示: 需要管理员权限运行\n");
        return 1;
    }

    int if_count = 0;
    if (intf_loop(i, print_interface, &if_count) < 0) {
        perror("intf_loop");
    } else {
        printf("\n总计: %d 个网络接口\n", if_count);
    }

    intf_close(i);

    /* ========== 扫描路由表 ========== */
    printf("\n========== 路由表扫描 ==========\n");

    route_t *r = route_open();
    if (r == NULL) {
        perror("route_open");
        return 1;
    }

    int route_count = 0;
    if (route_loop(r, print_route, &route_count) < 0) {
        perror("route_loop");
    } else {
        printf("\n总计: %d 条路由\n", route_count);
    }

    route_close(r);

    /* ========== 扫描 ARP 缓存 ========== */
    printf("\n========== ARP 缓存扫描 ==========\n");

    arp_t *a = arp_open();
    if (a == NULL) {
        perror("arp_open");
        return 1;
    }

    int arp_count = 0;
    if (arp_loop(a, print_arp_entry, &arp_count) < 0) {
        perror("arp_loop");
    } else {
        printf("\n总计: %d 个 ARP 条目\n", arp_count);
    }

    arp_close(a);

    /* ========== 地址操作演示 ========== */
    demo_address_operations();

    /* ========== 位宽掩码转换演示 ========== */
    demo_bitmask_conversion();

    /* ========== 查找接口演示 ========== */
    find_interface_for_address("8.8.8.8");

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     扫描完成                                             ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    return 0;
}
