# test4 - IP 数据包操作示例

## 概述

本示例演示 libdnet 的 IP 数据包操作功能，包括 IP 数据包构建、解析、校验和计算、分片处理等。

## 功能说明

### 1. IP 数据包构建

- `ip_pack_hdr()` - 构建 IP 头部

IP 头部格式：
```
+---+---+---+---+---+---+---+---+
|  版本  |  IHL  |  TOS      |
+---+---+---+---+---+---+---+---+
|        总长度               |
+---+---+---+---+---+---+---+---+
|        标识                 |
+---+---+---+---+---+---+---+---+
|  标志   |   分片偏移         |
+---+---+---+---+---+---+---+---+
|  TTL    |   协议             |
+---+---+---+---+---+---+---+---+
|        校验和               |
+---+---+---+---+---+---+---+---+
|        源 IP                |
+---+---+---+---+---+---+---+---+
|        目标 IP              |
+---+---+---+---+---+---+---+---+
```

### 2. IP 校验和

- `ip_checksum()` - 计算校验和

### 3. IP 协议类型

| 协议 | 数值 |
|------|------|
| ICMP | 1 |
| IGMP | 2 |
| TCP | 6 |
| UDP | 17 |
| GRE | 47 |
| ESP | 50 |

### 4. IP 标志位

- DF (Don't Fragment): 0x4000 - 不允许分片
- MF (More Fragments): 0x2000 - 后续还有分片
- RF (Reserved): 0x8000 - 保留位

## 编译与运行

```bash
# 编译
make

# 运行
./ip_packet

# 或直接运行
make run

# 清理
make clean
```

## 示例输出

```
libdnet IP 数据包操作示例
======================

=== IP 数据包构建 ===
构建 IP 数据包:
  版本: 4
  头部长度: 20 bytes
  总长度: 84
  TTL: 64
  协议: 1 (ICMP)
  源 IP: 192.168.1.100
  目标 IP: 8.8.8.8

=== IP 数据包解析 ===
版本: 4
头部长度: 20 bytes
总长度: 52
TTL: 128
协议: 6
源 IP: 10.0.0.1
目标 IP: 10.0.0.2

=== IP 校验和 ===
IP 校验和: 0xabcd
验证结果: 正确

=== IP 协议类型 ===
常用 IP 协议:
    1 - ICMP (Internet Control Message)
    2 - IGMP (Internet Group Management)
    6 - TCP (Transmission Control)
   17 - UDP (User Datagram)
   47 - GRE (Generic Routing Encapsulation)
   50 - ESP (Encapsulating Security Payload)
   51 - AH (Authentication Header)

=== IP 标志位 ===
DF (Don't Fragment): 0x4000
  - 不允许分片
MF (More Fragments): 0x2000
  - 后续还有分片
RF (Reserved): 0x8000
  - 保留位

=== IP 分片计算 ===
MTU: 1500 bytes
IP 头部: 20 bytes
最大 payload: 1480 bytes

5000 bytes 数据需要分片:
  分片数量: 4
  分片 1: 偏移 0, 长度 1480
  分片 2: 偏移 1480, 长度 1480
  分片 3: 偏移 2960, 长度 1480
  分片 4: 偏移 4440, 长度 560 (最后一个)

所有演示完成！
```

## 相关 API

| 函数 | 描述 |
|------|------|
| `ip_pack_hdr()` | 构建 IP 头部 |
| `ip_checksum()` | 计算 IP 校验和 |
| `ip_open()` | 打开 IP 套接字 |
| `ip_close()` | 关闭 IP 套接字 |
| `ip_send()` | 发送 IP 数据包 |
| `ip_add_option()` | 添加 IP 选项 |
| `ip_strip_options()` | 移除 IP 选项 |
