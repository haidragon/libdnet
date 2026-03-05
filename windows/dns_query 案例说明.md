# DNS Query 案例说明

## 概述

本案例演示如何使用 libdnet 发送自定义的 DNS 查询数据包，用于测试 DNS 服务器或进行 DNS 查询。

---

## 功能说明

- 发送 DNS 查询数据包
- 构造完整的 DNS 协议数据包
- 通过 UDP 协议发送到 DNS 服务器
- 支持任意域名查询

---

## 编译方式

```bash
cd windows
gcc -I../include -L../src/.libs -o dns_query.exe dns_query.c -ldnet -lws2_32 -liphlpapi
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
dns_query.exe <dns_server> <domain_name>
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|--------|
| dns_server | DNS 服务器 IP 地址 | 8.8.8.8 |
| domain_name | 要查询的域名 | example.com |

### 运行示例

```bash
# 查询 example.com 的 DNS 记录，使用 Google DNS
dns_query.exe 8.8.8.8 example.com

# 查询 google.com，使用本地 DNS
dns_query.exe 192.168.1.1 google.com
```

**注意**：
- 需要管理员权限
- 建议使用 Wireshark 捕获 DNS 响应
- 默认查询 A 记录（IPv4 地址）

---

## 输出示例

```
=== libdnet DNS Query Example ===
DNS Server: 8.8.8.8
Domain: example.com

Sending DNS Query for example.com
[OK] DNS Query sent
Query ID: 0x1234
Check Wireshark for DNS response

=== Done ===
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

#### 域名编码
- 将域名编码为 DNS 标签格式
- 处理多级域名（如 example.com）
- 每个标签前添加长度字节

### 数据结构

#### DNS Query 数据包结构

```
IP 头 (20 字节):
  - 版本: 4 位
  - 头长度: 4 位
  - 服务类型: 8 位
  - 总长度: 16 位
  - 标识: 16 位
  - 标志和分片偏移: 16 位
  - TTL: 8 位
  - 协议: 8 位 (17 for UDP)
  - 头校验和: 16 位
  - 源地址: 32 位
  - 目的地址: 32 位

UDP 头 (8 字节):
  - 源端口: 16 位 (随机)
  - 目的端口: 16 位 (53)
  - 长度: 16 位
  - 校验和: 16 位

DNS 查询 (可变长度):
  - 事务 ID: 16 位 (随机)
  - 标志: 16 位 (0x0100 for 标准查询)
  - 问题数: 16 位 (1)
  - 回答数: 16 位 (0)
  - 权威记录数: 16 位 (0)
  - 附加记录数: 16 位 (0)
  - 问题:
    - 域名标签编码
    - 查询类型: 16 位 (1 for A 记录)
    - 查询类: 16 位 (1 for IN)
```

### 域名编码示例

对于域名 "example.com"：

```
7 e x a m p l e
3 c o m
0
1 0 0 1
```

编码规则：
1. 将域名按点号分割
2. 每个部分前添加长度字节
3. 最后添加 0 字节表示结束
4. 添加查询类型和类

---

## DNS 协议基础

### DNS 查询类型

| 类型 | 值 | 说明 |
|------|-----|------|
| A | 1 | IPv4 地址 |
| AAAA | 28 | IPv6 地址 |
| CNAME | 5 | 别名 |
| MX | 15 | 邮件交换 |
| NS | 2 | 名称服务器 |
| TXT | 16 | 文本记录 |

### DNS 响应代码

| 代码 | 名称 | 说明 |
|------|------|------|
| 0 | NOERROR | 无错误 |
| 1 | FORMERR | 格式错误 |
| 2 | SERVFAIL | 服务器失败 |
| 3 | NXDOMAIN | 域名不存在 |
| 4 | NOTIMP | 未实现 |
| 5 | REFUSED | 拒绝 |

---

## 使用 Wireshark 分析

### 捕获设置

1. 启动 Wireshark
2. 选择网络接口
3. 设置过滤器：`dns && ip.addr == <dns_server_ip>`
4. 开始捕获

### 分析 DNS 响应

查看捕获的 DNS 响应数据包：

```
Domain Name System (response)
    Transaction ID: 0x1234
    Flags: 0x8180 (Standard query, Response, No error)
    Questions: 1
    Answer RRs: 1
    Queries
        example.com: type A, class IN
    Answers
        example.com: type A, class IN, addr 93.184.216.34
```

---

## 常见问题

### Q1: 提示 "Error: unable to open IP interface"

**原因**：没有管理员权限。

**解决方案**：
1. 以管理员身份运行命令提示符
2. 右键点击命令提示符 → "以管理员身份运行"

### Q2: 提示 "Error: invalid DNS server IP"

**原因**：DNS 服务器 IP 地址格式不正确。

**解决方案**：
1. 使用点分十进制格式（如 8.8.8.8）
2. 验证 IP 地址有效性
3. 使用正确的 DNS 服务器地址

### Q3: 发送成功但没有收到 DNS 响应

**原因**：
1. DNS 服务器不可达
2. 防火墙阻止了 DNS 流量
3. DNS 服务器拒绝查询

**解决方案**：
1. 使用 nslookup 测试 DNS 服务器
2. 检查防火墙设置
3. 尝试其他 DNS 服务器

### Q4: 如何查询其他类型的 DNS 记录？

**解决方案**：
修改代码中的查询类型：
```c
// 查询 A 记录（IPv4 地址）
dns[dns_len++] = 0x00;
dns[dns_len++] = 0x01;
dns[dns_len++] = 0x00;
dns[dns_len++] = 0x01;

// 查询 AAAA 记录（IPv6 地址）
dns[dns_len++] = 0x00;
dns[dns_len++] = 0x1c;  // 28
dns[dns_len++] = 0x00;
dns[dns_len++] = 0x01;
```

---

## 扩展应用

### 1. 实现 DNS 解析器

扩展本案例，实现完整的 DNS 解析器：
- 解析 DNS 响应
- 处理多种记录类型
- 支持 DNS 重定向
- 缓存 DNS 结果

### 2. DNS 服务器测试

使用本案例测试 DNS 服务器：
- 发送多个查询
- 测量响应时间
- 测试服务器的可靠性
- 检测 DNS 劫持

### 3. DNS 安全测试

使用 DNS 查询进行安全测试：
- DNS 放大检测
- DNS 劫持检测
- DNS 缓存投毒检测
- DNS 隧道检测

---

## 安全注意事项

1. **仅在授权网络中使用**：不要在公共网络中进行 DNS 查询测试
2. **避免 DNS 放大**：不要发送大量 DNS 查询
3. **保护 DNS 隐私**：不要泄露查询的域名信息
4. **遵守网络政策**：遵循组织的网络安全政策

---

## 常用 DNS 服务器

| DNS 服务器 | IP 地址 | 说明 |
|-----------|---------|------|
| Google DNS | 8.8.8.8, 8.8.4.4 | 公共 DNS |
| Cloudflare DNS | 1.1.1.1, 1.0.0.1 | 公共 DNS |
| OpenDNS | 208.67.222.222, 208.67.220.220 | 公共 DNS |
| Quad9 DNS | 9.9.9.9, 149.112.112.112 | 公共 DNS |

---

## 扩展阅读

- [DNS 协议规范](https://tools.ietf.org/html/rfc1035)
- [libdnet IP 模块分析](../doc/03-原始Socket封装实现.md)
- [Wireshark DNS 分析](https://wiki.wireshark.org/DNS)
