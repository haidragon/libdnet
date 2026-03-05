# UDP Flood 攻击示例说明

## 案例概述

本案例演示如何使用 libdnet 库发送大量 UDP 数据包，用于测试目的。程序会向指定目标 IP 和端口发送多个 UDP 数据包。

⚠️ **重要警告**：本案例仅用于测试和学习目的，严禁用于非法攻击！

## 功能说明

### 主要功能
1. **发送 UDP 数据包**：向目标主机发送大量 UDP 数据包
2. **随机源端口**：每个数据包使用不同的源端口
3. **自定义目标**：可指定源 IP、目标 IP、端口和数据包数量

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
1. 解析命令行参数（源 IP、目标 IP、端口、数据包数量）
2. 打开 IP 接口
3. 构造 IP 和 UDP 头部
4. 循环发送 UDP 数据包
5. 关闭 IP 接口

## 编译方式

```bash
gcc -I../include -L../src/.libs -o udp_flood.exe udp_flood.c -ldnet -lws2_32 -liphlpapi
```

## 运行方式

```bash
udp_flood.exe <src_ip> <dst_ip> <port> <count>
```

### 参数说明
- `src_ip`：源 IP 地址
- `dst_ip`：目标 IP 地址
- `port`：目标端口号
- `count`：要发送的数据包数量

### 运行示例

```bash
udp_flood.exe 192.168.1.100 192.168.1.1 53 100
```

### 输出示例

```
=== libdnet UDP Flood Example ===
Source: 192.168.1.100
Destination: 192.168.1.1:53
Packet count: 100

Sending UDP packets...
Sent 10/100 packets...
Sent 20/100 packets...
Sent 30/100 packets...
...
Sent 100/100 packets...
[OK] UDP flood completed

=== Done ===
```

## 注意事项

1. ⚠️ **必须以管理员权限运行**
2. ⚠️ **仅用于授权的测试环境**
3. ⚠️ **严禁用于未经授权的目标**
4. 需要正确配置网络环境
5. 目标主机可能会触发防火墙或入侵检测系统

## 数据包结构

### IP 头部字段
```c
iph->ip_v = 4;                    // IP 版本
iph->ip_hl = 5;                   // IP 头部长度（5 * 4 = 20 字节）
iph->ip_tos = 0;                  // 服务类型
iph->ip_len = htons(pkt_len);     // 总长度
iph->ip_off = 0;                  // 片偏移
iph->ip_ttl = 64;                 // 生存时间
iph->ip_p = IP_PROTO_UDP;         // 协议类型（UDP）
iph->ip_sum = 0;                  // 校验和（稍后计算）
```

### UDP 头部字段
```c
udph->uh_sport = htons(src_port); // 源端口（循环变化）
udph->uh_dport = htons(dst_port); // 目标端口
udph->uh_ulen = htons(8 + 32);    // UDP 长度（头部 + 数据）
udph->uh_sum = 0;                 // 校验和
```

## UDP Flood 原理

UDP Flood 是一种拒绝服务攻击，其原理是：
1. 攻击者向目标发送大量 UDP 数据包
2. 每个数据包可能使用不同的源端口
3. 目标主机需要为每个数据包分配资源处理
4. 大量数据包消耗目标主机的网络带宽和处理能力
5. 可能导致目标主机无法处理合法请求

## 常见攻击端口

UDP Flood 通常针对以下端口：
- **DNS (53)**：DNS 服务器
- **NTP (123)**：网络时间协议服务器
- **SNMP (161)**：简单网络管理协议
- **游戏服务器端口**：各种在线游戏

## 防御措施

1. **流量限制**：限制特定端口的流量速率
2. **防火墙规则**：过滤可疑 UDP 流量
3. **入侵检测系统**：检测异常流量模式
4. **流量清洗**：使用专业的流量清洗服务
5. **负载均衡**：分散流量到多台服务器

## 适用场景

⚠️ **仅限以下合法场景**：
- 网络安全培训
- 渗透测试（经授权）
- 系统压力测试
- 防火墙和 IDS 测试
- 网络安全研究

## 法律声明

本案例仅用于教育和测试目的。未经授权使用此工具进行网络攻击是非法的，可能导致：
- 民事责任
- 刑事指控
- 网络访问被终止
- 其他法律后果

## 相关案例

- `ip_send.exe`：发送原始 IP 数据包
- `tcp_syn_flood.exe`：TCP SYN Flood 攻击示例
- `port_scan.exe`：端口扫描工具
- `dns_query.exe`：DNS 查询工具
