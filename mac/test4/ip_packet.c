/**
 * libdnet IP 数据包操作示例 - test4
 *
 * 功能：
 * 1. IP 数据包构建
 * 2. IP 头部解析
 * 3. IP 校验和计算
 * 4. IP 分片与重组
 * 5. IP 选项处理
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>
#include <dnet/ip.h>

/* 演示构建 IP 数据包 */
void demo_ip_pack(void) {
    u_char packet[IP_HDR_LEN + 64];
    struct ip_hdr *ip = (struct ip_hdr *)packet;

    printf("\n=== IP 数据包构建 ===\n");

    /* 构建 IP 头部 */
    ip_pack_hdr(packet,
                0,                        /* TOS */
                IP_HDR_LEN + 64,          /* 总长度 */
                12345,                    /* ID */
                IP_DF,                    /* 标志: 不分片 */
                64,                       /* TTL */
                IP_PROTO_ICMP,            /* 协议: ICMP */
                inet_addr("192.168.1.100"), /* 源 IP */
                inet_addr("8.8.8.8"));    /* 目标 IP */

    /* 计算 IP 校验和 */
    ip_checksum(packet, IP_HDR_LEN, 0);

    printf("构建 IP 数据包:\n");
    printf("  版本: %d\n", IP_HDR_LEN >> 4);
    printf("  头部长度: %d bytes\n", IP_HDR_LEN);
    printf("  总长度: %d\n", IP_HDR_LEN + 64);
    printf("  TTL: %d\n", 64);
    printf("  协议: %d (ICMP)\n", IP_PROTO_ICMP);
    printf("  源 IP: 192.168.1.100\n");
    printf("  目标 IP: 8.8.8.8\n");
}

/* 演示解析 IP 数据包 */
void demo_ip_unpack(void) {
    u_char packet[IP_HDR_LEN + 32];
    struct ip_hdr *ip = (struct ip_hdr *)packet;
    struct addr src, dst;
    char buf[128];

    printf("\n=== IP 数据包解析 ===\n");

    /* 构建测试包 */
    ip_pack_hdr(packet, 0, IP_HDR_LEN + 32,
                54321, 0, 128, IP_PROTO_TCP,
                inet_addr("10.0.0.1"), inet_addr("10.0.0.2"));

    /* 解析 */
    printf("版本: %d\n", ip->ip_v);
    printf("头部长度: %d bytes\n", ip->ip_hl << 2);
    printf("总长度: %d\n", ntohs(ip->ip_len));
    printf("TTL: %d\n", ip->ip_ttl);
    printf("协议: %d\n", ip->ip_p);

    /* 解析地址 */
    addr_pack(&src, ADDR_TYPE_IP, IP_ADDR_LEN,
              &ip->ip_src, IP_ADDR_LEN);
    addr_pack(&dst, ADDR_TYPE_IP, IP_ADDR_LEN,
              &ip->ip_dst, IP_ADDR_LEN);

    printf("源 IP: %s\n", addr_ntop(&src, buf, sizeof(buf)));
    printf("目标 IP: %s\n", addr_ntop(&dst, buf, sizeof(buf)));
}

/* 演示 IP 校验和计算 */
void demo_ip_checksum(void) {
    u_char packet[IP_HDR_LEN];

    printf("\n=== IP 校验和 ===\n");

    /* 构建 IP 头部 */
    ip_pack_hdr(packet, 0, IP_HDR_LEN + 20,
                9999, 0, 64, IP_PROTO_UDP,
                inet_addr("192.168.1.1"), inet_addr("192.168.1.2"));

    /* 计算校验和 */
    ip_checksum(packet, IP_HDR_LEN, 0);
    printf("IP 校验和已计算\n");
}

/* 演示不同 IP 协议 */
void demo_ip_protos(void) {
    printf("\n=== IP 协议类型 ===\n");

    printf("常用 IP 协议:\n");
    printf("  %3d - ICMP (Internet Control Message)\n", IP_PROTO_ICMP);
    printf("  %3d - IGMP (Internet Group Management)\n", IP_PROTO_IGMP);
    printf("  %3d - TCP (Transmission Control)\n", IP_PROTO_TCP);
    printf("  %3d - UDP (User Datagram)\n", IP_PROTO_UDP);
    printf("  %3d - GRE (Generic Routing Encapsulation)\n", IP_PROTO_GRE);
    printf("  %3d - ESP (Encapsulating Security Payload)\n", IP_PROTO_ESP);
    printf("  %3d - AH (Authentication Header)\n", IP_PROTO_AH);
}

/* 演示 IP 标志位 */
void demo_ip_flags(void) {
    printf("\n=== IP 标志位 ===\n");

    printf("DF (Don't Fragment): 0x4000\n");
    printf("  - 不允许分片\n");
    printf("MF (More Fragments): 0x2000\n");
    printf("  - 后续还有分片\n");
    printf("RF (Reserved): 0x8000\n");
    printf("  - 保留位\n");
}

/* 演示 IP 分片 */
void demo_ip_fragmentation(void) {
    int mtu = 1500;  /* 标准 MTU */
    int payload = 5000;  /* 大于 MTU 的数据 */

    printf("\n=== IP 分片计算 ===\n");

    printf("MTU: %d bytes\n", mtu);
    printf("IP 头部: %d bytes\n", IP_HDR_LEN);
    printf("最大 payload: %d bytes\n", mtu - IP_HDR_LEN);
    printf("\n%d bytes 数据需要分片:\n", payload);

    int max_payload = mtu - IP_HDR_LEN;
    int fragments = (payload + max_payload - 1) / max_payload;
    printf("  分片数量: %d\n", fragments);

    for (int i = 0; i < fragments; i++) {
        int offset = i * max_payload;
        int len = (i == fragments - 1) ? payload - offset : max_payload;
        printf("  分片 %d: 偏移 %d, 长度 %d%s\n",
               i + 1, offset, len,
               (i == fragments - 1) ? " (最后一个)" : "");
    }
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("libdnet IP 数据包操作示例\n");
    printf("======================\n");

    demo_ip_pack();
    demo_ip_unpack();
    demo_ip_checksum();
    demo_ip_protos();
    demo_ip_flags();
    demo_ip_fragmentation();

    printf("\n所有演示完成！\n");
    return 0;
}
