# libdnet Windows Usage Examples

## Introduction

This directory contains usage examples for libdnet on the Windows platform.

## Example Code

### example.c

**Features**: Display network interface information

- List all network interface names
- Display interface flags (UP, LOOPBACK, BROADCAST, etc.)
- Display MTU value
- Display IP address and gateway
- Display MAC address
- Display IP aliases

### simple.c

**Features**: Get local default IP address

### arp_example.c

**Features**: ARP table operations

- Display current ARP table
- Query ARP entries for specific hosts

### route_example.c

**Features**: Route table query

- Display all system routes

### ip_send.c

**Features**: Construct and send raw IP packets

- Support ICMP Echo Request
- Support UDP packets
- Support TCP SYN packets

**Note**: Requires administrator privileges to run!

## Compilation Methods

### 1. Ensure libdnet is compiled

```bash
cd c:\Users\Administrator\libev\libdnet-1.13
make
```

### 2. Compile example programs

```bash
cd windows

# Network interface example
gcc -I../include -L../src/.libs -o example.exe example.c -ldnet -lws2_32 -liphlpapi

# Simple example
gcc -I../include -L../src/.libs -o simple.exe simple.c -ldnet -lws2_32 -liphlpapi

# ARP example
gcc -I../include -L../src/.libs -o arp_example.exe arp_example.c -ldnet -lws2_32 -liphlpapi

# Route example
gcc -I../include -L../src/.libs -o route_example.exe route_example.c -ldnet -lws2_32 -liphlpapi

# IP send example (requires administrator privileges)
gcc -I../include -L../src/.libs -o ip_send.exe ip_send.c -ldnet -lws2_32 -liphlpapi
```

**Compilation parameters**:
- `-I../include`: Specify header file path
- `-L../src/.libs`: Specify library file path
- `-ldnet`: Link libdnet library
- `-lws2_32`: Link Windows Socket library
- `-liphlpapi`: Link IP Helper API library

### 3. Run example programs

```bash
# Network interface example
example.exe

# Simple example
simple.exe

# ARP example
arp_example.exe

# Route example
route_example.exe

# IP send example (run as administrator!)
ip_send.exe 192.168.1.100 8.8.8.8 icmp
ip_send.exe 192.168.1.100 8.8.8.8 udp 53
ip_send.exe 192.168.1.100 93.184.216.34 tcp 80
```

## Output Examples

### example.exe output

```
=== libdnet Windows Usage Examples ===
Get network interface information

[Ethernet]
  Flags: 0x1003 UP BROADCAST MULTICAST
  MTU: 1500
  MAC:  00:1a:2b:3c:4d:5e
  Inet: 192.168.1.100

[Wireless Network Connection]
  Flags: 0x1003 UP BROADCAST MULTICAST
  MTU: 1500
  MAC:  00:11:22:33:44:55
  Inet: 192.168.1.101
  Alias: 192.168.1.102

[Loopback Pseudo-Interface]
  Flags: 0x1001 UP LOOPBACK
  MTU: 65536
  Inet: 127.0.0.1

=== Done ===
```

### ip_send.exe output

```
=== libdnet IP Packet Sending Example ===

Source IP:   192.168.1.100
Destination: 8.8.8.8
Protocol:    icmp

[OK] IP interface opened
Building ICMP Echo Request...

IP Packet Information:
  Version: 4
  Header Length: 20 bytes
  TOS: 0x00
  Total Length: 84 bytes
  Identification: 0x1234
  Flags: 0x00
  Fragment Offset: 0
  TTL: 64
  Protocol: 1
  Header Checksum: 0x1234
  Source: 192.168.1.100
  Destination: 8.8.8.8

Sending IP packet...
[OK] Sent successfully!

=== Done ===
```

## Code Explanation

### Main Functions

#### example.c
- `intf_open()`: Open network interface
- `intf_loop()`: Loop through all network interfaces
- `print_interface()`: Callback function to print interface information
- `intf_close()`: Close network interface

#### ip_send.c
- `ip_open()`: Open IP raw socket (requires administrator privileges)
- `in_cksum()`: Calculate Internet checksum
- `build_icmp()`: Build ICMP Echo Request packet
- `build_udp()`: Build UDP packet
- `build_tcp_syn()`: Build TCP SYN packet
- `ip_send()`: Send IP packet
- `ip_close()`: Close IP handle

### Data Structures

```c
struct intf_entry {
    char    intf_name[INTF_NAME_LEN];  // Interface name
    u_short intf_flags;                 // Interface flags
    u_int   intf_mtu;                   // MTU value
    struct addr intf_addr;              // IP address
    struct addr intf_link_addr;         // MAC address
    struct addr intf_dst_addr;          // Destination address
    struct addr intf_alias_addrs[INTF_MAXALIAS]; // IP aliases
    int     intf_alias_num;             // Number of aliases
};

struct ip_hdr {
    uint8_t  ip_v;        // Version
    uint8_t  ip_hl;       // Header length
    uint8_t  ip_tos;      // Type of service
    uint16_t ip_len;      // Total length
    uint16_t ip_id;       // Identification
    uint16_t ip_off;      // Fragment offset
    uint8_t  ip_ttl;      // Time to live
    uint8_t  ip_p;        // Protocol
    uint16_t ip_sum;      // Checksum
    uint32_t ip_src;      // Source address
    uint32_t ip_dst;      // Destination address
};
```

## Extended Usage

### Get specific interface information

```c
struct intf_entry entry;
char buf[1024];

memset(&entry, 0, sizeof(entry));
entry.intf_len = sizeof(buf);
strcpy(entry.intf_name, "Ethernet");

if (intf_get(intf, &entry) < 0) {
    fprintf(stderr, "Failed to get interface\n");
    return 1;
}

// Use the information in entry
printf("MAC: %s\n", addr_ntoa(&entry.intf_link_addr));
```

### Get interface by IP

```c
struct addr target;
struct intf_entry entry;
char buf[1024];

// Set target IP
addr_aton("192.168.1.100", &target);

memset(&entry, 0, sizeof(entry));
entry.intf_len = sizeof(buf);

if (intf_get_src(intf, &entry, &target) < 0) {
    fprintf(stderr, "Failed to get interface\n");
    return 1;
}

printf("Interface name: %s\n", entry.intf_name);
```

## Common Issues

### 1. Cannot find dnet.h header file

Ensure to specify the header file path when compiling:
```bash
gcc -I<libdnet_path>/include ...
```

### 2. Cannot find libdnet.a library file

Ensure libdnet is compiled and the path is correct:
```bash
gcc -L<libdnet_path>/src/.libs -ldnet ...
```

### 3. Link errors

Ensure to link the required Windows libraries:
```bash
gcc ... -ldnet -lws2_32 -liphlpapi
```

### 4. Cannot find DLL at runtime

If using dynamic linking, copy `libdnet.dll` to the same directory as the executable, or add it to the PATH environment variable.

### 5. ip_send.exe fails to open IP interface

The `ip_send` example requires administrator privileges to create raw sockets on Windows. Please:

1. Right-click on MSYS2 terminal or Command Prompt
2. Select "Run as administrator"
3. Run `ip_send.exe` again

Or use `sudo` equivalent in MSYS2 if available.

## More Examples

More example code can be found in the `test/dnet/` directory:
- `intf.c`: Network interface management
- `arp.c`: ARP table operations
- `route.c`: Route table operations
- `eth.c`: Ethernet frame operations
- `ip.c`: IP packet operations

## Important Notes

1. **Administrator privileges**: `ip_send.exe` requires administrator privileges to send raw IP packets on Windows
2. **Firewall**: Windows Firewall may block raw socket operations, please configure accordingly
3. **Antivirus software**: Some antivirus software may interfere with raw socket operations
4. **Network interface**: Ensure the specified source IP address exists on the local machine

## License

libdnet is released under a BSD-style license. See the LICENSE file for details.
