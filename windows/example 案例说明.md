# 网络接口信息获取案例说明

## 案例概述

本案例演示如何使用 libdnet 库在 Windows 系统中获取本机网络接口信息，包括：
- 接口名称和状态标志
- MTU（最大传输单元）
- IP 地址和目标地址
- MAC 地址
- IP 别名列表

## 功能说明

### 主要功能
1. **打开网络接口**：使用 `intf_open()` 函数打开网络接口
2. **遍历网络接口**：使用 `intf_loop()` 函数遍历所有网络接口
3. **显示接口信息**：打印每个接口的详细配置信息

### 核心函数

#### print_flags()
```c
static void print_flags(u_short flags)
```
- 功能：解析并打印网络接口标志位
- 参数：
  - `flags`：接口标志位
- 标志位说明：
  - `INTF_FLAG_UP`：接口已启动
  - `INTF_FLAG_LOOPBACK`：回环接口
  - `INTF_FLAG_POINTOPOINT`：点对点接口
  - `INTF_FLAG_NOARP`：不使用 ARP
  - `INTF_FLAG_BROADCAST`：支持广播
  - `INTF_FLAG_MULTICAST`：支持多播

#### print_interface()
```c
static int print_interface(const struct intf_entry *entry, void *arg)
```
- 功能：打印单个网络接口的详细信息
- 参数：
  - `entry`：网络接口条目结构体指针
  - `arg`：用户自定义参数（未使用）
- 返回值：0 表示成功

#### 主函数流程
1. 使用 `intf_open()` 打开网络接口
2. 使用 `intf_loop()` 遍历所有网络接口
3. 对每个接口调用 `print_interface()` 打印详细信息
4. 使用 `intf_close()` 关闭网络接口

## 编译方式

```bash
gcc -I../include -L../src/.libs -o example.exe example.c -ldnet -lws2_32 -liphlpapi
```

## 运行示例

```
=== libdnet Windows 使用案例 ===
获取本机网络接口信息

[以太网]
  Flags: 0x1103 UP BROADCAST MULTICAST
  MTU: 1500
  Inet: 192.168.1.100
  MAC:  00:11:22:33:44:55

[Loopback Pseudo-Interface]
  Flags: 0x0101 UP LOOPBACK
  MTU: 4294967295
  Inet: 127.0.0.1

=== 完成 ===
```

## 注意事项

1. 需要管理员权限才能获取完整的网络接口信息
2. 不同系统上接口名称可能不同
3. MTU 值对于回环接口可能显示异常大值

## 相关数据结构

### struct intf_entry
```c
struct intf_entry {
    char intf_name[INTF_NAME_LEN];  // 接口名称
    u_short intf_flags;             // 接口标志
    u_short intf_mtu;               // MTU
    struct addr intf_addr;          // 主IP地址
    struct addr intf_dst_addr;      // 目标地址（点对点接口）
    struct addr intf_link_addr;     // 链路层地址（MAC）
    struct addr intf_alias_addrs[INTF_MAX_ALIASES];  // IP别名
    int intf_alias_num;             // 别名数量
};
```

## 适用场景

- 网络配置管理
- 网络诊断工具
- 系统信息收集
- 网络监控应用
- 自动化部署脚本
