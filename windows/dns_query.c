/*
 * libdnet Windows DNS Query Example
 *
 * Features: Send DNS query packet
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o dns_query.exe dns_query.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * dns_query.exe <dns_server> <domain_name>
 *
 * Example:
 * dns_query.exe 8.8.8.8 example.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1
#define DNS_OPCODE_QUERY 0
#define DNS_RCODE_NOERROR 0

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
    u_char *dns;
    size_t pkt_len, dns_len;
    uint16_t dns_id;
    char *domain, *label;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dns_server> <domain_name>\n", argv[0]);
        fprintf(stderr, "Example: %s 8.8.8.8 example.com\n", argv[0]);
        return 1;
    }

    if (addr_aton(argv[1], &dst) < 0) {
        fprintf(stderr, "Error: invalid DNS server IP: %s\n", argv[1]);
        return 1;
    }

    domain = argv[2];

    printf("=== libdnet DNS Query Example ===\n");
    printf("DNS Server: %s\n", addr_ntoa(&dst));
    printf("Domain: %s\n\n", domain);

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    udph = (struct udp_hdr *)(pkt + 20);
    dns = pkt + 28;

    dns_id = (uint16_t)rand();

    dns[0] = (dns_id >> 8) & 0xFF;
    dns[1] = dns_id & 0xFF;
    dns[2] = 0x01;
    dns[3] = 0x00;
    dns[4] = 0x00;
    dns[5] = 0x01;
    dns[6] = 0x00;
    dns[7] = 0x00;
    dns[8] = 0x00;
    dns[9] = 0x00;
    dns[10] = 0x00;
    dns[11] = 0x00;

    dns_len = 12;
    label = domain;

    while (*label) {
        char *dot = strchr(label, '.');
        int len = dot ? (int)(dot - label) : (int)strlen(label);
        dns[dns_len++] = len;
        memcpy(dns + dns_len, label, len);
        dns_len += len;
        if (dot) {
            label = dot + 1;
        } else {
            break;
        }
    }
    dns[dns_len++] = 0;
    dns[dns_len++] = 0x00;
    dns[dns_len++] = 0x01;
    dns[dns_len++] = 0x00;
    dns[dns_len++] = 0x01;

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(20 + 8 + dns_len);
    iph->ip_id = htons(0x1234);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IP_PROTO_UDP;
    iph->ip_sum = 0;
    addr_aton("192.168.1.100", &src);
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    udph->uh_sport = htons(54321);
    udph->uh_dport = htons(53);
    udph->uh_ulen = htons(8 + dns_len);
    udph->uh_sum = 0;

    iph->ip_sum = in_cksum(pkt, 20);

    printf("Sending DNS Query for %s\n", domain);
    if (ip_send(ip, pkt, 20 + 8 + dns_len) < 0) {
        fprintf(stderr, "Error: send failed\n");
        ip_close(ip);
        return 1;
    }

    printf("[OK] DNS Query sent\n");
    printf("Query ID: 0x%04X\n", dns_id);
    printf("Check Wireshark for DNS response\n");

    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
