#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

void print_buffer(const char *label, const uint8_t *buf, size_t len) {
    printf("  %s (%zu 字节):\n", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n  ");
    }
    if (len > 0 && len % 16 != 0) printf("\n");
}

int main() {
    uint8_t packet[128];
    uint8_t options[64];
    struct ip_hdr *iph;
    
    printf("=== libdnet IP 选项处理示例 ===\n\n");
    
    printf("1. 基本 IP 头部结构:\n");
    printf("   ================================================\n");
    printf("   IP 头部长度: 最小 20 字节 (IHL=5)\n");
    printf("   IP 选项: 最多 40 字节 (IHL最大15)\n\n");
    
    printf("2. 构建 IP 数据包（无选项）:\n");
    printf("   ================================================\n\n");
    
    memset(packet, 0, sizeof(packet));
    iph = (struct ip_hdr *)packet;
    
    // 标准 IP 头部（20 字节）
    iph->ip_hl = 5;                    // 头部长度 5 * 4 = 20 字节
    iph->ip_v = 4;                     // IP 版本 4
    iph->ip_tos = 0;
    iph->ip_len = htons(20);          // 仅头部
    iph->ip_id = htons(1000);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IPPROTO_TCP;
    
    // 设置源和目的地址
    struct addr src, dst;
    addr_pton("192.168.1.100", &src);
    addr_pton("8.8.8.8", &dst);
    
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);
    
    // 计算校验和
    iph->ip_sum = ip_cksum_add(iph, 20, 0);
    iph->ip_sum = ip_cksum_carry(iph->ip_sum);
    
    printf("   版本: %d\n", iph->ip_v);
    printf("   头部长度: %d (%d 字节)\n", iph->ip_hl, iph->ip_hl * 4);
    printf("   TTL: %d\n", iph->ip_ttl);
    printf("   协议: %d (TCP)\n", iph->ip_p);
    printf("   校验和: 0x%04X\n\n", iph->ip_sum);
    
    print_buffer("IP 数据包", packet, 20);
    
    printf("\n3. IP 选项格式:\n");
    printf("   ================================================\n");
    printf("   选项格式: [类型] [长度] [数据]\n\n");
    
    printf("   常见选项类型:\n");
    printf("   - 7 (0x07): Record Route (记录路由)\n");
    printf("   - 68 (0x44): Time Stamp (时间戳)\n");
    printf("   - 137 (0x89): MTU Probe (MTU探测)\n\n");
    
    printf("4. 构建 Record Route 选项示例:\n");
    printf("   ================================================\n\n");
    
    memset(options, 0, sizeof(options));
    
    // Record Route 选项格式
    // 类型(1) + 长度(1) + 指针(1) + 路由地址(最多9个，每个4字节) = 39字节
    options[0] = 7;           // Record Route 选项类型
    options[1] = 39;          // 选项总长度
    options[2] = 4;           // 指针（指向下一个可用位置）
    
    // 剩余空间用于存储路由地址（初始化为0）
    
    printf("   选项类型: %d (Record Route)\n", options[0]);
    printf("   选项长度: %d 字节\n", options[1]);
    printf("   指针位置: %d\n", options[2]);
    printf("   最多记录: %d 个地址\n", (options[1] - 3) / 4);
    
    print_buffer("Record Route 选项", options, options[1]);
    
    printf("\n5. 构建 Time Stamp 选项示例:\n");
    printf("   ================================================\n\n");
    
    memset(options, 0, sizeof(options));
    
    // Time Stamp 选项格式
    options[0] = 68;          // Time Stamp 选项类型
    options[1] = 40;          // 选项长度
    options[2] = 5;           // 指针
    options[3] = 1;           // 标志：仅时间戳
    
    printf("   选项类型: %d (Time Stamp)\n", options[0]);
    printf("   选项长度: %d 字节\n", options[1]);
    printf("   标志: %d\n", options[3]);
    
    print_buffer("Time Stamp 选项", options, options[1]);
    
    printf("\n6. 带 Record Route 选项的 IP 数据包:\n");
    printf("   ================================================\n\n");
    
    // 构建带选项的 IP 头部
    memset(packet, 0, sizeof(packet));
    iph = (struct ip_hdr *)packet;
    
    iph->ip_hl = 6;              // 头部长度 6 * 4 = 24 字节（20头部 + 4选项）
    iph->ip_v = 4;
    iph->ip_tos = 0;
    iph->ip_len = htons(24);    // 头部20 + 选项4
    iph->ip_id = htons(1001);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IPPROTO_UDP;
    
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);
    
    // 添加选项（NOP + Record Route）
    uint8_t *opts = (uint8_t *)(packet + 20);
    opts[0] = 1;           // NOP (No Operation)
    opts[1] = 7;           // Record Route
    opts[2] = 3;           // 最小长度
    opts[3] = 4;           // 指针
    
    // 计算校验和（包括选项）
    iph->ip_sum = ip_cksum_add(iph, 24, 0);
    iph->ip_sum = ip_cksum_carry(iph->ip_sum);
    
    printf("   头部长度: %d (%d 字节)\n", iph->ip_hl, iph->ip_hl * 4);
    printf("   选项: NOP + Record Route\n");
    printf("   校验和: 0x%04X\n\n", iph->ip_sum);
    
    print_buffer("带选项的 IP 数据包", packet, 24);
    
    printf("\n7. 提示:\n");
    printf("   ================================================\n");
    printf("   - IP 选项会增加路由器处理负担\n");
    printf("   - 现代网络中很少使用 IP 选项\n");
    printf("   - 部分路由器可能会丢弃带选项的包\n");
    printf("   - Record Route 最多记录 9 个地址\n");
    
    printf("\n=== IP 选项处理示例完成 ===\n");
    return 0;
}
