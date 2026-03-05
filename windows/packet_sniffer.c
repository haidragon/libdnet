/*
 * libdnet Windows Packet Sniffer Example
 *
 * Features: Display network interface information for packet capture setup
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o packet_sniffer.exe packet_sniffer.c -ldnet -lws2_32 -liphlpapi
 *
 * Run:
 * packet_sniffer.exe
 *
 * Note: This example shows available interfaces.
 * For actual packet capture, use WinPcap or Npcap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

static int
print_interface(const struct intf_entry *entry, void *arg)
{
    int i;

    printf("\n[%s]\n", entry->intf_name);
    printf("  Flags: 0x%04X", entry->intf_flags);

    if (entry->intf_flags & INTF_FLAG_UP)
        printf(" UP");
    if (entry->intf_flags & INTF_FLAG_LOOPBACK)
        printf(" LOOPBACK");
    if (entry->intf_flags & INTF_FLAG_BROADCAST)
        printf(" BROADCAST");
    if (entry->intf_flags & INTF_FLAG_MULTICAST)
        printf(" MULTICAST");

    printf("\n");

    if (entry->intf_mtu != 0)
        printf("  MTU: %d\n", entry->intf_mtu);

    if (entry->intf_addr.addr_type == ADDR_TYPE_IP) {
        if (entry->intf_dst_addr.addr_type == ADDR_TYPE_IP) {
            printf("  Inet: %s -> %s\n",
                addr_ntoa(&entry->intf_addr),
                addr_ntoa(&entry->intf_dst_addr));
        } else {
            printf("  Inet: %s\n", addr_ntoa(&entry->intf_addr));
        }
    }

    if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
        printf("  MAC:  %s\n", addr_ntoa(&entry->intf_link_addr));
    }

    for (i = 0; i < entry->intf_alias_num; i++) {
        printf("  Alias: %s\n", addr_ntoa(&entry->intf_alias_addrs[i]));
    }

    return 0;
}

int main(void)
{
    intf_t *intf;
    eth_t *eth;
    ip_t *ip;
    arp_t *arp;
    route_t *route;

    printf("=== libdnet Packet Sniffer Information ===\n\n");

    printf("1. Network Interfaces:\n");
    printf("   -------------------\n");
    if ((intf = intf_open()) == NULL) {
        fprintf(stderr, "Error: unable to open interface\n");
        return 1;
    }

    if (intf_loop(intf, print_interface, NULL) < 0) {
        fprintf(stderr, "Error: unable to get interface list\n");
        intf_close(intf);
        return 1;
    }
    intf_close(intf);

    printf("\n\n2. Available Network Handle Types:\n");
    printf("   -----------------------------\n");

    printf("\n   Ethernet (eth_t):\n");
    if ((eth = eth_open("Ethernet")) != NULL) {
        printf("     [OK] Can open Ethernet handles for raw frame sending\n");
        eth_close(eth);
    } else {
        printf("     [INFO] Ethernet handles available (requires correct interface name)\n");
    }

    printf("\n   IP (ip_t):\n");
    if ((ip = ip_open()) != NULL) {
        printf("     [OK] Can open IP handles (requires admin privileges)\n");
        ip_close(ip);
    } else {
        printf("     [INFO] IP handles available for raw IP packet sending\n");
    }

    printf("\n   ARP (arp_t):\n");
    if ((arp = arp_open()) != NULL) {
        printf("     [OK] Can open ARP handles\n");
        arp_close(arp);
    }

    printf("\n   Route (route_t):\n");
    if ((route = route_open()) != NULL) {
        printf("     [OK] Can open Route handles\n");
        route_close(route);
    }

    printf("\n\n3. Note on Packet Capture:\n");
    printf("   ----------------------\n");
    printf("   libdnet primarily supports packet SENDING, not capturing.\n");
    printf("   For packet capture/sniffing on Windows, use:\n");
    printf("   - WinPcap: https://www.winpcap.org/\n");
    printf("   - Npcap:   https://npcap.com/\n");

    printf("\n\n4. Usage Examples:\n");
    printf("   --------------\n");
    printf("   - Send raw IP packets:   ip_send.exe\n");
    printf("   - Send Ethernet frames:  eth_send.exe\n");
    printf("   - Send ICMP pings:       ping_icmp.exe\n");
    printf("   - Port scanning:         port_scan.exe\n");
    printf("   - DNS queries:           dns_query.exe\n");
    printf("   - Traceroute:            traceroute.exe\n");

    printf("\n=== Done ===\n");
    return 0;
}
