# libdnet 防火墙操作示例 (test14)

## 功能说明

本示例展示如何使用 libdnet 库操作防火墙规则，包括：
- 创建防火墙规则
- 添加/删除防火墙规则
- 遍历现有规则
- 理解规则类型和参数

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
./firewall_ops
```

## 输出示例

```
=== libdnet 防火墙操作示例 ===

1. 创建防火墙规则示例:
   规则 #1:
     设备: any
     方向: IN (入站)
     协议: 6
     源地址: 10.0.0.0/24
     目标端口: 22-22
     操作: BLOCK (阻止)

   规则 #2:
     设备: any
     方向: OUT (出站)
     协议: 6
     目标端口: 80-80
     操作: ALLOW (允许)

   规则 #3:
     设备: any
     方向: OUT (出站)
     操作: BLOCK (阻止)

   规则 #4:
     设备: any
     方向: BOTH (双向)
     协议: 6
     源地址: 192.168.0.0/16
     目标地址: 192.168.0.0/16
     操作: ALLOW (允许)

2. 尝试添加防火墙规则:
   注意: 实际添加规则需要管理员权限，此处仅演示
   无法添加规则 (权限不足或系统限制)

3. 遍历现有防火墙规则:
   无法遍历规则 (权限不足或系统限制)

4. 防火墙规则类型说明:
   方向 (fw_dir):
     - FW_DIR_IN: 入站规则
     - FW_DIR_OUT: 出站规则
     - FW_DIR_BOTH: 双向规则

   操作 (fw_op):
     - FW_OP_ALLOW: 允许通过
     - FW_OP_BLOCK: 阻止通过

=== 防火墙操作示例完成 ===
```

## API 说明

### fw_open()
打开防火墙操作句柄。

```c
fw_t *fw_open(void);
```

### fw_close()
关闭防火墙操作句柄。

```c
int fw_close(fw_t *fw);
```

### fw_add()
添加防火墙规则。

```c
int fw_add(fw_t *fw, struct fw_rule *rule);
```

### fw_delete()
删除防火墙规则。

```c
int fw_delete(fw_t *fw, struct fw_rule *rule);
```

### fw_loop()
遍历防火墙规则。

```c
int fw_loop(fw_t *fw, struct fw_rule *rule);
```

## 规则参数

- `fw_dir`: 规则方向 (IN/OUT/BOTH)
- `fw_op`: 规则操作 (ALLOW/BLOCK)
- `fw_proto`: 协议类型 (TCP/UDP/ICMP 等)
- `fw_src`: 源地址和掩码
- `fw_dst`: 目标地址和掩码
- `fw_sport`: 源端口范围
- `fw_dport`: 目标端口范围
- `fw_device`: 网络设备名称

## 注意事项

1. 实际添加/删除规则需要 root 权限
2. macOS 的防火墙行为与其他系统可能不同
3. 防火墙规则可能有系统级限制

## 依赖

- libdnet
- 系统防火墙支持
- 管理员权限（用于实际修改规则）
