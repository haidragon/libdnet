#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define IPPROTO_ICMP 1
    #define ICMP_ECHO 8
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #define ICMP_ECHO 8
#endif

// 必须先包含 dnet.h，它会定义所有基础类型
#include <dnet.h>

// 自定义 ICMP 头部（避免兼容性问题）
struct icmp_hdr_custom {
    uint8_t  icmp_type;
    uint8_t  icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_id;
    uint16_t icmp_seq;
};

// 校验和计算函数 (RFC 1071)
static uint16_t in_cksum(uint16_t *ptr, int len)
{
    uint32_t sum = 0;
    uint16_t answer = 0;

    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1)
    {
        *(uint8_t *)&answer = *(uint8_t *)ptr;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

void print_mac(const u_char *mac)
{
    if (!mac)
        return;
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int main()
{
    intf_t *intf_hdl;
    arp_t *arp_hdl;
    ip_t *ip_hdl;
    struct addr src_ip, dst_ip;
    struct eth_addr src_mac, dst_mac;
    u_char packet[1500];
    struct ip_hdr *ip;
    struct icmp_hdr_custom *icmp;
    struct intf_entry entry;
    int len;
    uint16_t *cksum_ptr;

    printf("=== Libdnet Test Program ===\n");

    // 1. 打开接口
    if ((intf_hdl = intf_open()) == NULL)
    {
        perror("intf_open");
        return 1;
    }

    // 2. 设置目标 IP
    if (addr_pton("8.8.8.8", &dst_ip) < 0)
    {
        fprintf(stderr, "Invalid IP\n");
        intf_close(intf_hdl);
        return 1;
    }

    // 3. 查找路由
    memset(&entry, 0, sizeof(entry));
    entry.intf_len = sizeof(entry);

    printf("\n[1] 查找路由...\n");

    // Windows 下 intf_get_dst 未实现，使用替代方案
#ifdef _WIN32
    // Windows: 获取默认接口
    if (intf_get(intf_hdl, &entry) < 0)
    {
        fprintf(stderr, "Failed to get interface\n");
        intf_close(intf_hdl);
        return 1;
    }
    printf("Interface: %s\n", entry.intf_name);
    printf("Local IP : %s\n", addr_ntoa(&entry.intf_addr));
#else
    if (intf_get_dst(intf_hdl, &entry, &dst_ip) < 0)
    {
        fprintf(stderr, "No route to host or permission denied (try sudo)\n");
        intf_close(intf_hdl);
        return 1;
    }
    printf("Interface: %s\n", entry.intf_name);
    printf("Local IP : %s\n", addr_ntoa(&entry.intf_addr));
#endif

    // 获取源 MAC
#ifdef _WIN32
    memcpy(&src_mac, &entry.intf_link_addr.addr_eth, ETH_ADDR_LEN);
    printf("Local MAC: ");
    print_mac(src_mac.data);
    printf("\n");
#else
    memcpy(&src_mac, &entry.intf_link_addr.addr_eth, sizeof(struct eth_addr));
    printf("Local MAC: ");
    print_mac(src_mac.data);
    printf("\n");
#endif

    src_ip = entry.intf_addr;

    // 4. ARP 解析
    printf("\n[2] ARP 解析...\n");
    if ((arp_hdl = arp_open()) == NULL)
    {
        perror("arp_open");
        intf_close(intf_hdl);
        return 1;
    }

    // ARP 解析（需要 root 权限）
#ifdef _WIN32
    printf("Windows platform: ARP resolve simplified\n");
    memset(&dst_mac, 0, sizeof(dst_mac));
#else
    struct arp_entry arp_entry;
    memset(&arp_entry, 0, sizeof(arp_entry));
    memcpy(&arp_entry.arp_pa, &dst_ip, sizeof(struct addr));

    if (arp_get(arp_hdl, &arp_entry) < 0)
    {
        fprintf(stderr, "ARP resolve failed\n");
        arp_close(arp_hdl);
        intf_close(intf_hdl);
        return 1;
    }
    memcpy(&dst_mac, &arp_entry.arp_ha.addr_eth, ETH_ADDR_LEN);
    printf("Target MAC: ");
    print_mac(dst_mac.data);
    printf("\n");
#endif
    arp_close(arp_hdl);

    // 5. 构造包
    printf("\n[3] 构造数据包...\n");
    memset(packet, 0, sizeof(packet));

    // --- IP Header ---
    ip = (struct ip_hdr *)packet;
    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_tos = 0;
    len = sizeof(struct ip_hdr) + sizeof(struct icmp_hdr_custom) + 8;
    ip->ip_len = htons(len);
    ip->ip_id = htons(0x1234);
    ip->ip_off = 0;
    ip->ip_ttl = 64;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_sum = 0;

    // 填充 IP 地址
#ifdef _WIN32
    ip->ip_src = src_ip.addr_ip;
    ip->ip_dst = dst_ip.addr_ip;
#else
    // 使用宏访问 IP 地址
    ip->ip_src = src_ip.addr_ip;
    ip->ip_dst = dst_ip.addr_ip;
#endif

    // 计算 IP 校验和
    ip->ip_sum = in_cksum((uint16_t *)packet, sizeof(struct ip_hdr));

    // --- ICMP Header ---
    icmp = (struct icmp_hdr_custom *)(packet + sizeof(struct ip_hdr));
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_id = htons(12345);
    icmp->icmp_seq = htons(1);

    // Payload
    u_char *payload = (u_char *)(icmp + 1);
    memcpy(payload, "LINUXLIB", 8);

    // 计算 ICMP 校验和
    icmp->icmp_cksum = in_cksum((uint16_t *)icmp, sizeof(struct icmp_hdr_custom) + 8);

    // 6. 发送
    printf("\n[4] 发送...\n");
    if ((ip_hdl = ip_open()) == NULL)
    {
        perror("ip_open");
        intf_close(intf_hdl);
        return 1;
    }

    if (ip_send(ip_hdl, packet, len) < 0)
    {
        perror("ip_send");
    }
    else
    {
        printf("Sent %d bytes to %s\n", len, addr_ntoa(&dst_ip));
#ifdef _WIN32
        printf("Check with: Wireshark or similar tool\n");
#else
        printf("Check with: tcpdump -i %s icmp\n", entry.intf_name);
#endif
    }

    ip_close(ip_hdl);
    intf_close(intf_hdl);

    return 0;
}