# 获取本机默认IP地址案例说明

## 案例概述

本案例是 libdnet 库在 Windows 系统中的最简示例，演示如何：
- 打开网络接口
- 获取本机默认 IP 地址
- 获取接口名称和 MAC 地址

## 功能说明

### 主要功能
1. **打开网络接口**：使用 `intf_open()` 函数打开网络接口
2. **查询接口信息**：使用 `intf_get_src()` 函数获取接口详细信息
3. **显示基本信息**：打印接口名称、IP 地址和 MAC 地址

### 核心函数

#### 主函数流程
1. 使用 `intf_open()` 打开网络接口
2. 使用 `addr_aton()` 将字符串形式的 IP 地址转换为地址结构
3. 使用 `intf_get_src()` 获取接口信息
4. 使用 `addr_ntoa()` 将地址结构转换为字符串形式并显示
5. 使用 `intf_close()` 关闭网络接口

### 关键函数说明

- `intf_open()`：打开网络接口操作句柄
- `intf_get_src()`：根据目标地址获取源接口信息
- `addr_aton()`：将字符串地址转换为地址结构
- `addr_ntoa()`：将地址结构转换为字符串

## 编译方式

```bash
gcc -I../include -L../src/.libs -o simple.exe simple.c -ldnet -lws2_32 -liphlpapi
```

## 运行示例

```
获取本机默认 IP 地址...
接口名称: 以太网
IP 地址:  192.168.1.100
MAC 地址: 00:11:22:33:44:55
```

## 注意事项

1. 需要管理员权限才能获取完整的接口信息
2. 接口名称可能因系统而异
3. 如果系统有多个网络接口，返回的是与 127.0.0.1 关联的接口

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
    size_t intf_len;                // 结构体长度
};
```

### struct addr
```c
struct addr {
    uint16_t addr_type;        // 地址类型（IP、MAC 等）
    uint16_t addr_bits;        // 地址位数
    union {
        // 各种地址类型的联合体
        // ...
    } addr_data;
};
```

## 地址类型说明

- `ADDR_TYPE_IP`：IP 地址类型
- `ADDR_TYPE_ETH`：以太网 MAC 地址类型
- `ADDR_TYPE_IP6`：IPv6 地址类型

## 适用场景

- 获取本机网络配置
- 网络应用程序初始化
- 系统信息收集
- 网络诊断工具
- 自动化部署脚本

## 学习价值

本案例是 libdnet 库的入门示例，适合：
- 初次接触 libdnet 的开发者
- 学习网络编程基础知识
- 了解 libdnet 的基本 API 使用方法
- 快速验证 libdnet 环境配置

## 相关案例

- `example.exe`：完整的网络接口信息获取
- `arp_example.exe`：ARP 表操作
- `route_example.exe`：路由表查询
- `ping_icmp.exe`：ICMP ping 工具
