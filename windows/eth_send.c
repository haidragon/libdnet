/*
 * libdnet Windows Ethernet Frame Sending Example
 *
 * Features: Send raw data frames through Ethernet interface
 *
 * Compile:
 * gcc -I../include -L../src/.libs -o eth_send.exe eth_send.c -ldnet -lws2_32 -liphlpapi
 *
 * Run:
 * eth_send.exe <interface_name> <dst_mac>
 * 
 * Examples:
 * eth_send.exe "Local Area Connection" 00:11:22:33:44:55
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "dnet.h"

/*
 * Print Ethernet frame content
 */
static void
print_eth_frame(const u_char *data, size_t len)
{
    size_t i;
    
    printf("Ethernet Frame Content (%zu bytes):\n", len);
    for (i = 0; i < len && i < 64; i++) {
        if (i % 16 == 0)
            printf("  %04zX: ", i);
        printf("%02X ", data[i]);
        if ((i + 1) % 8 == 0)
            printf(" ");
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (len > 64)
        printf("  ... (%zu more bytes)\n", len - 64);
    else if (len % 16 != 0)
        printf("\n");
}

/*
 * Main function: send Ethernet frame
 */
int main(int argc, char *argv[])
{
    eth_t *eth;
    eth_addr_t src_mac, dst_mac;
    u_char frame[1500];
    size_t frame_len;
    const char *intf_name;
    const char *dst_mac_str;

    /* Check arguments */
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <interface_name> <dst_mac>\n", argv[0]);
        fprintf(stderr, "Example: %s \"Local Area Connection\" 00:11:22:33:44:55\n", argv[0]);
        return 1;
    }

    intf_name = argv[1];
    dst_mac_str = argv[2];

    printf("=== libdnet Ethernet Frame Sending Example ===\n\n");
    printf("Interface: %s\n", intf_name);
    printf("Destination MAC: %s\n\n", dst_mac_str);

    /* Open Ethernet interface */
    if ((eth = eth_open(intf_name)) == NULL) {
        fprintf(stderr, "Error: unable to open Ethernet interface %s\n", intf_name);
        return 1;
    }
    printf("[OK] Ethernet interface opened\n");

    /* Get source MAC address (interface hardware address) */
    memset(&src_mac, 0, sizeof(src_mac));
    if (eth_get(eth, &src_mac) < 0) {
        fprintf(stderr, "Warning: unable to get source MAC address, using default\n");
        /* Use default source MAC */
        src_mac.data[0] = 0x00;
        src_mac.data[1] = 0x0a;
        src_mac.data[2] = 0x95;
        src_mac.data[3] = 0x00;
        src_mac.data[4] = 0x00;
        src_mac.data[5] = 0x01;
    } else {
        printf("[OK] Source MAC address: %s\n", eth_ntoa(&src_mac));
    }

    /* Parse destination MAC address */
    if (eth_pton(dst_mac_str, &dst_mac) < 0) {
        fprintf(stderr, "Error: invalid MAC address format: %s\n", dst_mac_str);
        eth_close(eth);
        return 1;
    }
    printf("[OK] Destination MAC address: %s\n", eth_ntoa(&dst_mac));

    /* Build Ethernet frame */
    memset(frame, 0, sizeof(frame));
    
    /* Destination MAC (first 6 bytes) */
    memcpy(frame, dst_mac.data, 6);
    
    /* Source MAC (next 6 bytes) */
    memcpy(frame + 6, src_mac.data, 6);
    
    /* Ethernet type (next 2 bytes) - using 0x1234 as example */
    frame[12] = 0x12;
    frame[13] = 0x34;
    
    /* Data payload (from byte 14) */
    frame_len = 64; /* Minimum Ethernet frame length */
    memset(frame + 14, 0xAA, frame_len - 14); /* Fill with data */
    
    printf("\n[OK] Ethernet frame constructed:\n");
    print_eth_frame(frame, frame_len);

    /* Send Ethernet frame */
    printf("\nSending Ethernet frame...\n");
    if (eth_send(eth, frame, frame_len) < 0) {
        fprintf(stderr, "Error: send failed\n");
        eth_close(eth);
        return 1;
    }
    printf("[OK] Sent successfully!\n");

    /* Close Ethernet interface */
    eth_close(eth);

    printf("\n=== Done ===\n");
    return 0;
}
