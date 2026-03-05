# libdnet 校验和计算示例 (test16)

## 功能说明

本示例展示如何使用 libdnet 库计算各种网络协议的校验和，包括：
- IP 校验和
- TCP 校验和（含伪首部）
- UDP 校验和（含伪首部）
- ICMP 校验和
- 通用数据校验和

## 编译方法

```bash
make
```

## 运行方法

```bash
make run
```

或者直接运行：

```bash
./checksum_calc
```

## 输出示例

```
=== libdnet 校验和计算示例 ===

1. IP 校验和计算:
   IP 头 (校验和字段为0):
   45 00 00 14 30 39 00 00 40 06 00 00 c0 a8 01 64
   5d b8 d8 22 00 00 00 00
   计算的校验和: 0xABCD
   带校验和的 IP 头:
   45 00 00 14 30 39 00 00 40 06 ab cd c0 a8 01 64
   验证校验和 (应为0): 0x0000 ✓

2. TCP 校验和计算:
   伪首部:
   c0 a8 01 64 5d b8 d8 22 00 06 00 14
   TCP 头 (校验和字段为0):
   ...
   计算的 TCP 校验和: 0x1234

3. 通用数据校验和计算:
   数据1: "Hello, World!"
   数据1 校验和: 0x5678
   数据2: 01 02 03 04 05
   数据2 校验和: 0x0F0F

4. UDP 校验和计算:
   UDP 伪首部:
   ...
   UDP 数据: "Test UDP data"
   UDP 校验和: 0xABCD

5. ICMP 校验和计算:
   ICMP 头 (校验和字段为0):
   ...
   计算的 ICMP 校验和: 0x3456

=== 校验和计算示例完成 ===
```

## API 说明

### ip_cksum()
计算 IP 头的校验和。

```c
uint16_t ip_cksum(const struct ip_hdr *ip);
```

### cksum_add()
计算数据的互联网校验和（可增量计算）。

```c
uint16_t cksum_add(uint16_t sum, const void *buf, size_t len);
```

## 校验和类型

### 1. IP 校验和
- 只计算 IP 头部（不包括数据）
- 计算前校验和字段设为 0
- 正确的校验和使得验证时结果为 0

### 2. TCP/UDP 校验和
- 需要包含伪首部（12字节）
- 伪首部包含：源IP、目标IP、协议、长度
- TCP 校验和是强制的，UDP 可选但推荐

### 3. ICMP 校验和
- 计算整个 ICMP 报文
- 与 IP 校验和算法相同

## 伪首部结构

```c
struct pseudo_header {
    uint32_t src_ip;    // 源 IP 地址
    uint32_t dst_ip;    // 目标 IP 地址
    uint8_t zero;       // 零字节
    uint8_t protocol;   // 协议类型
    uint16_t length;    // TCP/UDP 长度
};
```

## 注意事项

1. 计算前必须将校验和字段设为 0
2. 使用 `~` 取反得到最终校验和
3. 网络字节序（大端）
4. 奇数长度需补零（通常编译器自动处理）

## 依赖

- libdnet
