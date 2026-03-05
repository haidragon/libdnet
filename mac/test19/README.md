# libdnet 原始 Socket 示例 (test19)

## 功能说明

本示例展示如何使用 libdnet 库创建和使用原始 IP socket，包括：
- 打开原始 IP socket
- 构建完整的 IP 数据包
- 发送自定义 IP 数据包
- 接收 IP 数据包
- ICMP Echo Request/Reply 通信

## 编译方法

```bash
make
```

## 运行方法

**注意**: 原始 socket 需要 root 权限

```bash
sudo make run
```

或者直接运行：

```bash
sudo ./raw_socket
```

## 输出示例

```
=== libdnet 原始 Socket 示例 ===

1. 打开原始 IP Socket...
   成功打开原始 IP socket

2. Socket 信息:
   Socket 类型: 原始 IP socket
   可以收发完整的 IP 数据包
   需要自己构建 IP 头部

3. 构建 ICMP Echo Request 数据包:
   IP 头:
     版本: 4
     头部长度: 20 字节
     总长度: 28 字节
     TTL: 64
     协议: 1 (ICMP)
     源地址: 127.0.0.1
     目标地址: 127.0.0.1
     校验和: 0xABCD

   ICMP 头:
     类型: 8 (Echo Request)
     代码: 0
     ID: 54321
     序列号: 1
     校验和: 0x1234

4. 发送数据包到 127.0.0.1:
   发送 28 字节成功

5. 等待响应 (最多 5 个数据包):
   ================================================

[数据包 #1]
  收到 28 字节
  IP 头长度: 20 字节
  IP 总长度: 28 字节
  协议: 1
  源: 127.0.0.1 -> 目标: 127.0.0.1
  ICMP 类型: 0 (Echo Reply)
  ICMP 代码: 0

====================================
监听完成: 共接收 1 个数据包

=== 原始 Socket 示例完成 ===
```

## API 说明

### ip_open()
打开原始 IP socket。

```c
ip_t *ip_open(void);
```

### ip_close()
关闭原始 IP socket。

```c
int ip_close(ip_t *i);
```

### ip_send()
发送 IP 数据包。

```c
ssize_t ip_send(ip_t *i, const void *buf, size_t len);
```

### ip_recv()
接收 IP 数据包。

```c
ssize_t ip_recv(ip_t *i, void *buf, size_t len);
```

## 数据包构建

### IP 头结构
```c
struct ip_hdr {
    uint8_t  ip_v;      // 版本
    uint8_t  ip_hl;     // 头部长度（以32位字为单位）
    uint8_t  ip_tos;    // 服务类型
    uint16_t ip_len;    // 总长度
    uint16_t ip_id;     // 标识
    uint16_t ip_off;    // 分片偏移
    uint8_t  ip_ttl;    // 生存时间
    uint8_t  ip_p;      // 协议
    uint16_t ip_sum;    // 校验和
    struct in_addr ip_src;   // 源地址
    struct in_addr ip_dst;   // 目标地址
};
```

## 使用步骤

1. **打开 socket**: `ip_open()`
2. **构建数据包**:
   - 设置 IP 头部字段
   - 添加协议头部（TCP/UDP/ICMP）
   - 计算校验和
3. **发送**: `ip_send()`
4. **接收**: `ip_recv()`
5. **关闭**: `ip_close()`

## 注意事项

1. **权限要求**: 原始 socket 需要 root 权限
2. **自行构建**: 必须手动构建 IP 头部和协议头部
3. **校验和**: 需要正确计算 IP 和协议校验和
4. **字节序**: 注意网络字节序（大端）
5. **安全**: 原始 socket 可能被滥用，需谨慎使用

## 与标准 socket 的区别

| 特性 | 标准 Socket | 原始 Socket |
|------|-----------|------------|
| 协议 | 指定（TCP/UDP） | 可自定义 |
| 头部 | 自动构建 | 手动构建 |
| 权限 | 不需要 | 需要 root |
| 用途 | 应用通信 | 网络工具/协议开发 |

## 常见应用

- Ping 工具（ICMP）
- Traceroute
- 网络扫描
- 自定义协议实现
- 网络测试工具

## 依赖

- libdnet
- root 权限
