#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

// 打印十六进制数据
void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main() {
    printf("=== libdnet 校验和计算示例 ===\n\n");

    // 1. IP 校验和计算
    printf("1. IP 校验和计算:\n");
    {
        struct ip_hdr ip;
        uint16_t sum, checksum;

        memset(&ip, 0, sizeof(ip));
        ip.ip_hl = 5;
        ip.ip_v = 4;
        ip.ip_tos = 0;
        ip.ip_len = htons(20);  // IP头长度
        ip.ip_id = htons(12345);
        ip.ip_off = 0;
        ip.ip_ttl = 64;
        ip.ip_p = IPPROTO_TCP;

        // 设置IP地址（直接设置32位值）
        ip.ip_src = htonl(0xC0A80164);  // 192.168.1.100
        ip.ip_dst = htonl(0x5DB8D822);  // 93.184.216.34
        ip.ip_sum = 0;

        printf("   IP 头 (校验和字段为0):\n");
        print_hex((uint8_t *)&ip, 20);

        // 计算 IP 校验和 (使用 libdnet 的两个步骤函数)
        sum = ip_cksum_add(&ip, sizeof(ip), 0);
        checksum = ip_cksum_carry(sum);
        ip.ip_sum = checksum;

        printf("   计算的校验和: 0x%04X\n", checksum);
        printf("   带校验和的 IP 头:\n");
        print_hex((uint8_t *)&ip, 20);

        // 验证校验和
        sum = ip_cksum_add(&ip, sizeof(ip), 0);
        uint16_t verify = ip_cksum_carry(sum);
        printf("   验证校验和 (应为0): 0x%04X %s\n",
               verify, verify == 0 ? "✓" : "✗");
    }

    // 2. TCP 校验和计算（带伪首部）
    printf("\n2. TCP 校验和计算:\n");
    {
        struct tcp_hdr tcp;
        uint16_t sum, checksum;

        // 准备伪首部
        struct {
            uint32_t src;
            uint32_t dst;
            uint8_t zero;
            uint8_t proto;
            uint16_t len;
        } ph;

        ph.src = htonl(0xC0A80164);  // 192.168.1.100
        ph.dst = htonl(0x5DB8D822);  // 93.184.216.34
        ph.zero = 0;
        ph.proto = IPPROTO_TCP;
        ph.len = htons(20);  // TCP 头长度

        printf("   伪首部:\n");
        print_hex((uint8_t *)&ph, sizeof(ph));

        // 准备 TCP 头
        memset(&tcp, 0, sizeof(tcp));
        tcp.th_sport = htons(54321);
        tcp.th_dport = htons(80);
        tcp.th_seq = htonl(1001);
        tcp.th_ack = htonl(0);
        tcp.th_off = 5;
        tcp.th_flags = TH_SYN;
        tcp.th_win = htons(65535);
        tcp.th_sum = 0;  // 计算前设为0

        printf("   TCP 头 (校验和字段为0):\n");
        print_hex((uint8_t *)&tcp, 20);

        // 计算 TCP 校验和 (伪首部 + TCP 头)
        uint8_t combined[sizeof(ph) + sizeof(tcp)];
        memcpy(combined, &ph, sizeof(ph));
        memcpy(combined + sizeof(ph), &tcp, sizeof(tcp));

        sum = ip_cksum_add(combined, sizeof(combined), 0);
        checksum = ip_cksum_carry(sum);
        tcp.th_sum = checksum;

        printf("   计算的 TCP 校验和: 0x%04X\n", checksum);
    }

    // 3. 通用校验和计算
    printf("\n3. 通用数据校验和计算:\n");
    {
        uint8_t data1[] = "Hello, World!";
        uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        uint16_t sum1, sum2;

        sum1 = ip_cksum_add(data1, strlen((char *)data1), 0);
        sum1 = ip_cksum_carry(sum1);

        sum2 = ip_cksum_add(data2, sizeof(data2), 0);
        sum2 = ip_cksum_carry(sum2);

        printf("   数据1: \"%s\"\n", data1);
        printf("   数据1 校验和: 0x%04X\n", sum1);

        printf("   数据2: ");
        for (size_t i = 0; i < sizeof(data2); i++) {
            printf("%02X ", data2[i]);
        }
        printf("\n");
        printf("   数据2 校验和: 0x%04X\n", sum2);

        printf("\n   注意: libdnet 使用 ip_cksum_add 和 ip_cksum_carry\n");
    }

    // 4. UDP 校验和计算
    printf("\n4. UDP 校验和计算:\n");
    {
        struct {
            uint32_t src;
            uint32_t dst;
            uint8_t zero;
            uint8_t proto;
            uint16_t len;
        } udp_pseudo;

        uint8_t udp_data[] = "Test UDP data";

        udp_pseudo.src = htonl(0xC0A80164);
        udp_pseudo.dst = htonl(0x5DB8D822);
        udp_pseudo.zero = 0;
        udp_pseudo.proto = IPPROTO_UDP;
        udp_pseudo.len = htons(8 + sizeof(udp_data));

        printf("   UDP 伪首部:\n");
        print_hex((uint8_t *)&udp_pseudo, sizeof(udp_pseudo));

        printf("   UDP 数据: \"%s\"\n", udp_data);
        print_hex(udp_data, sizeof(udp_data));

        // 计算 UDP 校验和
        uint16_t sum, udp_checksum;
        uint8_t combined[sizeof(udp_pseudo) + 8 + sizeof(udp_data)];
        memcpy(combined, &udp_pseudo, sizeof(udp_pseudo));

        // UDP 头 (简化)
        combined[sizeof(udp_pseudo) + 0] = 0x13;  // 端口
        combined[sizeof(udp_pseudo) + 1] = 0x88;
        combined[sizeof(udp_pseudo) + 2] = 0x04;  // 端口
        combined[sizeof(udp_pseudo) + 3] = 0xD2;
        combined[sizeof(udp_pseudo) + 4] = 0x00;  // 长度
        combined[sizeof(udp_pseudo) + 5] = 0x11;
        combined[sizeof(udp_pseudo) + 6] = 0x00;  // 校验和
        combined[sizeof(udp_pseudo) + 7] = 0x00;

        memcpy(combined + sizeof(udp_pseudo) + 8, udp_data, sizeof(udp_data));

        sum = ip_cksum_add(combined, sizeof(combined), 0);
        udp_checksum = ip_cksum_carry(sum);

        printf("   UDP 校验和: 0x%04X\n", udp_checksum);
    }

    // 5. ICMP 校验和计算
    printf("\n5. ICMP 校验和计算:\n");
    {
        struct {
            struct icmp_hdr hdr;
            struct icmp_msg_echo echo;
        } icmp;
        uint16_t sum, calc_checksum;

        memset(&icmp, 0, sizeof(icmp));
        icmp.hdr.icmp_type = ICMP_ECHO;
        icmp.hdr.icmp_code = 0;
        icmp.echo.icmp_id = htons(12345);
        icmp.echo.icmp_seq = htons(1);
        icmp.hdr.icmp_cksum = 0;

        printf("   ICMP 头 (校验和字段为0):\n");
        print_hex((uint8_t *)&icmp, 8);

        sum = ip_cksum_add(&icmp, 8, 0);
        calc_checksum = ip_cksum_carry(sum);
        icmp.hdr.icmp_cksum = calc_checksum;

        printf("   计算的 ICMP 校验和: 0x%04X\n", calc_checksum);

        sum = ip_cksum_add(&icmp, 8, 0);
        uint16_t verify = ip_cksum_carry(sum);
        printf("   验证: 0x%04X\n", verify);
    }

    printf("\n=== 校验和计算示例完成 ===\n");
    return 0;
}
