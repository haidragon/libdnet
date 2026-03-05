# libdnet 接口操作示例 (test12)

## 功能说明

本示例展示如何使用 libdnet 库进行网络接口操作，包括：
- 遍历所有网络接口
- 获取指定接口信息
- 获取默认接口
- 检查接口状态
- 接口索引与名称转换

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
./interface_ops
```

## 输出示例

```
=== libdnet 接口操作示例 ===

1. 遍历所有网络接口:
   名称       描述                 MAC地址           IP地址
   ----       ----                 -------           -------
   lo0        Loopback             N/A              127.0.0.1
   en0        Wi-Fi                AA:BB:CC:DD:EE:FF 192.168.1.100
   ...

2. 获取指定接口信息:
   接口名称: en0
   接口描述: Wi-Fi
   接口类型: ...
   MTU: 1500
   索引: 5
   IP地址: 192.168.1.100
   MAC地址: AA:BB:CC:DD:EE:FF
   标志: 0x...

3. 获取默认接口:
   默认接口名称: en0
   默认接口IP: 192.168.1.100

4. 检查接口是否启用:
   接口 en0 状态: UP (启用)

5. 接口列表信息:
   接口总数: ...

6. 接口索引查询:
   en0 接口索引: 5
   lo0 接口索引: 1

7. 接口名称查询:
   索引 5 的接口名称: en0

=== 接口操作示例完成 ===
```

## API 说明

### intf_open()
打开接口操作句柄。

```c
int_t *intf_open(void);
```

### intf_close()
关闭接口操作句柄。

```c
int intf_close(int_t *iop);
```

### intf_loop()
遍历所有接口。

```c
int intf_loop(int_t *iop, struct intf_entry *entry, int (*callback)(int_t *, const struct intf_entry *));
```

### intf_get()
获取指定接口信息。

```c
int intf_get(int_t *iop, struct intf_entry *entry, const char *name);
```

### intf_get_src()
获取默认接口或指定源地址的接口。

```c
int intf_get_src(int_t *iop, struct intf_entry *entry, struct addr *src, int bits);
```

### intf_name_to_index()
根据接口名称获取索引。

```c
int intf_name_to_index(const char *name);
```

### intf_index_to_name()
根据接口索引获取名称。

```c
int intf_index_to_name(int idx, char *name, size_t len);
```

## 注意事项

1. 此程序需要 root 权限才能获取完整的接口信息
2. 接口名称可能因系统而异（如 eth0, en0 等）
3. 某些接口可能没有 IP 或 MAC 地址

## 依赖

- libdnet
- 系统网络接口
