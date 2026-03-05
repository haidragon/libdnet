# ARP Spoofing 案例说明

## 概述

本案例演示如何使用 libdnet 发送自定义的 ARP 数据包，实现 ARP 欺骗（ARP Spoofing）功能。

**警告**：ARP 欺骗可能影响网络安全，请仅在授权的测试环境中使用！

---

## 功能说明

- 发送自定义 ARP Reply 数据包
- 指定目标 IP、网关 IP 和伪造的 MAC 地址
- 欺骗目标主机，使其认为网关位于伪造的 MAC 地址

---

## 编译方式

```bash
cd windows
gcc -I../include -L../src/.libs -o arp_spoof.exe arp_spoof.c -ldnet -lws2_32 -liphlpapi
```

**编译参数说明**：
- `-I../include`: 指定头文件路径
- `-L../src/.libs`: 指定库文件路径
- `-ldnet`: 链接 libdnet 库
- `-lws2_32`: 链接 Windows Socket 库
- `-liphlpapi`: 链接 IP Helper API 库

---

## 运行方式

### 基本用法

```bash
arp_spoof.exe <interface_name> <target_ip> <gateway_ip> <spoofed_mac>
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|--------|
| interface_name | 网络接口名称 | "Ethernet" |
| target_ip | 目标主机 IP | 192.168.1.100 |
| gateway_ip | 网关 IP | 192.168.1.1 |
| spoofed_mac | 伪造的 MAC 地址 | 00:11:22:33:44:55 |

### 运行示例

```bash
# 欺骗主机 192.168.1.100，使其认为网关 192.168.1.1 位于 MAC 00:11:22:33:44:55
arp_spoof.exe "Ethernet" 192.168.1.100 192.168.1.1 00:11:22:33:44:55
```

**注意**：
- 需要管理员权限
- 接口名称必须与系统中的实际接口名称匹配
- MAC 地址格式为 XX:XX:XX:XX:XX:XX

---

## 输出示例

```
=== libdnet ARP Spoofing Example ===
Interface: Ethernet
Target IP: 192.168.1.100
Gateway IP: 192.168.1.1
Spoofed MAC: 00:11:22:33:44:55

Sending ARP Reply...
  Telling 192.168.1.100 that 192.168.1.1 is at 00:11:22:33:44:55
[OK] ARP Reply sent

=== Done ===
```

---

## 代码分析

### 主要函数

#### eth_open()
- 打开以太网接口
- 返回 eth_t 句柄
- 需要正确的接口名称

#### intf_open() / intf_get()
- 打开网络接口
- 获取接口信息（IP、MAC 等）
- 用于获取本机 IP 和 MAC 地址

#### eth_send()
- 发送完整的以太网帧
- 数据包必须包含完整的以太网头

### 数据结构

#### ARP Reply 数据包结构

```
以太网头 (14 字节):
  - 目的 MAC: 6 字节
  - 源 MAC: 6 字节
  - 以太网类型: 2 字节 (0x0806 for ARP)

ARP 头 (28 字节):
  - 硬件类型: 2 字节 (0x0001 for Ethernet)
  - 协议类型: 2 字节 (0x0800 for IPv4)
  - 硬件地址长度: 1 字节 (6 for MAC)
  - 协议地址长度: 1 字节 (4 for IPv4)
  - 操作码: 2 字节 (0x0002 for Reply)
  - 发送方 MAC: 6 字节
  - 发送方 IP: 4 字节
  - 目标 MAC: 6 字节
  - 目标 IP: 4 字节
```

---

## ARP 欺骗原理

### 正常 ARP 流程

1. 主机 A 需要发送数据到主机 B
2. 主机 A 发送 ARP Request："谁是 192.168.1.2？"
3. 主机 B 收到请求，回复 ARP Reply："192.168.1.2 在 00:11:22:33:44:55"
4. 主机 A 学习到主机 B 的 MAC 地址

### ARP 欺骗流程

1. 攻击者发送伪造的 ARP Reply 给主机 A
2. 告诉主机 A："192.168.1.2（网关）在 00:11:22:33:44:55（攻击者的 MAC）"
3. 主机 A 更新其 ARP 缓存
4. 主机 A 发往网关的流量实际上发送到攻击者
5. 攻击者可以转发或丢弃这些数据包

---

## 防御措施

### 1. 静态 ARP 条目

```bash
# Windows
arp -s 192.168.1.1 00:aa:bb:cc:dd:ee:ff

# Linux
arp -s 192.168.1.1 00:aa:bb:cc:dd:ee:ff
```

### 2. ARP 监控

- 使用 Wireshark 监控 ARP 流量
- 检测异常的 ARP Reply
- 设置 ARP 流量警报

### 3. 网络隔离

- 在关键网络段使用端口隔离
- 限制 ARP 广播范围
- 使用 VLAN 隔离不同网段

---

## 常见问题

### Q1: 提示 "Error: unable to open Ethernet interface"

**原因**：接口名称不正确或接口不存在。

**解决方案**：
1. 使用 `ipconfig /all` 查看所有接口
2. 使用实际的接口名称（如 "Ethernet"、"Wireless Network Connection"）

### Q2: 发送成功但没有效果

**原因**：
1. 目标主机有静态 ARP 条目
2. 防火墙阻止了 ARP 流量
3. 目标主机不在同一网段

**解决方案**：
1. 清除目标主机的 ARP 缓存
2. 检查防火墙设置
3. 确认网络配置

### Q3: 如何恢复被欺骗的网络？

**解决方案**：
1. 清除 ARP 缓存：
   ```bash
   # Windows
   arp -d *

   # Linux
   ip neigh flush all
   ```

2. 添加静态 ARP 条目（见防御措施）

---

## 安全注意事项

1. **仅在授权环境中使用**：ARP 欺骗可能违反网络安全法律
2. **不要在生产环境测试**：可能影响正常网络通信
3. **及时恢复网络**：测试完成后立即清除 ARP 缓存
4. **记录测试过程**：保留测试日志以备审计

---

## 扩展阅读

- [ARP 协议规范](https://tools.ietf.org/html/rfc826)
- [ARP 欺骗防御](https://en.wikipedia.org/wiki/ARP_spoofing)
- [libdnet ARP 模块分析](../doc/04-ARP操作源码深入分析.md)
