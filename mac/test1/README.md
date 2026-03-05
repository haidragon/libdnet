# libdnet macOS 示例程序

> 基于 libdnet-1.13 的 macOS 平台网络编程示例

---

## 目录

1. [项目概述](#项目概述)
2. [环境准备](#环境准备)
3. [编译说明](#编译说明)
4. [示例程序](#示例程序)
5. [运行示例](#运行示例)
6. [代码解析](#代码解析)
7. [API 参考](#api-参考)
8. [常见问题](#常见问题)

---

## 项目概述

本目录包含两个基于 libdnet 的实用示例程序：

1. **interface_scanner** - 网络接口扫描工具
2. **packet_builder** - 数据包构建工具

### 功能特点

- 扫描系统网络接口信息
- 显示路由表和 ARP 缓存
- 演示地址操作和转换
- 构建各种协议数据包
- 支持 ICMP、TCP、UDP、ARP 等协议

---

## 环境准备

### 1. 编译 libdnet

在编译示例程序之前，需要先编译 libdnet 库：

```bash
cd ..
./configure
make
sudo make install
```

编译完成后，libdnet 的动态库和头文件会安装在系统目录中。

### 2. 验证安装

```bash
# 检查头文件
ls -la /usr/local/include/dnet/

# 检查库文件
ls -la /usr/local/lib/libdnet.*

# 检查 pkg-config
pkg-config --modversion dnet
```

### 3. 系统要求

- macOS 10.12 或更高版本
- Xcode Command Line Tools
- 管理员权限（sudo）用于运行某些程序

---

## 编译说明

### 方法 1: 使用 Makefile

```bash
# 进入示例目录
cd mac

# 编译所有程序
make

# 查看编译产物
ls -lh interface_scanner packet_builder
```

### 方法 2: 手动编译

```bash
# 编译 interface_scanner
gcc -Wall -Wextra -O2 -g \
    -I../include -L../.libs \
    -o interface_scanner interface_scanner.c -ldnet

# 编译 packet_builder
gcc -Wall -Wextra -O2 -g \
    -I../include -L../.libs \
    -o packet_builder packet_builder.c -ldnet
```

### 编译选项说明

| 选项 | 说明 |
|------|------|
| `-Wall` | 启用所有警告 |
| `-Wextra` | 启用额外警告 |
| `-O2` | 优化级别 2 |
| `-g` | 生成调试信息 |
| `-I../include` | 指定头文件路径 |
| `-L../.libs` | 指定库文件路径 |
| `-ldnet` | 链接 libdnet 库 |

---

## 示例程序

### 1. interface_scanner - 网络接口扫描工具

#### 功能描述

扫描并显示系统的网络接口信息，包括：

- 接口名称、类型、标志、MTU
- IP 地址和 MAC 地址
- 接口别名
- 路由表信息
- ARP 缓存内容
- 地址操作演示

#### 主要函数

```c
// 打印接口信息
int print_interface(const struct intf_entry *entry, void *arg);

// 打印路由条目
int print_route(const struct route_entry *entry, void *arg);

// 打印 ARP 条目
int print_arp_entry(const struct arp_entry *entry, void *arg);

// 演示地址操作
void demo_address_operations(void);

// 查找接口
void find_interface_for_address(const char *addr_str);
```

#### 运行示例

```bash
# 需要管理员权限
sudo ./interface_scanner
```

#### 输出示例

```
╔═══════════════════════════════════════════════════════════╗
║     libdnet 网络接口扫描工具 (macOS 示例)                ║
╚═══════════════════════════════════════════════════════════╝

========== 网络接口扫描 ==========

[接口 #1]
名称: lo0
索引: 1
类型: 24 (Loopback)
标志: 0x0002 (LOOPBACK )
MTU: 16384 字节
IP 地址: 127.0.0.1/8
别名地址 (1 个):
  [1] 127.0.0.1/8

[接口 #2]
名称: en0
索引: 5
类型: 6 (Ethernet)
标志: 0x0203 (UP BROADCAST MULTICAST )
MTU: 1500 字节
IP 地址: 192.168.1.100/24
MAC 地址: 00:11:22:33:44:55

总计: 2 个网络接口

========== 路由表扫描 ==========
[路由 #1] 0.0.0.0/0 via 192.168.1.1 dev en0 metric 100
[路由 #2] 192.168.1.0/24 dev en0 metric 100

总计: 2 条路由

========== ARP 缓存扫描 ==========
[ARP #1] 192.168.1.1 → aa:bb:cc:dd:ee:ff
[ARP #2] 192.168.1.2 → 11:22:33:44:55:66

总计: 2 个 ARP 条目
```

---

### 2. packet_builder - 数据包构建工具

#### 功能描述

演示如何使用 libdnet 构建各种网络数据包：

- ICMP Echo Request (Ping)
- TCP SYN / SYN-ACK / RST
- UDP 数据包
- ARP Request / Reply

#### 主要函数

```c
// 构建 ICMP Ping 数据包
void build_icmp_ping(uint8_t *packet, size_t *packet_len, ...);

// 构建 TCP SYN 数据包
void build_tcp_syn(uint8_t *packet, size_t *packet_len, ...);

// 构建 UDP 数据包
void build_udp_packet(uint8_t *packet, size_t *packet_len, ...);

// 构建 ARP 请求数据包
void build_arp_request(uint8_t *packet, size_t *packet_len, ...);
```

#### 运行示例

```bash
# 构建数据包（不需要 sudo）
./packet_builder

# 发送数据包（需要 sudo）
sudo ./packet_builder
```

#### 输出示例

```
╔═══════════════════════════════════════════════════════════╗
║     libdnet 数据包构建工具 (macOS 示例)                   ║
╚═══════════════════════════════════════════════════════════╝

========== ICMP Echo Request 数据包构建 ==========
数据包长度: 32 字节
  45 00 00 20 04 d2 00 00 40 01 00 00 0a 00 00 01
  0a 00 00 02 08 00 f7 7c 00 01 00 00 68 65 6c 6c
  6f

数据包结构:
  IP: 10.0.0.1 → 10.0.0.2
  ICMP Echo Request (id=1, seq=0)
  数据: "hello"

========== TCP SYN 数据包构建 ==========
数据包长度: 40 字节
  45 00 00 28 04 d2 40 00 40 06 00 00 0a 00 00 01
  0a 00 00 02 30 39 00 50 00 00 03 e8 00 00 00 00
  50 02 ff ff 00 00 00 00

数据包结构:
  IP: 10.0.0.1 → 10.0.0.2
  TCP: 10.0.0.1:12345 → 10.0.0.2:80
  Flags: SYN
  Seq: 1000, Ack: 0, Window: 65535
```

---

## 运行示例

### 基本运行

```bash
# 1. 编译程序
make

# 2. 运行网络接口扫描
sudo ./interface_scanner

# 3. 运行数据包构建工具
./packet_builder
```

### 高级用法

```bash
# 只编译特定程序
make interface_scanner

# 清理编译产物
make clean

# 安装到系统
sudo make install

# 卸载
sudo make uninstall

# 查看帮助
make help
```

### 调试模式

```bash
# 编译调试版本
gcc -Wall -Wextra -g -O0 \
    -I../include -L../.libs \
    -o interface_scanner interface_scanner.c -ldnet

# 使用 gdb 调试
gdb ./interface_scanner
(gdb) run
```

---

## 代码解析

### 核心数据结构

#### 1. 网络接口结构

```c
struct intf_entry {
    uint32_t intf_len;              // 条目长度
    char intf_name[32];             // 接口名称
    uint32_t intf_index;            // 接口索引
    uint16_t intf_type;             // 接口类型
    uint16_t intf_flags;            // 接口标志
    uint32_t intf_mtu;              // MTU
    struct addr intf_addr;          // IP 地址
    struct addr intf_link_addr;     // MAC 地址
    uint32_t intf_alias_num;        // 别名数量
    struct addr intf_alias_addrs[]; // 别名数组（变长）
};
```

#### 2. 通用地址结构

```c
struct addr {
    uint16_t addr_type;    // 地址类型
    uint16_t addr_bits;    // 网络前缀长度
    union {
        eth_addr_t __eth;   // 以太网地址
        ip_addr_t __ip;     // IPv4 地址
        ip6_addr_t __ip6;   // IPv6 地址
        uint8_t __data8[16];
    } __addr_u;
};
```

### 关键 API 使用

#### 1. 接口扫描

```c
// 打开接口句柄
intf_t *i = intf_open();

// 遍历所有接口
int count = 0;
intf_loop(i, print_interface, &count);

// 获取特定接口
struct intf_entry entry;
strlcpy(entry.intf_name, "en0", sizeof(entry.intf_name));
intf_get(i, &entry);

// 关闭句柄
intf_close(i);
```

#### 2. 地址操作

```c
struct addr addr1, addr2, net, bcast;

// 字符串 → 地址
addr_pton("192.168.1.100/24", &addr1);

// 地址 → 字符串
char buf[256];
addr_ntop(&addr1, buf, sizeof(buf));

// 计算网络地址
addr_net(&addr1, &net);

// 计算广播地址
addr_bcast(&addr1, &bcast);

// 比较地址
int result = addr_cmp(&addr1, &addr2);
```

#### 3. 数据包构建

```c
uint8_t packet[128];
size_t packet_len;

// 构建 IP 头部
ip_pack_hdr(packet,
            IP_TOS_DEFAULT,  // TOS
            sizeof(packet),   // 总长度
            1234,            // ID
            0,               // 分片偏移
            64,              // TTL
            IP_PROTO_ICMP,   // 协议
            src_ip,          // 源地址
            dst_ip);         // 目标地址

// 计算 IP 校验和
ip_checksum(packet, IP_HDR_LEN, 0);
```

#### 4. 以太网操作

```c
eth_addr_t mac;

// 字符串 → MAC
eth_pton("00:11:22:33:44:55", &mac);

// MAC → 字符串
eth_ntop(&mac, buf, sizeof(buf));

// 构建以太网头部
eth_pack_hdr(packet, dst_mac, src_mac, ETH_TYPE_IP);
```

### 回调函数模式

libdnet 使用回调函数遍历集合：

```c
// 回调函数类型
typedef int (*intf_handler)(const struct intf_entry *entry, void *arg);

// 回调函数实现
int print_interface(const struct intf_entry *entry, void *arg) {
    int *count = (int *)arg;
    (*count)++;
    printf("[#%d] %s\n", *count, entry->intf_name);
    return 0;  // 继续遍历
}

// 调用
int count = 0;
intf_loop(intf, print_interface, &count);
```

---

## API 参考

### 接口操作 (intf.h)

| 函数 | 说明 |
|------|------|
| `intf_open()` | 打开接口句柄 |
| `intf_get()` | 获取指定接口 |
| `intf_get_src()` | 根据源地址获取接口 |
| `intf_loop()` | 遍历所有接口 |
| `intf_close()` | 关闭句柄 |

### 地址操作 (addr.h)

| 函数 | 说明 |
|------|------|
| `addr_pton()` | 文本 → 二进制地址 |
| `addr_ntop()` | 二进制 → 文本地址 |
| `addr_cmp()` | 比较地址 |
| `addr_net()` | 计算网络地址 |
| `addr_bcast()` | 计算广播地址 |

### 路由操作 (route.h)

| 函数 | 说明 |
|------|------|
| `route_open()` | 打开路由句柄 |
| `route_loop()` | 遍历路由表 |
| `route_close()` | 关闭句柄 |

### ARP 操作 (arp.h)

| 函数 | 说明 |
|------|------|
| `arp_open()` | 打开 ARP 句柄 |
| `arp_loop()` | 遍历 ARP 缓存 |
| `arp_close()` | 关闭句柄 |

### IP 操作 (ip.h)

| 函数 | 说明 |
|------|------|
| `ip_open()` | 打开 IP 句柄 |
| `ip_send()` | 发送 IP 数据包 |
| `ip_checksum()` | 计算 IP 校验和 |
| `ip_close()` | 关闭句柄 |

### 以太网操作 (eth.h)

| 函数 | 说明 |
|------|------|
| `eth_open()` | 打开以太网设备 |
| `eth_send()` | 发送以太网帧 |
| `eth_pton()` | 文本 → MAC |
| `eth_ntop()` | MAC → 文本 |
| `eth_close()` | 关闭句柄 |

### 协议常量

#### 以太网类型

```c
#define ETH_TYPE_IP      0x0800   // IPv4
#define ETH_TYPE_ARP     0x0806   // ARP
#define ETH_TYPE_IPV6    0x86DD   // IPv6
```

#### IP 协议

```c
#define IP_PROTO_ICMP    1
#define IP_PROTO_TCP     6
#define IP_PROTO_UDP    17
```

#### TCP 标志

```c
#define TH_SYN  0x02
#define TH_ACK  0x10
#define TH_RST  0x04
```

#### ICMP 类型

```c
#define ICMP_ECHO       8   // Echo Request
#define ICMP_ECHOREPLY  0   // Echo Reply
```

---

## 常见问题

### Q1: 编译时找不到 libdnet.h

**问题**: `error: dnet.h: No such file or directory`

**解决方案**:

```bash
# 确保已安装 libdnet
cd ..
./configure
make
sudo make install

# 或使用 -I 参数指定路径
gcc -I../include -L../.libs -o test test.c -ldnet
```

### Q2: 运行时找不到 libdnet.dylib

**问题**: `dyld: Library not loaded: libdnet.dylib`

**解决方案**:

```bash
# 方法 1: 设置 DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=../.libs:$DYLD_LIBRARY_PATH
./interface_scanner

# 方法 2: 安装到系统
cd ..
sudo make install

# 方法 3: 使用 install_name_tool
install_name_tool -change @rpath/libdnet.dylib @executable_path/.libs/libdnet.dylib interface_scanner
```

### Q3: 需要 sudo 权限

**问题**: `Operation not permitted` 或 `Permission denied`

**原因**: 某些操作（如发送原始数据包、访问接口信息）需要管理员权限。

**解决方案**:

```bash
sudo ./interface_scanner
sudo ./packet_builder
```

### Q4: intf_open 返回 NULL

**问题**: `intf_open` 失败返回 NULL

**原因**: 缺少权限或系统不支持某些 ioctl 调用。

**解决方案**:

```bash
# 检查 errno
intf_t *i = intf_open();
if (i == NULL) {
    perror("intf_open");
    // 常见错误: EACCES (权限不足), EPERM (操作不允许)
}
```

### Q5: ip_send 失败

**问题**: `ip_send` 返回 -1

**原因**:

1. 没有足够的权限
2. 源地址或目标地址无效
3. 网络不可达

**解决方案**:

```bash
# 使用 sudo 运行
sudo ./packet_builder

# 检查网络连接
ping 8.8.8.8

# 使用 localhost 测试（不需要外部网络）
ip_addr_t src = htonl(0x7f000001);  // 127.0.0.1
```

### Q6: macOS 权限问题

**问题**: 即使使用 sudo，某些操作仍然失败

**原因**: macOS 的系统完整性保护 (SIP) 或网络扩展限制

**解决方案**:

```bash
# 检查 SIP 状态（不建议禁用）
csrutil status

# 使用开发者模式（需要重启）
sudo DevToolsSecurity -enable
```

### Q7: 如何调试

**解决方案**:

```bash
# 编译调试版本
make clean
gcc -Wall -Wextra -g -O0 -I../include -L../.libs \
    -o interface_scanner interface_scanner.c -ldnet

# 使用 gdb 或 lldb
lldb ./interface_scanner
(lldb) run
(lldb) bt  # 查看堆栈

# 添加调试输出
fprintf(stderr, "DEBUG: %s:%d: %s\n", __FILE__, __LINE__, __func__);
```

### Q8: 如何查看 BPF 设备

**问题**: 使用以太网发送时需要 BPF 设备

**解决方案**:

```bash
# 列出 BPF 设备
ls -l /dev/bpf*

# 检查权限
ls -la /dev/bpf*

# 修改权限（不推荐，有安全风险）
sudo chmod 666 /dev/bpf*
```

---

## 扩展示例

### 发送自定义 TCP 数据包

```c
#include <dnet.h>

int main(void) {
    ip_t *i = ip_open();
    if (i == NULL) return 1;

    uint8_t packet[64];
    ip_addr_t src_ip = htonl(0x0a000001);  // 10.0.0.1
    ip_addr_t dst_ip = htonl(0x0a000002);  // 10.0.0.2

    // 构建 TCP SYN
    ip_pack_hdr(packet, IP_TOS_DEFAULT, 40, 1234, 0, 64,
                IP_PROTO_TCP, src_ip, dst_ip);

    tcp_pack_hdr(packet + IP_HDR_LEN, 12345, 80, 1000, 0,
                 TH_SYN, 65535, 0);

    ip_checksum(packet, IP_HDR_LEN, 0);

    struct ip_hdr *ip = (struct ip_hdr *)packet;
    struct tcp_hdr *tcp = (struct tcp_hdr *)(packet + IP_HDR_LEN);
    tcp_checksum(ip, tcp, TCP_HDR_LEN);

    ip_send(i, packet, 40);
    ip_close(i);
    return 0;
}
```

### 扫描网络主机

```c
#include <dnet.h>

int scan_network(const char *network) {
    intf_t *i = intf_open();
    struct intf_entry entry;

    // 获取出接口
    struct addr dst;
    addr_pton(network, &dst);
    if (intf_get_src(i, &entry, &dst) < 0) {
        fprintf(stderr, "无法找到接口\n");
        return -1;
    }

    // 发送 ARP 请求
    arp_t *a = arp_open();
    struct arp_entry arp_entry;

    // 遍历 IP 范围并发送 ARP 请求...
    // (完整实现需要更多代码)

    arp_close(a);
    intf_close(i);
    return 0;
}
```

---

## 参考资源

- libdnet 源码: `../src/` 和 `../include/`
- libdnet 文档: `../doc/` (如果存在)
- RFC 文档:
  - RFC 791 - Internet Protocol
  - RFC 792 - Internet Control Message Protocol
  - RFC 793 - Transmission Control Protocol
  - RFC 768 - User Datagram Protocol
  - RFC 826 - An Ethernet Address Resolution Protocol

---

## 许可证

本示例代码遵循与 libdnet 相同的许可证（3-clause BSD）。

---

## 联系方式

如有问题或建议，请参考 libdnet 的官方文档或提交 issue。
