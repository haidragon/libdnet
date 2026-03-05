/**
 * libdnet 地址操作示例 - test2
 *
 * 功能：
 * 1. 地址字符串转换 (pton/ntop)
 * 2. 地址比较 (cmp)
 * 3. 网络地址计算 (net)
 * 4. 广播地址计算 (bcast)
 * 5. 子网掩码与位宽转换 (mtob/btom)
 * 6. 地址类型判断
 * 7. 回环地址检测
 * 8. 私有地址检测
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>
#include <dnet/addr.h>

/* 显示地址信息 */
void print_addr_info(struct addr *a, const char *label) {
    char buf[128];
    addr_ntop(a, buf, sizeof(buf));
    printf("%s: %s (type: %s)\n", label, buf,
           a->addr_type == ADDR_TYPE_IP ? "IPv4" :
           a->addr_type == ADDR_TYPE_IP6 ? "IPv6" :
           a->addr_type == ADDR_TYPE_ETH ? "Ethernet" : "Unknown");
}

/* 演示地址字符串转换 */
void demo_addr_conversion(void) {
    struct addr a;
    char buf[128];
    int rc;

    printf("\n=== 地址字符串转换 ===\n");

    /* IPv4 地址转换 */
    rc = addr_pton("192.168.1.100", &a);
    if (rc == 0) {
        addr_ntop(&a, buf, sizeof(buf));
        printf("IPv4: '192.168.1.100' → '%s'\n", buf);
    }

    /* IPv6 地址转换 */
    rc = addr_pton("2001:db8::1", &a);
    if (rc == 0) {
        addr_ntop(&a, buf, sizeof(buf));
        printf("IPv6: '2001:db8::1' → '%s'\n", buf);
    }

    /* 带掩码的地址 */
    rc = addr_pton("192.168.1.100/24", &a);
    if (rc == 0) {
        addr_ntop(&a, buf, sizeof(buf));
        printf("带掩码: '192.168.1.100/24' → '%s'\n", buf);
    }

    /* MAC 地址转换 */
    rc = addr_pton("00:11:22:33:44:55", &a);
    if (rc == 0) {
        addr_ntop(&a, buf, sizeof(buf));
        printf("MAC: '00:11:22:33:44:55' → '%s'\n", buf);
    }
}

/* 演示地址比较 */
void demo_addr_comparison(void) {
    struct addr a1, a2, a3;
    int rc;

    printf("\n=== 地址比较 ===\n");

    addr_pton("192.168.1.100", &a1);
    addr_pton("192.168.1.100", &a2);
    addr_pton("192.168.1.101", &a3);

    rc = addr_cmp(&a1, &a2);
    printf("192.168.1.100 == 192.168.1.100: %s\n", rc == 0 ? "是" : "否");

    rc = addr_cmp(&a1, &a3);
    printf("192.168.1.100 == 192.168.1.101: %s\n", rc == 0 ? "是" : "否");

    rc = addr_cmp(&a1, &a3);
    printf("192.168.1.100 与 192.168.1.101 比较: %d\n", rc);
}

/* 演示网络地址和广播地址计算 */
void demo_network_broadcast(void) {
    struct addr addr, net, bcast;
    char buf[128];

    printf("\n=== 网络地址和广播地址 ===\n");

    /* /24 网络 */
    addr_pton("192.168.1.100/24", &addr);
    addr_ntop(&addr, buf, sizeof(buf));
    printf("原始地址: %s\n", buf);

    addr_net(&addr, &net);
    addr_ntop(&net, buf, sizeof(buf));
    printf("网络地址: %s\n", buf);

    addr_bcast(&addr, &bcast);
    addr_ntop(&bcast, buf, sizeof(buf));
    printf("广播地址: %s\n", buf);

    /* /30 网络 */
    addr_pton("10.0.0.10/30", &addr);
    addr_net(&addr, &net);
    addr_bcast(&addr, &bcast);
    printf("\n10.0.0.10/30:\n");
    printf("  网络: ");
    addr_ntop(&net, buf, sizeof(buf));
    printf("%s\n", buf);
    printf("  广播: ");
    addr_ntop(&bcast, buf, sizeof(buf));
    printf("%s\n", buf);
}

/* 演示掩码转换 */
void demo_mask_conversion(void) {
    struct addr mask;
    uint16_t bits;
    char buf[128];

    printf("\n=== 掩码转换 ===\n");

    /* 位宽 → 掩码 */
    printf("位宽 → 掩码:\n");
    addr_btom(8, &mask.addr_ip, IP_ADDR_LEN);
    addr_pton("0.0.0.0", &mask);
    addr_btom(8, &mask.addr_ip, IP_ADDR_LEN);
    mask.addr_type = ADDR_TYPE_IP;
    addr_ntop(&mask, buf, sizeof(buf));
    printf("  /8  → %s\n", buf);

    addr_btom(16, &mask.addr_ip, IP_ADDR_LEN);
    mask.addr_type = ADDR_TYPE_IP;
    addr_ntop(&mask, buf, sizeof(buf));
    printf("  /16 → %s\n", buf);

    addr_btom(24, &mask.addr_ip, IP_ADDR_LEN);
    mask.addr_type = ADDR_TYPE_IP;
    addr_ntop(&mask, buf, sizeof(buf));
    printf("  /24 → %s\n", buf);

    addr_btom(32, &mask.addr_ip, IP_ADDR_LEN);
    mask.addr_type = ADDR_TYPE_IP;
    addr_ntop(&mask, buf, sizeof(buf));
    printf("  /32 → %s\n", buf);

    /* 掩码 → 位宽 */
    printf("\n掩码 → 位宽:\n");
    addr_pton("255.0.0.0", &mask);
    addr_mtob(&mask.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.0.0.0    → /%d\n", bits);

    addr_pton("255.255.0.0", &mask);
    addr_mtob(&mask.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.255.0.0  → /%d\n", bits);

    addr_pton("255.255.255.0", &mask);
    addr_mtob(&mask.addr_ip, IP_ADDR_LEN, &bits);
    printf("  255.255.255.0→ /%d\n", bits);
}

/* 演示地址类型判断 */
void demo_addr_type(void) {
    struct addr a;

    printf("\n=== 地址类型判断 ===\n");

    addr_pton("192.168.1.1", &a);
    printf("192.168.1.1 是 IPv4: %s\n",
           a.addr_type == ADDR_TYPE_IP ? "是" : "否");

    addr_pton("2001:db8::1", &a);
    printf("2001:db8::1 是 IPv6: %s\n",
           a.addr_type == ADDR_TYPE_IP6 ? "是" : "否");

    addr_pton("00:11:22:33:44:55", &a);
    printf("00:11:22:33:44:55 是 Ethernet: %s\n",
           a.addr_type == ADDR_TYPE_ETH ? "是" : "否");
}

/* 演示回环地址检测 */
void demo_loopback(void) {
    struct addr a;
    int is_loop;
    uint32_t ip;

    printf("\n=== 回环地址检测 ===\n");

    addr_pton("127.0.0.1", &a);
    ip = ntohl(a.addr_ip);
    is_loop = ((ip & 0xff000000) == 0x7f000000);
    printf("127.0.0.1 是回环地址: %s\n", is_loop ? "是" : "否");

    addr_pton("192.168.1.1", &a);
    ip = ntohl(a.addr_ip);
    is_loop = ((ip & 0xff000000) == 0x7f000000);
    printf("192.168.1.1 是回环地址: %s\n", is_loop ? "是" : "否");
}

/* 演示私有地址检测 */
void demo_private(void) {
    struct addr a;
    int is_priv;
    uint32_t ip;

    printf("\n=== 私有地址检测 ===\n");

    /* 10.0.0.0/8 */
    addr_pton("10.0.0.1", &a);
    ip = ntohl(a.addr_ip);
    is_priv = ((ip & 0xff000000) == 0x0a000000);
    printf("10.0.0.1 是私有地址: %s\n", is_priv ? "是" : "否");

    /* 172.16.0.0/12 */
    addr_pton("172.16.0.1", &a);
    ip = ntohl(a.addr_ip);
    is_priv = ((ip & 0xfff00000) == 0xac100000);
    printf("172.16.0.1 是私有地址: %s\n", is_priv ? "是" : "否");

    /* 192.168.0.0/16 */
    addr_pton("192.168.1.1", &a);
    ip = ntohl(a.addr_ip);
    is_priv = ((ip & 0xffff0000) == 0xc0a80000);
    printf("192.168.1.1 是私有地址: %s\n", is_priv ? "是" : "否");

    /* 公网地址 */
    addr_pton("8.8.8.8", &a);
    ip = ntohl(a.addr_ip);
    is_priv = ((ip & 0xff000000) == 0x0a000000 ||
               (ip & 0xfff00000) == 0xac100000 ||
               (ip & 0xffff0000) == 0xc0a80000);
    printf("8.8.8.8 是私有地址: %s\n", is_priv ? "是" : "否");
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("libdnet 地址操作示例\n");
    printf("====================\n");

    demo_addr_conversion();
    demo_addr_comparison();
    demo_network_broadcast();
    demo_mask_conversion();
    demo_addr_type();
    demo_loopback();
    demo_private();

    printf("\n所有演示完成！\n");
    return 0;
}
