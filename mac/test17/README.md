# libdnet 地址转换示例 (test17)

## 功能说明

本示例展示如何使用 libdnet 库进行网络地址转换和操作，包括：
- IPv4 地址转换
- MAC 地址转换
- 地址复制和比较
- 网络地址操作
- 线程安全的地址转换

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
./addr_conversion
```

## 输出示例

```
=== libdnet 地址转换示例 ===

   === IPv4 地址转换 ===
   字符串 -> 二进制:
     输入: 192.168.1.100
     类型: 2 (ADDR_TYPE_IP = 2)
     二进制: 0xC0A80164
   二进制 -> 字符串:
     输入: 0xC0A80164
     输出: 192.168.1.100

   CIDR 表示法:
     输入: 192.168.1.0/24
     网络地址: 192.168.1.0
     掩码位数: 24

   特殊 IPv4 地址:
     0.0.0.0         -> 0x00000000
     127.0.0.1       -> 0x7F000001
     255.255.255.255 -> 0xFFFFFFFF
     224.0.0.1       -> 0xE0000001

   === MAC 地址转换 ===
   字符串 -> 二进制:
     输入: AA:BB:CC:DD:EE:FF
     类型: 1 (ADDR_TYPE_ETH = 1)

   不同 MAC 地址格式:
     00:11:22:33:44:55 -> 00:11:22:33:44:55
     00-11-22-33-44-55 -> 00:11:22:33:44:55
     0011.2233.4455   -> 00:11:22:33:44:55
     001122334455     -> 00:11:22:33:44:55

   === 地址复制和比较 ===
   地址复制:
     原地址: 192.168.1.1
     复制后: 192.168.1.1

   地址比较:
     地址1: 192.168.1.1
     地址2: 192.168.1.2
     结果: 地址1 < 地址2

   === 地址操作 ===
   网络地址计算:
     主机地址: 192.168.1.100/24
     网络地址: 192.168.1.0/24
     广播地址: 192.168.1.255
     子网掩码: 255.255.255.0

   常见网络掩码:
     /8  -> 255.0.0.0
     /16 -> 255.255.0.0
     /24 -> 255.255.255.0
     /32 -> 255.255.255.255

   === 线程安全的地址转换 ===
   addr_ntoa (静态缓冲区):
     调用1: 192.168.1.1
     调用2: 192.168.1.1
     指针1: 0x123456, 指针2: 0x123456 (相同)

   addr_ntop (用户缓冲区):
     缓冲区1: 192.168.1.1
     缓冲区2: 192.168.1.1
     指针1: 0x789abc, 指针2: 0x789d00 (不同)

=== 地址转换示例完成 ===
```

## API 说明

### addr_pton()
地址字符串到二进制转换。

```c
int addr_pton(const char *src, struct addr *dst);
```

### addr_ntop()
地址二进制到字符串转换（线程安全）。

```c
int addr_ntop(const struct addr *src, char *dst, size_t size);
```

### addr_ntoa()
地址到字符串转换（静态缓冲区）。

```c
char *addr_ntoa(const struct addr *a);
```

### addr_cpy()
复制地址。

```c
void addr_cpy(struct addr *dst, struct addr *src);
```

### addr_cmp()
比较两个地址。

```c
int addr_cmp(struct addr *a, struct addr *b);
```

### addr_net()
计算网络地址。

```c
int addr_net(struct addr *a, struct addr *b);
```

### addr_bcast()
计算广播地址。

```c
int addr_bcast(struct addr *a, struct addr *b);
```

### addr_mask()
获取子网掩码。

```c
int addr_mask(struct addr *a, struct addr *b);
```

## 地址类型

```c
#define ADDR_TYPE_NONE  0  // 无效地址
#define ADDR_TYPE_ETH   1  // 以太网/MAC 地址
#define ADDR_TYPE_IP    2  // IP 地址
#define ADDR_TYPE_IP6   3  // IPv6 地址
```

## 支持的格式

### IPv4 地址
- 点分十进制：`192.168.1.1`
- CIDR 表示法：`192.168.1.0/24`

### MAC 地址
- 冒号分隔：`AA:BB:CC:DD:EE:FF`
- 连字符分隔：`AA-BB-CC-DD-EE-FF`
- Cisco 格式：`AABB.CCDD.EEFF`
- 连续格式：`AABBCCDDEEFF`

## 注意事项

1. `addr_ntoa` 使用静态缓冲区，多线程不安全
2. 多线程环境应使用 `addr_ntop`
3. 地址比较返回 `<0`, `0`, `>0` 表示小于、等于、大于
4. 网络操作前需要先设置 `addr_bits`（掩码位数）

## 依赖

- libdnet
