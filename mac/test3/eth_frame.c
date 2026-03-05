/**
 * libdnet 以太网帧操作示例 - test3
 *
 * 功能：
 * 1. 以太网帧构建
 * 2. 打开以太网设备
 * 3. 发送以太网帧
 * 4. 接收以太网帧
 * 5. 以太网地址解析
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dnet.h>
#include <dnet/eth.h>

/* 常用以太网类型 */
#define ETH_TYPE_IP   0x0800  /* IPv4 */
#define ETH_TYPE_ARP  0x0806  /* ARP */
#define ETH_TYPE_IPV6 0x86DD  /* IPv6 */

/* 演示构建以太网帧 */
void demo_eth_pack(void) {
    u_char packet[ETH_HDR_LEN + 64];
    struct addr src_mac, dst_mac;
    uint16_t type = ETH_TYPE_IP;

    printf("\n=== 以太网帧构建 ===\n");

    /* 设置源 MAC 和目标 MAC */
    addr_pton("00:11:22:33:44:55", &src_mac);
    addr_pton("ff:ff:ff:ff:ff:ff", &dst_mac);

    /* 构建以太网头部 */
    eth_pack_hdr(packet,
                 dst_mac.addr_eth,  /* 目标 MAC */
                 src_mac.addr_eth,  /* 源 MAC */
                 type);            /* 以太网类型 */

    printf("构建以太网帧:\n");
    printf("  目标 MAC: ff:ff:ff:ff:ff:ff (广播)\n");
    printf("  源 MAC:   00:11:22:33:44:55\n");
    printf("  类型:     0x%04x (IPv4)\n", type);
}

/* 演示以太网地址解析 */
void demo_eth_addr(void) {
    struct addr mac;
    char buf[128];
    int rc;

    printf("\n=== 以太网地址解析 ===\n");

    /* MAC 地址字符串转换 */
    rc = addr_pton("00:11:22:33:44:55", &mac);
    if (rc == 0) {
        addr_ntop(&mac, buf, sizeof(buf));
        printf("MAC 地址: %s\n", buf);
        printf("地址类型: %s\n",
               mac.addr_type == ADDR_TYPE_ETH ? "Ethernet" : "Other");
    }

    /* 广播地址 */
    rc = addr_pton("ff:ff:ff:ff:ff:ff", &mac);
    if (rc == 0) {
        addr_ntop(&mac, buf, sizeof(buf));
        printf("\n广播 MAC: %s\n", buf);
    }

    /* 组播地址 */
    rc = addr_pton("01:00:5e:00:00:01", &mac);
    if (rc == 0) {
        addr_ntop(&mac, buf, sizeof(buf));
        printf("组播 MAC: %s\n", buf);
    }
}

/* 演示打开以太网设备 */
void demo_eth_open(void) {
    eth_t *e;
    char *dev = "en0";  /* macOS 默认以太网接口 */

    printf("\n=== 以太网设备操作 ===\n");

    /* 打开以太网设备 */
    e = eth_open(dev);
    if (e == NULL) {
        printf("无法打开以太网设备 %s (需要 root 权限)\n", dev);
        return;
    }

    printf("成功打开以太网设备: %s\n", dev);
    printf("提示: 实际发送需要 root 权限\n");

    eth_close(e);
}

/* 演示获取以太网设备的 MAC 地址 */
void demo_eth_get_mac(void) {
    struct intf_entry ifentry;
    intf_t *intf;
    char dev[] = "en0";
    char buf[128];
    int rc;

    printf("\n=== 获取接口 MAC 地址 ===\n");

    intf = intf_open();
    if (intf == NULL) {
        printf("无法打开接口\n");
        return;
    }

    /* 查找指定接口 */
    ifentry.intf_len = sizeof(ifentry);
    strlcpy(ifentry.intf_name, dev, sizeof(ifentry.intf_name));

    rc = intf_get(intf, &ifentry);
    if (rc == 0) {
        struct addr mac;
        memcpy(&mac.addr_eth.data, ifentry.intf_link_addr.addr_eth.data, ETH_ADDR_LEN);
        mac.addr_type = ADDR_TYPE_ETH;

        addr_ntop(&mac, buf, sizeof(buf));
        printf("接口 %s 的 MAC 地址: %s\n", dev, buf);
    } else {
        printf("无法获取接口 %s 的 MAC 地址\n", dev);
    }

    intf_close(intf);
}

/* 演示不同以太网类型 */
void demo_eth_types(void) {
    printf("\n=== 以太网类型 ===\n");

    printf("常用以太网类型:\n");
    printf("  0x0800 - IPv4\n");
    printf("  0x0806 - ARP\n");
    printf("  0x86DD - IPv6\n");
    printf("  0x8847 - MPLS unicast\n");
    printf("  0x8848 - MPLS multicast\n");
    printf("  0x8100 - 802.1Q VLAN\n");
    printf("  0x88A8 - 802.1ad Q-in-Q\n");
    printf("  0x88CC - LLDP\n");
}

/* 演示十六进制转储 */
void hex_dump(const u_char *data, int len, const char *prefix) {
    int i, j;

    printf("%s (%d bytes):\n", prefix, len);
    for (i = 0; i < len; i += 16) {
        printf("  %04x: ", i);
        for (j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" ");
        }
        printf(" |");
        for (j = 0; j < 16 && i + j < len; j++) {
            u_char c = data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}

/* 演示完整的以太网帧构建 */
void demo_full_eth_frame(void) {
    u_char packet[256];
    struct addr src_mac, dst_mac;

    printf("\n=== 完整以太网帧示例 ===\n");

    /* 准备地址 */
    addr_pton("00:11:22:33:44:55", &src_mac);
    addr_pton("ff:ff:ff:ff:ff:ff", &dst_mac);

    /* 构建以太网帧 */
    eth_pack_hdr(packet,
                 dst_mac.addr_eth,
                 src_mac.addr_eth,
                 ETH_TYPE_IP);

    /* 添加一些负载数据 */
    memset(packet + ETH_HDR_LEN, 0xAA, 32);

    /* 十六进制转储 */
    hex_dump(packet, ETH_HDR_LEN + 32, "以太网帧");
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("libdnet 以太网帧操作示例\n");
    printf("======================\n");

    demo_eth_pack();
    demo_eth_addr();
    demo_eth_types();
    demo_eth_get_mac();
    demo_eth_open();
    demo_full_eth_frame();

    printf("\n所有演示完成！\n");
    printf("注意: 实际发送以太网帧需要 root 权限\n");

    return 0;
}
