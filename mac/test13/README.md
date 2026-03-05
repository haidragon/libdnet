# libdnet 网关查询示例 (test13)

## 功能说明

本示例展示如何使用 libdnet 库查询网络网关信息，包括：
- 通过路由表查询默认网关
- 获取各网络接口信息
- 查询访问特定目标的网关
- 遍历路由表

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
./gateway_query
```

## 输出示例

```
=== libdnet 网关查询示例 ===

1. 通过路由表查询默认网关:
   默认网关: 192.168.1.1
   目标网络: 0.0.0.0/0
   出接口: en0

2. 获取各网络接口的路由信息:
   接口       MAC地址           状态
   ----       -------           ----
   en0        AA:BB:CC:DD:EE:FF UP
   lo0        N/A               UP
   en1        DD:EE:FF:AA:BB:CC DOWN

3. 查询访问特定目标的网关:
   目标 8.8.8.8           -> 网关: 192.168.1.1   (接口: en0)
   目标 192.168.1.1      -> 直连 (接口: en0)
   目标 10.0.0.1         -> 网关: 192.168.1.1   (接口: en0)

4. 遍历路由表 (前5条):
   目标网络           网关               接口          标志
   --------           ---               ----          ----
   127.0.0.1/32       -                 lo0           0x1
   192.168.1.0/24    -                 en0           0x1
   0.0.0.0/0          192.168.1.1       en0           0x3

=== 网关查询示例完成 ===
```

## API 说明

### route_open()
打开路由操作句柄。

```c
route_t *route_open(void);
```

### route_close()
关闭路由操作句柄。

```c
int route_close(route_t *rt);
```

### route_get()
获取指定路由条目。

```c
int route_get(route_t *rt, struct route_entry *entry);
```

### route_loop()
遍历所有路由条目。

```c
int route_loop(route_t *rt, struct route_entry *entry);
```

### addr_pton()
地址字符串到二进制转换。

```c
int addr_pton(const char *src, struct addr *dst);
```

### addr_ntop()
地址二进制到字符串转换。

```c
int addr_ntop(const struct addr *src, char *dst, size_t size);
```

## 网关查询方法

1. **默认网关**: 查询路由表中目标为 0.0.0.0/0 的条目
2. **特定网关**: 设置目标地址，查询对应的网关
3. **直连路由**: 如果网关地址为空，表示目标在本地网络

## 注意事项

1. 查询路由信息通常需要 root 权限
2. 网关地址可能因网络配置而异
3. 本地回环接口 (lo0) 通常有特殊路由

## 依赖

- libdnet
- 系统路由表
