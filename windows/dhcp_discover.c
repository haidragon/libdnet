/*
 * libdnet Windows DHCP Discover Example
 *
 * Features: Send DHCP Discover packet
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o dhcp_discover.exe dhcp_discover.c -ldnet -lws2_32 -liphlpapi
 *
 * Run (requires administrator privileges):
 * dhcp_discover.exe <interface_name>
 *
 * Example:
 * dhcp_discover.exe "Ethernet"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY 2
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NACK 6
#define DHCP_RELEASE 7

#define DHCP_OPT_PAD 0
#define DHCP_OPT_SUBNET 1
#define DHCP_OPT_ROUTER 3
#define DHCP_OPT_DNS 6
#define DHCP_OPT_REQUEST 50
#define DHCP_OPT_MSGTYPE 53
#define DHCP_OPT_SERVER 54
#define DHCP_OPT_END 255

static uint16_t checksum(void *data, int len)
{
    uint16_t *buf = (uint16_t *)data;
    uint32_t sum = 0;
    uint16_t result;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(uint8_t *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

int main(int argc, char *argv[])
{
    ip_t *ip;
    struct addr src, dst;
    u_char pkt[1500];
    struct ip_hdr *iph;
    struct udp_hdr *udph;
    u_char *dhcp;
    struct addr src_addr, dst_addr;
    size_t pkt_len;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface_name>\n", argv[0]);
        fprintf(stderr, "Example: %s Ethernet\n", argv[0]);
        return 1;
    }

    printf("=== libdnet DHCP Discover Example ===\n");
    printf("Interface: %s\n\n", argv[1]);

    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "Error: unable to open IP interface (requires admin privileges)\n");
        return 1;
    }

    memset(pkt, 0, sizeof(pkt));
    iph = (struct ip_hdr *)pkt;
    udph = (struct udp_hdr *)(pkt + 20);
    dhcp = pkt + 28;

    iph->ip_v = 4;
    iph->ip_hl = 5;
    iph->ip_tos = 0;
    iph->ip_len = htons(328);
    iph->ip_id = htons(0x1234);
    iph->ip_off = 0;
    iph->ip_ttl = 64;
    iph->ip_p = IP_PROTO_UDP;
    iph->ip_sum = 0;

    addr_aton("0.0.0.0", &src);
    addr_aton("255.255.255.255", &dst);
    memcpy(&iph->ip_src, &src.addr_ip, 4);
    memcpy(&iph->ip_dst, &dst.addr_ip, 4);

    udph->uh_sport = htons(68);
    udph->uh_dport = htons(67);
    udph->uh_ulen = htons(308);
    udph->uh_sum = 0;

    dhcp[0] = DHCP_BOOTREQUEST;
    dhcp[1] = 1;
    dhcp[2] = 6;
    dhcp[3] = 0;
    memset(dhcp + 4, 0, 4);
    memset(dhcp + 8, 0, 4);
    memset(dhcp + 12, 0, 4);
    memset(dhcp + 16, 0, 4);
    memset(dhcp + 20, 0, 6);
    memset(dhcp + 28, 0, 10);
    memset(dhcp + 44, 0, 192);
    memset(dhcp + 236, 0, 128);

    uint8_t *opts = dhcp + 240;
    int opt_idx = 0;

    opts[opt_idx++] = DHCP_OPT_MSGTYPE;
    opts[opt_idx++] = 1;
    opts[opt_idx++] = DHCP_DISCOVER;

    opts[opt_idx++] = DHCP_OPT_REQUEST;
    opts[opt_idx++] = 4;
    opts[opt_idx++] = 0x00;
    opts[opt_idx++] = 0x00;
    opts[opt_idx++] = 0x00;
    opts[opt_idx++] = 0x00;

    opts[opt_idx++] = DHCP_OPT_END;

    iph->ip_sum = checksum(pkt, 20);

    printf("Sending DHCP Discover packet...\n");
    if (ip_send(ip, pkt, 328) < 0) {
        fprintf(stderr, "Error: send failed\n");
        ip_close(ip);
        return 1;
    }

    printf("[OK] DHCP Discover sent\n");
    printf("Check Wireshark for DHCP Offer responses\n");

    ip_close(ip);
    printf("\n=== Done ===\n");
    return 0;
}
