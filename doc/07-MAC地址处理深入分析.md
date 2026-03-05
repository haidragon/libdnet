# MAC地址处理深入分析

## 目录

1. [MAC地址基础概念](#1-mac地址基础概念)
2. [libdnet以太网接口设计](#2-libdnet以太网接口设计)
3. [Linux平台实现](#3-linux平台实现)
4. [macOS/BSD平台实现](#4-macosbsd平台实现)
5. [Solaris平台实现](#5-solaris平台实现)
6. [HP-UX平台实现](#6-hp-ux平台实现)
7. [AIX平台实现](#7-aix平台实现)
8. [Windows平台实现](#8-windows平台实现)
9. [跨平台对比分析](#9-跨平台对比分析)
10. [实际应用示例](#10-实际应用示例)
11. [常见问题与解决方案](#11-常见问题与解决方案)

---

## 1. MAC地址基础概念

### 1.1 MAC地址简介

MAC地址（Media Access Control Address）是网络设备的物理地址，用于数据链路层的设备标识。MAC地址具有以下特点：

- **长度**：48位（6字节）
- **格式**：十六进制表示，如`00:11:22:33:44:55`
- **唯一性**：理论上全球唯一
- **可变性**：大多数设备可以修改

### 1.2 MAC地址结构

```c
// include/dnet/eth.h:25-27
typedef struct eth_addr {
    uint8_t data[ETH_ADDR_LEN];  // 6字节数组
} eth_addr_t;
```

### 1.3 MAC地址分类

#### 1.3.1 单播地址（Unicast）
- 第一个字节的最低位为0
- 例如：`00:11:22:33:44:55`
- 用于点对点通信

#### 1.3.2 多播地址（Multicast）
- 第一个字节的最低位为1
- 例如：`01:00:5e:00:00:01`
- 用于组播通信

#### 1.3.3 广播地址（Broadcast）
- 所有48位都为1
- `ff:ff:ff:ff:ff:ff`
- 用于广播通信

**判断宏**：
```c
// include/dnet/eth.h:70
#define ETH_IS_MULTICAST(ea)    (*(ea) & 0x01)  // 检查是否为多播/广播
```

### 1.4 以太网帧结构

```c
// include/dnet/eth.h:29-33
struct eth_hdr {
    eth_addr_t eth_dst;    // 目标MAC地址（6字节）
    eth_addr_t eth_src;    // 源MAC地址（6字节）
    uint16_t eth_type;     // 以太网类型/长度（2字节）
};
```

**以太网帧长度**：
- **最小帧长**：64字节（包含4字节CRC）
- **最大帧长**：1518字节（包含4字节CRC）
- **MTU**：1500字节（不包括以太网头和CRC）

**常量定义**：
```c
// include/dnet/eth.h:13-23
#define ETH_ADDR_LEN     6      // MAC地址长度
#define ETH_ADDR_BITS    48     // MAC地址位数
#define ETH_TYPE_LEN     2      // 以太网类型字段长度
#define ETH_CRC_LEN      4      // CRC校验长度
#define ETH_HDR_LEN      14     // 以太网头长度
#define ETH_LEN_MIN      64     // 最小帧长
#define ETH_LEN_MAX      1518   // 最大帧长
#define ETH_MTU          1500   // 最大传输单元
```

### 1.5 以太网类型（EtherType）

常见的以太网类型值：

| EtherType | 名称 | 说明 |
|-----------|------|------|
| 0x0200 | PUP | Xerox PUP协议 |
| 0x0800 | IP | IPv4协议 |
| 0x0806 | ARP | 地址解析协议 |
| 0x8035 | RARP | 反向地址解析协议 |
| 0x8100 | 802.1Q | VLAN标签 |
| 0x86DD | IPv6 | IPv6协议 |
| 0x8847 | MPLS | MPLS单播 |
| 0x8848 | MPLS MCAST | MPLS组播 |
| 0x8863 | PPPOEDISC | PPPoE发现阶段 |
| 0x8864 | PPPOE | PPPoE会话阶段 |
| 0x88a8 | 802.1ad | 双标签VLAN |
| 0x9000 | Loopback | 回环测试 |
| 0x9100/9200/9300 | 802.1ad | Cisco双标签VLAN |

### 1.6 VLAN标签结构

```c
// include/dnet/eth.h:54-61
struct eth_8021q_hdr {
    uint16_t priority_c_vid;  // 优先级|VLAN ID（TCI）
    uint16_t len_eth_type;   // 长度或类型
};

#define ETH_8021Q_PRIMASK   0x0007  // 优先级掩码（3位）
#define ETH_8021Q_CFIMASK   0x0001  // 规范格式标识符（1位）
#define ETH_8021Q_VIDMASK   0x0fff  // VLAN ID掩码（12位）
```

**VLAN类型判断**：
```c
// include/dnet/eth.h:63-68
#define ETH_TYPE_IS_VLAN(type) \
    (((type) == ETH_TYPE_8021Q) || \
     ((type) == ETH_TYPE_8021ad_0) || \
     ((type) == ETH_TYPE_8021ad_1) || \
     ((type) == ETH_TYPE_8021ad_2) || \
     ((type) == ETH_TYPE_8021ad_3))
```

---

## 2. libdnet以太网接口设计

### 2.1 核心API接口

```c
// include/dnet/eth.h:84-93
eth_t *eth_open(const char *device);                    // 打开以太网设备
int eth_get(eth_t *e, eth_addr_t *ea);                   // 获取MAC地址
int eth_set(eth_t *e, const eth_addr_t *ea);             // 设置MAC地址
ssize_t eth_send(eth_t *e, const void *buf, size_t len); // 发送以太网帧
eth_t *eth_close(eth_t *e);                              // 关闭以太网设备

char *eth_ntop(const eth_addr_t *eth, char *dst, size_t len);  // MAC转字符串
int eth_pton(const char *src, eth_addr_t *dst);                  // 字符串转MAC
char *eth_ntoa(const eth_addr_t *eth);                          // MAC转字符串（简写）
```

### 2.2 以太网帧打包宏

```c
// include/dnet/eth.h:74-79
#define eth_pack_hdr(h, dst, src, type) do {           \
    struct eth_hdr *eth_pack_p = (struct eth_hdr *)(h);   \
    memcpy(&eth_pack_p->eth_dst, &(dst), ETH_ADDR_LEN);  \
    memcpy(&eth_pack_p->eth_src, &(src), ETH_ADDR_LEN);  \
    eth_pack_p->eth_type = htons(type);                 \
} while (0)
```

**使用示例**：
```c
eth_addr_t dst = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};  // 广播
eth_addr_t src = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
u_char frame[ETH_LEN_MAX];

eth_pack_hdr(frame, dst, src, ETH_TYPE_ARP);
```

### 2.3 MAC地址转换函数

#### 2.3.1 MAC转字符串

```c
// src/addr-util.c:75-92
char *eth_ntop(const eth_addr_t *eth, char *dst, size_t len)
{
    const char *x;
    char *p = dst;
    int i;

    if (len < 18)  // 需要至少18字节（"xx:xx:xx:xx:xx:xx" + '\0'）
        return (NULL);

    for (i = 0; i < ETH_ADDR_LEN; i++) {
        for (x = octet2hex[eth->data[i]]; (*p = *x) != '\0'; x++, p++)
            ;
        *p++ = ':';
    }
    p[-1] = '\0';  // 替换最后的':'为'\0'

    return (dst);
}
```

**格式**：`00:11:22:33:44:55`

#### 2.3.2 字符串转MAC

```c
// src/addr-util.c:104-119
int eth_pton(const char *p, eth_addr_t *eth)
{
    char *ep;
    long l;
    int i;

    for (i = 0; i < ETH_ADDR_LEN; i++) {
        l = strtol(p, &ep, 16);  // 十六进制解析
        if (ep == p || l < 0 || l > 0xff ||
            (i < ETH_ADDR_LEN - 1 && *ep != ':'))
            break;
        eth->data[i] = (u_char)l;
        p = ep + 1;
    }
    return ((i == ETH_ADDR_LEN && *ep == '\0') ? 0 : -1);
}
```

**支持的格式**：
- `00:11:22:33:44:55`（推荐）
- `00-11-22-33-44-55`
- `0011:2233:4455`（部分格式）

### 2.4 平台适配机制

libdnet通过编译时宏选择不同实现：

| 实现文件 | 平台 | 说明 |
|---------|------|------|
| `eth-linux.c` | Linux | Packet Socket |
| `eth-bsd.c` | macOS/BSD | BPF |
| `eth-dlpi.c` | Solaris/HP-UX | DLPI |
| `eth-snoop.c` | Solaris | SNOOP |
| `eth-pfilt.c` | Tru64 | Packet Filter |
| `eth-ndd.c` | AIX | NDD |
| `eth-win32.c` | Windows | 不支持 |
| `eth-none.c` | 无 | 空实现 |

---

## 3. Linux平台实现

### 3.1 实现文件

**主文件**：`src/eth-linux.c`

### 3.2 数据结构

```c
// src/eth-linux.c:36-40
struct eth_handle {
    int                 fd;         // Packet Socket文件描述符
    struct ifreq        ifr;        // 接口请求结构
    struct sockaddr_ll  sll;        // 链路层socket地址
};
```

### 3.3 打开以太网设备

```c
// src/eth-linux.c:42-67
eth_t *eth_open(const char *device)
{
    eth_t *e;
    int n;

    if ((e = calloc(1, sizeof(*e))) != NULL) {
        // 创建Packet Socket
        if ((e->fd = socket(PF_PACKET, SOCK_RAW,
                 htons(ETH_P_ALL))) < 0)
            return (eth_close(e));

#ifdef SO_BROADCAST
        // 允许广播
        n = 1;
        if (setsockopt(e->fd, SOL_SOCKET, SO_BROADCAST, &n,
            sizeof(n)) < 0)
            return (eth_close(e));
#endif
        // 保存接口名称
        strlcpy(e->ifr.ifr_name, device, sizeof(e->ifr.ifr_name));

        // 获取接口索引
        if (ioctl(e->fd, SIOCGIFINDEX, &e->ifr) < 0)
            return (eth_close(e));

        // 配置链路层socket地址
        e->sll.sll_family = AF_PACKET;
        e->sll.sll_ifindex = e->ifr.ifr_ifindex;
    }
    return (e);
}
```

**关键技术**：

1. **Packet Socket**：
   - `PF_PACKET`：协议族为数据包协议
   - `SOCK_RAW`：原始套接字
   - `ETH_P_ALL`：接收所有以太网帧

2. **SIOCGIFINDEX**：
   - 获取接口的索引号
   - 用于`sockaddr_ll`的`sll_ifindex`字段

3. **SO_BROADCAST**：
   - 允许发送广播帧

### 3.4 发送以太网帧

```c
// src/eth-linux.c:69-78
ssize_t eth_send(eth_t *e, const void *buf, size_t len)
{
    struct eth_hdr *eth = (struct eth_hdr *)buf;

    // 设置协议类型
    e->sll.sll_protocol = eth->eth_type;

    // 发送数据帧
    return (sendto(e->fd, buf, len, 0, (struct sockaddr *)&e->sll,
        sizeof(e->sll)));
}
```

**说明**：
- `eth->eth_type`：以太网类型字段
- `sendto`：使用链路层地址发送

### 3.5 获取MAC地址

```c
// src/eth-linux.c:91-104
int eth_get(eth_t *e, eth_addr_t *ea)
{
    struct addr ha;

    // 使用ioctl获取硬件地址
    if (ioctl(e->fd, SIOCGIFHWADDR, &e->ifr) < 0)
        return (-1);

    // 转换为libdnet地址格式
    if (addr_ston(&e->ifr.ifr_hwaddr, &ha) < 0)
        return (-1);

    // 提取MAC地址
    memcpy(ea, &ha.addr_eth, sizeof(*ea));
    return (0);
}
```

**ioctl命令**：
- `SIOCGIFHWADDR`：获取接口硬件地址

### 3.6 设置MAC地址

```c
// src/eth-linux.c:106-118
int eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct addr ha;

    // 构造地址结构
    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);

    // 转换为sockaddr格式
    addr_ntos(&ha, &e->ifr.ifr_hwaddr);

    // 设置硬件地址
    return (ioctl(e->fd, SIOCSIFHWADDR, &e->ifr));
}
```

**ioctl命令**：
- `SIOCSIFHWADDR`：设置接口硬件地址

**注意**：需要root权限

### 3.7 关闭以太网设备

```c
// src/eth-linux.c:80-89
eth_t *eth_close(eth_t *e)
{
    if (e != NULL) {
        if (e->fd >= 0)
            close(e->fd);
        free(e);
    }
    return (NULL);
}
```

---

## 4. macOS/BSD平台实现

### 4.1 实现文件

**主文件**：`src/eth-bsd.c`

### 4.2 数据结构

```c
// src/eth-bsd.c:34-37
struct eth_handle {
    int     fd;         // BPF文件描述符
    char    device[16]; // 接口名称
};
```

### 4.3 打开以太网设备

```c
// src/eth-bsd.c:39-74
eth_t *eth_open(const char *device)
{
    struct ifreq ifr;
    char file[32];
    eth_t *e;
    int i;

    if ((e = calloc(1, sizeof(*e))) != NULL) {
        // 尝试打开BPF设备（/dev/bpf0 - /dev/bpf127）
        for (i = 0; i < 128; i++) {
            snprintf(file, sizeof(file), "/dev/bpf%d", i);
            /* Mac OS X 10.6的bug：O_WRONLY会导致其他进程无法接收流量 */
            e->fd = open(file, O_RDWR);
            if (e->fd != -1 || errno != EBUSY)
                break;
        }
        if (e->fd < 0)
            return (eth_close(e));

        // 绑定到指定接口
        memset(&ifr, 0, sizeof(ifr));
        strlcpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

        if (ioctl(e->fd, BIOCSETIF, (char *)&ifr) < 0)
            return (eth_close(e));

#ifdef BIOCSHDRCMPLT
        // 设置标志：驱动不自动填充源MAC地址
        i = 1;
        if (ioctl(e->fd, BIOCSHDRCMPLT, &i) < 0)
            return (eth_close(e));
#endif
        // 保存设备名称
        strlcpy(e->device, device, sizeof(e->device));
    }
    return (e);
}
```

**关键技术**：

1. **BPF设备**：
   - BSD Packet Filter
   - 设备文件：`/dev/bpf0`到`/dev/bpf127`
   - 自动查找可用的BPF设备

2. **BIOCSETIF**：
   - 将BPF绑定到指定网络接口

3. **BIOCSHDRCMPLT**：
   - 指示驱动程序不自动完成以太网头
   - 允许应用程序发送自定义源MAC地址

### 4.4 发送以太网帧

```c
// src/eth-bsd.c:76-80
ssize_t eth_send(eth_t *e, const void *buf, size_t len)
{
    return (write(e->fd, buf, len));
}
```

**说明**：
- 直接向BPF文件描述符写入数据
- 驱动程序自动处理以太网帧的发送

### 4.5 获取MAC地址（sysctl方式）

```c
// src/eth-bsd.c:93-138
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_ROUTE_RT_MSGHDR)
int eth_get(eth_t *e, eth_addr_t *ea)
{
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;
    struct addr ha;
    u_char *p, *buf;
    size_t len;
    int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 };

    // 获取接口列表所需的缓冲区大小
    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return (-1);

    // 分配缓冲区
    if ((buf = malloc(len)) == NULL)
        return (-1);

    // 获取接口列表
    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        free(buf);
        return (-1);
    }

    // 遍历接口列表
    for (p = buf; p < buf + len; p += ifm->ifm_msglen) {
        ifm = (struct if_msghdr *)p;
        sdl = (struct sockaddr_dl *)(ifm + 1);

        if (ifm->ifm_type != RTM_IFINFO ||
            (ifm->ifm_addrs & RTA_IFP) == 0)
            continue;

        // 检查接口名称匹配
        if (sdl->sdl_family != AF_LINK || sdl->sdl_nlen == 0 ||
            memcmp(sdl->sdl_data, e->device, sdl->sdl_nlen) != 0)
            continue;

        // 提取MAC地址
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
#endif
```

**关键技术**：

1. **sysctl**：
   - 系统控制接口
   - 获取内核信息

2. **MIB标识符**：
   - `CTL_NET`：网络相关
   - `AF_ROUTE`：路由套接字
   - `AF_LINK`：链路层地址
   - `NET_RT_IFLIST`：接口列表

3. **sockaddr_dl**：
   - 链路层socket地址
   - 包含接口名称和MAC地址

### 4.6 设置MAC地址

```c
// src/eth-bsd.c:148-164
#if defined(SIOCSIFLLADDR)
int eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct ifreq ifr;
    struct addr ha;

    // 构造地址结构
    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);

    // 配置ifreq
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, e->device, sizeof(ifr.ifr_name));
    addr_ntos(&ha, &ifr.ifr_addr);

    // 设置链路层地址
    return (ioctl(e->fd, SIOCSIFLLADDR, &ifr));
}
#endif
```

**ioctl命令**：
- `SIOCSIFLLADDR`：设置接口链路层地址（BSD特定）

---

## 5. Solaris平台实现

### 5.1 实现文件

**主文件**：`src/eth-dlpi.c`、`src/eth-snoop.c`

### 5.2 DLPI实现（Data Link Provider Interface）

#### 5.2.1 数据结构

```c
// src/eth-dlpi.c:42-45
struct eth_handle {
    int     fd;         // DLPI流文件描述符
    int     sap_len;    // SAP（服务访问点）长度
};
```

#### 5.2.2 打开以太网设备

```c
// src/eth-dlpi.c:132-205
eth_t *eth_open(const char *device)
{
    union DL_primitives *dlp;
    uint32_t buf[8192];
    char *p, dev[16];
    eth_t *e;
    int ppa;

    if ((e = calloc(1, sizeof(*e))) == NULL)
        return (NULL);

#ifdef HAVE_SYS_DLPIHDR_H
    // OSF1/Tru64：使用流设备
    if ((e->fd = open("/dev/streams/dlb", O_RDWR)) < 0)
        return (eth_close(e));

    if ((ppa = eth_match_ppa(e, device)) < 0) {
        errno = ESRCH;
        return (eth_close(e));
    }
#else
    // Solaris/HP-UX：使用DLPI设备
    e->fd = -1;
    snprintf(dev, sizeof(dev), "/dev/%s", device);
    
    // 查找PPA（物理点访问点）
    if ((p = dev_find_ppa(dev)) == NULL) {
        errno = EINVAL;
        return (eth_close(e));
    }
    ppa = atoi(p);
    *p = '\0';

    // 尝试打开设备
    if ((e->fd = open(dev, O_RDWR)) < 0) {
        snprintf(dev, sizeof(dev), "/dev/%s", device);
        if ((e->fd = open(dev, O_RDWR)) < 0) {
            snprintf(dev, sizeof(dev), "/dev/net/%s", device);
            if ((e->fd = open(dev, O_RDWR)) < 0)
                return (eth_close(e));
        }
    }
#endif

    // 获取DLPI提供者信息
    dlp = (union DL_primitives *)buf;
    dlp->info_req.dl_primitive = DL_INFO_REQ;

    if (dlpi_msg(e->fd, dlp, DL_INFO_REQ_SIZE, RS_HIPRI,
        DL_INFO_ACK, DL_INFO_ACK_SIZE, sizeof(buf)) < 0)
        return (eth_close(e));

    e->sap_len = dlp->info_ack.dl_sap_length;

    // STYLE2：需要附加到PPA
    if (dlp->info_ack.dl_provider_style == DL_STYLE2) {
        dlp->attach_req.dl_primitive = DL_ATTACH_REQ;
        dlp->attach_req.dl_ppa = ppa;

        if (dlpi_msg(e->fd, dlp, DL_ATTACH_REQ_SIZE, 0,
            DL_OK_ACK, DL_OK_ACK_SIZE, sizeof(buf)) < 0)
            return (eth_close(e));
    }

    // 绑定到SAP
    memset(&dlp->bind_req, 0, DL_BIND_REQ_SIZE);
    dlp->bind_req.dl_primitive = DL_BIND_REQ;
#ifdef DL_HP_RAWDLS
    // HP-UX：原始数据链路服务
    dlp->bind_req.dl_sap = 24;
    dlp->bind_req.dl_service_mode = DL_HP_RAWDLS;
#else
    dlp->bind_req.dl_sap = DL_ETHER;
    dlp->bind_req.dl_service_mode = DL_CLDLS;
#endif
    if (dlpi_msg(e->fd, dlp, DL_BIND_REQ_SIZE, 0,
        DL_BIND_ACK, DL_BIND_ACK_SIZE, sizeof(buf)) < 0)
        return (eth_close(e));

#ifdef DLIOCRAW
    // 设置原始模式
    if (strioctl(e->fd, DLIOCRAW, 0, NULL) < 0)
        return (eth_close(e));
#endif
    return (e);
}
```

**关键技术**：

1. **DLPI原语**：
   - `DL_INFO_REQ`：请求提供者信息
   - `DL_ATTACH_REQ`：附加到PPA（STYLE2）
   - `DL_BIND_REQ`：绑定到SAP
   - `DL_OK_ACK`：操作成功确认

2. **DLPI风格**：
   - **STYLE1**：打开时直接附加到PPA
   - **STYLE2**：需要显式附加到PPA

3. **DLIOCRAW**：
   - 设置原始模式
   - 允许发送完整的以太网帧

#### 5.2.3 发送以太网帧

```c
// src/eth-dlpi.c:207-257
ssize_t eth_send(eth_t *e, const void *buf, size_t len)
{
#if defined(DLIOCRAW)
    // 原始模式：直接写入
    return (write(e->fd, buf, len));
#else
    union DL_primitives *dlp;
    struct strbuf ctl, data;
    struct eth_hdr *eth;
    uint32_t ctlbuf[8192];
    u_char sap[4] = { 0, 0, 0, 0 };
    int dlen;

    dlp = (union DL_primitives *)ctlbuf;
#ifdef DL_HP_RAWDATA_REQ
    dlp->dl_primitive = DL_HP_RAWDATA_REQ;
    dlen = DL_HP_RAWDATA_REQ_SIZE;
#else
    dlp->unitdata_req.dl_primitive = DL_UNITDATA_REQ;
    dlp->unitdata_req.dl_dest_addr_length = ETH_ADDR_LEN;
    dlp->unitdata_req.dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
    dlp->unitdata_req.dl_priority.dl_min =
        dlp->unitdata_req.dl_priority.dl_max = 0;
    dlen = DL_UNITDATA_REQ_SIZE;
#endif

    // 提取以太网类型
    eth = (struct eth_hdr *)buf;
    *(uint16_t *)sap = ntohs(eth->eth_type);

    // 构造控制消息
    ctl.maxlen = 0;
    ctl.len = dlen + ETH_ADDR_LEN + abs(e->sap_len);
    ctl.buf = (char *)ctlbuf;

    // 设置SAP和目标地址
    if (e->sap_len >= 0) {
        memcpy(ctlbuf + dlen, sap, e->sap_len);
        memcpy(ctlbuf + dlen + e->sap_len,
            eth->eth_dst.data, ETH_ADDR_LEN);
    } else {
        memcpy(ctlbuf + dlen, eth->eth_dst.data, ETH_ADDR_LEN);
        memcpy(ctlbuf + dlen + ETH_ADDR_LEN, sap, abs(e->sap_len));
    }

    // 设置数据消息
    data.maxlen = 0;
    data.len = len;
    data.buf = (char *)buf;

    // 发送消息
    if (putmsg(e->fd, &ctl, &data, 0) < 0)
        return (-1);

    return (len);
#endif
}
```

**STREAMS消息**：
- `putmsg`：发送消息
- `strbuf`：控制消息和数据消息

#### 5.2.4 获取MAC地址

```c
// src/eth-dlpi.c:270-287
int eth_get(eth_t *e, eth_addr_t *ea)
{
    union DL_primitives *dlp;
    u_char buf[2048];

    dlp = (union DL_primitives *)buf;
    dlp->physaddr_req.dl_primitive = DL_PHYS_ADDR_REQ;
    dlp->physaddr_req.dl_addr_type = DL_CURR_PHYS_ADDR;

    if (dlpi_msg(e->fd, dlp, DL_PHYS_ADDR_REQ_SIZE, 0,
        DL_PHYS_ADDR_ACK, DL_PHYS_ADDR_ACK_SIZE, sizeof(buf)) < 0)
        return (-1);

    memcpy(ea, buf + dlp->physaddr_ack.dl_addr_offset, sizeof(*ea));

    return (0);
}
```

#### 5.2.5 设置MAC地址

```c
// src/eth-dlpi.c:289-304
int eth_set(eth_t *e, const eth_addr_t *ea)
{
    union DL_primitives *dlp;
    u_char buf[2048];

    dlp = (union DL_primitives *)buf;
    dlp->set_physaddr_req.dl_primitive = DL_SET_PHYS_ADDR_REQ;
    dlp->set_physaddr_req.dl_addr_length = ETH_ADDR_LEN;
    dlp->set_physaddr_req.dl_addr_offset = DL_SET_PHYS_ADDR_REQ_SIZE;

    memcpy(buf + DL_SET_PHYS_ADDR_REQ_SIZE, ea, sizeof(*ea));

    return (dlpi_msg(e->fd, dlp, DL_SET_PHYS_ADDR_REQ_SIZE + ETH_ADDR_LEN,
        0, DL_OK_ACK, DL_OK_ACK_SIZE, sizeof(buf)));
}
```

### 5.3 SNOOP实现（Solaris）

#### 5.3.1 数据结构

```c
// src/eth-snoop.c:25-28
struct eth_handle {
    int     fd;         // SNOOP socket
    struct ifreq ifr;   // 接口请求结构
};
```

#### 5.3.2 打开以太网设备

```c
// src/eth-snoop.c:30-57
eth_t *eth_open(const char *device)
{
    struct sockaddr_raw sr;
    eth_t *e;
    int n;
    
    if ((e = calloc(1, sizeof(*e))) == NULL)
        return (NULL);

    // 创建SNOOP原始socket
    if ((e->fd = socket(PF_RAW, SOCK_RAW, RAWPROTO_SNOOP)) < 0)
        return (eth_close(e));
    
    // 配置socket地址
    memset(&sr, 0, sizeof(sr));
    sr.sr_family = AF_RAW;
    strlcpy(sr.sr_ifname, device, sizeof(sr.sr_ifname));

    // 绑定到接口
    if (bind(e->fd, (struct sockaddr *)&sr, sizeof(sr)) < 0)
        return (eth_close(e));
    
    // 设置发送缓冲区
    n = 60000;
    if (setsockopt(e->fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) < 0)
        return (eth_close(e));
    
    strlcpy(e->ifr.ifr_name, device, sizeof(e->ifr.ifr_name));
    
    return (e);
}
```

#### 5.3.3 获取/设置MAC地址

```c
// src/eth-snoop.c:59-92
int eth_get(eth_t *e, eth_addr_t *ea)
{
    struct addr ha;
    
    if (ioctl(e->fd, SIOCGIFADDR, &e->ifr) < 0)
        return (-1);

    if (addr_ston(&e->ifr.ifr_addr, &ha) < 0)
        return (-1);

    if (ha.addr_type != ADDR_TYPE_ETH) {
        errno = EINVAL;
        return (-1);
    }
    memcpy(ea, &ha.addr_eth, sizeof(*ea));
    
    return (0);
}

int eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct addr ha;

    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);
    
    if (addr_ntos(&ha, &e->ifr.ifr_addr) < 0)
        return (-1);
    
    return (ioctl(e->fd, SIOCSIFADDR, &e->ifr));
}
```

---

## 6. HP-UX平台实现

HP-UX使用DLPI实现，已在5.2节详细说明。

**特殊之处**：
```c
// src/eth-dlpi.c:190-192
#ifdef DL_HP_RAWDLS
    dlp->bind_req.dl_sap = 24;  // HP-UX专用值
    dlp->bind_req.dl_service_mode = DL_HP_RAWDLS;
#endif
```

---

## 7. AIX平台实现

### 7.1 实现文件

**主文件**：`src/eth-ndd.c`

### 7.2 数据结构

```c
// src/eth-ndd.c:25-28
struct eth_handle {
    char    device[16];  // 接口名称
    int     fd;          // NDD socket
};
```

### 7.3 打开以太网设备

```c
// src/eth-ndd.c:30-58
eth_t *eth_open(const char *device)
{
    struct sockaddr_ndd_8022 sa;
    eth_t *e;

    if ((e = calloc(1, sizeof(*e))) == NULL)
        return (NULL);

    // 创建NDD socket
    if ((e->fd = socket(AF_NDD, SOCK_DGRAM, NDD_PROT_ETHER)) < 0)
        return (eth_close(e));

    // 配置socket地址
    sa.sndd_8022_family = AF_NDD;
    sa.sndd_8022_len = sizeof(sa);
    sa.sndd_8022_filtertype = NS_ETHERTYPE;
    sa.sndd_8022_ethertype = 0;
    sa.sndd_8022_filterlen = sizeof(struct ns_8022);
    strlcpy(sa.sndd_8022_nddname, device, sizeof(sa.sndd_8022_nddname));

    // 绑定socket
    if (bind(e->fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        return (eth_close(e));

    // 连接socket
    if (connect(e->fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        return (eth_close(e));

    return (e);
}
```

**NDD（Network Device Driver）**：
- AIX专用的网络设备驱动接口
- 用于直接访问底层网络设备

### 7.4 获取MAC地址

```c
// src/eth-ndd.c:77-110
int eth_get(eth_t *e, eth_addr_t *ea)
{
    struct kinfo_ndd *nddp;
    int size;
    void *end;

    // 获取NDD信息所需的缓冲区大小
    if ((size = getkerninfo(KINFO_NDD, 0, 0, 0)) == 0) {
        errno = ENOENT;
        return (-1);
    } else if (size < 0)
        return (-1);

    // 分配缓冲区
    if ((nddp = malloc(size)) == NULL)
        return (-1);

    // 获取NDD信息
    if (getkerninfo(KINFO_NDD, nddp, &size, 0) < 0) {
        free(nddp);
        return (-1);
    }

    // 遍历NDD列表
    for (end = (void *)nddp + size; (void *)nddp < end; nddp++) {
        if (strcmp(nddp->ndd_alias, e->device) == 0 ||
            strcmp(nddp->ndd_name, e->device) == 0) {
            memcpy(ea, nddp->ndd_addr, sizeof(*ea));
        }
    }
    free(nddp);

    if ((void *)nddp >= end) {
        errno = ESRCH;
        return (-1);
    }
    return (0);
}
```

**getkerninfo**：
- AIX的内核信息接口
- `KINFO_NDD`：网络设备驱动信息

---

## 8. Windows平台实现

### 8.1 实现文件

**主文件**：`src/eth-win32.c`

### 8.2 当前状态

```c
// src/eth-win32.c:23-50
eth_t *eth_open(const char *device)
{
    return (NULL);  // 不支持
}

ssize_t eth_send(eth_t *eth, const void *buf, size_t len)
{
    return (-1);  // 不支持
}

eth_t *eth_close(eth_t *eth)
{
    return (NULL);
}

int eth_get(eth_t *eth, eth_addr_t *ea)
{
    return (-1);  // 不支持
}

int eth_set(eth_t *eth, const eth_addr_t *ea)
{
    return (-1);  // 不支持
}
```

**说明**：
- Windows平台不直接支持原始以太网帧发送
- 需要使用第三方库（如WinPcap/Npcap）
- libdnet的Windows实现仅提供框架

### 8.3 替代方案

在Windows上，可以使用以下库：
1. **WinPcap/Npcap**：提供类似BPF的功能
2. **Raw Sockets**：功能有限，仅支持部分协议
3. **Packet.dll**：底层数据包捕获和发送

---

## 9. 跨平台对比分析

### 9.1 API设计对比

| 功能 | Linux | macOS/BSD | Solaris | HP-UX | AIX | Windows |
|-----|-------|-----------|---------|-------|-----|---------|
| **打开设备** | Packet Socket | BPF | DLPI/SNOOP | DLPI | NDD | 不支持 |
| **获取MAC** | SIOCGIFHWADDR | sysctl | DL_PHYS_ADDR_REQ | DL_PHYS_ADDR_REQ | getkerninfo | 不支持 |
| **设置MAC** | SIOCSIFHWADDR | SIOCSIFLLADDR | DL_SET_PHYS_ADDR_REQ | DL_SET_PHYS_ADDR_REQ | 不支持 | 不支持 |
| **发送帧** | sendto | write | putmsg/write | putmsg/write | write | 不支持 |
| **接收帧** | recvfrom | read | getmsg | getmsg | read | 不支持 |

### 9.2 数据结构对比

| 平台 | 主要结构 | 说明 |
|------|---------|------|
| **Linux** | `sockaddr_ll` | 链路层socket地址 |
| **macOS/BSD** | `sockaddr_dl` | 链路层socket地址 |
| **Solaris** | `DL_primitives` | DLPI原语联合体 |
| **AIX** | `sockaddr_ndd_8022` | NDD socket地址 |

### 9.3 操作方式对比

#### Linux
```c
// Packet Socket方式
int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
struct sockaddr_ll sll;
sendto(fd, buf, len, 0, (struct sockaddr *)&sll, sizeof(sll));
```

#### macOS/BSD
```c
// BPF方式
int fd = open("/dev/bpf0", O_RDWR);
ioctl(fd, BIOCSETIF, &ifr);
write(fd, buf, len);
```

#### Solaris (DLPI)
```c
// DLPI方式
int fd = open("/dev/hme", O_RDWR);
putmsg(fd, &ctl, &data, 0);
```

### 9.4 性能对比

| 操作 | Linux | macOS/BSD | Solaris | AIX |
|------|-------|-----------|---------|-----|
| **打开设备** | 快 | 中 | 慢（STREAMS） | 慢 |
| **发送帧** | 快 | 快 | 中（putmsg） | 中 |
| **获取MAC** | 快（ioctl） | 中（sysctl） | 慢（DLPI） | 慢（getkerninfo） |
| **设置MAC** | 快（ioctl） | 快（ioctl） | 慢（DLPI） | 不支持 |

### 9.5 权限要求对比

| 操作 | Linux | macOS/BSD | Solaris | HP-UX | AIX |
|------|-------|-----------|---------|-------|-----|
| **打开设备** | root/CAP_NET_RAW | root | root | root | root |
| **获取MAC** | 普通用户 | 普通用户 | 普通用户 | 普通用户 | 普通用户 |
| **设置MAC** | root | root | root | root | 不支持 |
| **发送帧** | root | root | root | root | root |

### 9.6 代码复杂度对比

| 文件 | 行数 | 平台 | 复杂度 |
|------|------|------|--------|
| `eth-linux.c` | 119 | Linux | 低 |
| `eth-bsd.c` | 173 | macOS/BSD | 中 |
| `eth-dlpi.c` | 305 | Solaris/HP-UX | 高 |
| `eth-snoop.c` | 110 | Solaris | 中 |
| `eth-pfilt.c` | 88 | Tru64 | 低 |
| `eth-ndd.c` | 118 | AIX | 中 |
| `eth-win32.c` | 51 | Windows | 低（不支持） |

### 9.7 维护性分析

| 方面 | Linux | macOS/BSD | Solaris | AIX |
|------|-------|-----------|---------|-----|
| **API稳定性** | 高 | 高 | 中 | 低 |
| **平台差异** | 低 | 低 | 高（DLPI风格） | 高 |
| **代码复用** | 高 | 高 | 中 | 低 |
| **测试难度** | 低 | 中 | 高 | 高 |

### 9.8 特殊功能对比

| 功能 | Linux | macOS/BSD | Solaris | AIX |
|------|-------|-----------|---------|-----|
| **VLAN支持** | 原生 | 原生 | 原生 | 有限 |
| **混杂模式** | 支持 | 支持 | 支持 | 支持 |
| **自定义源MAC** | 支持 | 支持（BIOCSHDRCMPLT） | 支持 | 有限 |
| **驱动信息** | 支持（ethtool） | 不支持 | 不支持 | 不支持 |

---

## 10. 实际应用示例

### 10.1 基本用法：获取MAC地址

```c
#include <stdio.h>
#include <dnet.h>

int main(void)
{
    eth_t *eth;
    eth_addr_t ea;
    char buf[18];

    if ((eth = eth_open("eth0")) == NULL) {
        perror("eth_open");
        return (1);
    }

    if (eth_get(eth, &ea) < 0) {
        perror("eth_get");
        return (1);
    }

    printf("MAC地址: %s\n", eth_ntop(&ea, buf, sizeof(buf)));

    eth_close(eth);
    return (0);
}
```

### 10.2 设置MAC地址（需要root权限）

```c
#include <stdio.h>
#include <dnet.h>

int main(void)
{
    eth_t *eth;
    eth_addr_t new_ea = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

    if ((eth = eth_open("eth0")) == NULL) {
        perror("eth_open");
        return (1);
    }

    if (eth_set(eth, &new_ea) < 0) {
        perror("eth_set");
        return (1);
    }

    printf("MAC地址设置成功\n");

    eth_close(eth);
    return (0);
}
```

### 10.3 发送ARP请求

```c
#include <stdio.h>
#include <dnet.h>
#include <stdint.h>

struct arp_hdr {
    uint16_t arp_hrd;  // 硬件类型
    uint16_t arp_pro;  // 协议类型
    uint8_t  arp_hln;  // 硬件地址长度
    uint8_t  arp_pln;  // 协议地址长度
    uint16_t arp_op;   // 操作码
    eth_addr_t arp_sha; // 发送方硬件地址
    uint32_t  arp_spa; // 发送方协议地址
    eth_addr_t arp_tha; // 目标硬件地址
    uint32_t  arp_tpa; // 目标协议地址
};

int main(void)
{
    eth_t *eth;
    eth_addr_t dst_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    eth_addr_t src_mac;
    u_char frame[512];
    struct eth_hdr *eth_hdr;
    struct arp_hdr *arp_hdr;

    if ((eth = eth_open("eth0")) == NULL) {
        perror("eth_open");
        return (1);
    }

    // 获取本地MAC地址
    if (eth_get(eth, &src_mac) < 0) {
        perror("eth_get");
        return (1);
    }

    // 构造以太网头
    eth_hdr = (struct eth_hdr *)frame;
    eth_pack_hdr(eth_hdr, dst_mac, src_mac, ETH_TYPE_ARP);

    // 构造ARP头
    arp_hdr = (struct arp_hdr *)(frame + ETH_HDR_LEN);
    arp_hdr->arp_hrd = htons(1);  // 以太网
    arp_hdr->arp_pro = htons(0x0800);  // IPv4
    arp_hdr->arp_hln = 6;
    arp_hdr->arp_pln = 4;
    arp_hdr->arp_op = htons(1);  // ARP请求
    memcpy(&arp_hdr->arp_sha, &src_mac, ETH_ADDR_LEN);
    arp_hdr->arp_spa = inet_addr("192.168.1.100");
    memset(&arp_hdr->arp_tha, 0, ETH_ADDR_LEN);
    arp_hdr->arp_tpa = inet_addr("192.168.1.1");

    // 发送ARP请求
    if (eth_send(eth, frame, ETH_HDR_LEN + sizeof(struct arp_hdr)) < 0) {
        perror("eth_send");
        return (1);
    }

    printf("ARP请求已发送\n");

    eth_close(eth);
    return (0);
}
```

### 10.4 MAC地址转换

```c
#include <stdio.h>
#include <dnet.h>

int main(void)
{
    eth_addr_t ea;
    char buf[18];

    // 字符串转MAC
    if (eth_pton("00:11:22:33:44:55", &ea) < 0) {
        perror("eth_pton");
        return (1);
    }

    // MAC转字符串
    printf("MAC地址: %s\n", eth_ntop(&ea, buf, sizeof(buf)));

    // MAC转字符串（简写）
    printf("MAC地址: %s\n", eth_ntoa(&ea));

    return (0);
}
```

### 10.5 扫描本地网络MAC地址

```c
#include <stdio.h>
#include <dnet.h>

int scan_intf(const struct intf_entry *entry, void *arg)
{
    eth_t *eth;
    eth_addr_t ea;
    char buf[18];

    // 只处理以太网接口
    if (entry->intf_type != INTF_TYPE_ETH)
        return (0);

    // 只处理up状态的接口
    if (!(entry->intf_flags & INTF_FLAG_UP))
        return (0);

    printf("接口: %s\n", entry->intf_name);

    if ((eth = eth_open(entry->intf_name)) == NULL) {
        printf("  无法打开: %s\n", strerror(errno));
        return (0);
    }

    if (eth_get(eth, &ea) < 0) {
        printf("  无法获取MAC: %s\n", strerror(errno));
    } else {
        printf("  MAC: %s\n", eth_ntop(&ea, buf, sizeof(buf)));
    }

    eth_close(eth);
    return (0);
}

int main(void)
{
    intf_t *intf;

    if ((intf = intf_open()) == NULL) {
        perror("intf_open");
        return (1);
    }

    intf_loop(intf, scan_intf, NULL);

    intf_close(intf);
    return (0);
}
```

### 10.6 检测MAC地址重复

```c
#include <stdio.h>
#include <dnet.h>
#include <string.h>

#define MAX_INTERFACES 32

struct mac_entry {
    char name[16];
    eth_addr_t mac;
};

int main(void)
{
    intf_t *intf;
    eth_t *eth;
    struct intf_entry entry;
    struct mac_entry macs[MAX_INTERFACES];
    int count = 0, i, j;
    char buf[18], ebuf[1024];

    if ((intf = intf_open()) == NULL) {
        perror("intf_open");
        return (1);
    }

    memset(&entry, 0, sizeof(entry));
    entry.intf_len = sizeof(ebuf);

    // 遍历所有接口
    while (count < MAX_INTERFACES) {
        if (intf_loop(intf, NULL, NULL) < 0)
            break;

        // 简化示例：实际需要完整遍历
        break;
    }

    // 检查重复MAC
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (memcmp(&macs[i].mac, &macs[j].mac, ETH_ADDR_LEN) == 0) {
                printf("警告: MAC地址重复！\n");
                printf("  %s: %s\n", macs[i].name, 
                       eth_ntop(&macs[i].mac, buf, sizeof(buf)));
                printf("  %s: %s\n", macs[j].name,
                       eth_ntop(&macs[j].mac, buf, sizeof(buf)));
            }
        }
    }

    intf_close(intf);
    return (0);
}
```

### 10.7 使用dnet命令行工具

```bash
# 构造以太网帧
$ echo "hello" | dnet eth type 0x0800 src 00:11:22:33:44:55 dst ff:ff:ff:ff:ff:ff > frame.bin

# 发送以太网帧（需要root）
$ sudo dnet send eth0 < frame.bin

# 显示ARP缓存
$ dnet arp show

# 添加ARP条目
$ sudo dnet arp add 192.168.1.1 00:11:22:33:44:55
```

---

## 11. 常见问题与解决方案

### 11.1 权限问题

**问题**：
```
eth_open: Permission denied
```

**原因**：打开原始socket需要root权限

**解决方案**：
```bash
# Linux/macOS/BSD
sudo ./your_program

# 添加CAP_NET_RAW能力（仅Linux）
sudo setcap cap_net_raw+ep ./your_program
```

### 11.2 设备不存在

**问题**：
```
eth_open: No such device
```

**原因**：接口名称错误

**解决方案**：
```c
// 首先列出所有接口
intf_loop(intf, print_intf, NULL);

// 确认接口名称后再使用
strlcpy(device, "eth0", sizeof(device));
```

### 11.3 BPF设备全部忙碌

**问题**：
```
eth_open: Device busy
```

**原因**：所有BPF设备都被占用

**解决方案**：
```c
// 检查BPF设备占用
$ ls -l /dev/bpf*
$ lsof /dev/bpf*

// 释放未使用的BPF设备
// 或增加BPF设备数量（需要配置内核）
```

### 11.4 macOS/BSD无法发送自定义源MAC

**问题**：发送的帧源MAC总是被覆盖

**原因**：驱动程序自动完成以太网头

**解决方案**：
```c
// 设置BIOCSHDRCMPLT标志
int i = 1;
ioctl(fd, BIOCSHDRCMPLT, &i);
```

### 11.5 DLPI附加失败

**问题**：
```
eth_open: DLPI attach failed
```

**原因**：PPA值错误或设备不支持

**解决方案**：
```c
// 检查PPA值
$ ls -l /dev/net/
$ netstat -in

// 尝试不同的DLPI风格
```

### 11.6 AIX getkerninfo失败

**问题**：
```
eth_get: No such file or directory
```

**原因**：NDD信息不可用

**解决方案**：
```bash
# 检查NDD配置
$ lsdev -C | grep -i ether
$ netstat -ia

# 可能需要加载NDD驱动
```

### 11.7 MAC地址格式错误

**问题**：
```
eth_pton: Invalid argument
```

**原因**：MAC地址格式不正确

**解决方案**：
```c
// 支持的格式
"00:11:22:33:44:55"  // 推荐
"00-11-22-33-44-55"
"0011:2233:4455"

// 不支持的格式
"001122334455"
"00:11:22:33:44:55:66"  // 长度错误
```

### 11.8 发送帧长度错误

**问题**：
```
eth_send: Message too long
```

**原因**：帧长度超过MTU

**解决方案**：
```c
// 检查帧长度
if (len > ETH_MTU) {
    fprintf(stderr, "帧长度超过MTU: %zu > %d\n", len, ETH_MTU);
    return (-1);
}

// 或分片发送
```

### 11.9 性能优化：缓冲区管理

**问题**：频繁打开/关闭设备导致性能下降

**解决方案**：
```c
// 缓存设备句柄
static eth_t *eth_cache[MAX_INTERFACES];
static char dev_cache[MAX_INTERFACES][16];

eth_t *eth_get_cached(const char *device)
{
    int i;

    // 查找缓存
    for (i = 0; i < MAX_INTERFACES; i++) {
        if (dev_cache[i][0] != '\0' &&
            strcmp(dev_cache[i], device) == 0) {
            return (eth_cache[i]);
        }
    }

    // 未找到，打开新设备
    for (i = 0; i < MAX_INTERFACES; i++) {
        if (dev_cache[i][0] == '\0') {
            eth_cache[i] = eth_open(device);
            if (eth_cache[i] != NULL) {
                strlcpy(dev_cache[i], device, sizeof(dev_cache[i]));
            }
            return (eth_cache[i]);
        }
    }

    return (NULL);
}
```

### 11.10 调试技巧

#### 启用详细日志

```c
#define DEBUG_ETH 1

#ifdef DEBUG_ETH
#define DEBUG(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

// 在关键位置添加日志
DEBUG("打开设备: %s", device);
DEBUG("获取MAC: %s", eth_ntop(&ea, buf, sizeof(buf)));
DEBUG("发送帧: %zu 字节", len);
```

#### 使用系统工具验证

```bash
# Linux
$ ip link show
$ ip link set eth0 address 00:11:22:33:44:55

# macOS/BSD
$ ifconfig en0
$ ifconfig en0 ether 00:11:22:33:44:55

# Solaris
$ ifconfig -a
$ ifconfig hme0 ether 00:11:22:33:44:55

# AIX
$ netstat -ia
$ chdev -l en0 -a netaddr=0x001122334455
```

#### 抓包验证

```bash
# 使用tcpdump抓包
$ sudo tcpdump -i eth0 -XX

# 使用Wireshark分析
$ sudo tshark -i eth0
```

---

## 总结

本文档深入分析了libdnet MAC地址处理模块的跨平台实现，涵盖了以下核心内容：

### 主要特点

1. **七大平台支持**：
   - **Linux**：Packet Socket
   - **macOS/BSD**：BPF
   - **Solaris**：DLPI/SNOOP
   - **HP-UX**：DLPI
   - **AIX**：NDD
   - **Tru64**：Packet Filter
   - **Windows**：不支持（需要第三方库）

2. **核心功能**：
   - MAC地址获取/设置
   - 以太网帧发送
   - MAC地址格式转换
   - 支持VLAN标签

3. **跨平台适配**：
   - 条件编译处理平台差异
   - 统一的API接口
   - 灵活的数据结构设计

### 实现亮点

- **Linux**：简单高效的Packet Socket
- **macOS/BSD**：成熟的BPF机制，sysctl获取MAC
- **Solaris**：复杂的DLPI STREAMS接口
- **AIX**：独特的getkerninfo接口

### 使用建议

1. **优先使用读取操作**：避免频繁修改MAC地址
2. **注意权限要求**：大多数操作需要root权限
3. **处理平台差异**：不同平台支持的功能不同
4. **错误处理**：正确处理平台特定的错误码

通过本文档，开发者可以全面理解libdnet MAC地址处理模块的设计思想和实现细节，并在不同平台上正确使用该库。
