# 第 3 章：原始 Socket 封装实现

## 3.1 概述

libdnet 提供了跨平台的原始 Socket 封装，主要用于发送和接收原始 IP 数据包。本章深入分析其内部实现细节，对比 Linux、macOS 和 Windows 平台的不同实现方式。

### 核心模块架构

```
┌─────────────────────────────────────────────────────┐
│              libdnet IP Module                       │
├─────────────────────────────────────────────────────┤
│  ip_open()      ip_send()      ip_close()           │
├─────────────────────────────────────────────────────┤
│  ip-cooked.c    ip.c          ip-win32.c             │
│  (用户态实现)    (Linux/BSD)   (Windows)             │
└─────────────────────────────────────────────────────┘
         │                │              │
         ▼                ▼              ▼
┌───────────────┐ ┌──────────────┐ ┌──────────────────┐
│   eth_open()  │ │ socket()     │ │ WSASocket()      │
│   eth_send()  │ │ sendto()     │ │ sendto()         │
│  (用户态以太网)│ │ (PF_PACKET)  │ │ (AF_INET, RAW)   │
│  (BPF on BSD) │ │ (Raw Socket) │ │ (WinSocket)      │
└───────────────┘ └──────────────┘ └──────────────────┘
```

### 支持的平台

| 平台 | 实现文件 | 底层技术 | 特点 |
|------|---------|---------|------|
| **Linux** | `ip.c` + `eth-linux.c` | Raw Socket + PF_PACKET | 权限要求高，性能最优 |
| **macOS/BSD** | `ip.c` + `eth-bsd.c` | Raw Socket + BPF | Berkeley Packet Filter，灵活强大 |
| **Windows** | `ip-win32.c` | WinSocket Raw Socket | 需要 IP_HDRINCL，权限要求高 |
| **Cooked** | `ip-cooked.c` | 用户态以太网发送 | 无需特殊权限，跨平台 |

---

## 3.2 Linux 平台实现 (ip.c + eth-linux.c)

### 3.2.1 数据结构定义

```c
/* Linux 平台的 IP 句柄结构 (ip.c:21-23) */
struct ip_handle {
    int fd;  /* Raw socket 文件描述符 */
};
```

**设计特点：**

- 极简设计，只包含一个 socket 文件描述符
- Linux 的 raw socket 功能强大，无需额外状态维护

### 3.2.2 `ip_open` - 打开原始 Socket

**源码分析 (ip.c:25-61):**

```c
ip_t *
ip_open(void)
{
    ip_t *i;
    int n;
    socklen_t len;

    /* 1. 分配句柄内存 */
    if ((i = calloc(1, sizeof(*i))) == NULL)
        return (NULL);

    /* 2. 创建 IPv4 原始 socket
     * AF_INET: IPv4 协议族
     * SOCK_RAW: 原始 socket 类型
     * IPPROTO_RAW: 原始 IP 协议，允许用户完全控制 IP 头部
     */
    if ((i->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        return (ip_close(i));

    /* 3. 设置 IP_HDRINCL 选项
     * IP_HDRINCL: 告诉内核 IP 头部由应用程序提供
     * 某些系统（如 OpenBSD）不支持此选项
     */
#ifdef IP_HDRINCL
    n = 1;
    if (setsockopt(i->fd, IPPROTO_IP, IP_HDRINCL, &n, sizeof(n)) < 0)
        return (ip_close(i));
#endif

    /* 4. 调整发送缓冲区大小
     * Linux 默认的发送缓冲区可能较小，通过循环测试找到最大可用大小
     */
#ifdef SO_SNDBUF
    len = sizeof(n);
    if (getsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, &len) < 0)
        return (ip_close(i));

    /* 从当前缓冲区大小开始，每次增加 128 字节
     * 直到达到 1MB 或失败为止
     */
    for (n += 128; n < 1048576; n += 128) {
        if (setsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) {
            if (errno == ENOBUFS)  /* 缓冲区耗尽 */
                break;
            return (ip_close(i));
        }
    }
#endif

    /* 5. 允许发送广播包 */
#ifdef SO_BROADCAST
    n = 1;
    if (setsockopt(i->fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n)) < 0)
        return (ip_close(i));
#endif

    return (i);
}
```

**关键点解析：**

| 步骤         | 功能                                     | 说明                           |
| ------------ | ---------------------------------------- | ------------------------------ |
| socket 创建  | `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)` | 创建原始套接字，需要 root 权限 |
| IP_HDRINCL   | 应用层构造 IP 头                         | 内核不会自动添加 IP 头         |
| SO_SNDBUF    | 发送缓冲区优化                           | 动态调整以提高性能             |
| SO_BROADCAST | 支持广播                                 | 允许发送到广播地址             |

**权限要求：**

- Linux: 需要 root 权限或 CAP_NET_RAW 能力
- 创建 raw socket 时会检查 `/proc/sys/net/core/wmem_max`

### 3.2.3 `ip_send` - 发送原始 IP 数据包

**源码分析 (ip.c:63-93):**

```c
ssize_t
ip_send(ip_t *i, const void *buf, size_t len)
{
    struct ip_hdr *ip;
    struct sockaddr_in sin;

    ip = (struct ip_hdr *)buf;

    /* 1. 构造目标地址结构 */
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SOCKADDR_SA_LEN
    sin.sin_len = sizeof(sin);  /* BSD 特有：地址长度字段 */
#endif
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip->ip_dst;  /* 使用 IP 头中的目标地址 */

    /* 2. 某些平台的特殊处理
     * HAVE_RAWIP_HOST_OFFLEN: 表示平台在传输前需要将字节序转换为主机序
     * 主要是某些 BSD 系统
     */
#ifdef HAVE_RAWIP_HOST_OFFLEN
    /* 转换为主机序 */
    ip->ip_len = ntohs(ip->ip_len);
    ip->ip_off = ntohs(ip->ip_off);

    /* 发送数据包 */
    len = sendto(i->fd, buf, len, 0,
        (struct sockaddr *)&sin, sizeof(sin));

    /* 转换回网络序 */
    ip->ip_len = htons(ip->ip_len);
    ip->ip_off = htons(ip->ip_off);

    return (len);
#else
    /* 标准 Linux 实现：直接发送 */
    return (sendto(i->fd, buf, len, 0,
        (struct sockaddr *)&sin, sizeof(sin)));
#endif
}
```

**执行流程：**

```
用户态数据包
    │
    ├─ 解析 IP 头，获取目标地址 ip->ip_dst
    │
    ├─ 填充 struct sockaddr_in
    │   ├─ sin_family = AF_INET
    │   ├─ sin_addr.s_addr = ip->ip_dst
    │   └─ sin_port = 0 (IP 层不需要端口)
    │
    ├─ sendto() 系统调用
    │
    └─ 内核网络栈
        ├─ 路由查找 (查找出口网卡)
        ├─ 分片处理 (如果需要)
        └─ 发送到网络
```

**Linux 内核处理过程：**

1. **路由查找**：根据目标地址确定出口接口
2. **分片判断**：如果数据包超过 MTU，进行分片
3. **IP 头校验**：内核会重新计算 IP 校验和（虽然用户已计算）
4. **传输**：通过网络接口发送

### 3.2.4 `ip_close` - 关闭 Socket

**源码分析 (ip.c:95-104):**

```c
ip_t *
ip_close(ip_t *i)
{
    if (i != NULL) {
        if (i->fd >= 0)
            close(i->fd);  /* 关闭 socket 文件描述符 */
        free(i);           /* 释放句柄内存 */
    }
    return (NULL);
}
```

---

## 3.3 Windows 平台实现 (ip-win32.c)

### 3.3.1 数据结构定义

```c
/* Windows 平台的 IP 句柄结构 (ip-win32.c:18-22) */
struct ip_handle {
    WSADATA         wsdata;        /* WinSocket 初始化数据 */
    SOCKET          fd;            /* Socket 句柄 */
    struct sockaddr_in sin;        /* 目标地址结构 */
};
```

**设计特点：**

- 包含 WinSocket 初始化数据（WSADATA）
- 预先分配目标地址结构，避免重复构造
- Windows 的 SOCKET 类型实际上是 `unsigned int`，不同于 Linux 的 `int`

### 3.3.2 `ip_open` - 打开原始 Socket

**源码分析 (ip-win32.c:24-49):**

```c
ip_t *
ip_open(void)
{
    BOOL on;
    ip_t *ip;

    /* 1. 分配句柄内存 */
    if ((ip = calloc(1, sizeof(*ip))) != NULL) {

        /* 2. 初始化 WinSocket
         * WSAStartup(MAKEWORD(2, 2), &ip->wsdata):
         * - 请求 WinSocket 2.2 版本
         * - 将版本信息存储在 wsdata 中
         */
        if (WSAStartup(MAKEWORD(2, 2), &ip->wsdata) != 0) {
            free(ip);
            return (NULL);
        }

        /* 3. 创建原始 Socket
         * AF_INET: IPv4 协议族
         * SOCK_RAW: 原始 socket 类型
         * IPPROTO_RAW: 原始 IP 协议
         */
        if ((ip->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) ==
            INVALID_SOCKET)
            return (ip_close(ip));

        /* 4. 设置 IP_HDRINCL 选项
         * 在 Windows 上，这个选项是必须的
         * 如果失败，设置错误代码为 ERROR_NETWORK_ACCESS_DENIED
         */
        on = TRUE;
        if (setsockopt(ip->fd, IPPROTO_IP, IP_HDRINCL,
            (const char *)&on, sizeof(on)) == SOCKET_ERROR) {
            SetLastError(ERROR_NETWORK_ACCESS_DENIED);
            return (ip_close(ip));
        }

        /* 5. 初始化地址结构
         * sin_family = AF_INET
         * sin_port = 666 (任意端口，IP 层不使用)
         */
        ip->sin.sin_family = AF_INET;
        ip->sin.sin_port = htons(666);
    }
    return (ip);
}
```

**Windows 特有特点：**

| 特性            | 说明                                     |
| --------------- | ---------------------------------------- |
| WSAStartup      | Windows 必须显式初始化 WinSocket 库      |
| INVALID_SOCKET  | Windows 使用宏而非 -1 表示错误           |
| setsockopt 参数 | 第三个参数需要 `(const char *)` 强制转换 |
| 端口设置        | 虽然不使用，但需要初始化避免未定义行为   |

**权限要求：**

- Windows: 需要管理员权限
- 否则会失败并返回 `WSAEACCES` (10013)

**常见错误：**

| 错误代码                   | 含义              | 解决方案                    |
| -------------------------- | ----------------- | --------------------------- |
| WSAEACCES (10013)          | 权限不足          | 以管理员身份运行            |
| WSAEPROTONOSUPPORT (10043) | 不支持原始 socket | Windows Home 版本可能不支持 |
| WSAENETRESET (10052)       | 连接重置          | 需要重新打开 socket         |

### 3.3.3 `ip_send` - 发送原始 IP 数据包

**源码分析 (ip-win32.c:51-63):**

```c
ssize_t
ip_send(ip_t *ip, const void *buf, size_t len)
{
    struct ip_hdr *hdr = (struct ip_hdr *)buf;

    /* 1. 设置目标地址
     * 注意：Windows 使用 ip_src 作为目标地址！
     * 这与 Linux 不同（Linux 使用 ip_dst）
     */
    ip->sin.sin_addr.s_addr = hdr->ip_src;

    /* 2. 发送数据包 */
    if ((len = sendto(ip->fd, (const char *)buf, len, 0,
        (struct sockaddr *)&ip->sin, sizeof(ip->sin))) != SOCKET_ERROR)
        return (len);

    return (-1);
}
```

**⚠️ 重要差异：Windows 使用源地址作为发送目标！**

这是一个非常容易出错的差异：

```c
/* Linux: 使用目标地址 */
sin.sin_addr.s_addr = ip->ip_dst;  // 正确

/* Windows: 使用源地址 */
ip->sin.sin_addr.s_addr = hdr->ip_src;  // 注意这里！
```

**原因分析：**

- Windows 的原始 socket 实现，sendto 的目标地址是**绑定的本地地址**
- IP 头中的目标地址 (`ip_dst`) 由内核解析
- 因此需要将 socket 绑定到源地址 (`ip_src`)

**Windows 内核处理流程：**

```
sendto(..., sin_src, ...)
    │
    ├─ 将 socket 绑定到源地址 (sin.sin_addr = ip_src)
    │
    ├─ 解析用户提供的 IP 头
    │   ├─ ip_src: 源 IP 地址
    │   ├─ ip_dst: 目标 IP 地址 (实际目标)
    │   └─ ip_p: 上层协议
    │
    ├─ 路由查找 (基于 ip_dst)
    │
    └─ 发送到网络
```

### 3.3.4 `ip_close` - 关闭 Socket

**源码分析 (ip-win32.c:65-75):**

```c
ip_t *
ip_close(ip_t *ip)
{
    if (ip != NULL) {
        WSACleanup();  /* 清理 WinSocket 资源 */

        if (ip->fd != INVALID_SOCKET)
            closesocket(ip->fd);  /* 关闭 socket */
        free(ip);
    }
    return (NULL);
}
```

**清理顺序：**

1. 调用 `WSACleanup()` 释放 WinSocket 资源
2. 关闭 socket 句柄
3. 释放内存

---

## 3.4 macOS/BSD 平台实现 (ip.c + eth-bsd.c)

### 3.4.1 概述

macOS 和其他 BSD 系统（FreeBSD、OpenBSD、NetBSD）使用 Berkeley Packet Filter (BPF) 进行原始数据包发送。BPF 是一个强大的内核级数据包捕获和过滤框架，提供了比 Linux PF_PACKET 更灵活的接口。

### 3.4.2 数据结构定义

```c
/* IP 层句柄结构 (ip.c:21-23) - 与 Linux 相同 */
struct ip_handle {
    int fd;  /* Raw socket 文件描述符 */
};

/* 以太网层句柄结构 (eth-bsd.c:34-37) */
struct eth_handle {
    int     fd;      /* BPF 文件描述符 */
    char    device[16];  /* 设备名称 (如 en0) */
};
```

**设计特点：**
- IP 层与 Linux 共享相同的 `ip.c` 实现
- 以太网层使用 BPF 设备，而非 socket
- 设备名称为 `en0`、`en1` 等（macOS 风格）

### 3.4.3 `eth_open` - 打开 BPF 设备

**源码分析 (eth-bsd.c:39-74):**

```c
eth_t *
eth_open(const char *device)
{
    struct ifreq ifr;
    char file[32];
    eth_t *e;
    int i;

    if ((e = calloc(1, sizeof(*e))) != NULL) {
        /* 1. 打开 BPF 设备
         * BPF 设备文件路径: /dev/bpf0, /dev/bpf1, ...
         * 尝试从 0 开始，最多尝试 128 次
         */
        for (i = 0; i < 128; i++) {
            snprintf(file, sizeof(file), "/dev/bpf%d", i);

            /* 注意：macOS 10.6 有一个 bug
             * 使用 O_WRONLY 会导致其他进程无法接收流量
             * 因此必须使用 O_RDWR
             */
            e->fd = open(file, O_RDWR);
            if (e->fd != -1 || errno != EBUSY)
                break;
        }

        if (e->fd < 0)
            return (eth_close(e));

        /* 2. 绑定到指定网络接口
         * 使用 BIOCSETIF ioctl 绑定 BPF 设备到网络接口
         */
        memset(&ifr, 0, sizeof(ifr));
        strlcpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

        if (ioctl(e->fd, BIOCSETIF, (char *)&ifr) < 0)
            return (eth_close(e));

        /* 3. 设置完整标志 (BIOCSHDRCMPLT)
         * BIOCSHDRCMPLT: BPF 将不自动补全以太网头
         * 这意味着用户必须提供完整的以太网帧
         */
#ifdef BIOCSHDRCMPLT
        i = 1;
        if (ioctl(e->fd, BIOCSHDRCMPLT, &i) < 0)
            return (eth_close(e));
#endif

        /* 4. 保存设备名称 */
        strlcpy(e->device, device, sizeof(e->device));
    }
    return (e);
}
```

**BPF 设备特点：**

| 特性 | 说明 |
|------|------|
| **设备文件** | `/dev/bpf0`, `/dev/bpf1`, ... |
| **并发控制** | 每个设备一次只能被一个进程打开 (EBUSY) |
| **权限要求** | 需要 root 权限 |
| **接口绑定** | 通过 `BIOCSETIF` ioctl 绑定到网络接口 |
| **自动补全** | `BIOCSHDRCMPLT` 控制是否自动补全以太网头 |

### 3.4.4 `eth_send` - 发送以太网帧

**源码分析 (eth-bsd.c:76-80):**

```c
ssize_t
eth_send(eth_t *e, const void *buf, size_t len)
{
    return (write(e->fd, buf, len));
}
```

**设计特点：**
- BPF 设备使用标准的 `write()` 系统调用发送数据
- 用户必须提供完整的以太网帧（14 字节头部 + 数据）
- 简洁高效，不需要复杂的 sendto 参数

**与 Linux 对比：**

```c
/* Linux (eth-linux.c) */
sendto(e->fd, buf, len, 0, (struct sockaddr *)&e->sll, sizeof(e->sll));

/* macOS/BSD (eth-bsd.c) */
write(e->fd, buf, len);
```

### 3.4.5 `eth_get` - 获取 MAC 地址

**源码分析 (eth-bsd.c:94-138):**

```c
int
eth_get(eth_t *e, eth_addr_t *ea)
{
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;
    struct addr ha;
    u_char *p, *buf;
    size_t len;
    int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 };

    /* 1. 获取接口列表所需缓冲区大小 */
    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return (-1);

    /* 2. 分配缓冲区 */
    if ((buf = malloc(len)) == NULL)
        return (-1);

    /* 3. 获取接口列表
     * sysctl 返回所有网络接口的信息
     */
    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        free(buf);
        return (-1);
    }

    /* 4. 遍历接口列表，查找指定设备 */
    for (p = buf; p < buf + len; p += ifm->ifm_msglen) {
        ifm = (struct if_msghdr *)p;
        sdl = (struct sockaddr_dl *)(ifm + 1);

        /* 过滤条件 */
        if (ifm->ifm_type != RTM_IFINFO ||
            (ifm->ifm_addrs & RTA_IFP) == 0)
            continue;

        /* 检查接口名称是否匹配 */
        if (sdl->sdl_family != AF_LINK || sdl->sdl_nlen == 0 ||
            memcmp(sdl->sdl_data, e->device, sdl->sdl_nlen) != 0)
            continue;

        /* 转换 MAC 地址 */
        if (addr_ston((struct sockaddr *)sdl, &ha) == 0)
            break;
    }
    free(buf);

    if (p >= buf + len) {
        errno = ESRCH;
        return (-1);
    }
    memcpy(ea, &ha.addr_eth, sizeof(*ea));

    return (0);
}
```

**sysctl 获取接口信息的原理：**

```c
/* sysctl 参数详解 */
int mib[] = {
    CTL_NET,           /* 网络 (NET) */
    AF_ROUTE,          /* 路由域 (PF_ROUTE) */
    0,                 /* 协议族 (0 = 任意) */
    AF_LINK,           /* 地址族 (AF_LINK = 链路层) */
    NET_RT_IFLIST,     /* 操作 (获取接口列表) */
    0                  /* 地址 (0 = 所有接口) */
};
```

### 3.4.6 macOS 特有问题与解决

#### 问题 1: macOS 10.6 的 O_WRONLY Bug

**问题描述：**
macOS 10.6 (Snow Leopard) 有一个 bug，如果以 `O_WRONLY` 模式打开 BPF 设备，会导致其他进程无法接收网络流量。

**解决方案：**
```c
/* 使用 O_RDWR 而非 O_WRONLY */
e->fd = open(file, O_RDWR);
```

#### 问题 2: BPF 设备数量限制

**问题描述：**
系统默认创建的 BPF 设备数量有限，可能不够多个进程同时使用。

**解决方案：**
```bash
# 增加 BPF 设备数量
sudo sysctl -w kern.bpfcache_max=128
```

#### 问题 3: 设备命名差异

| 平台 | 设备命名示例 |
|------|------------|
| Linux | eth0, wlan0, enp3s0 |
| macOS | en0, en1, bridge0 |
| FreeBSD | em0, igb0, re0 |

---

## 3.5 用户态 IP 实现 (ip-cooked.c)

### 3.4.1 实现动机

在某些平台或场景下，原始 socket 不可用或受到限制：

- **权限限制**：非 root 用户无法创建 raw socket
- **平台限制**：某些平台（如 Windows Home）不支持原始 socket
- **性能需求**：需要更精细的控制（如 ARP 解析、分片处理）

libdnet 提供了 `ip-cooked.c`，在用户态实现 IP 层功能，通过以太网 socket 发送数据包。

### 3.4.2 数据结构定义

```c
/* 接口链表元素 (ip-cooked.c:23-30) */
struct ip_intf {
    eth_t           *eth;           /* 以太网句柄 */
    char             name[INTF_NAME_LEN];  /* 接口名称 */
    struct addr      ha;            /* 硬件地址 (MAC) */
    struct addr      pa;            /* 协议地址 (IP) */
    int              mtu;           /* 最大传输单元 */
    LIST_ENTRY(ip_intf) next;       /* 链表指针 */
};

/* IP 句柄 (ip-cooked.c:32-40) */
struct ip_handle {
    arp_t           *arp;           /* ARP 模块 */
    intf_t          *intf;          /* 接口模块 */
    route_t         *route;         /* 路由模块 */
    int              fd;            /* UDP socket 用于路由查找 */
    struct sockaddr_in sin;         /* 地址结构 */

    LIST_HEAD(, ip_intf) ip_intf_list;  /* 接口链表 */
};
```

**设计特点：**

- 集成了多个底层模块（eth、arp、intf、route）
- 维护一个接口链表，缓存所有可用的网络接口
- 使用 UDP socket 进行路由查找（Linux 特有的技巧）

### 3.4.3 `ip_open` - 初始化用户态 IP

**源码分析 (ip-cooked.c:67-93):**

```c
ip_t *
ip_open(void)
{
    ip_t *ip;

    if ((ip = calloc(1, sizeof(*ip))) != NULL) {
        ip->fd = -1;

        /* 1. 打开底层模块 */
        if ((ip->arp = arp_open()) == NULL ||
            (ip->intf = intf_open()) == NULL ||
            (ip->route = route_open()) == NULL)
            return (ip_close(ip));

        /* 2. 创建 UDP socket 用于路由查找
         * 创建 SOCK_DGRAM socket，内核会自动路由
         */
        if ((ip->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            return (ip_close(ip));

        /* 3. 初始化地址结构 */
        memset(&ip->sin, 0, sizeof(ip->sin));
        ip->sin.sin_family = AF_INET;
        ip->sin.sin_port = htons(666);

        /* 4. 初始化接口链表 */
        LIST_INIT(&ip->ip_intf_list);

        /* 5. 遍历所有接口，添加符合条件的接口到链表
         * 条件：
         *   - 类型为以太网
         *   - 接口 UP
         *   - MTU >= 64 字节
         *   - 有 IP 地址
         *   - 有 MAC 地址
         */
        if (intf_loop(ip->intf, _add_ip_intf, ip) != 0)
            return (ip_close(ip));
    }
    return (ip);
}
```

**`_add_ip_intf` 回调函数 (ip-cooked.c:42-65):**

```c
static int
_add_ip_intf(const struct intf_entry *entry, void *arg)
{
    ip_t *ip = (ip_t *)arg;
    struct ip_intf *ipi;

    /* 过滤条件 */
    if (entry->intf_type == INTF_TYPE_ETH &&
        (entry->intf_flags & INTF_FLAG_UP) != 0 &&
        entry->intf_mtu >= ETH_LEN_MIN &&
        entry->intf_addr.addr_type == ADDR_TYPE_IP &&
        entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {

        if ((ipi = calloc(1, sizeof(*ipi))) == NULL)
            return (-1);

        /* 保存接口信息 */
        strlcpy(ipi->name, entry->intf_name, sizeof(ipi->name));
        memcpy(&ipi->ha, &entry->intf_link_addr, sizeof(ipi->ha));
        memcpy(&ipi->pa, &entry->intf_addr, sizeof(ipi->pa));
        ipi->mtu = entry->intf_mtu;

        /* 添加到链表头部 */
        LIST_INSERT_HEAD(&ip->ip_intf_list, ipi, next);
    }
    return (0);
}
```

### 3.4.4 `_lookup_ip_intf` - 查找出口接口

**源码分析 (ip-cooked.c:95-124):**

```c
static struct ip_intf *
_lookup_ip_intf(ip_t *ip, ip_addr_t dst)
{
    struct ip_intf *ipi;
    int n;

    /* 1. 设置目标地址 */
    ip->sin.sin_addr.s_addr = dst;
    n = sizeof(ip->sin);

    /* 2. connect() 用于路由查找
     * 这是一个巧妙的技巧：
     * - UDP socket 的 connect() 不会实际发送数据
     * - 内核会执行路由查找，确定出口接口
     * - 我们可以通过 getsockname() 获取出口 IP
     */
    if (connect(ip->fd, (struct sockaddr *)&ip->sin, n) < 0)
        return (NULL);

    /* 3. 获取本地地址 (出口 IP) */
    if (getsockname(ip->fd, (struct sockaddr *)&ip->sin, n) < 0)
        return (NULL);

    /* 4. 在接口链表中查找对应的接口 */
    LIST_FOREACH(ipi, &ip->ip_intf_list, next) {
        if (ipi->pa.addr_ip == ip->sin.sin_addr.s_addr) {
            /* 延迟打开以太网 socket */
            if (ipi->eth == NULL) {
                if ((ipi->eth = eth_open(ipi->name)) == NULL)
                    return (NULL);
            }
            /* LRU 优化：将使用的接口移到链表头部 */
            if (ipi != LIST_FIRST(&ip->ip_intf_list)) {
                LIST_REMOVE(ipi, next);
                LIST_INSERT_HEAD(&ip->ip_intf_list, ipi, next);
            }
            return (ipi);
        }
    }
    return (NULL);
}
```

**路由查找技巧详解：**

```c
/* 利用 UDP socket 进行路由查找 */
int fd = socket(AF_INET, SOCK_DGRAM, 0);

/* 目标地址 */
struct sockaddr_in target;
target.sin_family = AF_INET;
target.sin_addr.s_addr = dst_ip;

/* connect() 不会发送数据，但会触发路由查找 */
connect(fd, (struct sockaddr *)&target, sizeof(target));

/* getsockname() 返回出口地址（本地地址） */
struct sockaddr_in local;
socklen_t len = sizeof(local);
getsockname(fd, (struct sockaddr *)&local, &len);

/* local.sin_addr 就是出口 IP */
exit_ip = local.sin_addr.s_addr;
```

**优势：**

- 无需 root 权限
- 利用内核的路由表
- 准确找到出口接口

### 3.4.5 `_request_arp` - 发送 ARP 请求

**源码分析 (ip-cooked.c:126-138):**

```c
static void
_request_arp(struct ip_intf *ipi, struct addr *dst)
{
    u_char frame[ETH_HDR_LEN + ARP_HDR_LEN + ARP_ETHIP_LEN];

    /* 1. 构造以太网头 */
    eth_pack_hdr(frame, ETH_ADDR_BROADCAST, ipi->ha.addr_eth,
        ETH_TYPE_ARP);

    /* 2. 构造 ARP 头 */
    arp_pack_hdr_ethip(frame + ETH_HDR_LEN, ARP_OP_REQUEST,
        ipi->ha.addr_eth,  /* 发送方 MAC */
        ipi->pa.addr_ip,    /* 发送方 IP */
        ETH_ADDR_BROADCAST, /* 目标 MAC (广播) */
        dst->addr_ip);      /* 目标 IP */

    /* 3. 发送 ARP 请求 */
    eth_send(ipi->eth, frame, sizeof(frame));
}
```

**ARP 请求帧格式：**

```
┌──────────────┬──────────────┬────────────┐
│  以太网头      │    ARP 头     │    填充    │
│  14 bytes    │   28 bytes  │   18 bytes │
└──────────────┴──────────────┴────────────┘

以太网头:
┌────────┬────────┬────────────┐
│ 目标MAC │ 源MAC  │ 类型=0x0806 │
│ FF:FF...│ 本机MAC│ (ARP)      │
└────────┴────────┴────────────┘

ARP 头:
┌────┬────┬────┬────┬────────┬────┬────────┬────┐
│ HW │ PRO │ HW │ PL │ OPCODE │ SM │ SPA    │ TM │
│ TY │ TY │ SL │ SL │  1=Req │ 6  │ 源IP   │ 6  │
│ 1  │ 800│ 6  │ 4  │  2=Rep │    │        │    │
└────┴────┴────┴────┴────────┴────┴────────┴────┘
```

### 3.4.6 `ip_send` - 用户态 IP 发送

**源码分析 (ip-cooked.c:140-220):**

```c
ssize_t
ip_send(ip_t *ip, const void *buf, size_t len)
{
    struct ip_hdr *iph;
    struct ip_intf *ipi;
    struct arp_entry arpent;
    struct route_entry rtent;
    u_char frame[ETH_LEN_MAX];
    int i, usec;

    iph = (struct ip_hdr *)buf;

    /* 1. 查找出口接口 */
    if ((ipi = _lookup_ip_intf(ip, iph->ip_dst)) == NULL) {
        errno = EHOSTUNREACH;
        return (-1);
    }

    /* 2. 准备 ARP 查询 */
    arpent.arp_pa.addr_type = ADDR_TYPE_IP;
    arpent.arp_pa.addr_bits = IP_ADDR_BITS;
    arpent.arp_pa.addr_ip = iph->ip_dst;

    memcpy(&rtent.route_dst, &arpent.arp_pa, sizeof(rtent.route_dst));

    /* 3. 尝试获取目标 MAC 地址（最多 3 次） */
    for (i = 0, usec = 10; i < 3; i++, usec *= 100) {
        /* 查询 ARP 缓存 */
        if (arp_get(ip->arp, &arpent) == 0)
            break;

        /* 检查是否需要通过网关 */
        if (route_get(ip->route, &rtent) == 0 &&
            rtent.route_gw.addr_ip != ipi->pa.addr_ip) {
            memcpy(&arpent.arp_pa, &rtent.route_gw,
                sizeof(arpent.arp_pa));
            if (arp_get(ip->arp, &arpent) == 0)
                break;
        }
        /* 发送 ARP 请求 */
        _request_arp(ipi, &arpent.arp_pa);

        usleep(usec);
    }

    /* 4. 如果 3 次都失败，使用广播 MAC */
    if (i == 3)
        memset(&arpent.arp_ha.addr_eth, 0xff, ETH_ADDR_LEN);

    /* 5. 构造以太网头 */
    eth_pack_hdr(frame, arpent.arp_ha.addr_eth,
        ipi->ha.addr_eth, ETH_TYPE_IP);

    /* 6. 分片处理 */
    if (len > ipi->mtu) {
        u_char *p, *start, *end, *ip_data;
        int ip_hl, fraglen;

        ip_hl = iph->ip_hl << 2;
        fraglen = ipi->mtu - ip_hl;

        iph = (struct ip_hdr *)(frame + ETH_HDR_LEN);
        memcpy(iph, buf, ip_hl);
        ip_data = (u_char *)iph + ip_hl;

        start = (u_char *)buf + ip_hl;
        end = (u_char *)buf + len;

        /* 分片发送 */
        for (p = start; p < end; ) {
            memcpy(ip_data, p, fraglen);

            iph->ip_len = htons(ip_hl + fraglen);
            iph->ip_off = htons(((p + fraglen < end) ? IP_MF : 0) |
                ((p - start) >> 3));

            ip_checksum(iph, ip_hl + fraglen);

            i = ETH_HDR_LEN + ip_hl + fraglen;
            if (eth_send(ipi->eth, frame, i) != i)
                return (-1);
            p += fraglen;
            if (end - p < fraglen)
                fraglen = end - p;
        }
        return (len);
    }

    /* 7. 不需要分片，直接发送 */
    memcpy(frame + ETH_HDR_LEN, buf, len);
    i = ETH_HDR_LEN + len;
    if (eth_send(ipi->eth, frame, i) != i)
        return (-1);

    return (len);
}
```

**发送流程详解：**

```
┌─────────────────────────────────────────────────────────┐
│ 1. 查找出口接口                                          │
│    - 使用 UDP socket + connect + getsockname           │
│    - 确定出口 IP 和对应的网卡                             │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│ 2. ARP 解析                                              │
│    - 查询 ARP 缓存                                      │
│    - 如果不在缓存中，发送 ARP 请求（最多 3 次）          │
│    - 获取目标 MAC 地址                                  │
│    - 如果有网关，查询网关的 MAC 地址                     │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│ 3. 分片判断                                              │
│    - 检查数据包大小是否超过 MTU                          │
│    - 如果超过，进行 IP 分片                              │
│    - 设置 MF 标志和分片偏移                              │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│ 4. 构造以太网帧                                          │
│    ┌────────┬────────┬────────────┐                     │
│    │ 目标MAC │ 源MAC  │ 类型=0x0800 │                     │
│    └────────┴────────┴────────────┘                     │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│ 5. 发送数据包                                            │
│    - 通过以太网 socket 发送                              │
│    - 使用 sendto() 系统调用                              │
└─────────────────────────────────────────────────────────┘
```

**IP 分片算法：**

```c
/* 分片参数 */
int ip_hl = iph->ip_hl << 2;        /* IP 头部长度 */
int fraglen = mtu - ip_hl;          /* 每个分片的数据长度 */
int offset = 0;                     /* 分片偏移（8 字节为单位） */

u_char *data_start = buf + ip_hl;
u_char *data_end = buf + len;

for (u_char *p = data_start; p < data_end; ) {
    /* 计算当前分片的实际长度 */
    int current_len = (p + fraglen < data_end) ? fraglen : (data_end - p);

    /* 设置 IP 头 */
    iph->ip_len = htons(ip_hl + current_len);
    iph->ip_off = htons(offset | IP_MF);  /* 设置 MF 标志 */

    /* 最后一个分片清除 MF 标志 */
    if (p + current_len >= data_end) {
        iph->ip_off = htons(offset);
    }

    /* 发送分片 */
    send_packet(iph, ip_hl + current_len);

    /* 更新偏移 */
    offset += (current_len >> 3);  /* 偏移以 8 字节为单位 */
    p += current_len;
}
```

### 3.4.7 `ip_close` - 清理资源

**源码分析 (ip-cooked.c:222-246):**

```c
ip_t *
ip_close(ip_t *ip)
{
    struct ip_intf *ipi, *nxt;

    if (ip != NULL) {
        /* 1. 释放接口链表 */
        for (ipi = LIST_FIRST(&ip->ip_intf_list);
            ipi != LIST_END(&ip->ip_intf_list); ipi = nxt) {
            nxt = LIST_NEXT(ipi, next);
            if (ipi->eth != NULL)
                eth_close(ipi->eth);
            free(ipi);
        }

        /* 2. 关闭其他模块 */
        if (ip->fd >= 0)
            close(ip->fd);
        if (ip->route != NULL)
            route_close(ip->route);
        if (ip->intf != NULL)
            intf_close(ip->intf);
        if (ip->arp != NULL)
            arp_close(ip->arp);

        /* 3. 释放句柄 */
        free(ip);
    }
    return (NULL);
}
```

---

## 3.6 平台对比总结

### 3.6.1 数据结构对比

| 平台        | IP 句柄结构       | Eth 句柄结构       | 成员                                                    | 说明                                |
| ----------- | ----------------- | ----------------- | ------------------------------------------------------- | ----------------------------------- |
| **Linux**   | `struct ip_handle` | `struct eth_handle`| `int fd`, `struct ifreq ifr`, `struct sockaddr_ll sll`     | socket + PF_PACKET                 |
| **macOS/BSD** | `struct ip_handle` | `struct eth_handle`| `int fd`, `char device[16]`                              | socket + BPF 设备                   |
| **Windows** | `struct ip_handle` | `struct eth_handle`| `WSADATA wsdata`, `SOCKET fd`, `struct sockaddr_in sin`   | WinSocket + 无实现                  |
| **Cooked**  | `struct ip_handle` | `struct eth_handle`| `arp_t*`, `intf_t*`, `route_t*`, `int fd`, `LIST_HEAD`    | 集成多个模块                        |

### 3.6.2 `ip_open` 对比

| 特性            | Linux (ip.c)                             | macOS/BSD (ip.c)                       | Windows (ip-win32.c)                     | Cooked (ip-cooked.c)             |
| --------------- | ---------------------------------------- | ---------------------------------------- | ---------------------------------------- | -------------------------------- |
| **Socket 创建** | `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)` | 同 Linux                                 | `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)` | `socket(AF_INET, SOCK_DGRAM, 0)` |
| **权限要求**    | root / CAP_NET_RAW                       | root                                     | Administrator                            | 无特殊权限                       |
| **IP_HDRINCL**  | 设置 `IP_HDRINCL`                        | 设置 `IP_HDRINCL`                        | 设置 `IP_HDRINCL`                        | 不使用                           |
| **缓冲区优化**  | 调整 `SO_SNDBUF`                         | 调整 `SO_SNDBUF`                         | 不调整                                   | 不适用                           |
| **额外初始化**  | 无                                       | 无                                       | `WSAStartup()`                           | 遍历接口、打开 arp/intf/route    |
| **依赖模块**    | 无                                       | 无                                       | 无                                       | eth, arp, intf, route            |

### 3.6.3 以太网发送对比

| 特性              | Linux (PF_PACKET)                     | macOS/BSD (BPF)                   | Windows (无实现) | Cooked                 |
| ----------------- | ------------------------------------ | ---------------------------------- | ---------------- | ---------------------- |
| **打开方式**      | `socket(PF_PACKET, SOCK_RAW)`        | `open("/dev/bpf0")`                | -                | `eth_open()`           |
| **发送函数**      | `sendto()`                           | `write()`                          | -                | `eth_send()`           |
| **需要地址结构**  | ✅ `struct sockaddr_ll`              | ❌ 直接写入                        | -                | ❌ 直接写入            |
| **自动补全头部**  | ❌                                   | ❌ (BIOCSHDRCMPLT)                 | -                | ❌                     |
| **设备绑定**      | ✅ `bind()` + `SIOCGIFINDEX`         | ✅ `ioctl(BIOCSETIF)`              | -                | ✅ 设备文件           |
| **BPF 过滤支持**  | ❌                                   | ✅                                 | -                | ❌                     |

### 3.6.4 `ip_send` 对比

| 特性         | Linux (ip.c)            | macOS/BSD (ip.c)          | Windows (ip-win32.c)        | Cooked (ip-cooked.c)        |
| ------------ | ----------------------- | ------------------------- | --------------------------- | --------------------------- |
| **目标地址** | `sin_addr = ip->ip_dst` | 同 Linux                  | `sin_addr = hdr->ip_src` ⚠️ | N/A (以太网发送)            |
| **ARP 解析** | 内核自动处理            | 内核自动处理              | 内核自动处理                | 用户态实现（最多 3 次重试） |
| **分片处理** | 内核自动处理            | 内核自动处理              | 内核自动处理                | 用户态实现                  |
| **校验和**   | 内核计算                | 内核计算                  | 内核计算                    | 用户计算 `ip_checksum()`    |
| **以太网头** | 内核添加                | 内核添加                  | 内核添加                    | 用户构造 `eth_pack_hdr()`   |
| **发送方式** | `sendto()`              | `sendto()`                | `sendto()`                  | `eth_send()`                |

### 3.6.5 使用场景

| 场景               | Linux            | macOS/BSD       | Windows          | Cooked               |
| ------------------ | ---------------- | --------------- | ---------------- | -------------------- |
| **需要 root 权限** | ✅               | ✅              | ❌ (需要 Admin)  | ❌                   |
| **需要管理员权限** | ❌               | ❌              | ✅               | ❌                   |
| **发送广播/组播**  | 支持             | 支持            | 支持             | 支持                 |
| **自定义 IP 头**   | 支持             | 支持            | 支持             | 支持                 |
| **自定义以太网头** | ❌               | ❌              | ❌               | ✅                   |
| **精确控制 ARP**   | ❌               | ❌              | ❌               | ✅                   |
| **手动分片**       | ❌               | ❌              | ❌               | ✅                   |
| **BPF 过滤**       | ❌               | ✅              | ❌               | ❌                   |
| **性能**           | 最高（内核处理） | 最高（内核处理） | 最高（内核处理） | 较低（用户态处理）   |
| **适用场景**       | Linux 服务器     | macOS 开发      | Windows 环境     | 无权限环境、精确控制 |

---

## 3.7 实战案例：构建自定义 IP 数据包

### 3.6.1 完整示例代码

```c
/*
 * 自定义 IP 数据包发送器
 * 支持跨平台编译和运行
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dnet.h"

/* 构造自定义 IP 数据包 */
void build_ip_packet(struct ip_hdr *ip,
                     uint32_t src_ip,
                     uint32_t dst_ip,
                     uint8_t protocol,
                     const void *data,
                     size_t data_len)
{
    /* IP 头部字段 */
    ip->ip_v = 4;                          /* IPv4 */
    ip->ip_hl = 5;                         /* 头部长度 20 字节 (5 * 4) */
    ip->ip_tos = 0;                        /* 服务类型 */
    ip->ip_len = htons(IP_HDR_LEN + data_len);  /* 总长度 */
    ip->ip_id = htons(12345);              /* 标识 */
    ip->ip_off = htons(IP_DF);             /* 不分片 */
    ip->ip_ttl = 64;                       /* TTL */
    ip->ip_p = protocol;                   /* 协议 */
    ip->ip_sum = 0;                        /* 校验和（先清零） */
    ip->ip_src = src_ip;
    ip->ip_dst = dst_ip;

    /* 复制数据 */
    memcpy((uint8_t *)ip + IP_HDR_LEN, data, data_len);

    /* 计算 IP 校验和 */
    ip_checksum(ip, IP_HDR_LEN + data_len);
}

/* 构造 ICMP Echo Request 数据包 */
void build_icmp_packet(uint8_t *packet, size_t len,
                       uint32_t src_ip, uint32_t dst_ip)
{
    struct ip_hdr *ip = (struct ip_hdr *)packet;
    struct icmp_hdr *icmp;

    /* ICMP 数据 */
    char data[64];
    memset(data, 0xAA, sizeof(data));

    /* 构造 IP 头 */
    build_ip_packet(ip, src_ip, dst_ip, IP_PROTO_ICMP,
                    data, sizeof(data));

    /* 构造 ICMP 头 */
    icmp = (struct icmp_hdr *)((uint8_t *)ip + IP_HDR_LEN);
    icmp->icmp_type = ICMP_ECHO;           /* Echo Request */
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = htons(1);
    icmp->icmp_id = htons(54321);

    /* 计算 ICMP 校验和 */
    icmp_checksum(icmp, sizeof(*icmp) + sizeof(data));
}

int main(int argc, char *argv[])
{
    ip_t *ip;
    uint8_t packet[1500];
    uint32_t src_ip, dst_ip;
    ssize_t ret;

    if (argc < 3) {
        printf("Usage: %s <src_ip> <dst_ip>\n", argv[0]);
        return 1;
    }

    /* 解析 IP 地址 */
    if (ip_pton(argv[1], &src_ip) < 0) {
        fprintf(stderr, "Invalid source IP: %s\n", argv[1]);
        return 1;
    }
    if (ip_pton(argv[2], &dst_ip) < 0) {
        fprintf(stderr, "Invalid destination IP: %s\n", argv[2]);
        return 1;
    }

    printf("Opening IP socket...\n");
    if ((ip = ip_open()) == NULL) {
        fprintf(stderr, "ip_open failed: %s\n", strerror(errno));
        return 1;
    }

    /* 构造 ICMP Echo Request */
    build_icmp_packet(packet, sizeof(packet), src_ip, dst_ip);

    printf("Sending ICMP Echo Request:\n");
    printf("  Source: %s\n", argv[1]);
    printf("  Destination: %s\n", argv[2]);

    ret = ip_send(ip, packet, ntohs(((struct ip_hdr *)packet)->ip_len));

    if (ret < 0) {
        fprintf(stderr, "ip_send failed: %s\n", strerror(errno));
        ip_close(ip);
        return 1;
    }

    printf("Sent %zd bytes\n", ret);

    ip_close(ip);
    return 0;
}
```

### 3.6.2 编译与运行

**Linux:**

```bash
# 编译
gcc -o ip_sender ip_sender.c -I include -L . -ldnet

# 需要 root 权限运行
sudo ./ip_sender 192.168.1.100 192.168.1.1
```

**Windows (MSVC):**

```cmd
REM 编译
cl ip_sender.c /I include libdnet.lib ws2_32.lib iphlpapi.lib

REM 需要管理员权限运行
ip_sender.exe 192.168.1.100 192.168.1.1
```

**Cooked 模式（无需 root/管理员权限）:**

```bash
# 编译时定义 USE_COOKED
gcc -DUSE_COOKED -o ip_sender ip_sender.c -I include -L . -ldnet

# 可以普通用户运行
./ip_sender 192.168.1.100 192.168.1.1
```

---

## 3.8 校验和计算实现 (ip-util.c)

### 3.7.1 IP 校验和算法

**源码分析 (ip-util.c:131-182):**

```c
void
ip_checksum(void *buf, size_t len, int flags)
{
    struct ip_hdr *ip;
    int hl, off, sum;

    if (len < IP_HDR_LEN)
        return;

    ip = (struct ip_hdr *)buf;
    hl = ip->ip_hl << 2;      /* IP 头部长度 */
    ip->ip_sum = 0;
    sum = ip_cksum_add(ip, hl, 0);
    ip->ip_sum = ip_cksum_carry(sum);

    /* 如果只计算分片校验和，则返回 */
    if (flags & IP_CHECKSUM_FRAGMENT)
        return;

    off = htons(ip->ip_off);

    /* 如果是分片，不计算上层校验和 */
    if ((off & IP_OFFMASK) != 0 || (off & IP_MF) != 0)
        return;

    len -= hl;

    /* 根据上层协议计算相应的校验和 */
    if (ip->ip_p == IP_PROTO_TCP) {
        struct tcp_hdr *tcp = (struct tcp_hdr *)((u_char *)ip + hl);
        if (len >= TCP_HDR_LEN) {
            tcp_checksum(ip, tcp, len);
        }
    } else if (ip->ip_p == IP_PROTO_UDP) {
        struct udp_hdr *udp = (struct udp_hdr *)((u_char *)ip + hl);
        if (len >= UDP_HDR_LEN) {
            udp_checksum(ip, udp, len);
        }
    } else if (ip->ip_p == IP_PROTO_ICMP || ip->ip_p == IP_PROTO_IGMP) {
        struct icmp_hdr *icmp = (struct icmp_hdr *)((u_char *)ip + hl);
        if (len >= ICMP_HDR_LEN) {
            icmp_checksum(icmp, len);
        }
    } else if (ip->ip_p == IP_PROTO_SCTP) {
        struct sctp_hdr *sctp = (struct sctp_hdr *)((u_char *)ip + hl);
        if (len >= SCTP_HDR_LEN) {
            sctp->sh_sum = 0;
            sctp->sh_sum = htonl(_crc32c((u_char *)sctp, len));
        }
    }
}
```

**IP 校验和计算原理：**

```c
/* IP 校验和算法 (RFC 1071) */
uint16_t compute_ip_checksum(uint16_t *buf, int len)
{
    uint32_t sum = 0;

    /* 16 位累加 */
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    /* 处理奇数长度 */
    if (len == 1) {
        sum += *(uint8_t *)buf;
    }

    /* 将高 16 位加到低 16 位 */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* 取反 */
    return (uint16_t)(~sum);
}
```

### 3.7.2 TCP/UDP 伪首部校验和

**TCP 校验和 (ip-util.c:101-109):**

```c
void
tcp_checksum(struct ip_hdr *ip, struct tcp_hdr *tcp, size_t len)
{
    int sum;
    tcp->th_sum = 0;

    /* 伪首部：源IP + 目的IP + 协议 + TCP长度 */
    sum = ip_cksum_add(tcp, len, 0) + htonl(ip->ip_p + len);
    sum = ip_cksum_add(&ip->ip_src, 8, sum);  /* 源IP + 目的IP (8 bytes) */

    tcp->th_sum = ip_cksum_carry(sum);
}
```

**TCP 伪首部结构：**

```
┌─────────────┬─────────────┬──────────┬─────────────┐
│  源 IP 地址  │ 目的 IP 地址  │  协议    │  TCP 长度    │
│   4 bytes   │   4 bytes   │ 1 byte  │  2 bytes    │
└─────────────┴─────────────┴──────────┴─────────────┘
         12 字节 (伪首部)
```

**伪首部校验和计算：**

```c
uint16_t compute_tcp_pseudo_checksum(uint32_t src_ip, uint32_t dst_ip,
                                     uint8_t protocol, uint16_t tcp_len,
                                     uint8_t *tcp_data, size_t data_len)
{
    uint32_t sum = 0;

    /* 源 IP (拆分为两个 16 位) */
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;

    /* 目的 IP (拆分为两个 16 位) */
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;

    /* 协议 + 长度 */
    sum += (protocol << 8) | (tcp_len >> 8);
    sum += tcp_len & 0xFF;

    /* TCP 数据 */
    sum += ip_cksum_add(tcp_data, data_len, 0);

    /* 取反 */
    return (uint16_t)~sum;
}
```

### 3.7.3 高性能校验和计算 (Duff's Device)

**源码分析 (ip-util.c:184-238):**

```c
int
ip_cksum_add(const void *buf, size_t len, int cksum)
{
    uint16_t *sp = (uint16_t *)buf;
    int n, sn;

    /* 单字节特殊处理 */
    if (len == 1) {
        const uint8_t *b = buf;
        return htons(((uint16_t)*b) << 8);
    }

    sn = len / 2;  /* 16 位字数 */
    n = (sn + 15) / 16;  /* 循环次数 */

    /* Duff's Device - 展开循环 */
    switch (sn % 16) {
    case 0: do {
        cksum += *sp++;
    case 15:
        cksum += *sp++;
    case 14:
        cksum += *sp++;
    case 13:
        cksum += *sp++;
    case 12:
        cksum += *sp++;
    case 11:
        cksum += *sp++;
    case 10:
        cksum += *sp++;
    case 9:
        cksum += *sp++;
    case 8:
        cksum += *sp++;
    case 7:
        cksum += *sp++;
    case 6:
        cksum += *sp++;
    case 5:
        cksum += *sp++;
    case 4:
        cksum += *sp++;
    case 3:
        cksum += *sp++;
    case 2:
        cksum += *sp++;
    case 1:
        cksum += *sp++;
        } while (--n > 0);
    }

    /* 处理奇数长度 */
    if (len & 1)
        cksum += htons(*(u_char *)sp << 8);

    return (cksum);
}

/* 进位处理 */
#define ip_cksum_carry(x) \
    (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))
```

**Duff's Device 原理：**

```c
/* 传统循环 (16 次迭代) */
for (int i = 0; i < 16; i++) {
    cksum += *sp++;
}

/* Duff's Device (减少循环控制开销) */
switch (i % 16) {
    case 0: do { cksum += *sp++;
    case 15:      cksum += *sp++;
    case 14:      cksum += *sp++;
    /* ... */
    case 1:       cksum += *sp++;
    } while (--n > 0);
}
```

**性能优势：**

- 减少循环条件判断次数
- 在处理长数据包时效果明显
- 一次处理 16 个 16 位字

---

## 3.9 常见问题与调试

### 3.8.1 Linux 常见问题

| 问题            | 错误信息                    | 原因                       | 解决方案                        |
| --------------- | --------------------------- | -------------------------- | ------------------------------- |
| 权限不足        | `Operation not permitted`   | 非 root 用户               | 使用 `sudo` 或设置 CAP_NET_RAW  |
| Socket 创建失败 | `Socket type not supported` | 内核未编译 RAW socket 支持 | 重新编译内核                    |
| 缓冲区不足      | `No buffer space available` | 发送速率过快               | 增加 `SO_SNDBUF` 或降低发送速率 |
| 路由失败        | `Network is unreachable`    | 目标不可达                 | 检查路由表 `ip route`           |

### 3.8.2 Windows 常见问题

| 问题       | 错误代码                   | 错误信息               | 解决方案                          |
| ---------- | -------------------------- | ---------------------- | --------------------------------- |
| 权限不足   | WSAEACCES (10013)          | Permission denied      | 以管理员身份运行                  |
| 协议不支持 | WSAEPROTONOSUPPORT (10043) | Protocol not supported | 使用 Professional/Enterprise 版本 |
| 无效参数   | WSAEINVAL (10022)          | Invalid argument       | 检查 IP 头格式                    |
| 网络不可达 | WSAENETUNREACH (10051)     | Network is unreachable | 检查网络连接                      |

### 3.8.3 调试技巧

**1. 启用详细日志**

```c
void log_packet(struct ip_hdr *ip, size_t len)
{
    char src[16], dst[16];

    ip_ntop(&ip->ip_src, src, sizeof(src));
    ip_ntop(&ip->ip_dst, dst, sizeof(dst));

    printf("IP Packet:\n");
    printf("  Version: %u\n", ip->ip_v);
    printf("  Header Length: %u bytes\n", ip->ip_hl * 4);
    printf("  TOS: 0x%02X\n", ip->ip_tos);
    printf("  Total Length: %u\n", ntohs(ip->ip_len));
    printf("  ID: %u\n", ntohs(ip->ip_id));
    printf("  Flags: %s%s\n",
           (ntohs(ip->ip_off) & IP_DF) ? "DF " : "",
           (ntohs(ip->ip_off) & IP_MF) ? "MF" : "");
    printf("  Fragment Offset: %u\n", ntohs(ip->ip_off) & IP_OFFMASK);
    printf("  TTL: %u\n", ip->ip_ttl);
    printf("  Protocol: %u\n", ip->ip_p);
    printf("  Checksum: 0x%04X\n", ntohs(ip->ip_sum));
    printf("  Source: %s\n", src);
    printf("  Destination: %s\n", dst);
    printf("  Payload Length: %zu\n", len - (ip->ip_hl * 4));
}
```

**2. 使用 tcpdump 抓包**

```bash
# Linux
sudo tcpdump -i eth0 -v -n 'ip host 192.168.1.100'

# Windows (使用 Wireshark)
# 安装 WinPcap 或 Npcap 后使用 Wireshark 抓包
```

**3. 检查内核日志**

```bash
# Linux
sudo dmesg | grep -i "raw\|socket"

# Windows
# 使用事件查看器 (Event Viewer) 查看系统日志
```

---

## 3.10 总结

### 3.9.1 核心要点

1. **三种实现方式**
   - **Linux/Unix (ip.c)**：使用内核 raw socket，性能最高
   - **Windows (ip-win32.c)**：使用 WinSocket raw socket
   - **Cooked (ip-cooked.c)**：用户态实现，无需特殊权限

2. **平台差异**
   - Windows 使用 `ip_src` 作为 sendto 目标地址 ⚠️
   - Windows 需要 `WSAStartup/WSACleanup`
   - Linux 可以动态调整发送缓冲区
   - Cooked 模式需要手动处理 ARP 和分片

3. **校验和计算**
   - IP 校验和：覆盖 IP 头部
   - TCP/UDP 校验和：包含伪首部
   - 使用 Duff's Device 优化性能

4. **权限要求**
   - Raw socket 需要 root/管理员权限
   - Cooked 模式无权限限制
   - 非特权环境考虑使用 cooked 模式

### 3.9.2 最佳实践

1. **错误处理**
   - 始终检查 `ip_open()` 返回值
   - 处理 `errno` 和 `GetLastError()`
   - 提供清晰的错误信息

2. **资源管理**
   - 确保调用 `ip_close()` 释放资源
   - 使用 `goto` 进行错误清理
   - 避免资源泄漏

3. **性能优化**
   - 复用 socket 句柄
   - 批量发送数据包
   - 使用适当大小的缓冲区

4. **跨平台兼容**
   - 使用条件编译处理平台差异
   - 统一错误处理机制
   - 提供平台特定的回退方案

---
