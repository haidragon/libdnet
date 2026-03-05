/*
 * libdnet Windows IP Packet Sending Example
 *
 * Features: Construct and send raw IP packets (TCP/UDP/ICMP support)
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o ip_send.exe ip_send.c -ldnet -lws2_32 -liphlpapi
 *
 * Run:
 * ip_send.exe <src_ip> <dst_ip> <protocol> [port]
 * 
 * Examples:
 * ip_send.exe 192.168.1.100 192.168.1.1 icmp
 * ip_send.exe 192.168.1.100 8.8.8.8 udp 53
 * ip_send.exe 192.168.1.100 93.184.216.34 tcp 80
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

/*
 * Calculate Internet checksum (RFC 1071)
 */
static uint16_t
in_cksum(const u_char *buf, size_t len)
{
    const u_short *w = (const u_short *)buf;
    uint32_t sum = 0;
    int nleft = (int)len;

    /* Accumulate 16-bit words */
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    /* Handle odd length */
    if (nleft == 1) {
        *(u_char *)(&sum) = *(const u_char *)w;
    }

    /* Fold 32-bit sum to 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

/*
 * Print IP packet information
 */
static void
print_ip_info(const u_char *ip, size_t len)
{
    struct ip_hdr *iph = (struct ip_hdr *)ip;
    
    printf("\nIP Packet Information:\n");
    printf("  Version: %u\n", iph->ip_v);
    printf("  Header Length: %u bytes\n", iph->ip_hl * 4);
    printf("  TOS: 0x%02X\n", iph->ip_tos);
    printf("  Total Length: %u bytes\n", ntohs(iph->ip_len));
    printf("  Identification: 0x%04X\n", ntohs(iph->ip_id));
    printf("  Flags: 0x%02X\n", (ntohs(iph->ip_off) >> 13) & 0x07);
    printf("  Fragment Offset: %u\n", (ntohs(iph->ip_off) & 0x1FFF) * 8);
    printf("  TTL: %u\n", iph->ip_ttl);
    printf("  Protocol: %u\n", iph->ip_p);
    printf("  Header Checksum: 0x%04X\n", ntohs(iph->ip_sum));
    
    /* Use temporary variables to store IP addresses */
    struct addr src_addr, dst_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    memset(&dst_addr, 0, sizeof(dst_addr));
    src_addr.addr_type = ADDR_TYPE_IP;
    dst_addr.addr_type = ADDR_TYPE_IP;
    src_addr.addr_ip = iph->ip_src;
    dst_addr.addr_ip = iph->ip_dst;
    
    printf("  Source: %s\n", addr_ntoa(&src_addr));
    printf("  Destination: %s\n", addr_ntoa(&dst_addr));
}

/*
 * Build ICMP Echo Request
 */
static int
build_icmp(u_char *pkt, size_t *len)
{
    struct icmp_hdr *icmph;
    struct icmp_msg_echo *echo;
    
    icmph = (struct icmp_hdr *)(pkt + 20); /* After IP header */
    memset(icmph, 0, 8);
    
    icmph->icmp_type = ICMP_ECHO;
    icmph->icmp_code = 0;
    icmph->icmp_cksum = 0;
    
    echo = (struct icmp_msg_echo *)(pkt + 20 + 4);
    echo->icmp_id = htons(0x1234);
    echo->icmp_seq = htons(1);
    
    /* Fill data */
    memset(pkt + 20 + 8, 'A', 56);
    *len = 20 + 8 + 56; /* IP header + ICMP header + data */
    
    /* Calculate checksum */
    icmph->icmp_cksum = in_cksum(pkt + 20, *len - 20);
    
    return 0;
}

/*
 * Build UDP packet
 */
static int
build_udp(u_char *pkt, size_t *len, uint16_t src_port, uint16_t dst_port)
{
    struct udp_hdr *udph;
    size_t udp_len;
    
    udph = (struct udp_hdr *)(pkt + 20);
    udp_len = 8 + 4; /* UDP header + data */
    
    udph->uh_sport = htons(src_port);
    udph->uh_dport = htons(dst_port);
    udph->uh_ulen = htons((uint16_t)udp_len);
    udph->uh_sum = 0; /* UDP checksum is optional in IPv4 */
    
    /* Fill data */
    memset(pkt + 20 + 8, 'U', 4);
    *len = 20 + udp_len;
    
    return 0;
}

/*
 * Build TCP SYN packet
 */
static int
build_tcp_syn(u_char *pkt, size_t *len, uint16_t src_port, uint16_t dst_port)
{
    struct tcp_hdr *tcph;
    u_char *tcp_opts;
    size_t tcp_opt_len;
    
    tcph = (struct tcp_hdr *)(pkt + 20);
    tcp_opt_len = 12; /* MSS + NOP + NOP + SACK Permitted + End */
    
    memset(tcph, 0, 20 + tcp_opt_len);
    tcph->th_sport = htons(src_port);
    tcph->th_dport = htons(dst_port);
    tcph->th_seq = htonl(0x12345678);
    tcph->th_off = (20 + tcp_opt_len) / 4;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htons(65535);
    tcph->th_sum = 0;
    
    /* TCP options */
    tcp_opts = (u_char *)(tcph + 1);
    tcp_opts[0] = TCP_OPT_MSS;
    tcp_opts[1] = TCP_OPT_MSS_LEN;
    tcp_opts[2] = 0x05;
    tcp_opts[3] = 0xB4;
    tcp_opts[4] = TCP_OPT_NOP;
    tcp_opts[5] = TCP_OPT_NOP;
    tcp_opts[6] = TCP_OPT_SACKOK;
    tcp_opts[7] = 4;
    tcp_opts[8] = TCP_OPT_NOP;
    tcp_opts[9] = TCP_OPT_NOP;
    tcp_opts[10] = TCP_OPT_EOL;
    
    *len = 20 + 20 + tcp_opt_len;
    
    return 0;
}

/*
 * Main function: send IP packets
 */
int main(int argc, char *argv[])
{
    ip_t *ip;
    struct addr src, dst;
    u_char pkt[1500];
    size_t pkt_len;
    struct ip_hdr *iph;
    const char *proto_str;
    uint16_t src_port = 12345, dst_port = 80;
    int proto;

    /* Check arguments */
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <src_ip> <dst_ip> <protocol> [port]\n", argv[0]);
        fprintf(stderr, "Protocol: icmp, udp, tcp\n");
        fprintf(stderr, "Example: %s 192.168.1.100 8.8.8.8 icmp\n", argv[0]);
        fprintf(stderr, "         %s 192.168.1.100 8.8.8.8 udp 53\n", argv[0]);
        fprintf(stderr, "         %s 192.168.1.100 93.184.216.34 tcp 80\n", argv[0]);
        return 1;
    }

    /* Parse source IP */
    if (addr_aton(argv[1], &src) < 0) {
        fprintf(stderr, "Error: invalid source IP address: %s\n", argv[1]);
        return 1;
    }

    /* Parse destination IP */
    if (addr_aton(argv[2], &dst) < 0) {
        fprintf(stderr, "Error: invalid destination IP address: %s\n", argv[2]);
        return 1;
    }

    /* Parse protocol */
    proto_str = argv[3];
    if (strcmp(proto_str, "icmp") == 0) {
        proto = IP_PROTO_ICMP;
    } else if (strcmp(proto_str, "udp") == 0) {
        proto = IP_PROTO_UDP;
        if (argc > 4)
            dst_port = atoi(argv[4]);
    } else if (strcmp(proto_str, "tcp") == 0) {
        proto = IP_PROTO_TCP;
        if (argc > 4)
            dst_port = atoi(argv[4]);
    } else {
        fprintf(stderr, "Error: unknown protocol type: %s\n", proto_str);
        return 1;
    }

    printf("=== libdnet IP Packet Sending Example ===\n\n");
    printf("Source IP:   %s\n", addr_ntoa(&src));
    printf("Destination: %s\n", addr_ntoa(&dst));
    printf("Protocol:    %s", proto_str);
    if (proto == IP_PROTO_UDP || proto == IP_PROTO_TCP)
        printf(" (Destination Port: %u)", dst_port);
    printf("\n\n");

    /* Open IP handle */
    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface\n");
        return 1;
    }
    printf("[OK] IP interface opened\n");

    /* Initialize IP header */
    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    
    iph->ip_v = 4;
    iph->ip_hl = 5; /* 20 bytes, no options */
    iph->ip_tos = 0;
    iph->ip_len = 0; /* Set later */
    iph->ip_id = htons(0x1234);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = (uint8_t)proto;
    iph->ip_sum = 0;
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    /* Build payload based on protocol */
    switch (proto) {
        case IP_PROTO_ICMP:
            printf("Building ICMP Echo Request...\n");
            build_icmp(pkt, &pkt_len);
            break;
        case IP_PROTO_UDP:
            printf("Building UDP packet...\n");
            build_udp(pkt, &pkt_len, src_port, dst_port);
            break;
        case IP_PROTO_TCP:
            printf("Building TCP SYN packet...\n");
            build_tcp_syn(pkt, &pkt_len, src_port, dst_port);
            break;
        default:
            fprintf(stderr, "Unsupported protocol\n");
            ip_close(ip);
            return 1;
    }

    /* Set IP total length */
    iph->ip_len = htons((uint16_t)pkt_len);
    
    /* Calculate IP header checksum */
    iph->ip_sum = in_cksum(pkt, 20);

    /* Print IP packet info */
    print_ip_info(pkt, pkt_len);

    /* Send IP packet */
    printf("\nSending IP packet...\n");
    if (ip_send(ip, pkt, pkt_len) < 0) {
        fprintf(stderr, "Error: send failed\n");
        ip_close(ip);
        return 1;
    }
    printf("[OK] Sent successfully!\n");

    /* Close IP interface */
    ip_close(ip);

    printf("\n=== Done ===\n");
    return 0;
}
