# Traceroute 路由追踪案例说明

## 案例概述

本案例演示如何使用 libdnet 库实现 Traceroute 功能，通过发送 TTL（生存时间）递增的数据包来追踪到达目标主机的网络路径。

## 功能说明

### 主要功能
1. **发送 TTL 递增的数据包**：从 TTL=1 开始，逐步增加 TTL 值
2. **UDP 数据包构造**：构造带有特定端口范围的 UDP 数据包
3. **路径追踪**：通过 TTL 递增触发路由器返回 ICMP 超时消息

### 核心函数

#### in_cksum()
```c
static uint16_t in_cksum(const u_char *buf, size_t len)
```
- 功能：计算 IP 校验和
- 参数：
  - `buf`：数据缓冲区指针
  - `len`：数据长度
- 返回值：16 位校验和

#### 主函数流程
1. 解析命令行参数（源 IP、目标 IP、最大 TTL）
2. 打开 IP 接口
3. 构造 IP 和 UDP 头部
4. 循环发送 TTL 从 1 到 max_ttl 的数据包
5. 每个数据包使用不同的目标端口
6. 关闭 IP 接口

## 编译方式

```bash
gcc -I../include -L../src/.libs -o traceroute.exe traceroute.c -ldnet -lws2_32 -liphlpapi
```

## 运行方式

```bash
traceroute.exe <src_ip> <dst_ip> [max_ttl]
```

### 参数说明
- `src_ip`：源 IP 地址
- `dst_ip`：目标 IP 地址
- `max_ttl`：最大 TTL 值（可选，默认 30）

### 运行示例

```bash
traceroute.exe 192.168.1.100 8.8.8.8 30
```

### 输出示例

```
=== libdnet Traceroute Example ===
Source: 192.168.1.100
Destination: 8.8.8.8
Max TTL: 30

Sending packets with increasing TTL values...
(Use Wireshark to capture ICMP Time Exceeded responses)

TTL  1: packet sent (port 33435)
TTL  2: packet sent (port 33436)
TTL  3: packet sent (port 33437)
...
TTL 30: packet sent (port 33464)

[OK] Traceroute packets sent
Check Wireshark for ICMP Time Exceeded messages from routers

=== Done ===
```

## 注意事项

1. ⚠️ **必须以管理员权限运行**
2. 需要使用 Wireshark 或其他抓包工具查看 ICMP 超时响应
3. 源 IP 必须是本机的有效 IP 地址
4. 某些网络设备可能限制 Traceroute 数据包
5. 防火墙可能会阻止 ICMP 消息

## Traceroute 工作原理

Traceroute 利用 IP 协议中的 TTL（Time To Live）字段工作：

1. **TTL 机制**：每个路由器在转发数据包时将 TTL 减 1
2. **TTL 超时**：当 TTL 减为 0 时，路由器丢弃数据包并返回 ICMP Time Exceeded 消息
3. **逐步探测**：
   - 发送 TTL=1 的数据包 → 第一跳路由器返回 ICMP
   - 发送 TTL=2 的数据包 → 第二跳路由器返回 ICMP
   - 以此类推，直到到达目标主机

## 数据包结构

### IP 头部字段
```c
iph->ip_v = 4;                    // IP 版本
iph->ip_hl = 5;                   // IP 头部长度（5 * 4 = 20 字节）
iph->ip_tos = 0;                  // 服务类型
iph->ip_len = htons(28 + 12);     // 总长度（IP + UDP + 数据）
iph->ip_off = 0;                  // 片偏移
iph->ip_p = IP_PROTO_UDP;         // 协议类型（UDP）
iph->ip_sum = 0;                  // 校验和（稍后计算）
```

### UDP 头部字段
```c
udph->uh_sport = htons(54321);    // 源端口
udph->uh_dport = htons(33434 + i);// 目标端口（随 TTL 变化）
udph->uh_ulen = htons(20);        // UDP 长度
udph->uh_sum = 0;                 // 校验和
```

## ICMP 响应类型

1. **ICMP Time Exceeded (Type 11)**：
   - Code 0：TTL 在传输中过期
   - 由中间路由器返回

2. **ICMP Destination Unreachable (Type 3)**：
   - Code 3：端口不可达
   - 由目标主机返回（当数据包到达目标时）

## 使用 Wireshark 捕获响应

1. 启动 Wireshark 并选择网络接口
2. 设置过滤器：`icmp.type == 11 or icmp.type == 3`
3. 运行 traceroute.exe
4. 观察 ICMP 响应消息

### Wireshark 显示示例

```
ICMP 114 Time-to-live exceeded (Time to live exceeded)
    Type: 11 (Time to live exceeded)
    Code: 0 (Time to live exceeded in transit)
    Internet Protocol, Src: 192.168.1.1, Dst: 192.168.1.100
        ...
    User Datagram Protocol, Src Port: 33435, Dst Port: 54321
```

## 适用场景

- 网络故障诊断
- 网络路径分析
- 网络性能测试
- 路由问题排查
- 网络拓扑发现
- 安全审计

## 常见问题

### 1. 看不到 ICMP 响应
- 检查防火墙设置
- 确认 Wireshark 过滤器正确
- 验证源 IP 地址是否正确

### 2. 某些跳数无响应
- 部分路由器可能配置为不响应 ICMP
- 防火墙可能阻止 ICMP 消息
- 网络策略限制

### 3. 路径不完整
- 目标主机不可达
- 网络中存在防火墙
- 路由配置问题

## 相关案例

- `ping_icmp.exe`：ICMP ping 工具
- `ip_send.exe`：发送原始 IP 数据包
- `port_scan.exe`：端口扫描工具
- `route_example.exe`：路由表查询
