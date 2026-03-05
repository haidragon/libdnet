# libdnet Windows 平台深入源码分析

## 概述

libdnet 在 Windows 平台上的实现主要依赖 Windows IP Helper API (`iphlpapi.dll`) 和 Winsock API。由于 Windows 架构与 Unix 系统差异较大，某些功能（如原始以太网帧发送）需要依赖第三方库（如 WinPcap）。

## Windows 平台实现文件清单

```
src/
├── eth-win32.c      // 以太网（空实现，依赖 WinPcap）
├── arp-win32.c      // ARP 表操作
├── route-win32.c    // 路由表操作
├── intf-win32.c     // 网络接口配置
├── ip-win32.c       // IP 层数据包发送
├── fw-pktfilter.c   // 防火墙（依赖 PktFilter）
└── intf-win32.c     // 接口管理
```

---

## 1. 接口管理 (intf-win32.c)

### 数据结构

```c
// 22-29行: 接口类型组合
struct ifcombo {
    struct {
        DWORD   ipv4;  // IPv4 接口索引
        DWORD   ipv6;  // IPv6 接口索引
    } *idx;
    int      cnt;     // 当前数量
    int      max;     // 最大容量
};

// 35-38行: 接口句柄
struct intf_handle {
    struct ifcombo              ifcombo[MIB_IF_TYPE_MAX];
    IP_ADAPTER_ADDRESSES       *iftable;  // 适配器地址表
};
```

### 核心实现

#### 刷新适配器表

```c
// 219-269行: _refresh_tables
static int _refresh_tables(intf_t *intf)
{
    IP_ADAPTER_ADDRESSES *p;
    DWORD ret;
    ULONG len;

    // 初始分配 16KB 缓冲区（避免 Windows 2003 的 BUG）
    len = 16384;
    do {
        free(p);
        p = malloc(len);
        if (p == NULL)
            return (-1);

        // 获取所有适配器地址信息
        // GAA_FLAG_INCLUDE_PREFIX: 包含前缀信息（子网掩码）
        // GAA_FLAG_SKIP_ANYCAST: 跳过任播地址
        // GAA_FLAG_SKIP_MULTICAST: 跳过多播地址
        ret = GetAdaptersAddresses(AF_UNSPEC,
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
            NULL, p, &len);
    } while (ret == ERROR_BUFFER_OVERFLOW);  // 缓冲区不足则重试

    if (ret != NO_ERROR) {
        free(p);
        return (-1);
    }
    intf->iftable = p;

    // 映射 Windows 接口索引到 libdnet 的索引
    for (p = intf->iftable; p != NULL; p = p->Next) {
        int type = _if_type_canonicalize(p->IfType);
        if (type < MIB_IF_TYPE_MAX)
            _ifcombo_add(&intf->ifcombo[type],
                          p->IfIndex,       // IPv4 索引
                          p->Ipv6IfIndex);  // IPv6 索引
        else
            return (-1);
    }
    return (0);
}
```

**关键 API：**
- `GetAdaptersAddresses()`: 获取所有网络适配器信息
- 支持同时获取 IPv4 和 IPv6 信息
- 返回链表结构 `IP_ADAPTER_ADDRESSES`

---

#### 适配器信息转换

```c
// 124-217行: _adapter_address_to_entry
static void _adapter_address_to_entry(intf_t *intf,
                                       IP_ADAPTER_ADDRESSES *a,
                                       struct intf_entry *entry)
{
    // 1. 设置接口名称
    type = _if_type_canonicalize(a->IfType);
    snprintf(entry->intf_name, sizeof(entry->intf_name),
             "%s%u", _ifcombo_name(a->IfType), i);

    // 2. 设置操作系统接口名称
    strncpy_wchar(friendly_name, a->FriendlyName, MAX_INTERFACE_NAME_LEN);
    snprintf(entry->os_intf_name, MAX_INTERFACE_NAME_LEN, "%s %s",
             friendly_name, a->AdapterName);

    // 3. 设置 WinPcap 接口名称（用于原始套接字）
    const char *name_start = strstr(a->AdapterName, "{");
    if (name_start) {
        snprintf(entry->pcap_intf_name, MAX_INTERFACE_NAME_LEN - 1,
                 "\\Device\\NPF_%s", name_start);
    }

    // 4. 设置接口标志
    entry->intf_flags = 0;
    if (a->OperStatus == IfOperStatusUp)
        entry->intf_flags |= INTF_FLAG_UP;
    if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        entry->intf_flags |= INTF_FLAG_LOOPBACK;
    else
        entry->intf_flags |= INTF_FLAG_MULTICAST;

    // 5. 获取 MTU
    entry->intf_mtu = a->Mtu;

    // 6. 获取硬件地址（MAC）
    if (a->PhysicalAddressLength == ETH_ADDR_LEN) {
        entry->intf_link_addr.addr_type = ADDR_TYPE_ETH;
        entry->intf_link_addr.addr_bits = ETH_ADDR_BITS;
        memcpy(&entry->intf_link_addr.addr_eth,
               a->PhysicalAddress, ETH_ADDR_LEN);
    }

    // 7. 获取 IP 地址
    for (addr = a->FirstUnicastAddress; addr != NULL; addr = addr->Next) {
        // 查找匹配的前缀长度（子网掩码）
        for (prefix = a->FirstPrefix; prefix != NULL; prefix = prefix->Next) {
            if (prefix->Address.lpSockaddr->sa_family ==
                addr->Address.lpSockaddr->sa_family) {
                bits = (unsigned short) prefix->PrefixLength;
                break;
            }
        }

        if (entry->intf_addr.addr_type == ADDR_TYPE_NONE) {
            // 设置主地址
            addr_ston(addr->Address.lpSockaddr, &entry->intf_addr);
            entry->intf_addr.addr_bits = bits;
        } else if (ap < lap) {
            // 设置别名地址
            addr_ston(addr->Address.lpSockaddr, ap);
            ap->addr_bits = bits;
            ap++;
            entry->intf_alias_num++;
        }
    }
}
```

**关键信息映射：**
| Windows 结构 | libdnet 字段 | 说明 |
|-------------|--------------|------|
| `a->OperStatus` | `intf_flags` | 接口状态 |
| `a->Mtu` | `intf_mtu` | 最大传输单元 |
| `a->PhysicalAddress` | `intf_link_addr` | MAC 地址 |
| `a->FirstUnicastAddress` | `intf_addr` | IP 地址 |
| `prefix->PrefixLength` | `addr_bits` | 子网掩码长度 |

---

#### 遍历接口

```c
// 396-416行: intf_loop
int intf_loop(intf_t *intf, intf_handler callback, void *arg)
{
    IP_ADAPTER_ADDRESSES *a;
    struct intf_entry *entry;
    u_char ebuf[1024];
    int ret;

    if (_refresh_tables(intf) < 0)
        return (-1);

    entry = (struct intf_entry *)ebuf;

    // 遍历所有适配器
    for (a = intf->iftable; a != NULL; a = a->Next) {
        entry->intf_len = sizeof(ebuf);
        _adapter_address_to_entry(intf, a, entry);

        if ((ret = (*callback)(entry, arg)) != 0)
            break;
    }
    return (ret);
}
```

---

## 2. ARP 表操作 (arp-win32.c)

### 数据结构

```c
// 20-22行: ARP 句柄
struct arp_handle {
    MIB_IPNETTABLE *iptable;  // IP 网络表
};
```

### 核心实现

#### 添加 ARP 条目

```c
// 30-50行: arp_add
int arp_add(arp_t *arp, const struct arp_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    MIB_IPNETROW iprow;

    // 1. 查找到达目标 IP 的最佳路由
    if (GetBestRoute(entry->arp_pa.addr_ip,
                     IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    // 2. 设置 ARP 条目
    iprow.dwIndex = ipfrow.dwForwardIfIndex;  // 接口索引
    iprow.dwPhysAddrLen = ETH_ADDR_LEN;       // MAC 地址长度
    memcpy(iprow.bPhysAddr,
           &entry->arp_ha.addr_eth, ETH_ADDR_LEN);  // MAC 地址
    iprow.dwAddr = entry->arp_pa.addr_ip;     // IP 地址
    iprow.dwType = 4;  // 静态条目类型

    // 3. 创建 ARP 条目
    if (CreateIpNetEntry(&iprow) != NO_ERROR)
        return (-1);

    return (0);
}
```

**关键 API：**
- `GetBestRoute()`: 查找最佳路由
- `CreateIpNetEntry()`: 添加静态 ARP 条目

---

#### 删除 ARP 条目

```c
// 52-71行: arp_delete
int arp_delete(arp_t *arp, const struct arp_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    MIB_IPNETROW iprow;

    // 1. 查找到达目标 IP 的最佳路由
    if (GetBestRoute(entry->arp_pa.addr_ip,
                     IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    // 2. 设置要删除的条目
    memset(&iprow, 0, sizeof(iprow));
    iprow.dwIndex = ipfrow.dwForwardIfIndex;
    iprow.dwAddr = entry->arp_pa.addr_ip;

    // 3. 删除 ARP 条目
    if (DeleteIpNetEntry(&iprow) != NO_ERROR) {
        errno = ENXIO;
        SetLastError(ERROR_NO_DATA);
        return (-1);
    }
    return (0);
}
```

---

#### 遍历 ARP 表

```c
// 96-132行: arp_loop
int arp_loop(arp_t *arp, arp_handler callback, void *arg)
{
    struct arp_entry entry;
    ULONG len;
    int i, ret;

    // 1. 获取 ARP 表（动态缓冲区）
    for (len = sizeof(arp->iptable[0]); ; ) {
        if (arp->iptable)
            free(arp->iptable);
        arp->iptable = malloc(len);
        if (arp->iptable == NULL)
            return (-1);

        ret = GetIpNetTable(arp->iptable, &len, FALSE);
        if (ret == NO_ERROR)
            break;
        else if (ret != ERROR_INSUFFICIENT_BUFFER)
            return (-1);
    }

    // 2. 遍历 ARP 条目
    entry.arp_pa.addr_type = ADDR_TYPE_IP;
    entry.arp_pa.addr_bits = IP_ADDR_BITS;
    entry.arp_ha.addr_type = ADDR_TYPE_ETH;
    entry.arp_ha.addr_bits = ETH_ADDR_BITS;

    for (i = 0; i < (int)arp->iptable->dwNumEntries; i++) {
        if (arp->iptable->table[i].dwPhysAddrLen != ETH_ADDR_LEN)
            continue;  // 跳过非以太网条目

        entry.arp_pa.addr_ip = arp->iptable->table[i].dwAddr;
        memcpy(&entry.arp_ha.addr_eth,
               arp->iptable->table[i].bPhysAddr, ETH_ADDR_LEN);

        if ((ret = (*callback)(&entry, arg)) != 0)
            return (ret);
    }
    return (0);
}
```

**关键 API：**
- `GetIpNetTable()`: 获取 IP 网络表（ARP 表）

---

## 3. 路由表操作 (route-win32.c)

### 数据结构

```c
// 28-34行: 路由句柄
struct route_handle {
    HINSTANCE                 iphlpapi;      // IP Helper DLL 句柄
    MIB_IPFORWARDTABLE       *ipftable;      // IPv4 路由表
    MIB_IPFORWARD_TABLE2    *ipftable2;     // IPv4/IPv6 路由表 (Vista+)
};
```

### 核心实现

#### 动态加载新 API

```c
// 28行: 函数指针类型定义
typedef DWORD (WINAPI *GETIPFORWARDTABLE2)(ADDRESS_FAMILY,
                                            PMIB_IPFORWARD_TABLE2 *);

// 36-47行: route_open
route_t *route_open(void)
{
    route_t *r;

    r = calloc(1, sizeof(route_t));
    if (r == NULL)
        return NULL;

    // 加载 IP Helper DLL
    r->iphlpapi = GetModuleHandle("iphlpapi.dll");

    return r;
}
```

**设计考虑：**
- 动态加载 API 以兼容不同 Windows 版本
- `GetIpForwardTable2` 只在 Vista 及更高版本可用

---

#### 添加路由

```c
// 49-76行: route_add
int route_add(route_t *route, const struct route_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    struct addr net;

    memset(&ipfrow, 0, sizeof(ipfrow));

    // 1. 获取输出接口
    if (GetBestInterface(entry->route_gw.addr_ip,
                         &ipfrow.dwForwardIfIndex) != NO_ERROR)
        return (-1);

    // 2. 计算网络地址
    if (addr_net(&entry->route_dst, &net) < 0 ||
        net.addr_type != ADDR_TYPE_IP)
        return (-1);

    // 3. 填充路由条目
    ipfrow.dwForwardDest = net.addr_ip;                    // 目标网络
    addr_btom(entry->route_dst.addr_bits,
              &ipfrow.dwForwardMask, IP_ADDR_LEN);         // 子网掩码
    ipfrow.dwForwardNextHop = entry->route_gw.addr_ip;     // 网关
    ipfrow.dwForwardType = 4;                               // 间接路由
    ipfrow.dwForwardProto = 3;                              // 手动添加

    // 4. 创建路由
    if (CreateIpForwardEntry(&ipfrow) != NO_ERROR)
        return (-1);

    return (0);
}
```

---

#### 删除路由

```c
// 78-101行: route_delete
int route_delete(route_t *route, const struct route_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    DWORD mask;

    // 1. 查找路由
    if (entry->route_dst.addr_type != ADDR_TYPE_IP ||
        GetBestRoute(entry->route_dst.addr_ip,
                     IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    // 2. 验证子网掩码
    addr_btom(entry->route_dst.addr_bits, &mask, IP_ADDR_LEN);

    if (ipfrow.dwForwardDest != entry->route_dst.addr_ip ||
        ipfrow.dwForwardMask != mask) {
        errno = ENXIO;
        SetLastError(ERROR_NO_DATA);
        return (-1);
    }

    // 3. 删除路由
    if (DeleteIpForwardEntry(&ipfrow) != NO_ERROR)
        return (-1);

    return (0);
}
```

---

#### 查询路由

```c
// 103-140行: route_get
int route_get(route_t *route, struct route_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    DWORD mask;
    intf_t *intf;
    struct intf_entry intf_entry;

    // 1. 获取最佳路由
    if (entry->route_dst.addr_type != ADDR_TYPE_IP ||
        GetBestRoute(entry->route_dst.addr_ip,
                     IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    // 2. 检查路由类型
    if (ipfrow.dwForwardProto == 2 &&  // MIB_IPPROTO_LOCAL
        (ipfrow.dwForwardNextHop | IP_CLASSA_NET) !=
        (IP_ADDR_LOOPBACK | IP_CLASSA_NET) &&
        !IP_LOCAL_GROUP(ipfrow.dwForwardNextHop)) {
        errno = ENXIO;
        SetLastError(ERROR_NO_DATA);
        return (-1);
    }

    // 3. 填充路由信息
    addr_btom(entry->route_dst.addr_bits, &mask, IP_ADDR_LEN);
    entry->route_gw.addr_type = ADDR_TYPE_IP;
    entry->route_gw.addr_bits = IP_ADDR_BITS;
    entry->route_gw.addr_ip = ipfrow.dwForwardNextHop;
    entry->metric = ipfrow.dwForwardMetric1;

    // 4. 查找接口名称
    entry->intf_name[0] = '\0';
    intf = intf_open();
    if (intf_get_index(intf, &intf_entry,
                       AF_INET, ipfrow.dwForwardIfIndex) == 0) {
        strlcpy(entry->intf_name, intf_entry.intf_name,
                sizeof(entry->intf_name));
    }
    intf_close(intf);

    return (0);
}
```

---

#### 遍历路由表（兼容旧版 API）

```c
// 142-197行: route_loop_getipforwardtable
static int route_loop_getipforwardtable(route_t *r,
                                         route_handler callback,
                                         void *arg)
{
    struct route_entry entry;
    intf_t *intf;
    ULONG len;
    int i, ret;

    // 1. 获取路由表
    for (len = sizeof(r->ipftable[0]); ; ) {
        if (r->ipftable)
            free(r->ipftable);
        r->ipftable = malloc(len);
        if (r->ipftable == NULL)
            return (-1);

        ret = GetIpForwardTable(r->ipftable, &len, FALSE);
        if (ret == NO_ERROR)
            break;
        else if (ret != ERROR_INSUFFICIENT_BUFFER)
            return (-1);
    }

    // 2. 遍历路由条目
    intf = intf_open();

    ret = 0;
    for (i = 0; i < (int)r->ipftable->dwNumEntries; i++) {
        struct intf_entry intf_entry;

        entry.route_dst.addr_type = ADDR_TYPE_IP;
        entry.route_dst.addr_bits = IP_ADDR_BITS;
        entry.route_gw.addr_type = ADDR_TYPE_IP;
        entry.route_gw.addr_bits = IP_ADDR_BITS;

        entry.route_dst.addr_ip = r->ipftable->table[i].dwForwardDest;
        addr_mtob(&r->ipftable->table[i].dwForwardMask,
                  IP_ADDR_LEN, &entry.route_dst.addr_bits);
        entry.route_gw.addr_ip = r->ipftable->table[i].dwForwardNextHop;
        entry.metric = r->ipftable->table[i].dwForwardMetric1;

        // 查找接口名称
        entry.intf_name[0] = '\0';
        intf_entry.intf_len = sizeof(intf_entry);
        if (intf_get_index(intf, &intf_entry,
                           AF_INET, r->ipftable->table[i].dwForwardIfIndex) == 0) {
            strlcpy(entry.intf_name, intf_entry.intf_name,
                    sizeof(entry->intf_name));
        }

        if ((ret = (*callback)(&entry, arg)) != 0)
            break;
    }

    intf_close(intf);
    return ret;
}
```

---

#### 遍历路由表（新版 API，支持 IPv6）

```c
// 199-254行: route_loop_getipforwardtable2
static int route_loop_getipforwardtable2(GETIPFORWARDTABLE2 GetIpForwardTable2,
                                         route_t *r,
                                         route_handler callback,
                                         void *arg)
{
    struct route_entry entry;
    intf_t *intf;
    ULONG i;
    int ret;

    // 1. 获取路由表（IPv4 + IPv6）
    ret = GetIpForwardTable2(AF_UNSPEC, &r->ipftable2);
    if (ret != NO_ERROR)
        return (-1);

    // 2. 遍历路由条目
    intf = intf_open();

    ret = 0;
    for (i = 0; i < r->ipftable2->NumEntries; i++) {
        struct intf_entry intf_entry;
        MIB_IPFORWARD_ROW2 *row;
        MIB_IPINTERFACE_ROW ifrow;
        ULONG metric;

        row = &r->ipftable2->Table[i];

        // 解析目标前缀
        addr_ston((struct sockaddr *) &row->DestinationPrefix.Prefix,
                  &entry.route_dst);
        entry.route_dst.addr_bits = row->DestinationPrefix.PrefixLength;

        // 解析下一跳
        addr_ston((struct sockaddr *) &row->NextHop, &entry.route_gw);

        // 查找接口名称
        entry.intf_name[0] = '\0';
        intf_entry.intf_len = sizeof(intf_entry);
        if (intf_get_index(intf, &intf_entry,
                           row->DestinationPrefix.Prefix.si_family,
                           row->InterfaceIndex) == 0) {
            strlcpy(entry->intf_name, intf_entry.intf_name,
                    sizeof(entry->intf_name));
        }

        // 计算总度量值
        ifrow.Family = row->DestinationPrefix.Prefix.si_family;
        ifrow.InterfaceLuid = row->InterfaceLuid;
        ifrow.InterfaceIndex = row->InterfaceIndex;
        if (GetIpInterfaceEntry(&ifrow) != NO_ERROR)
            return (-1);

        metric = ifrow.Metric + row->Metric;
        entry.metric = (metric < INT_MAX) ? metric : INT_MAX;

        if ((ret = (*callback)(&entry, arg)) != 0)
            break;
    }

    intf_close(intf);
    return ret;
}
```

---

#### 路由遍历入口

```c
// 256-270行: route_loop
int route_loop(route_t *r, route_handler callback, void *arg)
{
    GETIPFORWARDTABLE2 GetIpForwardTable2;

    // 动态加载 GetIpForwardTable2（仅 Vista+）
    GetIpForwardTable2 = NULL;
    if (r->iphlpapi != NULL)
        GetIpForwardTable2 = (GETIPFORWARDTABLE2)
            GetProcAddress(r->iphlpapi, "GetIpForwardTable2");

    if (GetIpForwardTable2 == NULL)
        // 使用旧版 API（仅 IPv4）
        return route_loop_getipforwardtable(r, callback, arg);
    else
        // 使用新版 API（IPv4 + IPv6）
        return route_loop_getipforwardtable2(GetIpForwardTable2,
                                             r, callback, arg);
}
```

**版本兼容策略：**
- Windows XP/2003: 使用 `GetIpForwardTable()`
- Windows Vista+: 使用 `GetIpForwardTable2()`（支持 IPv6）

---

## 4. IP 数据包发送 (ip-win32.c)

### 数据结构

```c
// 18-22行: IP 句柄
struct ip_handle {
    WSADATA          wsdata;           // Winsock 数据
    SOCKET           fd;               // 原始套接字
    struct sockaddr_in sin;           // 目标地址
};
```

### 核心实现

#### 打开 IP 套接字

```c
// 24-49行: ip_open
ip_t *ip_open(void)
{
    BOOL on;
    ip_t *ip;

    if ((ip = calloc(1, sizeof(*ip))) != NULL) {
        // 1. 初始化 Winsock
        if (WSAStartup(MAKEWORD(2, 2), &ip->wsdata) != 0) {
            free(ip);
            return (NULL);
        }

        // 2. 创建原始套接字
        if ((ip->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) ==
            INVALID_SOCKET)
            return (ip_close(ip));

        // 3. 设置 IP_HDRINCL 选项（自己构造 IP 头）
        on = TRUE;
        if (setsockopt(ip->fd, IPPROTO_IP, IP_HDRINCL,
                       (const char *)&on, sizeof(on)) == SOCKET_ERROR) {
            SetLastError(ERROR_NETWORK_ACCESS_DENIED);
            return (ip_close(ip));
        }

        ip->sin.sin_family = AF_INET;
        ip->sin.sin_port = htons(666);  // 任意端口
    }
    return (ip);
}
```

**关键选项：**
- `IP_HDRINCL`: 允许用户自己构造 IP 头部
- 需要管理员权限

---

#### 发送 IP 数据包

```c
// 51-63行: ip_send
ssize_t ip_send(ip_t *ip, const void *buf, size_t len)
{
    struct ip_hdr *hdr = (struct ip_hdr *)buf;

    // 使用 IP 头中的源地址作为目标
    ip->sin.sin_addr.s_addr = hdr->ip_src;

    // 发送数据包
    if ((len = sendto(ip->fd, (const char *)buf, len, 0,
                      (struct sockaddr *)&ip->sin,
                      sizeof(ip->sin))) != SOCKET_ERROR)
        return (len);

    return (-1);
}
```

---

## 5. 防火墙 (fw-pktfilter.c)

### 设计思路

由于 Windows 没有内置类似 iptables 的命令行防火墙，libdnet 依赖第三方服务 **PktFilter**（由 HSC France 开发）。

### 通信机制

```c
// 22行: 命名管道
#define PKTFILTER_PIPE "\\\\.\\pipe\\PktFltPipe"

// 277-313行: call_pipe - 与 PktFilter 服务通信
static char *call_pipe(const char *msg, int len)
{
    HANDLE *pipe;
    DWORD i;
    char *reply, status;

    // 1. 等待命名管道
    if (!WaitNamedPipe(PKTFILTER_PIPE, NMPWAIT_USE_DEFAULT_WAIT) ||
        (pipe = CreateFile(PKTFILTER_PIPE,
                           GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, 0, NULL)) ==
        INVALID_HANDLE_VALUE) {
        return (NULL);
    }

    // 2. 发送命令
    if (WriteFile(pipe, msg, len, &i, NULL)) {
        if (ReadFile(pipe, &status, sizeof(status), &i, NULL)) {
            if (status == FILTER_FAILURE) {
                // 读取错误消息
                ReadFile(pipe, &status, sizeof(status), &i, NULL);
            } else if (status == FILTER_MESSAGE) {
                // 读取消息
                ReadFile(pipe, &len, 4, &i, NULL);
                reply = calloc(1, len + 1);
                if (!ReadFile(pipe, reply, len, &i, NULL)) {
                    free(reply);
                    reply = NULL;
                }
            } else if (status == FILTER_SUCCESS)
                reply = strdup("");  // 成功
        }
    }
    CloseHandle(pipe);
    return (reply);
}
```

---

#### 规则格式化

```c
// 214-275行: format_rule - 将规则转换为 PktFilter 语法
static int format_rule(const struct fw_rule *rule, char *buf, int len)
{
    char tmp[128];

    // 动作
    strlcpy(buf, (rule->fw_op == FW_OP_ALLOW) ? "pass " : "block ", len);

    // 方向
    strlcat(buf, (rule->fw_dir == FW_DIR_IN) ? "in " : "out ", len);

    // 接口
    snprintf(tmp, sizeof(tmp), "on %s ", rule->fw_device);
    strlcat(buf, tmp, len);

    // 协议
    if (rule->fw_proto != 0) {
        snprintf(tmp, sizeof(tmp), "proto %d ", rule->fw_proto);
        strlcat(buf, tmp, len);
    }

    // 源地址
    if (rule->fw_src.addr_type != ADDR_TYPE_NONE) {
        snprintf(tmp, sizeof(tmp), "from %s ", addr_ntoa(&rule->fw_src));
        strlcat(buf, tmp, len);
    } else
        strlcat(buf, "from any ", len);

    // 源端口（TCP/UDP）
    if (rule->fw_proto == IP_PROTO_TCP ||
        rule->fw_proto == IP_PROTO_UDP) {
        if (rule->fw_sport[0] == rule->fw_sport[1])
            snprintf(tmp, sizeof(tmp), "port = %d ", rule->fw_sport[0]);
        else
            snprintf(tmp, sizeof(tmp), "port %d >< %d ",
                     rule->fw_sport[0] - 1,
                     rule->fw_sport[1] + 1);
        strlcat(buf, tmp, len);
    }

    // 目标地址
    if (rule->fw_dst.addr_type != ADDR_TYPE_NONE) {
        snprintf(tmp, sizeof(tmp), "to %s ", addr_ntoa(&rule->fw_dst));
        strlcat(buf, tmp, len);
    } else
        strlcat(buf, "to any ", len);

    // 目标端口（TCP/UDP）
    if (rule->fw_proto == IP_PROTO_TCP ||
        rule->fw_proto == IP_PROTO_UDP) {
        if (rule->fw_dport[0] == rule->fw_dport[1])
            snprintf(tmp, sizeof(tmp), "port = %d", rule->fw_dport[0]);
        else
            snprintf(tmp, sizeof(tmp), "port %d >< %d",
                     rule->fw_dport[0] - 1,
                     rule->fw_dport[1] + 1);
        strlcat(buf, tmp, len);
    } else if (rule->fw_proto == IP_PROTO_ICMP) {
        // ICMP 类型和代码
        if (rule->fw_sport[1]) {
            snprintf(tmp, sizeof(tmp), "icmp-type %d",
                     rule->fw_sport[0]);
            strlcat(buf, tmp, len);
            if (rule->fw_dport[1]) {
                snprintf(tmp, sizeof(tmp), " code %d",
                         rule->fw_dport[0]);
                strlcat(buf, tmp, len);
            }
        }
    }
    return (strlen(buf));
}
```

**示例规则：**
```
block in on eth0 proto tcp from 192.168.1.100 port > 1024 to any port 80
pass out on eth0 proto icmp all icmp-type echo
```

---

## 6. 以太网接口 (eth-win32.c)

### 实现现状

```c
// 所有函数返回错误（空实现）
eth_t *eth_open(const char *device) { return (NULL); }
ssize_t eth_send(eth_t *eth, const void *buf, size_t len) { return (-1); }
eth_t *eth_close(eth_t *eth) { return (NULL); }
int eth_get(eth_t *eth, eth_addr_t *ea) { return (-1); }
int eth_set(eth_t *eth, const eth_addr_t *ea) { return (-1); }
```

**原因：**
- Windows 不提供直接发送原始以太网帧的系统 API
- 需要依赖 **WinPcap** 库

**替代方案：**
```c
// 使用 WinPcap API
#include <pcap.h>

pcap_t *pcap_open(const char *device, int snaplen, int promisc, int to_ms,
                  char *errbuf);
pcap_sendpacket(pcap_t *p, const u_char *buf, int len);
```

---

## Windows API 汇总

| 功能 | Windows API | 说明 |
|-----|-------------|------|
| 网络接口 | `GetAdaptersAddresses()` | 获取适配器信息 |
| ARP 表 | `GetIpNetTable()` / `CreateIpNetEntry()` / `DeleteIpNetEntry()` | ARP 操作 |
| 路由表 | `GetIpForwardTable()` / `CreateIpForwardEntry()` / `DeleteIpForwardEntry()` | 路由操作 |
| 最佳路由 | `GetBestRoute()` / `GetBestInterface()` | 路由查询 |
| 原始套接字 | `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)` | IP 层数据包 |
| 防火墙 | PktFilter 命名管道 | 第三方服务 |
| WinPcap | `pcap_sendpacket()` | 以太网帧 |

## 与 Unix 平台差异

| 功能 | Linux | Windows |
|-----|-------|---------|
| 原始以太网 | PF_PACKET | WinPcap |
| ARP | ioctl | IP Helper API |
| 路由 | Netlink + procfs | IP Helper API |
| 接口 | ioctl + sysctl | GetAdaptersAddresses |
| 防火墙 | setsockopt / iptables | PktFilter（第三方） |
| 原始 IP | SOCK_RAW | SOCK_RAW + IP_HDRINCL |

## 开发注意事项

1. **管理员权限**：原始套接字和某些 API 需要 UAC 权限
2. **版本兼容**：使用动态加载适配不同 Windows 版本
3. **字符编码**：Windows API 使用宽字符（wchar_t），需要转换
4. **错误处理**：同时设置 `errno` 和 `SetLastError()`
5. **缓冲区管理**：IP Helper API 需要动态调整缓冲区大小

## 依赖库

1. **iphlpapi.dll** - Windows IP Helper API（系统自带）
2. **ws2_32.dll** - Winsock 2.2（系统自带）
3. **WinPcap** - 原始以太网访问（可选，第三方）
4. **PktFilter** - 防火墙（可选，第三方）
