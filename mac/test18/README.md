# libdnet IP 选项示例 (test18)

## 功能说明

本示例展示如何使用 libdnet 库构建和解析 IP 数据包选项，包括：
- Record Route（记录路径）
- Loose Source Routing（宽松源路由）
- Strict Source Routing（严格源路由）
- Time Stamp（时间戳）
- Security（安全选项）
- 选项解析

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
./ip_options
```

## 输出示例

```
=== libdnet IP 选项示例 ===

   === IP 选项构建 ===

   1. Record Route (记录路径):
   选项类型: 7 (Record Route)
   选项长度: 11 字节
   选项数据:
   07 0b 03 00 00 00 00 00 00 00 00

   2. Loose Source Routing (宽松源路由):
   选项类型: 131 (Loose Source Routing)
   选项长度: 7 字节
   指定网关: 10.0.0.1
   选项数据:
   83 07 04 0a 00 00 01

   3. Strict Source Routing (严格源路由):
   选项类型: 137 (Strict Source Routing)
   选项长度: 7 字节
   指定网关: 192.168.1.1
   选项数据:
   89 07 04 c0 a8 01 01

   4. Time Stamp (时间戳):
   选项类型: 68 (Time Stamp)
   选项长度: 8 字节
   标志: 1 (仅时间戳)
   时间戳: 1234567890
   选项数据:
   44 08 05 01 49 96 02 d2

   === IP 选项说明 ===
   常用 IP 选项:
   - 0 (EOL): End of Option List
   - 1 (NOP): No Operation
   - 7: Record Route
   - 68: Time Stamp
   - 130: Security
   - 131: Loose Source Routing
   - 137: Strict Source Routing

=== IP 选项示例完成 ===
```

## IP 选项类型

| 类型 | 名称 | 说明 |
|------|------|------|
| 0 | EOL | 选项列表结束 |
| 1 | NOP | 无操作（用于对齐） |
| 7 | Record Route | 记录路径，最多9个IP |
| 68 | Time Stamp | 记录时间戳 |
| 130 | Security | 安全信息 |
| 131 | Loose Source Routing | 宽松源路由 |
| 137 | Strict Source Routing | 严格源路由 |

## 选项格式

每个 IP 选项的格式：

```
+--------+--------+--------+--------+ ... +--------+
|  Type  | Length |  Pointer|  Data  | ... |  Data  |
+--------+--------+--------+--------+ ... +--------+
```

- **Type**: 选项类型（1字节）
- **Length**: 选项总长度（1字节，EOL和NOP除外）
- **Pointer**: 指针字段（某些选项使用）
- **Data**: 选项数据（变长）

## 选项详解

### Record Route (选项7)
- 最多记录9个IP地址
- 每个路由器添加自己的IP
- 指针指向下一个空槽位

### Source Routing
- **Loose**: 可以经过中间路由器
- **Strict**: 必须严格按指定路径

### Time Stamp (选项68)
标志值：
- 0: 仅时间戳
- 1: IP地址 + 时间戳
- 3: IP地址 + 时间戳（预设IP）

## 注意事项

1. **安全风险**: IP选项可能被用于网络扫描或攻击
2. **性能影响**: 每个路由器需要处理选项
3. **MPLS限制**: 最大40字节的选项空间
4. **防火墙**: 可能被防火墙丢弃
5. **现代网络**: 使用较少，tracoute使用ICMP/UDP

## 安全建议

- 不要在生产环境中使用Source Routing
- 考虑禁用IP选项处理以提高性能
- 使用防火墙过滤带选项的数据包

## 依赖

- libdnet
