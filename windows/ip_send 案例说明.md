# libdnet Windows IP 数据包发送案例说明

## 案例名称
IP 数据包发送 (ip_send)

## 功能描述
本案例演示如何构造并发送原始的 IP 数据包，支持三种常见协议：ICMP、UDP 和 TCP。这是网络编程的核心技术，可用于实现自定义协议栈、网络探测工具等。

## 技术要点

### 1. IP 数据包结构
```
+--------+--------+
| IP 头  | 数据   |
| (20B)  |        |
+--------+--------+
```

### 2. 支持的协议类型

#### ICMP (Internet Control Message Protocol)
- 类型：`ICMP_ECHO` (Ping 请求)
- 用途：网络连通性测试
- 数据结构：IP 头 + ICMP 头 + 数据

#### UDP (User Datagram Protocol)
- 无连接传输协议
- 数据结构：IP 头 + UDP 头 + 数据
- 特点：简单、快速、不可靠

#### TCP (Transmission Control Protocol)
- 面向连接的可靠协议
- 本例发送 TCP SYN 包（连接请求）
- 数据结构：IP 头 + TCP 头 + 选项 + 数据

### 3. 核心 API
- `ip_open()`: 打开原始 IP 套接字
- `ip_send()`: 发送 IP 数据包
- `ip_close()`: 关闭 IP 接口
- `addr_aton()`: IP 地址字符串转换
- `addr_ntoa()`: IP 地址转字符串

### 4. 校验和计算
使用标准的 Internet 校验和算法 (RFC 1071)：
- 按 16 位累加所有字段
- 处理进位
- 取反

## 编译方法

```bash
gcc -I../include -L../src/.libs -o ip_send.exe ip_send.c -ldnet -lws2_32 -liphlpapi
```

**依赖库说明：**
- `-ldnet`: libdnet 库
- `-lws2_32`: Windows Socket 2 库
- `-liphlpapi`: IP Helper API 库

## 运行方法

### 基本用法
```bash
ip_send.exe <源 IP> <目标 IP> <协议> [端口]
```

### 示例

#### 1. 发送 ICMP Echo Request (Ping)
```bash
ip_send.exe 192.168.1.100 8.8.8.8 icmp
```

#### 2. 发送 UDP 数据包到 DNS 服务器
```bash
ip_send.exe 192.168.1.100 8.8.8.8 udp 53
```

#### 3. 发送 TCP SYN 包（端口扫描）
```bash
ip_send.exe 192.168.1.100 93.184.216.34 tcp 80
```

#### 4. 发送到本地网关
```bash
ip_send.exe 192.168.1.100 192.168.1.1 udp 123
```

## 参数说明

### 源 IP
- 必须是本机配置的 IP 地址之一
- 可以使用 `example.exe` 查看本机 IP
- 格式：标准 IPv4 点分十进制

### 目标 IP
- 任意可达的 IPv4 地址
- 可以是公网 IP 或内网 IP
- 格式：标准 IPv4 点分十进制

### 协议类型
支持以下三种协议：
- `icmp` - ICMP 协议（Ping）
- `udp` - UDP 协议
- `tcp` - TCP 协议

### 端口号（可选）
- 仅对 UDP 和 TCP 有效
- 范围：1-65535
- 默认值：80（HTTP）

常用端口：
- 20/21 - FTP
- 22 - SSH
- 23 - Telnet
- 25 - SMTP
- 53 - DNS
- 80 - HTTP
- 443 - HTTPS
- 3306 - MySQL

## 输出示例

### ICMP 示例
```
=== libdnet IP 数据包发送案例 ===

源 IP:   192.168.1.100
目标 IP: 8.8.8.8
协议：   icmp

✓ IP 接口已打开
正在构造 ICMP Echo Request...

IP 数据包信息:
  版本：4
  首部长度：20 字节
  服务类型：0x00
  总长度：84 字节
  标识：0x1234
  标志：0x00
  片偏移：0
  TTL: 64
  协议：1
  首部校验和：0xABCD
  源地址：192.168.1.100
  目的地址：8.8.8.8

正在发送 IP 数据包...
✓ 发送成功！

=== 完成 ===
```

### UDP 示例
```
=== libdnet IP 数据包发送案例 ===

源 IP:   192.168.1.100
目标 IP: 8.8.8.8
协议：   udp (目标端口：53)

✓ IP 接口已打开
正在构造 UDP 数据包...

IP 数据包信息:
  版本：4
  首部长度：20 字节
  ...
  协议：17
  ...

正在发送 IP 数据包...
✓ 发送成功！

=== 完成 ===
```

### TCP 示例
```
=== libdnet IP 数据包发送案例 ===

源 IP:   192.168.1.100
目标 IP: 93.184.216.34
协议：   tcp (目标端口：80)

✓ IP 接口已打开
正在构造 TCP SYN 包...

IP 数据包信息:
  版本：4
  首部长度：20 字节
  ...
  协议：6
  ...

正在发送 IP 数据包...
✓ 发送成功！

=== 完成 ===
```

## 应用场景

### 1. 网络连通性测试
使用 ICMP Echo Request 实现自定义 Ping 工具。

### 2. 端口扫描
发送 TCP SYN 包检测目标主机的开放端口。

### 3. DNS 查询
构造 UDP 包向 DNS 服务器发送查询请求。

### 4. 网络协议研究
实现和测试自定义的网络协议。

### 5. 安全测试
- 防火墙规则测试
- IDS/IPS 系统测试
- 网络漏洞扫描

### 6. 性能测试
生成特定类型的流量进行压力测试。

## 注意事项

### 1. 权限要求
需要管理员权限才能发送原始 IP 数据包：
- Windows: 以管理员身份运行
- Linux: root 权限或 CAP_NET_RAW

### 2. 源 IP 选择
- 必须使用本机真实 IP
- 不要伪造他人 IP（可能违法）
- 某些系统会检查源 IP 合法性

### 3. 网络限制
- ICMP 可能被防火墙阻止
- TCP SYN 可能触发入侵检测
- 不要发送大量包（可能被视为攻击）

### 4. 协议细节

#### ICMP
- 类型 8 = Echo Request
- 类型 0 = Echo Reply
- 需要正确计算校验和

#### UDP
- IPv4 中校验和可选
- 注意字节序转换（htons）

#### TCP
- 本例只发送 SYN 包
- 完整 TCP 连接需要三次握手
- 需要处理序列号和确认号

### 5. 法律风险
未经授权的网络扫描可能违反法律，请仅在以下环境使用：
- 自己的网络
- 获得授权的网络
- 实验环境

## 扩展练习

### 1. 实现 Ping 工具
修改 ICMP 部分，添加：
- 序列号递增
- 时间戳记录
- 接收回复并统计

### 2. TCP 端口扫描器
批量发送 TCP SYN 包：
```c
for (port = 1; port <= 65535; port++) {
    build_tcp_syn(pkt, &len, src_port, port);
    ip_send(ip, pkt, len);
}
```

### 3. UDP Flood 测试
快速发送大量 UDP 包（仅用于测试）：
```c
for (i = 0; i < 1000; i++) {
    build_udp(pkt, &len, src_port, dst_port);
    ip_send(ip, pkt, len);
}
```

### 4. 添加 TCP 选项
实现更多 TCP 选项：
- Window Scale
- Timestamps
- SACK

### 5. 支持 IPv6
使用 `ip6_open()` 和 `ip6_send()` 发送 IPv6 数据包。

## 故障排查

### 问题 1: 无法打开 IP 接口
**原因**: 
- 没有管理员权限
- WinPcap/Npcap 未安装

**解决方法**:
- 以管理员身份运行
- 安装 Npcap

### 问题 2: 发送失败
**原因**:
- 源 IP 不是本机 IP
- 网络连接断开
- 路由不可达

**解决方法**:
- 使用 `example.exe` 查看正确 IP
- 检查网络连接
- 验证路由表

### 问题 3: 对方收不到包
**原因**:
- 防火墙阻止
- NAT 设备过滤
- 中间路由器丢弃

**解决方法**:
- 临时关闭防火墙测试
- 在局域网内测试
- 使用 Wireshark 抓包分析

### 问题 4: 校验和错误
**原因**:
- 校验和计算错误
- 字节序问题

**解决方法**:
- 使用 Wireshark 验证
- 检查 htons/htonl 使用
- 参考 RFC 1071

## 相关案例

- `simple.c` - 获取本机 IP
- `example.c` - 查看网络接口
- `eth_send.c` - 以太网帧发送
- `packet_sniff.c` - 网络抓包

## 高级话题

### 1. 原始套接字原理
Windows 使用 `WSASocket()` 创建原始套接字：
```c
SOCKET s = WSASocket(AF_INET, SOCK_RAW, IPPROTO_RAW, ...);
```

### 2. IP 选项
可以添加 IP 选项实现特殊功能：
- 记录路由 (RR)
- 时间戳 (TS)
- 松散源路由 (LSR)

### 3. 分片处理
大数据包需要分片：
- 设置 MF 标志
- 设置片偏移
- 注意 MTU 限制

### 4. TOS/QoS
通过 TOS 字段设置服务质量：
- 低延迟
- 高吞吐
- 高可靠

## 参考资料

- RFC 791 - Internet Protocol
- RFC 792 - ICMP
- RFC 768 - UDP
- RFC 793 - TCP
- 《TCP/IP 详解 卷 1》
- libdnet 官方文档
- Windows Raw Sockets API
