/*
 * libdnet Windows TCP Port Scan Example
 *
 * Features: Send TCP SYN packets to scan ports
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o port_scan.exe port_scan.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * port_scan.exe <target_ip> <port_start> <port_end>
 *
 * Example:
 * port_scan.exe 192.168.1.100 20 100
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
    struct tcp_hdr *tcph;
    size_t pkt_len;
    int port_start, port_end, port;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <target_ip> <port_start> <port_end>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 20 100\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &dst) < 0) {
        fprintf(stderr, "Error: invalid IP address: %s\n", argv[1]);
        return 1;
    }

    port_start = atoi(argv[2]);
    port_end = atoi(argv[3]);

    printf("=== libdnet TCP Port Scan Example ===\n");
    printf("Target: %s\n", addr_ntoa(&dst));
    printf("Port range: %d - %d\n\n", port_start, port_end);

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    for (port = port_start; port <= port_end; port++) {
        memset(pkt, 0, sizeof(pkt));
        iph = (struct ip_hdr *)pkt;
        tcph = (struct tcp_hdr *)(pkt + 20);

        iph->ip_v = 4;
        iph->ip_hl = 5;
        iph->ip_tos = 0;
        iph->ip_len = htons(40);
        iph->ip_id = htons(0x1234);
        iph->ip_off = 0;
        iph->ip_ttl = 64;
        iph->ip_p = IP_PROTO_TCP;
        iph->ip_sum = 0;
        memcpy(&iph->ip_dst, &dst.addr_ip, 4);

        tcph->th_sport = htons(54321);
        tcph->th_dport = htons(port);
        tcph->th_seq = htonl(0x12345678);
        tcph->th_ack = 0;
        tcph->th_off = 5;
        tcph->th_flags = TH_SYN;
        tcph->th_win = htons(65535);
        tcph->th_sum = 0;
        tcph->th_urp = 0;

        iph->ip_sum = in_cksum(pkt, 20);

        printf("Scanning port %d...\n", port);
        if (ip_send(ip, pkt, 40) < 0) {
            printf("  Failed to send\n");
        } else {
            printf("  SYN packet sent to port %d\n", port);
        }
    }

    printf("\n[OK] Port scan completed (Use Wireshark to see responses)\n");
    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
