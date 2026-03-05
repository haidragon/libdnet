# test3 - 以太网帧操作示例

## 概述

本示例演示 libdnet 的以太网帧操作功能，包括以太网帧构建、设备操作、地址解析等。

## 功能说明

### 1. 以太网帧构建

- `eth_pack_hdr()` - 构建以太网帧头部

格式：
```
+----------------+----------------+
|  目标 MAC (6B) |  源 MAC (6B)   |
+----------------+----------------+---------------+
|     以太网类型 (2B)              |   数据...     |
+----------------+---------------+---------------+
```

### 2. 以太网地址操作

- `addr_pton()` - 字符串转 MAC 地址
- `addr_ntop()` - MAC 地址转字符串

常见 MAC 地址类型：
- 单播地址: 首字节最低位为 0
- 多播地址: 首字节最低位为 1
- 广播地址: `ff:ff:ff:ff:ff:ff`

### 3. 以太网设备操作

- `eth_open()` - 打开以太网设备
- `eth_close()` - 关闭以太网设备
- `eth_send()` - 发送以太网帧

注意：需要 root 权限

### 4. 以太网类型

| 类型值 | 协议 |
|--------|------|
| 0x0800 | IPv4 |
| 0x0806 | ARP |
| 0x86DD | IPv6 |
| 0x8100 | 802.1Q VLAN |
| 0x88CC | LLDP |

## 编译与运行

```bash
# 编译
make

# 运行
./eth_frame

# 或直接运行
make run

# 清理
make clean
```

## 示例输出

```
libdnet 以太网帧操作示例
======================

=== 以太网帧构建 ===
构建以太网帧:
  目标 MAC: ff:ff:ff:ff:ff:ff (广播)
  源 MAC:   00:11:22:33:44:55
  类型:     0x0800 (IPv4)

=== 以太网地址解析 ===
MAC 地址: 00:11:22:33:44:55
地址类型: Ethernet

广播 MAC: ff:ff:ff:ff:ff:ff
组播 MAC: 01:00:5e:00:00:01

=== 以太网类型 ===
常用以太网类型:
  0x0800 - IPv4
  0x0806 - ARP
  0x86DD - IPv6
  0x8847 - MPLS unicast
  0x8848 - MPLS multicast
  0x8100 - 802.1Q VLAN
  0x88A8 - 802.1ad Q-in-Q
  0x88CC - LLDP

=== 获取接口 MAC 地址 ===
接口 en0 的 MAC 地址: aa:bb:cc:dd:ee:ff

=== 以太网设备操作 ===
无法打开以太网设备 en0 (需要 root 权限)

=== 完整以太网帧示例 ===
以太网帧 (48 bytes):
  0000: ff ff ff ff ff ff 00 11 22 33 44 55 08 00 aa aa  |........"3DU....|
  0010: aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa  |................|
  0020: aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa  |................|
  0030: aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa  |................|

所有演示完成！
注意: 实际发送以太网帧需要 root 权限
```

## 相关 API

| 函数 | 描述 |
|------|------|
| `eth_open()` | 打开以太网设备 |
| `eth_close()` | 关闭以太网设备 |
| `eth_send()` | 发送以太网帧 |
| `eth_get()` | 接收以太网帧 |
| `eth_pack_hdr()` | 构建以太网头部 |
| `eth_ntohs()` | 网络字节序转主机序 |
| `eth_htons()` | 主机序转网络字节序 |

## 注意事项

1. 发送以太网帧需要 root 权限
2. 使用 `sudo ./eth_frame` 运行可能需要权限
3. MAC 地址使用冒号分隔格式
4. 广播地址用于 ARP 请求等场景
