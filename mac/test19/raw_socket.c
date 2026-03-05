#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dnet.h>

int main() {
    ip_t *ip;
    struct ip_hdr iph;
    uint8_t buf[65536];
    struct addr src, dst;
    char src_str[32], dst_str[32];
    
    printf("=== libdnet 原始 IP Socket 示例 ===\n");
    printf("注意: 此示例需要 root 权限\n\n");
    
    // 构建源和目的地址
    addr_pton("192.168.1.100", &src);
    addr_pton("8.8.8.8", &dst);
    
    addr_ntop(&src, src_str, sizeof(src_str));
    addr_ntop(&dst, dst_str, sizeof(dst_str));
    
    printf("1. 构建自定义 IP 数据包:\n");
    printf("   ================================================\n\n");
    
    printf("   源地址: %s\n", src_str);
    printf("   目的地址: %s\n\n", dst_str);
    
    // 构建IP头部
    memset(&iph, 0, sizeof(iph));
    iph.ip_hl = 5;                    // IP 头部长度（5 * 4 = 20 字节）
    iph.ip_v = 4;                     // IP 版本 4
    iph.ip_tos = 0;                   // 服务类型
    iph.ip_len = htons(sizeof(iph));  // 总长度
    iph.ip_id = htons(12345);         // 标识
    iph.ip_off = 0;                   // 片偏移
    iph.ip_ttl = 64;                  // 生存时间
    iph.ip_p = IPPROTO_ICMP;          // 协议（ICMP）
    
    // 复制地址到 IP 头部
    memcpy(&iph.ip_src, &src.addr_ip, 4);
    memcpy(&iph.ip_dst, &dst.addr_ip, 4);
    
    printf("2. IP 头部信息:\n");
    printf("   ================================================\n");
    printf("   版本: %d\n", iph.ip_v);
    printf("   头部长度: %d (%d 字节)\n", iph.ip_hl, iph.ip_hl * 4);
    printf("   服务类型: %d\n", iph.ip_tos);
    printf("   总长度: %d\n", ntohs(iph.ip_len));
    printf("   标识: %d\n", ntohs(iph.ip_id));
    printf("   片偏移: %d\n", ntohs(iph.ip_off));
    printf("   TTL: %d\n", iph.ip_ttl);
    printf("   协议: %d (ICMP)\n", iph.ip_p);
    
    // 计算校验和
    printf("\n3. 校验和计算:\n");
    printf("   ================================================\n");
    iph.ip_sum = ip_cksum_add(&iph, sizeof(iph), 0);
    iph.ip_sum = ip_cksum_carry(iph.ip_sum);
    printf("   IP 校验和: 0x%04X\n", iph.ip_sum);
    
    // 复制到缓冲区
    memcpy(buf, &iph, sizeof(iph));
    
    // 打印十六进制
    printf("\n4. IP 数据包十六进制:\n");
    printf("   ================================================\n");
    for (size_t i = 0; i < sizeof(iph); i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n   ");
    }
    printf("\n");
    
    // 尝试打开原始 IP socket（需要 root 权限）
    printf("\n5. 尝试打开原始 IP socket:\n");
    printf("   ================================================\n");
    
    ip = ip_open();
    if (ip == NULL) {
        printf("   无法打开原始 IP socket\n");
        printf("   这需要 root 权限\n");
        printf("   尝试使用: sudo ./raw_socket\n\n");
    } else {
        printf("   成功打开原始 IP socket\n");
        printf("   (实际发送需要正确的源地址)\n");
        ip_close(ip);
    }
    
    printf("6. 提示:\n");
    printf("   ================================================\n");
    printf("   - 原始 socket 需要管理员权限\n");
    printf("   - 使用 ip_send() 发送数据包\n");
    printf("   - 使用 ip_recv() 接收数据包\n");
    printf("   - 发送数据包时源地址必须有效\n");
    printf("   - 谨慎使用原始 socket，避免网络问题\n");
    
    printf("\n=== 原始 IP Socket 示例完成 ===\n");
    return 0;
}
