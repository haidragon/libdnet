# 路由表查询案例说明

## 案例概述

本案例演示如何使用 libdnet 库在 Windows 系统中查询系统路由表，包括：
- 打开路由表
- 遍历显示所有路由条目
- 显示每个路由的目标地址、子网掩码、网关和出口接口

## 功能说明

### 主要功能
1. **打开路由表**：使用 `route_open()` 函数打开系统路由表
2. **遍历路由表**：使用 `route_loop()` 函数遍历所有路由条目
3. **显示路由信息**：打印每个路由的详细信息

### 核心函数

#### print_route_entry()
```c
static int print_route_entry(const struct route_entry *entry, void *arg)
```
- 功能：打印单个路由条目的详细信息
- 参数：
  - `entry`：路由条目结构体指针
  - `arg`：用户自定义参数（未使用）
- 返回值：0 表示成功
- 显示信息：
  - 目标地址和子网掩码（CIDR 表示法）
  - 网关地址
  - 出口接口名称（如果存在）

#### 主函数流程
1. 使用 `route_open()` 打开路由表
2. 使用 `route_loop()` 遍历所有路由条目
3. 对每个路由条目调用 `print_route_entry()` 打印详细信息
4. 使用 `route_close()` 关闭路由表

## 编译方式

```bash
gcc -I../include -L../src/.libs -o route_example.exe route_example.c -ldnet -lws2_32 -liphlpapi
```

## 运行示例

```
=== libdnet 路由表使用案例 ===

系统路由表：
  0.0.0.0/0 -> 192.168.1.1 (via 以太网)
  127.0.0.0/8 -> 127.0.0.1 (via Loopback Pseudo-Interface)
  192.168.1.0/24 -> 192.168.1.100 (via 以太网)
  192.168.1.100/32 -> 127.0.0.1 (via Loopback Pseudo-Interface)

=== 完成 ===
```

## 注意事项

1. 需要管理员权限才能访问完整的路由表信息
2. 路由条目数量和内容取决于系统网络配置
3. 接口名称可能因系统而异

## 相关数据结构

### struct route_entry
```c
struct route_entry {
    struct addr route_dst;     // 目标地址
    struct addr route_gw;      // 网关地址
    char intf_name[INTF_NAME_LEN];  // 出口接口名称
    // 其他字段...
};
```

### struct addr
```c
struct addr {
    uint16_t addr_type;        // 地址类型（IP、MAC 等）
    uint16_t addr_bits;        // 地址位数（子网掩码）
    union {
        // 各种地址类型的联合体
        // ...
    } addr_data;
};
```

## 路由表字段说明

- **目标地址**：数据包的目标网络或主机地址
- **子网掩码**：目标地址的子网掩码，以 CIDR 表示法显示（如 /24）
- **网关**：下一跳路由器的 IP 地址
- **出口接口**：数据包发出的网络接口

## 适用场景

- 网络路由分析
- 网络故障诊断
- 网络拓扑发现
- 路由配置管理
- 网络监控工具
- 安全审计

## 常见路由类型

1. **默认路由**：0.0.0.0/0，用于所有不匹配其他路由的数据包
2. **主机路由**：/32 掩码，指向特定主机
3. **网络路由**：指向特定网络段
4. **回环路由**：127.0.0.0/8，用于本地通信

## 相关案例

- `arp_example.exe`：ARP 表操作
- `example.exe`：网络接口信息
- `ping_icmp.exe`：ICMP ping 工具
- `traceroute.exe`：路由追踪工具
