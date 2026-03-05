# TCP SYN Flood 攻击示例说明

## 案例概述

本案例演示如何使用 libdnet 库发送 TCP SYN 数据包，用于测试目的。程序会向指定目标 IP 和端口发送多个带有随机源 IP 的 TCP SYN 数据包。

⚠️ **重要警告**：本案例仅用于测试和学习目的，严禁用于非法攻击！

## 功能说明

### 主要功能
1. **发送 TCP SYN 数据包**：向目标主机发送大量 TCP SYN 数据包
2. **随机源地址**：每个数据包使用随机生成的源 IP 地址和端口
3. **自定义目标**：可指定目标 IP、端口和数据包数量

### 核心函数

#### in_cksum()
```c
static uint16_t in_cksum(const u_char *buf, size_t len)
```
- 功能：计算 IP 校验和
- 参数：
  - `buf`：数据缓冲区指针
  - `len`：数据长度
- 返回值：16 位校验和

#### 主函数流程
1. 解析命令行参数（目标 IP、端口、数据包数量）
2. 初始化随机数生成器
3. 打开 IP 接口
4. 构造 IP 和 TCP 头部
5. 循环发送 TCP SYN 数据包
6. 关闭 IP 接口

## 编译方式

```bash
gcc -I../include -L../src/.libs -o tcp_syn_flood.exe tcp_syn_flood.c -ldnet -lws2_32 -liphlpapi
```

## 运行方式

```bash
tcp_syn_flood.exe <dst_ip> <dst_port> <count>
```

### 参数说明
- `dst_ip`：目标 IP 地址
- `dst_port`：目标端口号
- `count`：要发送的数据包数量

### 运行示例

```bash
tcp_syn_flood.exe 192.168.1.100 80 50
```

### 输出示例

```
=== libdnet TCP SYN Flood Example ===
Target: 192.168.1.100:80
Packet count: 50

WARNING: This is for testing purposes only!

Sending TCP SYN packets...
Sent 10/50 SYN packets...
Sent 20/50 SYN packets...
Sent 30/50 SYN packets...
Sent 40/50 SYN packets...
Sent 50/50 SYN packets...
[OK] TCP SYN flood completed

=== Done ===
```

## 注意事项

1. ⚠️ **必须以管理员权限运行**
2. ⚠️ **仅用于授权的测试环境**
3. ⚠️ **严禁用于未经授权的目标**
4. 需要正确配置网络环境
5. 目标主机可能会触发防火墙或入侵检测系统

## 数据包结构

### IP 头部字段
```c
iph->ip_v = 4;                    // IP 版本
iph->ip_hl = 5;                   // IP 头部长度（5 * 4 = 20 字节）
iph->ip_tos = 0;                  // 服务类型
iph->ip_len = htons(40);          // 总长度（IP + TCP 头部）
iph->ip_off = 0;                  // 片偏移
iph->ip_ttl = 64;                 // 生存时间
iph->ip_p = IP_PROTO_TCP;         // 协议类型（TCP）
iph->ip_sum = 0;                  // 校验和（稍后计算）
```

### TCP 头部字段
```c
tcph->th_off = 5;                 // TCP 头部长度（5 * 4 = 20 字节）
tcph->th_flags = TH_SYN;          // 标志位（SYN）
tcph->th_win = htons(65535);      // 窗口大小
tcph->th_sum = 0;                 // 校验和
tcph->th_urp = 0;                 // 紧急指针
```

## TCP SYN Flood 原理

TCP SYN Flood 是一种拒绝服务攻击，其原理是：
1. 攻击者向目标发送大量 TCP SYN 数据包
2. 每个数据包使用不同的源 IP 地址
3. 目标主机为每个 SYN 分配资源并回复 SYN-ACK
4. 由于源 IP 是伪造的，目标永远收不到 ACK
5. 目标主机的连接队列被耗尽，无法处理新的合法连接

## 防御措施

1. **启用 SYN Cookies**：不预先分配资源，而是基于 TCP 参数计算 cookie
2. **增加连接队列大小**：提高半连接队列的容量
3. **启用 RST Cookies**：对可疑 SYN 包回复 RST
4. **部署防火墙**：过滤可疑流量
5. **入侵检测系统**：检测异常流量模式

## 适用场景

⚠️ **仅限以下合法场景**：
- 网络安全培训
- 渗透测试（经授权）
- 系统压力测试
- 防火墙和 IDS 测试
- 网络安全研究

## 法律声明

本案例仅用于教育和测试目的。未经授权使用此工具进行网络攻击是非法的，可能导致：
- 民事责任
- 刑事指控
- 网络访问被终止
- 其他法律后果

## 相关案例

- `ip_send.exe`：发送原始 IP 数据包
- `port_scan.exe`：端口扫描工具
- `ping_icmp.exe`：ICMP ping 工具
- `udp_flood.exe`：UDP Flood 攻击示例
