#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dnet.h>

int main() {
    eth_t *eth;
    struct intf_entry entry;
    intf_t *intf;
    struct eth_hdr ethh;
    struct ip_hdr iph;
    uint8_t buf[65536];
    char ifname[16] = "en0";
    char src_mac[32], src_ip[32];
    int len = 0;
    
    printf("=== libdnet 数据包注入示例 ===\n");
    printf("注意: 此示例需要 root 权限\n\n");
    
    // 打开接口操作
    intf = intf_open();
    if (intf != NULL) {
        memset(&entry, 0, sizeof(entry));
        entry.intf_len = sizeof(entry);
        
        printf("1. 获取接口信息:\n");
        printf("   ================================================\n");
        printf("   接口: %s\n", ifname);
        printf("   MTU: %d\n", entry.intf_mtu);
        
        // 获取接口信息（libdnet API可能需要设置名称）
        // 由于intf_get参数差异，使用默认值演示
        
        // 使用示例MAC和IP
        printf("   MAC: [需要实际查询]\n");
        printf("   IP: [需要实际查询]\n\n");
        
        intf_close(intf);
    }
    
    printf("2. 构建以太网帧:\n");
    printf("   ================================================\n\n");
    
    // 构建以太网头部 - 使用正确的libdnet API
    memset(&ethh, 0, sizeof(ethh));
    
    // 目标 MAC（广播地址）
    ethh.eth_dst.data[0] = 0xff;
    ethh.eth_dst.data[1] = 0xff;
    ethh.eth_dst.data[2] = 0xff;
    ethh.eth_dst.data[3] = 0xff;
    ethh.eth_dst.data[4] = 0xff;
    ethh.eth_dst.data[5] = 0xff;
    
    // 源 MAC（需要替换为实际的）
    memset(&ethh.eth_src, 0, sizeof(ethh.eth_src));
    
    ethh.eth_type = htons(ETH_TYPE_IP);
    
    printf("   目的 MAC: ff:ff:ff:ff:ff:ff (广播)\n");
    printf("   源 MAC: [需要实际接口地址]\n");
    printf("   以太网类型: 0x%04X (IPv4)\n\n", ntohs(ethh.eth_type));
    
    // 复制以太网头部到缓冲区
    memcpy(buf, &ethh, sizeof(ethh));
    len = sizeof(ethh);
    
    printf("3. 构建 IP 数据包:\n");
    printf("   ================================================\n\n");
    
    // 构建 IP 头部
    memset(&iph, 0, sizeof(iph));
    iph.ip_hl = 5;
    iph.ip_v = 4;
    iph.ip_tos = 0;
    iph.ip_len = htons(sizeof(iph));
    iph.ip_id = htons(54321);
    iph.ip_off = 0;
    iph.ip_ttl = 64;
    iph.ip_p = IPPROTO_UDP;
    
    // 源和目的地址（示例）
    struct addr src, dst;
    addr_pton("0.0.0.0", &src);
    addr_pton("255.255.255.255", &dst);
    
    addr_ntop(&src, src_ip, sizeof(src_ip));
    
    printf("   源 IP: %s\n", src_ip);
    printf("   目的 IP: 255.255.255.255\n");
    printf("   协议: UDP\n");
    printf("   TTL: %d\n", iph.ip_ttl);
    
    // 计算校验和
    iph.ip_sum = ip_cksum_add(&iph, sizeof(iph), 0);
    iph.ip_sum = ip_cksum_carry(iph.ip_sum);
    printf("   校验和: 0x%04X\n\n", iph.ip_sum);
    
    // 复制 IP 头部到缓冲区
    memcpy(buf + len, &iph, sizeof(iph));
    len += sizeof(iph);
    
    printf("4. 数据包信息:\n");
    printf("   ================================================\n");
    printf("   总长度: %d 字节\n", len);
    printf("   以太网头: %zu 字节\n", sizeof(ethh));
    printf("   IP 头: %zu 字节\n", sizeof(iph));
    
    // 打印十六进制
    printf("\n5. 数据包十六进制:\n");
    printf("   ================================================\n");
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n   ");
    }
    printf("\n");
    
    printf("\n6. 尝试发送数据包:\n");
    printf("   ================================================\n");
    
    // 尝试打开以太网设备
    eth = eth_open(ifname);
    if (eth == NULL) {
        printf("   无法打开以太网设备 %s\n", ifname);
        printf("   这需要 root 权限\n");
        printf("   尝试使用: sudo ./packet_injection\n\n");
    } else {
        printf("   成功打开以太网设备\n");
        printf("   (实际注入需要正确的源 MAC 和 IP)\n");
        eth_close(eth);
    }
    
    printf("7. 提示:\n");
    printf("   ================================================\n");
    printf("   - 数据包注入需要管理员权限\n");
    printf("   - 使用 eth_send() 发送以太网帧\n");
    printf("   - 源 MAC 和 IP 必须是有效的接口地址\n");
    printf("   - 谨慎使用，避免网络干扰\n");
    printf("   - 仅用于学习和测试目的\n");
    
    printf("\n=== 数据包注入示例完成 ===\n");
    return 0;
}
