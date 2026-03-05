/*
 * libdnet Windows UDP Flood Example
 *
 * Features: Send multiple UDP packets rapidly
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o udp_flood.exe udp_flood.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * udp_flood.exe <src_ip> <dst_ip> <port> <count>
 *
 * Example:
 * udp_flood.exe 192.168.1.100 192.168.1.1 53 100
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
    uint16_t dst_port, src_port;
    int count, i;
    size_t pkt_len;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s <src_ip> <dst_ip> <port> <count>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 192.168.1.1 53 100\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &src) < 0 || addr_aton(argv[2], &dst) < 0) {
        fprintf(stderr, "Error: invalid IP address\n");
        return 1;
    }

    dst_port = atoi(argv[3]);
    count = atoi(argv[4]);

    printf("=== libdnet UDP Flood Example ===\n");
    printf("Source: %s\n", addr_ntoa(&src));
    printf("Destination: %s:%d\n", addr_ntoa(&dst), dst_port);
    printf("Packet count: %d\n\n", count);

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    udph = (struct udp_hdr *)(pkt + 20);

    pkt_len = 28 + 32;
    memset(pkt + 28, 'U', 32);

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(pkt_len);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IP_PROTO_UDP;
    iph->ip_sum = 0;
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    udph->uh_ulen = htons(8 + 32);
    udph->uh_sum = 0;

    printf("Sending UDP packets...\n");
    for (i = 0; i < count; i++) {
        src_port = 10000 + (i % 10000);
        iph->ip_id = htons(i);
        udph->uh_sport = htons(src_port);
        udph->uh_dport = htons(dst_port);

        iph->ip_sum = in_cksum(pkt, 20);

        if (ip_send(ip, pkt, pkt_len) < 0) {
            printf("Packet %d: send failed\n", i + 1);
        } else {
            if ((i + 1) % 10 == 0 || i == count - 1) {
                printf("Sent %d/%d packets...\n", i + 1, count);
            }
        }
    }

    printf("[OK] UDP flood completed\n");
    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
