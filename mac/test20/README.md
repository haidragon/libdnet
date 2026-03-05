# libdnet 数据包注入示例 (test20)

## 功能说明

本示例展示如何使用 libdnet 库注入自定义数据包到网络，包括：
- 打开以太网接口
- 构建完整的数据包（以太网 + IP + UDP + 载荷）
- 计算各层校验和
- 注入数据包到网络

## 编译方法

```bash
make
```

## 运行方法

**注意**: 数据包注入需要 root 权限

```bash
sudo make run
```

或者直接运行：

```bash
sudo ./packet_injection
```

**建议同时使用 tcpdump 抓包观察**:

```bash
sudo tcpdump -i en0 udp port 54321
```

## 输出示例

```
=== libdnet 数据包注入示例 ===

1. 打开以太网接口 en0:
   成功打开接口 en0

2. 构建以太网帧:
   目标 MAC: FF:FF:FF:FF:FF:FF (广播)
   源 MAC: 02:00:00:00:00:01
   以太网类型: 0x0800 (IP)

3. 构建 IP 数据包:
   版本: 4
   头部长度: 20 字节
   TTL: 64
   协议: 17 (UDP)
   源 IP: 192.168.1.100
   目标 IP: 192.168.1.255 (广播)

4. 构建 UDP 数据包:
   源端口: 12345
   目标端口: 54321

5. 添加载荷:
   载荷: "libdnet packet injection test"
   载荷长度: 30 字节

6. 计算各层长度:
   UDP 长度: 38 字节
   IP 长度: 58 字节
   总长度: 78 字节

7. 计算校验和:
   UDP 校验和: 0x1234
   IP 校验和: 0xABCD

8. 注入数据包 (共 5 次):
   ================================================

[数据包 #1] 注入中...
   成功注入 78 字节
   时间戳: 1234567890

====================================
注入完成: 共注入 5 个数据包
```

## 数据包结构

```
+------------------+
|  以太网头 (14B)  |
|  - 目标 MAC      |
|  - 源 MAC        |
|  - 以太网类型    |
+------------------+
|  IP 头 (20B)     |
|  - 版本/长度     |
|  - TOS           |
|  - 总长度        |
|  - ID            |
|  - 分片          |
|  - TTL           |
|  - 协议          |
|  - 校验和        |
|  - 源 IP         |
|  - 目标 IP       |
+------------------+
|  UDP 头 (8B)     |
|  - 源端口        |
|  - 目标端口      |
|  - UDP 长度      |
|  - UDP 校验和    |
+------------------+
|  载荷 (变长)     |
+------------------+
```

## API 说明

### eth_open()
打开以太网接口。

```c
eth_t *eth_open(const char *device);
```

### eth_send()
发送以太网帧。

```c
ssize_t eth_send(eth_t *e, const void *buf, size_t len);
```

### eth_close()
关闭以太网接口。

```c
int eth_close(eth_t *e);
```

## 校验和计算

### IP 校验和
```c
ip_hdr->ip_sum = 0;
ip_hdr->ip_sum = ip_cksum(ip_hdr);
```

### UDP 校验和
UDP 校验和需要包含伪首部（12字节）：

```c
struct pseudo_header {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_len;
};
```

## 使用场景

1. **网络测试**: 生成特定流量进行测试
2. **协议开发**: 开发新的网络协议
3. **安全测试**: 网络渗透测试
4. **网络工具**: 自定义网络工具
5. **故障诊断**: 发送测试数据包

## 注意事项

1. **权限要求**: 需要 root 权限
2. **合规性**: 仅用于授权的测试环境
3. **网络影响**: 大量注入可能影响网络
4. **正确性**: 确保数据包格式正确
5. **抓包验证**: 使用 tcpdump 或 Wireshark 验证

## 抓包命令

使用 tcpdump 查看注入的数据包：

```bash
sudo tcpdump -i en0 udp
sudo tcpdump -i en0 udp port 54321
sudo tcpdump -i en0 -v udp port 54321
```

使用 Wireshark 可以进行更详细的分析。

## 依赖

- libdnet
- root 权限
- 网络接口
