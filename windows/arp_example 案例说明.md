# ARP 表操作案例说明

## 案例概述

本案例演示如何使用 libdnet 库在 Windows 系统中操作 ARP 表，包括：
- 打开 ARP 表
- 遍历显示所有 ARP 条目
- 查询指定 IP 地址的 ARP 条目

## 功能说明

### 主要功能
1. **打开 ARP 表**：使用 `arp_open()` 函数打开系统的 ARP 表
2. **遍历 ARP 表**：使用 `arp_loop()` 函数遍历并打印所有 ARP 条目
3. **查询 ARP 条目**：使用 `arp_get()` 函数查询指定 IP 地址的 MAC 地址

### 核心函数

#### print_arp_entry()
```c
static int print_arp_entry(const struct arp_entry *entry, void *arg)
```
- 功能：打印单个 ARP 条目
- 参数：
  - `entry`：ARP 条目结构体指针
  - `arg`：用户自定义参数（未使用）
- 返回值：0 表示成功

#### 主函数流程
1. 使用 `arp_open()` 打开 ARP 表
2. 使用 `arp_loop()` 遍历所有 ARP 条目并显示
3. 使用 `arp_get()` 查询指定 IP（192.168.1.1）的 ARP 信息
4. 使用 `arp_close()` 关闭 ARP 表

## 编译方式

```bash
gcc -I../include -L../src/.libs -o arp_example.exe arp_example.c -ldnet -lws2_32 -liphlpapi
```

## 运行示例

```
=== libdnet ARP 使用案例 ===

当前 ARP 表：
  192.168.1.1 -> 00:11:22:33:44:55
  192.168.1.100 -> aa:bb:cc:dd:ee:ff

查询网关 ARP 信息（192.168.1.1）：
  192.168.1.1 -> 00:11:22:33:44:55

=== 完成 ===
```

## 注意事项

1. 需要管理员权限才能访问 ARP 表
2. 查询的 IP 地址需要根据实际网络环境修改
3. 如果查询的 IP 不在 ARP 表中，会显示"未找到 ARP 条目"

## 相关数据结构

### struct arp_entry
```c
struct arp_entry {
    struct addr arp_pa;  // 协议地址（IP地址）
    struct addr arp_ha;  // 硬件地址（MAC地址）
};
```

## 适用场景

- 网络设备发现
- 网络拓扑分析
- 网络故障诊断
- 安全审计
