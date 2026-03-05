/*
 * libdnet Windows Traceroute Example
 *
 * Features: Send packets with increasing TTL values
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o traceroute.exe traceroute.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * traceroute.exe <src_ip> <dst_ip>
 *
 * Example:
 * traceroute.exe 192.168.1.100 8.8.8.8
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
    struct addr src, dst;
    u_char pkt[1500];
    struct ip_hdr *iph;
    struct udp_hdr *udph;
    int i, max_ttl;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <src_ip> <dst_ip> [max_ttl]\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 8.8.8.8 30\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &src) < 0 || addr_aton(argv[2], &dst) < 0) {
        fprintf(stderr, "Error: invalid IP address\n");
        return 1;
    }

    max_ttl = (argc > 3) ? atoi(argv[3]) : 30;

    printf("=== libdnet Traceroute Example ===\n");
    printf("Source: %s\n", addr_ntoa(&src));
    printf("Destination: %s\n", addr_ntoa(&dst));
    printf("Max TTL: %d\n\n", max_ttl);
    printf("Sending packets with increasing TTL values...\n");
    printf("(Use Wireshark to capture ICMP Time Exceeded responses)\n\n");

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    udph = (struct udp_hdr *)(pkt + 20);

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(28 + 12);
    iph->ip_off = 0;
    iph->ip_p = IP_PROTO_UDP;
    iph->ip_sum = 0;
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    udph->uh_sport = htons(54321);
    udph->uh_dport = htons(33434);
    udph->uh_ulen = htons(20);
    udph->uh_sum = 0;

    memset(pkt + 28, 'T', 12);

    for (i = 1; i <= max_ttl; i++) {
        iph->ip_id = htons(i);
        iph->ip_ttl = i;

        udph->uh_dport = htons(33434 + i);

        iph->ip_sum = in_cksum(pkt, 20);

        printf("TTL %2d: ", i);
        if (ip_send(ip, pkt, 40) < 0) {
            printf("send failed\n");
        } else {
            printf("packet sent (port %d)\n", 33434 + i);
        }
    }

    printf("\n[OK] Traceroute packets sent\n");
    printf("Check Wireshark for ICMP Time Exceeded messages from routers\n");
    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
