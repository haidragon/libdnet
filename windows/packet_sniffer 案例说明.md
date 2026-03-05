# 数据包嗅探器信息案例说明

## 案例概述

本案例演示如何使用 libdnet 库获取网络接口信息和可用的网络句柄类型，为数据包捕获和发送做准备。需要注意的是，libdnet 主要支持数据包**发送**，而不是捕获。

## 功能说明

### 主要功能
1. **网络接口信息**：显示所有可用的网络接口及其配置
2. **网络句柄类型**：检查各种网络句柄的可用性
3. **使用说明**：提供数据包捕获和发送的相关信息

### 核心功能模块

#### 1. 网络接口信息显示
- 显示接口名称和状态标志
- 显示 MTU（最大传输单元）
- 显示 IP 地址和目标地址
- 显示 MAC 地址
- 显示 IP 别名列表

#### 2. 网络句柄类型检查
- **eth_t**：以太网句柄，用于发送原始以太网帧
- **ip_t**：IP 句柄，用于发送原始 IP 数据包（需要管理员权限）
- **arp_t**：ARP 句柄，用于操作 ARP 表
- **route_t**：路由句柄，用于操作路由表

#### 3. 数据包捕获说明
libdnet 主要用于数据包发送，如需数据包捕获功能，应使用：
- WinPcap：https://www.winpcap.org/
- Npcap：https://npcap.com/

## 编译方式

```bash
gcc -I../include -L../src/.libs -o packet_sniffer.exe packet_sniffer.c -ldnet -lws2_32 -liphlpapi
```

## 运行示例

```
=== libdnet Packet Sniffer Information ===

1. Network Interfaces:
   -------------------

[以太网]
  Flags: 0x1103 UP BROADCAST MULTICAST
  MTU: 1500
  Inet: 192.168.1.100
  MAC:  00:11:22:33:44:55

[Loopback Pseudo-Interface]
  Flags: 0x0101 UP LOOPBACK
  MTU: 4294967295
  Inet: 127.0.0.1


2. Available Network Handle Types:
   -----------------------------

   Ethernet (eth_t):
     [INFO] Ethernet handles available (requires correct interface name)

   IP (ip_t):
     [INFO] IP handles available for raw IP packet sending

   ARP (arp_t):
     [OK] Can open ARP handles

   Route (route_t):
     [OK] Can open Route handles


3. Note on Packet Capture:
   ----------------------
   libdnet primarily supports packet SENDING, not capturing.
   For packet capture/sniffing on Windows, use:
   - WinPcap: https://www.winpcap.org/
   - Npcap:   https://npcap.com/


4. Usage Examples:
   --------------
   - Send raw IP packets:   ip_send.exe
   - Send Ethernet frames:  eth_send.exe
   - Send ICMP pings:       ping_icmp.exe
   - Port scanning:         port_scan.exe
   - DNS queries:           dns_query.exe
   - Traceroute:            traceroute.exe

=== Done ===
```

## 注意事项

1. 需要管理员权限才能打开某些网络句柄
2. 以太网句柄需要正确的接口名称
3. libdnet 不提供数据包捕获功能，需要配合 WinPcap 或 Npcap 使用
4. MTU 值对于回环接口可能显示异常大值

## 相关数据结构

### struct intf_entry
```c
struct intf_entry {
    char intf_name[INTF_NAME_LEN];  // 接口名称
    u_short intf_flags;             // 接口标志
    u_short intf_mtu;               // MTU
    struct addr intf_addr;          // 主IP地址
    struct addr intf_dst_addr;      // 目标地址（点对点接口）
    struct addr intf_link_addr;     // 链路层地址（MAC）
    struct addr intf_alias_addrs[INTF_MAX_ALIASES];  // IP别名
    int intf_alias_num;             // 别名数量
};
```

## 接口标志位说明

- `INTF_FLAG_UP`：接口已启动
- `INTF_FLAG_LOOPBACK`：回环接口
- `INTF_FLAG_POINTOPOINT`：点对点接口
- `INTF_FLAG_NOARP`：不使用 ARP
- `INTF_FLAG_BROADCAST`：支持广播
- `INTF_FLAG_MULTICAST`：支持多播

## 适用场景

- 网络工具开发前的环境检查
- 网络配置诊断
- 网络应用程序开发
- 网络安全工具开发
- 系统网络能力评估

## 相关案例

- `ip_send.exe`：发送原始 IP 数据包
- `eth_send.exe`：发送以太网帧
- `ping_icmp.exe`：ICMP ping 工具
- `port_scan.exe`：端口扫描工具
- `dns_query.exe`：DNS 查询工具
- `traceroute.exe`：路由追踪工具
