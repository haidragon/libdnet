# DHCP Discover 案例说明

## 概述

本案例演示如何使用 libdnet 发送 DHCP Discover 数据包，用于发现网络中的 DHCP 服务器。

---

## 功能说明

- 发送 DHCP Discover 数据包
- 构造完整的 DHCP 协议数据包
- 通过 UDP 协议发送到 DHCP 服务器端口

---

## 编译方式

```bash
cd windows
gcc -I../include -L../src/.libs -o dhcp_discover.exe dhcp_discover.c -ldnet -lws2_32 -liphlpapi
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
dhcp_discover.exe <interface_name>
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|--------|
| interface_name | 网络接口名称 | "Ethernet" |

### 运行示例

```bash
# 使用 Ethernet 接口发送 DHCP Discover
dhcp_discover.exe "Ethernet"
```

**注意**：
- 需要管理员权限
- 接口名称必须与系统中的实际接口名称匹配
- 建议使用 Wireshark 捕获 DHCP Offer 响应

---

## 输出示例

```
=== libdnet DHCP Discover Example ===
Interface: Ethernet

Sending DHCP Discover packet...
[OK] DHCP Discover sent
Check Wireshark for DHCP Offer responses

=== Done ===
```

---

## 代码分析

### 主要函数

#### ip_open()
- 打开 IP 原始套接字
- 需要管理员权限
- 用于发送自定义 IP 数据包

#### checksum()
- 计算 IP 校验和
- 标准的互联网校验和算法
- 确保数据包完整性

### 数据结构

#### DHCP Discover 数据包结构

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
  - 源地址: 32 位 (0.0.0.0)
  - 目的地址: 32 位 (255.255.255.255)

UDP 头 (8 字节):
  - 源端口: 16 位 (68)
  - 目的端口: 16 位 (67)
  - 长度: 16 位
  - 校验和: 16 位

DHCP 消息 (240+ 字节):
  - 操作码: 1 字节 (1 for BOOTREQUEST)
  - 硬件类型: 1 字节 (1 for Ethernet)
  - 硬件地址长度: 1 字节 (6)
  - 跳数: 1 字节
  - 事务 ID: 4 字节
  - 秒数: 2 字节
  - 标志: 2 字节
  - 客户端 IP: 4 字节
  - 您的 IP: 4 字节
  - 服务器 IP: 4 字节
  - 网关 IP: 4 字节
  - 客户端硬件地址: 16 字节
  - 服务器主机名: 64 字节
  - 启动文件: 128 字节
  - 选项: 可变长度
```

### DHCP 选项

本案例包含以下 DHCP 选项：

| 选项代码 | 选项名称 | 值 |
|---------|---------|-----|
| 53 | 消息类型 | DHCP_DISCOVER (1) |
| 50 | 请求的 IP 地址 | 0.0.0.0 |
| 255 | 结束标记 | - |

---

## DHCP 协议流程

### DHCP Discover 流程

1. 客户端发送 DHCP Discover（广播）
2. DHCP 服务器收到 Discover，发送 DHCP Offer
3. 客户端收到 Offer，发送 DHCP Request
4. DHCP 服务器收到 Request，发送 DHCP ACK
5. 客户端收到 ACK，配置网络参数

### DHCP 消息类型

| 类型 | 值 | 说明 |
|------|-----|------|
| DHCP_DISCOVER | 1 | 客户端广播寻找服务器 |
| DHCP_OFFER | 2 | 服务器提供 IP 地址 |
| DHCP_REQUEST | 3 | 客户端请求特定 IP |
| DHCP_DECLINE | 4 | 服务器拒绝请求 |
| DHCP_ACK | 5 | 服务器确认分配 |
| DHCP_NACK | 6 | 服务器拒绝分配 |
| DHCP_RELEASE | 7 | 客户端释放 IP |

---

## 使用 Wireshark 分析

### 捕获设置

1. 启动 Wireshark
2. 选择网络接口
3. 设置过滤器：`bootp`
4. 开始捕获

### 分析 DHCP Offer

查看捕获的 DHCP Offer 数据包：

```
Bootstrap Protocol (Discover)
    Message type: Discover
    Client MAC address: xx:xx:xx:xx:xx:xx
    Parameter request list: 
        - Subnet Mask (1)
        - Router (3)
        - Domain Name Server (6)
        - Domain Name (15)
```

---

## 常见问题

### Q1: 提示 "Error: unable to open IP interface"

**原因**：没有管理员权限。

**解决方案**：
1. 以管理员身份运行命令提示符
2. 右键点击命令提示符 → "以管理员身份运行"

### Q2: 发送成功但没有收到 DHCP Offer

**原因**：
1. 网络中没有 DHCP 服务器
2. 防火墙阻止了 DHCP 流量
3. 网络接口不正确

**解决方案**：
1. 检查网络中是否有 DHCP 服务器
2. 检查防火墙设置
3. 使用正确的网络接口名称

### Q3: 如何查看完整的 DHCP 交互？

**解决方案**：
1. 使用 Wireshark 捕获完整的 DHCP 交互
2. 设置过滤器：`bootp && ip.addr == <your_ip>`
3. 分析 Discover、Offer、Request、ACK 流程

---

## 扩展应用

### 1. 实现 DHCP 客户端

扩展本案例，实现完整的 DHCP 客户端：
- 解析 DHCP Offer
- 发送 DHCP Request
- 处理 DHCP ACK
- 配置网络接口

### 2. DHCP 服务器测试

使用本案例测试 DHCP 服务器：
- 发送多个 Discover 数据包
- 分析不同的 Offer 响应
- 测试服务器的负载能力

### 3. 网络发现

使用 DHCP Discover 发现网络信息：
- 发现网络中的所有 DHCP 服务器
- 分析服务器提供的网络参数
- 检测配置冲突

---

## 安全注意事项

1. **仅在授权网络中使用**：不要在公共网络中发送 DHCP Discover
2. **避免网络干扰**：频繁发送可能影响正常网络通信
3. **遵守网络政策**：遵循组织的网络安全政策
4. **保护敏感信息**：不要泄露网络配置信息

---

## 扩展阅读

- [DHCP 协议规范](https://tools.ietf.org/html/rfc2131)
- [DHCP 选项规范](https://tools.ietf.org/html/rfc2132)
- [libdnet IP 模块分析](../doc/03-原始Socket封装实现.md)
