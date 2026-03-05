# TCP Port Scan 案例说明

## 概述

本案例演示如何使用 libdnet 发送 TCP SYN 数据包进行端口扫描，用于检测目标主机的开放端口。

**警告**：端口扫描可能影响网络安全，请仅在授权的测试环境中使用！

---

## 功能说明

- 发送 TCP SYN 数据包
- 扫描指定端口范围
- 检测目标主机的开放端口
- 使用原始套接字发送自定义 TCP 数据包

---

## 编译方式

```bash
cd windows
gcc -I../include -L../src/.libs -o port_scan.exe port_scan.c -ldnet -lws2_32 -liphlpapi
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
port_scan.exe <target_ip> <port_start> <port_end>
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|--------|
| target_ip | 目标主机 IP 地址 | 192.168.1.100 |
| port_start | 起始端口号 | 20 |
| port_end | 结束端口号 | 100 |

### 运行示例

```bash
# 扫描常见端口 (20-100)
port_scan.exe 192.168.1.100 20 100

# 扫描 Web 端口 (80-443)
port_scan.exe 192.168.1.100 80 443

# 扫描高端端口 (1024-49152)
port_scan.exe 192.168.1.100 1024 49152
```

**注意**：
- 需要管理员权限
- 建议使用 Wireshark 捕获 TCP SYN/ACK 响应
- 当前版本仅发送 SYN，不接收响应

---

## 输出示例

```
=== libdnet TCP Port Scan Example ===
Target: 192.168.1.100
Port range: 20 - 100

Scanning port 20...
  SYN packet sent to port 20
Scanning port 21...
  SYN packet sent to port 21
...
Scanning port 80...
  SYN packet sent to port 80
...
Scanning port 100...
  SYN packet sent to port 100

[OK] Port scan completed (Use Wireshark to see responses)
```

---

## 代码分析

### 主要函数

#### ip_open()
- 打开 IP 原始套接字
- 需要管理员权限
- 用于发送自定义 IP 数据包

#### in_cksum()
- 计算 IP 校验和
- 标准的互联网校验和算法
- 确保数据包完整性

### 数据结构

#### TCP SYN 数据包结构

```
IP 头 (20 字节):
  - 版本: 4 位
  - 头长度: 4 位
  - 服务类型: 8 位
  - 总长度: 16 位
  - 标识: 16 位
  - 标志和分片偏移: 16 位
  - TTL: 8 位
  - 协议: 8 位 (6 for TCP)
  - 头校验和: 16 位
  - 源地址: 32 位
  - 目的地址: 32 位

TCP 头 (20 字节):
  - 源端口: 16 位
  - 目的端口: 16 位
  - 序列号: 32 位
  - 确认号: 32 位
  - 数据偏移: 4 位
  - 保留: 6 位
  - 标志: 6 位
  - 窗口: 16 位
  - 校验和: 16 位
  - 紧急指针: 16 位
```

### TCP 标志位

| 标志 | 值 | 说明 |
|------|-----|------|
| FIN | 0x01 | 发送方完成数据发送 |
| SYN | 0x02 | 建立连接 |
| RST | 0x04 | 重置连接 |
| PSH | 0x08 | 接收方应立即将数据交给应用层 |
| ACK | 0x10 | 确认号有效 |
| URG | 0x20 | 紧急指针有效 |

---

## TCP 三次握手

### 正常连接建立流程

1. 客户端发送 SYN（建立连接请求）
2. 服务器收到 SYN，发送 SYN+ACK（确认并请求）
3. 客户端收到 SYN+ACK，发送 ACK（确认）
4. 连接建立完成

### 端口扫描原理

1. 扫描器发送 SYN 到目标端口
2. 如果端口开放：
   - 目标返回 SYN+ACK
   - 端口状态：开放
3. 如果端口关闭：
   - 目标返回 RST
   - 端口状态：关闭
4. 如果端口被过滤：
   - 目标不响应
   - 端口状态：被过滤

---

## 常见端口

| 端口 | 服务 | 说明 |
|------|------|------|
| 20 | FTP | 文件传输协议 |
| 21 | FTP | 文件传输协议（数据） |
| 22 | SSH | 安全 Shell |
| 23 | Telnet | 远程登录 |
| 25 | SMTP | 邮件传输 |
| 53 | DNS | 域名解析 |
| 80 | HTTP | Web 服务 |
| 110 | POP3 | 邮件接收 |
| 143 | IMAP | 邮件访问 |
| 443 | HTTPS | 安全 Web 服务 |
| 3306 | MySQL | 数据库 |
| 3389 | RDP | 远程桌面 |
| 5432 | PostgreSQL | 数据库 |

---

## 使用 Wireshark 分析

### 捕获设置

1. 启动 Wireshark
2. 选择网络接口
3. 设置过滤器：`tcp && ip.addr == <target_ip> && tcp.flags.syn==1`
4. 开始捕获

### 分析 TCP 响应

查看捕获的 TCP SYN/ACK 数据包：

```
Transmission Control Protocol
    Source Port: 54321
    Destination Port: 80
    Sequence Number: 0
    Acknowledgment Number: 0
    Header Length: 20 bytes
    Flags: 0x012 (SYN, ACK)
    Window: 65535
    Checksum: 0xabcd [correct]
```

---

## 常见问题

### Q1: 提示 "Error: unable to open IP interface"

**原因**：没有管理员权限。

**解决方案**：
1. 以管理员身份运行命令提示符
2. 右键点击命令提示符 → "以管理员身份运行"

### Q2: 提示 "Error: invalid IP address"

**原因**：目标 IP 地址格式不正确。

**解决方案**：
1. 使用点分十进制格式（如 192.168.1.100）
2. 验证 IP 地址有效性
3. 使用正确的目标 IP 地址

### Q3: 发送成功但无法确定端口状态

**原因**：
1. 防火墙阻止了 TCP SYN/ACK 响应
2. 目标主机配置了端口过滤
3. 网络设备不正确

**解决方案**：
1. 使用 Wireshark 捕获 TCP 响应
2. 检查防火墙设置
3. 尝试其他扫描方法

### Q4: 如何实现完整的端口扫描工具？

**解决方案**：
扩展本案例，实现完整的端口扫描功能：
1. 使用原始套接字接收 TCP 响应
2. 解析 SYN+ACK 和 RST 响应
3. 识别开放、关闭和过滤的端口
4. 支持多种扫描技术（SYN、FIN、XMAS 等）
5. 显示扫描结果和统计信息

---

## 扩展应用

### 1. 实现端口扫描器

扩展本案例，实现完整的端口扫描器：
- 支持多种扫描技术
- 识别服务版本
- 检测操作系统指纹
- 生成扫描报告
- 支持多线程扫描

### 2. 网络服务发现

使用端口扫描发现网络服务：
- 扫描常见服务端口
- 识别运行的服务
- 检测服务版本
- 生成服务清单

### 3. 安全评估

使用端口扫描进行安全评估：
- 检测未授权的服务
- 识别潜在的安全风险
- 评估网络暴露面
- 提供安全建议

---

## 安全注意事项

1. **仅在授权网络中使用**：端口扫描可能违反网络安全法律
2. **不要在生产环境测试**：可能影响正常网络服务
3. **遵守扫描速率限制**：避免触发入侵检测系统
4. **保护扫描隐私**：不要泄露扫描目标和结果
5. **遵循道德准则**：遵守网络安全道德规范

---

## 扫描技术对比

| 技术 | 优点 | 缺点 |
|------|------|------|
| TCP SYN | 快速，隐蔽 | 可能被防火墙检测 |
| TCP Connect | 可靠，简单 | 易被检测，慢 |
| TCP FIN | 隐蔽 | 不准确 |
| TCP XMAS | 隐蔽 | 不准确 |
| UDP | 发现 UDP 服务 | 不可靠，慢 |

---

## 扩展阅读

- [TCP 协议规范](https://tools.ietf.org/html/rfc793)
- [libdnet IP 模块分析](../doc/03-原始Socket封装实现.md)
- [Wireshark TCP 分析](https://wiki.wireshark.org/TCP)
- [Nmap 扫描技术](https://nmap.org/book/man.html)
