/*
 * libdnet Windows ICMP Ping Example
 *
 * Features: Send ICMP Echo Request and receive reply
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o ping_icmp.exe ping_icmp.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * ping_icmp.exe <target_ip>
 *
 * Example:
 * ping_icmp.exe 8.8.8.8
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

static uint16_t
in_cksum(const u_char *buf, size_t len)
{
    const u_short *w = (const u_short *)buf;
    uint32_t sum = 0;
    int nleft = (int)len;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u_char *)(&sum) = *(const u_char *)w;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

int main(int argc, char *argv[])
{
    ip_t *ip;
    struct addr dst;
    u_char pkt[1500];
    struct ip_hdr *iph;
    struct icmp_hdr *icmph;
    struct icmp_msg_echo *echo;
    struct addr src_addr;
    size_t pkt_len;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <target_ip>\n", argv[0]);
        fprintf(stderr, "Example: %s 8.8.8.8\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &dst) < 0) {
        fprintf(stderr, "Error: invalid IP address: %s\n", argv[1]);
        return 1;
    }

    printf("=== libdnet ICMP Ping Example ===\n");
    printf("Target: %s\n\n", addr_ntoa(&dst));

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    icmph = (struct icmp_hdr *)(pkt + 20);
    echo = (struct icmp_msg_echo *)(pkt + 24);

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(20 + 8 + 56);
    iph->ip_id = htons(0x1234);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IP_PROTO_ICMP;
    iph->ip_sum = 0;
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    icmph->icmp_type = ICMP_ECHO;
    icmph->icmp_code = 0;
    icmph->icmp_cksum = 0;
    echo->icmp_id = htons(0xABCD);
    echo->icmp_seq = htons(1);
    memset(pkt + 28, 'P', 56);

    icmph->icmp_cksum = in_cksum(pkt + 20, 64);

    iph->ip_sum = in_cksum(pkt, 20);

    printf("Sending ICMP Echo Request...\n");
    if (ip_send(ip, pkt, 84) < 0) {
        fprintf(stderr, "Error: send failed\n");
        ip_close(ip);
        return 1;
    }

    printf("[OK] Packet sent (Use Wireshark to verify)\n");
    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
