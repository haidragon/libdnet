#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

void test_ipv4_conversion() {
    struct addr addr;
    char buf[32];

    printf("   === IPv4 地址转换 ===\n");

    // 1. 字符串到二进制
    if (addr_pton("192.168.1.100", &addr) < 0) {
        printf("   地址转换失败\n");
        return;
    }

    printf("   字符串 -> 二进制:\n");
    printf("     输入: 192.168.1.100\n");
    printf("     类型: %d (ADDR_TYPE_IP = %d)\n", addr.addr_type, ADDR_TYPE_IP);
    printf("     二进制: 0x%08X\n", ntohl(addr.addr_ip));

    // 2. 二进制到字符串
    if (addr_ntop(&addr, buf, sizeof(buf)) == NULL) {
        printf("   转换失败\n");
        return;
    }

    printf("   二进制 -> 字符串:\n");
    printf("     输入: 0x%08X\n", ntohl(addr.addr_ip));
    printf("     输出: %s\n", buf);

    // 3. 带掩码的地址
    if (addr_pton("192.168.1.0/24", &addr) == 0) {
        printf("\n   CIDR 表示法:\n");
        printf("     输入: 192.168.1.0/24\n");
        printf("     网络地址: %s\n", addr_ntoa(&addr));
        printf("     掩码位数: %d\n", addr.addr_bits);
    }

    // 4. 特殊地址
    const char *special_addrs[] = {
        "0.0.0.0",
        "127.0.0.1",
        "255.255.255.255",
        "224.0.0.1",
        NULL
    };

    printf("\n   特殊 IPv4 地址:\n");
    for (int i = 0; special_addrs[i] != NULL; i++) {
        if (addr_pton(special_addrs[i], &addr) == 0) {
            addr_ntop(&addr, buf, sizeof(buf));
            printf("     %-15s -> 0x%08X\n", buf, ntohl(addr.addr_ip));
        }
    }
}

void test_mac_conversion() {
    struct addr addr;
    char buf[32];

    printf("\n   === MAC 地址转换 ===\n");

    // 1. 字符串到二进制
    if (addr_pton("AA:BB:CC:DD:EE:FF", &addr) < 0) {
        printf("   MAC 地址转换失败\n");
        return;
    }

    printf("   字符串 -> 二进制:\n");
    printf("     输入: AA:BB:CC:DD:EE:FF\n");
    printf("     类型: %d (ADDR_TYPE_ETH = %d)\n", addr.addr_type, ADDR_TYPE_ETH);

    // 2. 二进制到字符串
    if (addr_ntop(&addr, buf, sizeof(buf)) != NULL) {
        printf("   二进制 -> 字符串:\n");
        printf("     输出: %s\n", buf);
    }

    // 3. 不同格式
    const char *mac_formats[] = {
        "00:11:22:33:44:55",
        "00-11-22-33-44-55",
        "0011.2233.4455",
        "001122334455",
        NULL
    };

    printf("\n   不同 MAC 地址格式:\n");
    for (int i = 0; mac_formats[i] != NULL; i++) {
        if (addr_pton(mac_formats[i], &addr) == 0) {
            addr_ntop(&addr, buf, sizeof(buf));
            printf("     %-18s -> %s\n", mac_formats[i], buf);
        }
    }
}

void test_addr_copy_and_compare() {
    struct addr addr1, addr2;
    int ret;

    printf("\n   === 地址复制和比较 ===\n");

    // 1. 复制地址 (使用 memcpy)
    addr_pton("192.168.1.1", &addr1);
    memcpy(&addr2, &addr1, sizeof(addr1));

    printf("   地址复制:\n");
    printf("     原地址: %s\n", addr_ntoa(&addr1));
    printf("     复制后: %s\n", addr_ntoa(&addr2));

    // 2. 比较地址
    addr_pton("192.168.1.1", &addr1);
    addr_pton("192.168.1.2", &addr2);

    printf("\n   地址比较:\n");
    printf("     地址1: %s\n", addr_ntoa(&addr1));
    printf("     地址2: %s\n", addr_ntoa(&addr2));

    ret = addr_cmp(&addr1, &addr2);
    if (ret == 0) {
        printf("     结果: 地址相等\n");
    } else if (ret < 0) {
        printf("     结果: 地址1 < 地址2\n");
    } else {
        printf("     结果: 地址1 > 地址2\n");
    }

    // 3. 相同地址比较
    addr_pton("192.168.1.1", &addr1);
    addr_pton("192.168.1.1", &addr2);

    ret = addr_cmp(&addr1, &addr2);
    printf("\n     相同地址比较: %s == %s -> %d\n",
           addr_ntoa(&addr1), addr_ntoa(&addr2), ret);
}

void test_addr_operations() {
    struct addr addr;

    printf("\n   === 地址操作 ===\n");

    // 1. 网络地址计算
    addr_pton("192.168.1.100/24", &addr);

    printf("   网络地址计算:\n");
    printf("     主机地址: %s/%d\n", addr_ntoa(&addr), addr.addr_bits);

    // 使用 libdnet 的 addr_net 和 addr_bcast
    struct addr net, bcast;
    addr_net(&addr, &net);
    addr_bcast(&addr, &bcast);

    printf("     网络地址: %s\n", addr_ntoa(&net));
    printf("     广播地址: %s\n", addr_ntoa(&bcast));

    // 2. 不同掩码演示
    printf("\n   常见网络掩码:\n");
    printf("     /8  -> 255.0.0.0\n");
    printf("     /16 -> 255.255.0.0\n");
    printf("     /24 -> 255.255.255.0\n");
    printf("     /32 -> 255.255.255.255\n");
}

void test_addr_ntoa_r() {
    struct addr addr;
    char buf1[32], buf2[32];
    const char *result1, *result2;

    printf("\n   === 线程安全的地址转换 ===\n");

    addr_pton("192.168.1.1", &addr);

    // 使用 addr_ntoa（静态缓冲区）
    result1 = addr_ntoa(&addr);
    result2 = addr_ntoa(&addr);

    printf("   addr_ntoa (静态缓冲区):\n");
    printf("     调用1: %s\n", result1);
    printf("     调用2: %s (指针可能相同)\n", result2);
    printf("     指针1: %p, 指针2: %p\n", (void *)result1, (void *)result2);

    // 使用 addr_ntop（用户缓冲区）
    addr_ntop(&addr, buf1, sizeof(buf1));
    addr_ntop(&addr, buf2, sizeof(buf2));

    printf("\n   addr_ntop (用户缓冲区):\n");
    printf("     缓冲区1: %s\n", buf1);
    printf("     缓冲区2: %s\n", buf2);
    printf("     指针1: %p, 指针2: %p (不同)\n", (void *)buf1, (void *)buf2);

    printf("\n   注意: 多线程环境中应使用 addr_ntop 而非 addr_ntoa\n");
}

int main() {
    printf("=== libdnet 地址转换示例 ===\n\n");

    // 测试各种地址转换
    test_ipv4_conversion();
    test_mac_conversion();
    test_addr_copy_and_compare();
    test_addr_operations();
    test_addr_ntoa_r();

    printf("\n=== 地址转换示例完成 ===\n");
    return 0;
}
