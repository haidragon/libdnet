# ARP操作源码深入分析

## 目录

1. [ARP协议概述](#1-arp协议概述)
2. [libdnet ARP接口设计](#2-libdnet-arp接口设计)
3. [Linux平台实现](#3-linux平台实现)
4. [macOS/BSD平台实现](#4-macosbsd平台实现)
5. [Windows平台实现](#5-windows平台实现)
6. [跨平台对比分析](#6-跨平台对比分析)
7. [实际应用示例](#7-实际应用示例)
8. [常见问题与解决方案](#8-常见问题与解决方案)

---

## 1. ARP协议概述

### 1.1 ARP协议简介

ARP（Address Resolution Protocol）是网络层和数据链路层之间的核心协议，用于将IP地址解析为MAC地址。RFC 826定义了ARP协议标准。

### 1.2 ARP消息格式

```c
/* 文件: include/dnet/arp.h */
#define ARP_HDR_LEN    8   /* 基础ARP头部长度 */
#define ARP_ETHIP_LEN  20  /* Ethernet/IP ARP消息长度 */

/* ARP头部结构 */
struct arp_hdr {
    uint16_t ar_hrd;    /* 硬件地址格式 */
    uint16_t ar_pro;    /* 协议地址格式 */
    uint8_t  ar_hln;    /* 硬件地址长度 (ETH_ADDR_LEN) */
    uint8_t  ar_pln;    /* 协议地址长度 (IP_ADDR_LEN) */
    uint16_t ar_op;     /* 操作类型 */
} __attribute__((__packed__));

/* Ethernet/IP ARP消息 */
struct arp_ethip {
    eth_addr_t ar_sha;  /* 发送方硬件地址 */
    ip_addr_t  ar_spa;  /* 发送方协议地址 */
    eth_addr_t ar_tha;  /* 目标硬件地址 */
    ip_addr_t  ar_tpa;  /* 目标协议地址 */
} __attribute__((__packed__));
```

**字段说明：**
- `ar_hrd`: 硬件类型，Ethernet为0x0001
- `ar_pro`: 协议类型，IPv4为0x0800
- `ar_hln`: 硬件地址长度，Ethernet为6字节
- `ar_pln`: 协议地址长度，IPv4为4字节
- `ar_op`: 操作类型
  - `ARP_OP_REQUEST` (1): ARP请求
  - `ARP_OP_REPLY` (2): ARP应答
  - `ARP_OP_REVREQUEST` (3): 反向ARP请求
  - `ARP_OP_REVREPLY` (4): 反向ARP应答

### 1.3 ARP缓存条目

```c
/* ARP缓存条目 */
struct arp_entry {
    struct addr arp_pa;  /* 协议地址(IP地址) */
    struct addr arp_ha;  /* 硬件地址(MAC地址) */
};
```

---

## 2. libdnet ARP接口设计

### 2.1 核心API接口

```c
/* 文件: include/dnet/arp.h */
typedef struct arp_handle arp_t;
typedef int (*arp_handler)(const struct arp_entry * entry, void *arg);

/* ARP句柄操作 */
arp_t *arp_open(void);                                   // 打开ARP句柄
arp_t *arp_close(arp_t *arp);                           // 关闭ARP句柄

/* ARP缓存操作 */
int arp_add(arp_t *arp, const struct arp_entry *entry);  // 添加ARP条目
int arp_delete(arp_t *arp, const struct arp_entry *entry); // 删除ARP条目
int arp_get(arp_t *arp, struct arp_entry *entry);        // 获取ARP条目
int arp_loop(arp_t *arp, arp_handler callback, void *arg); // 遍历ARP缓存
```

### 2.2 ARP消息打包宏

```c
/* 打包Ethernet/IP ARP消息的便捷宏 */
#define arp_pack_hdr_ethip(hdr, op, sha, spa, tha, tpa) do { \
    struct arp_hdr *pack_arp_p = (struct arp_hdr *)(hdr);    \
    struct arp_ethip *pack_ethip_p = (struct arp_ethip *)(pack_arp_p + 1); \
    pack_arp_p->ar_hrd = htons(ARP_HRD_ETH);        \
    pack_arp_p->ar_pro = htons(ARP_PRO_IP);         \
    pack_arp_p->ar_hln = ETH_ADDR_LEN;              \
    pack_arp_p->ar_pln = IP_ADDR_LEN;               \
    pack_arp_p->ar_op = htons(op);                  \
    memcpy(&pack_ethip_p->ar_sha, &(sha), ETH_ADDR_LEN); \
    memcpy(&pack_ethip_p->ar_spa, &(spa), IP_ADDR_LEN); \
    memcpy(&pack_ethip_p->ar_tha, &(tha), ETH_ADDR_LEN); \
    memcpy(&pack_ethip_p->ar_tpa, &(tpa), IP_ADDR_LEN); \
} while (0)
```

### 2.3 平台适配机制

libdnet通过编译时检测自动选择对应的平台实现：

**configure.ac中的选择逻辑：**
```c
/* 文件: configure.ac 第219-228行 */
dnl Check for arp interface.
if test "$ac_cv_header_iphlpapi_h" = yes ; then
    AC_LIBOBJ([arp-win32])        /* Windows平台 */
elif test "$ac_cv_dnet_ioctl_arp" = yes ; then
    AC_LIBOBJ([arp-ioctl])       /* Linux/Unix ioctl方式 */
elif test "$ac_cv_dnet_route_h_has_rt_msghdr" = yes ; then
    AC_LIBOBJ([arp-bsd])         /* BSD/macOS route socket方式 */
else
    AC_LIBOBJ([arp-none])        /* 不支持的平台 */
fi
```

---

## 3. Linux平台实现

### 3.1 实现架构

Linux平台使用`ioctl`系统调用操作ARP缓存，核心文件为`src/arp-ioctl.c`。

**平台检测：**
```c
/* 文件: m4/acinclude.m4 第201-221行 */
AC_DEFUN([AC_DNET_IOCTL_ARP],
    [AC_MSG_CHECKING(for arp(7) ioctls)
    AC_CACHE_VAL(ac_cv_dnet_ioctl_arp,
    AC_EGREP_CPP(werd, [
        #include <sys/types.h>
        #define BSD_COMP
        #include <sys/ioctl.h>
        #ifdef SIOCGARP    /* 检查ARP相关ioctl定义 */
        werd
        #endif],
        ac_cv_dnet_ioctl_arp=yes,
        ac_cv_dnet_ioctl_arp=no))
    case "$host_os" in
    irix*)
        ac_cv_dnet_ioctl_arp=no ;;  /* IRIX不支持 */
    esac])
```

### 3.2 句柄管理

**ARP句柄结构：**
```c
/* 文件: src/arp-ioctl.c 第47-52行 */
struct arp_handle {
    int   fd;          /* socket文件描述符 */
#ifdef HAVE_ARPREQ_ARP_DEV
    intf_t *intf;      /* 网络接口句柄(需要指定设备时) */
#endif
};
```

**打开ARP句柄：**
```c
/* 文件: src/arp-ioctl.c 第54-74行 */
arp_t *
arp_open(void)
{
    arp_t *a;

    if ((a = calloc(1, sizeof(*a))) != NULL) {
#ifdef HAVE_STREAMS_MIB2
        /* Solaris STREAMS方式 */
        if ((a->fd = open(IP_DEV_NAME, O_RDWR)) < 0)
#elif defined(HAVE_STREAMS_ROUTE)
        /* Solaris/IRIX STREAMS路由设备 */
        if ((a->fd = open("/dev/route", O_WRONLY, 0)) < 0)
#else
        /* 标准Linux: 创建AF_INET socket用于ioctl */
        if ((a->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
#endif
            return (arp_close(a));

#ifdef HAVE_ARPREQ_ARP_DEV
        /* 需要指定网络设备的平台 */
        if ((a->intf = intf_open()) == NULL)
            return (arp_close(a));
#endif
    }
    return (a);
}
```

**关键技术点：**
1. 使用`AF_INET` + `SOCK_DGRAM`创建socket，而不是原始socket
2. 该socket仅用于`ioctl`操作，不进行实际网络通信
3. 某些Linux发行版需要指定网络接口名称

### 3.3 添加ARP条目

**实现源码：**
```c
/* 文件: src/arp-ioctl.c 第100-163行 */
int
arp_add(arp_t *a, const struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    /* 设置协议地址(IP) */
    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

    /* 设置硬件地址(MAC) - 平台差异处理 */
#ifdef __linux__
    /* Linux特定: 设置sa_family为ARP_HRD_ETH */
    if (addr_ntos(&entry->arp_ha, &ar.arp_ha) < 0)
        return (-1);
    ar.arp_ha.sa_family = ARP_HRD_ETH;
#else
    /* Solaris, HP-UX, IRIX等其他Unix系统 */
    ar.arp_ha.sa_family = AF_UNSPEC;
    memcpy(ar.arp_ha.sa_data, &entry->arp_ha.addr_eth, ETH_ADDR_LEN);
#endif

    /* 指定网络接口 */
#ifdef HAVE_ARPREQ_ARP_DEV
    if (intf_loop(a->intf, _arp_set_dev, &ar) != 1) {
        errno = ESRCH;
        return (-1);
    }
#endif

    /* 设置ARP标志: 永久且完成 */
    ar.arp_flags = ATF_PERM | ATF_COM;

#ifdef hpux
    /* HP-UX特殊处理: 扩展arpreq结构 */
    {
        struct sockaddr_in *sin;
        ar.arp_hw_addr_len = ETH_ADDR_LEN;
        sin = (struct sockaddr_in *)&ar.arp_pa_mask;
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = IP_ADDR_BROADCAST;
    }
#endif

    /* 执行ioctl添加ARP条目 */
    if (ioctl(a->fd, SIOCSARP, &ar) < 0)
        return (-1);

#ifdef HAVE_STREAMS_MIB2
    /* Solaris MIB2特殊处理: 强制进入ipNetToMediaTable */
    {
        struct sockaddr_in sin;
        int fd;

        addr_ntos(&entry->arp_pa, (struct sockaddr *)&sin);
        sin.sin_port = htons(666);  /* 魔术端口触发ARP查询 */

        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            return (-1);

        if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            close(fd);
            return (-1);
        }
        write(fd, NULL, 0);  /* 触发ARP解析 */
        close(fd);
    }
#endif
    return (0);
}
```

**网络设备查找辅助函数：**
```c
/* 文件: src/arp-ioctl.c 第76-98行 */
static int
_arp_set_dev(const struct intf_entry *entry, void *arg)
{
    struct arpreq *ar = (struct arpreq *)arg;
    struct addr dst;
    uint32_t mask;

    /* 只处理Ethernet接口 */
    if (entry->intf_type == INTF_TYPE_ETH &&
        entry->intf_addr.addr_type == ADDR_TYPE_IP) {

        /* 计算子网掩码 */
        addr_btom(entry->intf_addr.addr_bits, &mask, IP_ADDR_LEN);
        addr_ston((struct sockaddr *)&ar->arp_pa, &dst);

        /* 检查目标IP是否在该接口的子网内 */
        if ((entry->intf_addr.addr_ip & mask) ==
            (dst.addr_ip & mask)) {
            /* 设置设备名称 */
            strlcpy(ar->arp_dev, entry->intf_name,
                sizeof(ar->arp_dev));
            return (1);  /* 找到匹配的接口 */
        }
    }
    return (0);
}
```

**ioctl命令详解：**
- `SIOCSARP` (Set ARP): 添加或修改ARP条目
- `SIOCDARP` (Delete ARP): 删除ARP条目
- `SIOCGARP` (Get ARP): 获取ARP条目

**ARP标志位：**
```c
/* Linux kernel定义的ARP标志 */
#define ATF_COM     0x02   /* 地址完成(已解析) */
#define ATF_PERM    0x04   /* 永久条目 */
#define ATF_PUBL    0x08   /* 发布条目(代理ARP) */
#define ATF_USETRAILERS 0x10   /* 使用trailers */
```

### 3.4 删除ARP条目

**实现源码：**
```c
/* 文件: src/arp-ioctl.c 第165-179行 */
int
arp_delete(arp_t *a, const struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    /* 只需要指定IP地址 */
    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

    /* 执行ioctl删除 */
    if (ioctl(a->fd, SIOCDARP, &ar) < 0)
        return (-1);

    return (0);
}
```

### 3.5 获取ARP条目

**实现源码：**
```c
/* 文件: src/arp-ioctl.c 第181-205行 */
int
arp_get(arp_t *a, struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    /* 设置要查询的IP地址 */
    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

    /* 指定网络接口 */
#ifdef HAVE_ARPREQ_ARP_DEV
    if (intf_loop(a->intf, _arp_set_dev, &ar) != 1) {
        errno = ESRCH;
        return (-1);
    }
#endif

    /* 执行ioctl查询 */
    if (ioctl(a->fd, SIOCGARP, &ar) < 0)
        return (-1);

    /* 检查条目是否完成 */
    if ((ar.arp_flags & ATF_COM) == 0) {
        errno = ESRCH;
        return (-1);
    }

    /* 提取硬件地址 */
    return (addr_ston(&ar.arp_ha, &entry->arp_ha));
}
```

### 3.6 遍历ARP缓存

Linux提供三种遍历ARP缓存的方式：

#### 方式1: /proc/net/arp 文件解析（推荐）

```c
/* 文件: src/arp-ioctl.c 第207-240行 */
#ifdef HAVE_LINUX_PROCFS
int
arp_loop(arp_t *a, arp_handler callback, void *arg)
{
    FILE *fp;
    struct arp_entry entry;
    char buf[BUFSIZ], ipbuf[100], macbuf[100], maskbuf[100], devbuf[100];
    int i, type, flags, ret;

    /* 打开/proc/net/arp文件 */
    if ((fp = fopen(PROC_ARP_FILE, "r")) == NULL)
        return (-1);

    ret = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        /* 解析ARP条目行
         * 格式: IP地址  HW类型  标志  MAC地址  掩码  设备
         * 示例: 192.168.1.1  0x1  0x2  00:11:22:33:44:55  *  eth0
         */
        i = sscanf(buf, "%s 0x%x 0x%x %99s %99s %99s\n",
            ipbuf, &type, &flags, macbuf, maskbuf, devbuf);

        if (i < 4 || (flags & ATF_COM) == 0)
            continue;  /* 跳过未完成的条目 */

        /* 转换地址并调用回调 */
        if (addr_aton(ipbuf, &entry.arp_pa) == 0 &&
            addr_aton(macbuf, &entry.arp_ha) == 0) {
            if ((ret = callback(&entry, arg)) != 0)
                break;
        }
    }
    if (ferror(fp)) {
        fclose(fp);
        return (-1);
    }
    fclose(fp);

    return (ret);
}
#endif
```

**/proc/net/arp文件格式示例：**
```
IP address       HW type  Flags  HW address            Mask     Device
192.168.1.1      0x1      0x2    00:11:22:33:44:55     *        eth0
192.168.1.254    0x1      0x6    aa:bb:cc:dd:ee:ff     *        eth0
```

**Flags字段说明：**
- `0x0`: 未完成
- `0x2`: 完成
- `0x4`: 永久
- `0x6`: 永久且完成
- `0x8`: 代理ARP

#### 方式2: STREAMS MIB2（Solaris/Irix）

```c
/* 文件: src/arp-ioctl.c 第241-337行 */
#elif defined (HAVE_STREAMS_MIB2)
int
arp_loop(arp_t *r, arp_handler callback, void *arg)
{
    struct arp_entry entry;
    struct strbuf msg;
    struct T_optmgmt_req *tor;
    struct T_optmgmt_ack *toa;
    struct T_error_ack *tea;
    struct opthdr *opt;
    mib2_ipNetToMediaEntry_t *arp, *arpend;
    u_char buf[8192];
    int flags, rc, atable, ret;

    tor = (struct T_optmgmt_req *)buf;
    toa = (struct T_optmgmt_ack *)buf;
    tea = (struct T_error_ack *)buf;

    /* 构建MIB2查询请求 */
    tor->PRIM_type = T_OPTMGMT_REQ;
    tor->OPT_offset = sizeof(*tor);
    tor->OPT_length = sizeof(*opt);
    tor->MGMT_flags = T_CURRENT;

    opt = (struct opthdr *)(tor + 1);
    opt->level = MIB2_IP;
    opt->name = opt->len = 0;

    msg.maxlen = sizeof(buf);
    msg.len = sizeof(*tor) + sizeof(*opt);
    msg.buf = buf;

    /* 发送查询请求 */
    if (putmsg(r->fd, &msg, NULL, 0) < 0)
        return (-1);

    opt = (struct opthdr *)(toa + 1);
    msg.maxlen = sizeof(buf);

    /* 循环读取响应 */
    for (;;) {
        flags = 0;
        if ((rc = getmsg(r->fd, &msg, NULL, &flags)) < 0)
            return (-1);

        /* 检查是否完成 */
        if (rc == 0 &&
            msg.len >= sizeof(*toa) &&
            toa->PRIM_type == T_OPTMGMT_ACK &&
            toa->MGMT_flags == T_SUCCESS && opt->len == 0)
            break;

        if (msg.len >= sizeof(*tea) && tea->PRIM_type == T_ERROR_ACK)
            return (-1);

        if (rc != MOREDATA || msg.len < (int)sizeof(*toa) ||
            toa->PRIM_type != T_OPTMGMT_ACK ||
            toa->MGMT_flags != T_SUCCESS)
            return (-1);

        /* 检查是否是ARP表 */
        atable = (opt->level == MIB2_IP && opt->name == MIB2_IP_22);

        msg.maxlen = sizeof(buf) - (sizeof(buf) % sizeof(*arp));
        msg.len = 0;
        flags = 0;

        do {
            rc = getmsg(r->fd, NULL, &msg, &flags);

            if (rc != 0 && rc != MOREDATA)
                return (-1);

            if (!atable)
                continue;

            /* 解析ARP条目 */
            arp = (mib2_ipNetToMediaEntry_t *)msg.buf;
            arpend = (mib2_ipNetToMediaEntry_t *)
                (msg.buf + msg.len);

            entry.arp_pa.addr_type = ADDR_TYPE_IP;
            entry.arp_pa.addr_bits = IP_ADDR_BITS;

            entry.arp_ha.addr_type = ADDR_TYPE_ETH;
            entry.arp_ha.addr_bits = ETH_ADDR_BITS;

            for ( ; arp < arpend; arp++) {
                entry.arp_pa.addr_ip =
                    arp->ipNetToMediaNetAddress;

                memcpy(&entry.arp_ha.addr_eth,
                    arp->ipNetToMediaPhysAddress.o_bytes,
                    ETH_ADDR_LEN);

                if ((ret = callback(&entry, arg)) != 0)
                    return (ret);
            }
        } while (rc == MOREDATA);
    }
    return (0);
}
#endif
```

#### 方式3: 内核内存读取（HP-UX/Tru64）

```c
/* 文件: src/arp-ioctl.c 第338-385行 */
#elif defined(HAVE_SYS_MIB_H)
#define MAX_ARPENTRIES  512  /* 最大条目数 */

int
arp_loop(arp_t *r, arp_handler callback, void *arg)
{
    struct nmparms nm;
    struct arp_entry entry;
    mib_ipNetToMediaEnt arpentries[MAX_ARPENTRIES];
    int fd, i, n, ret;

    /* 打开MIB设备 */
    if ((fd = open_mib("/dev/ip", O_RDWR, 0, 0)) < 0)
        return (-1);

    /* 查询ARP表 */
    nm.objid = ID_ipNetToMediaTable;
    nm.buffer = arpentries;
    n = sizeof(arpentries);
    nm.len = &n;

    if (get_mib_info(fd, &nm) < 0) {
        close_mib(fd);
        return (-1);
    }
    close_mib(fd);

    entry.arp_pa.addr_type = ADDR_TYPE_IP;
    entry.arp_pa.addr_bits = IP_ADDR_BITS;

    entry.arp_ha.addr_type = ADDR_TYPE_ETH;
    entry.arp_ha.addr_bits = ETH_ADDR_BITS;

    n /= sizeof(*arpentries);
    ret = 0;

    for (i = 0; i < n; i++) {
        /* 跳过无效条目 */
        if (arpentries[i].Type == INTM_INVALID ||
            arpentries[i].PhysAddr.o_length != ETH_ADDR_LEN)
            continue;

        entry.arp_pa.addr_ip = arpentries[i].NetAddr;
        memcpy(&entry.arp_ha.addr_eth,
            arpentries[i].PhysAddr.o_bytes, ETH_ADDR_LEN);

        if ((ret = callback(&entry, arg)) != 0)
            break;
    }
    return (ret);
}
#endif
```

### 3.7 Linux平台特定问题

**权限要求：**
- 添加/删除ARP条目需要`CAP_NET_ADMIN`能力（root或sudo）
- 读取ARP缓存普通用户即可

**内核版本差异：**
```c
/* 某些旧内核不支持arpreq中的arp_dev字段 */
#ifndef HAVE_ARPREQ_ARP_DEV
    /* 需要手动设置网络设备 */
#endif
```

**IPv6支持：**
- libdnet的ARP模块仅支持IPv4
- IPv6使用NDP（Neighbor Discovery Protocol），不在ARP模块处理

---

## 4. macOS/BSD平台实现

### 4.1 实现架构

macOS和BSD系列系统使用路由套接字（Route Socket）管理ARP缓存，核心文件为`src/arp-bsd.c`。

**平台选择逻辑：**
```c
/* configure.ac 第224-225行 */
elif test "$ac_cv_dnet_route_h_has_rt_msghdr" = yes ; then
    AC_LIBOBJ([arp-bsd])
```

### 4.2 路由套接字机制

BSD系统通过`PF_ROUTE`套接字接收和发送路由消息，包括ARP表项变更。

**句柄结构：**
```c
/* 文件: src/arp-bsd.c 第38-46行 */
struct arp_handle {
    int   fd;    /* 路由套接字文件描述符 */
    int   seq;   /* 序列号，用于匹配请求和响应 */
};

/* ARP消息结构 */
struct arpmsg {
    struct rt_msghdr rtm;  /* 路由消息头 */
    u_char          addrs[256];  /* 地址数据 */
};
```

**打开句柄：**
```c
/* 文件: src/arp-bsd.c 第48-62行 */
arp_t *
arp_open(void)
{
    arp_t *arp;

    if ((arp = calloc(1, sizeof(*arp))) != NULL) {
#ifdef HAVE_STREAMS_ROUTE
        /* STREAMS路由设备（Solaris/Irix） */
        if ((arp->fd = open("/dev/route", O_RDWR, 0)) < 0)
#else
        /* 标准BSD/macOS: 创建路由套接字 */
        if ((arp->fd = socket(PF_ROUTE, SOCK_RAW, 0)) < 0)
#endif
            return (arp_close(arp));
    }
    return (arp);
}
```

### 4.3 路由消息通信

**核心消息处理函数：**
```c
/* 文件: src/arp-bsd.c 第64-107行 */
static int
arp_msg(arp_t *arp, struct arpmsg *msg)
{
    struct arpmsg smsg;
    int len, i = 0;
    pid_t pid;

    /* 设置消息版本和序列号 */
    msg->rtm.rtm_version = RTM_VERSION;
    msg->rtm.rtm_seq = ++arp->seq;
    memcpy(&smsg, msg, sizeof(smsg));

#ifdef HAVE_STREAMS_ROUTE
    /* STREAMS方式: 使用ioctl */
    return (ioctl(arp->fd, RTSTR_SEND, &msg->rtm));
#else
    /* 标准BSD/macOS: 使用write/read */

    /* 发送路由消息 */
    if (write(arp->fd, &smsg, smsg.rtm.rtm_msglen) < 0) {
        /* 处理特殊情况: 删除不存在的条目 */
        if (errno != ESRCH || msg->rtm.rtm_type != RTM_DELETE)
            return (-1);
    }

    pid = getpid();

    /* 读取响应消息 */
    while ((len = read(arp->fd, msg, sizeof(*msg))) > 0) {
        if (len < (int)sizeof(msg->rtm))
            return (-1);

        /* 检查是否是我们的消息 */
        if (msg->rtm.rtm_pid == pid) {
            if (msg->rtm.rtm_seq == arp->seq)
                break;  /* 找到匹配的响应 */
            continue;
        } else if ((i++ % 2) == 0)
            continue;

        /* 重复请求 */
        if (write(arp->fd, &smsg, smsg.rtm.rtm_msglen) < 0) {
            if (errno != ESRCH || msg->rtm.rtm_type != RTM_DELETE)
                return (-1);
        }
    }
    if (len < 0)
        return (-1);

    return (0);
#endif
}
```

**路由消息类型：**
```c
/* FreeBSD/macOS内核定义 */
#define RTM_ADD        0x1   /* 添加路由 */
#define RTM_DELETE     0x2   /* 删除路由 */
#define RTM_CHANGE     0x3   /* 修改路由 */
#define RTM_GET        0x4   /* 获取路由 */
```

**地址标志位：**
```c
#define RTA_DST        0x1   /* 目标地址 */
#define RTA_GATEWAY    0x2   /* 网关地址 */
#define RTA_NETMASK    0x4   /* 网络掩码 */
```

**路由标志位：**
```c
#define RTF_LLINFO     0x400   /* 链路层信息(ARP) */
#define RTF_STATIC     0x800   /* 手动添加 */
#define RTF_HOST       0x4     /* 主机路由 */
#define RTF_GATEWAY    0x2     /* 网关路由 */
```

### 4.4 添加ARP条目

**实现源码：**
```c
/* 文件: src/arp-bsd.c 第109-173行 */
int
arp_add(arp_t *arp, const struct arp_entry *entry)
{
    struct arpmsg msg;
    struct sockaddr_in *sin;
    struct sockaddr *sa;
    int index, type;

    /* 验证地址类型 */
    if (entry->arp_pa.addr_type != ADDR_TYPE_IP ||
        entry->arp_ha.addr_type != ADDR_TYPE_ETH) {
        errno = EAFNOSUPPORT;
        return (-1);
    }

    sin = (struct sockaddr_in *)msg.addrs;
    sa = (struct sockaddr *)(sin + 1);

    /* 首先查询现有路由 */
    if (addr_ntos(&entry->arp_pa, (struct sockaddr *)sin) < 0)
        return (-1);

    memset(&msg.rtm, 0, sizeof(msg.rtm));
    msg.rtm.rtm_type = RTM_GET;
    msg.rtm.rtm_addrs = RTA_DST;
    msg.rtm.rtm_msglen = sizeof(msg.rtm) + sizeof(*sin);

    if (arp_msg(arp, &msg) < 0)
        return (-1);

    /* 验证响应 */
    if (msg.rtm.rtm_msglen < (int)sizeof(msg.rtm) +
        sizeof(*sin) + sizeof(*sa)) {
        errno = EADDRNOTAVAIL;
        return (-1);
    }

    /* 检查是否是ARP条目而非网关 */
    if (sin->sin_addr.s_addr == entry->arp_pa.addr_ip) {
        if ((msg.rtm.rtm_flags & RTF_LLINFO) == 0 ||
            (msg.rtm.rtm_flags & RTF_GATEWAY) != 0) {
            errno = EADDRINUSE;
            return (-1);
        }
    }

    /* 保存接口信息 */
    if (sa->sa_family != AF_LINK) {
        errno = EADDRNOTAVAIL;
        return (-1);
    } else {
        index = ((struct sockaddr_dl *)sa)->sdl_index;  /* 接口索引 */
        type = ((struct sockaddr_dl *)sa)->sdl_type;    /* 接口类型 */
    }

    /* 构建添加消息 */
    if (addr_ntos(&entry->arp_pa, (struct sockaddr *)sin) < 0 ||
        addr_ntos(&entry->arp_ha, sa) < 0)
        return (-1);

    /* 恢复接口信息 */
    ((struct sockaddr_dl *)sa)->sdl_index = index;
    ((struct sockaddr_dl *)sa)->sdl_type = type;

    /* 设置消息参数 */
    memset(&msg.rtm, 0, sizeof(msg.rtm));
    msg.rtm.rtm_type = RTM_ADD;
    msg.rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;
    msg.rtm.rtm_inits = RTV_EXPIRE;  /* 设置过期时间 */
    msg.rtm.rtm_flags = RTF_HOST | RTF_STATIC;  /* 主机路由、静态 */
#ifdef HAVE_SOCKADDR_SA_LEN
    msg.rtm.rtm_msglen = sizeof(msg.rtm) + sin->sin_len + sa->sa_len;
#else
    msg.rtm.rtm_msglen = sizeof(msg.rtm) + sizeof(*sin) + sizeof(*sa);
#endif

    return (arp_msg(arp, &msg));
}
```

**关键点解析：**

1. **两步操作：**
   - 先用`RTM_GET`查询接口信息
   - 再用`RTM_ADD`添加条目

2. **地址结构：**
   - `RTA_DST`: 目标IP地址
   - `RTA_GATEWAY`: 实际存储MAC地址（通过sockaddr_dl）

3. **接口索引：**
   - BSD系统使用接口索引而非名称
   - 通过`sdl_index`字段指定接口

4. **过期时间：**
   - `RTV_EXPIRE`标志表示设置过期时间
   - `RTF_STATIC`标志创建静态条目

### 4.5 删除ARP条目

**实现源码：**
```c
/* 文件: src/arp-bsd.c 第175-219行 */
int
arp_delete(arp_t *arp, const struct arp_entry *entry)
{
    struct arpmsg msg;
    struct sockaddr_in *sin;
    struct sockaddr *sa;

    /* 验证地址类型 */
    if (entry->arp_pa.addr_type != ADDR_TYPE_IP) {
        errno = EAFNOSUPPORT;
        return (-1);
    }

    sin = (struct sockaddr_in *)msg.addrs;
    sa = (struct sockaddr *)(sin + 1);

    /* 查询现有路由 */
    if (addr_ntos(&entry->arp_pa, (struct sockaddr *)sin) < 0)
        return (-1);

    memset(&msg.rtm, 0, sizeof(msg.rtm));
    msg.rtm.rtm_type = RTM_GET;
    msg.rtm.rtm_addrs = RTA_DST;
    msg.rtm.rtm_msglen = sizeof(msg.rtm) + sizeof(*sin);

    if (arp_msg(arp, &msg) < 0)
        return (-1);

    /* 验证响应 */
    if (msg.rtm.rtm_msglen < (int)sizeof(msg.rtm) +
        sizeof(*sin) + sizeof(*sa)) {
        errno = ESRCH;
        return (-1);
    }

    /* 检查是否是ARP条目 */
    if (sin->sin_addr.s_addr == entry->arp_pa.addr_ip) {
        if ((msg.rtm.rtm_flags & RTF_LLINFO) == 0 ||
            (msg.rtm.rtm_flags & RTF_GATEWAY) != 0) {
            errno = EADDRINUSE;
            return (-1);
        }
    }

    /* 验证地址族 */
    if (sa->sa_family != AF_LINK) {
        errno = ESRCH;
        return (-1);
    }

    /* 发送删除请求 */
    msg.rtm.rtm_type = RTM_DELETE;

    return (arp_msg(arp, &msg));
}
```

### 4.6 获取ARP条目

**实现源码：**
```c
/* 文件: src/arp-bsd.c 第221-258行 */
int
arp_get(arp_t *arp, struct arp_entry *entry)
{
    struct arpmsg msg;
    struct sockaddr_in *sin;
    struct sockaddr *sa;

    /* 验证地址类型 */
    if (entry->arp_pa.addr_type != ADDR_TYPE_IP) {
        errno = EAFNOSUPPORT;
        return (-1);
    }

    sin = (struct sockaddr_in *)msg.addrs;
    sa = (struct sockaddr *)(sin + 1);

    /* 设置查询参数 */
    if (addr_ntos(&entry->arp_pa, (struct sockaddr *)sin) < 0)
        return (-1);

    memset(&msg.rtm, 0, sizeof(msg.rtm));
    msg.rtm.rtm_type = RTM_GET;
    msg.rtm.rtm_addrs = RTA_DST;
    msg.rtm.rtm_flags = RTF_LLINFO;  /* 只查询ARP条目 */
    msg.rtm.rtm_msglen = sizeof(msg.rtm) + sizeof(*sin);

    /* 发送查询 */
    if (arp_msg(arp, &msg) < 0)
        return (-1);

    /* 验证响应 */
    if (msg.rtm.rtm_msglen < (int)sizeof(msg.rtm) +
        sizeof(*sin) + sizeof(*sa) ||
        sin->sin_addr.s_addr != entry->arp_pa.addr_ip ||
        sa->sa_family != AF_LINK) {
        errno = ESRCH;
        return (-1);
    }

    /* 提取MAC地址 */
    if (addr_ston(sa, &entry->arp_ha) < 0)
        return (-1);

    return (0);
}
```

### 4.7 遍历ARP缓存

**sysctl方式遍历（macOS/BSD推荐）：**
```c
/* 文件: src/arp-bsd.c 第260-304行 */
#ifdef HAVE_SYS_SYSCTL_H
int
arp_loop(arp_t *arp, arp_handler callback, void *arg)
{
    struct arp_entry entry;
    struct rt_msghdr *rtm;
    struct sockaddr_in *sin;
    struct sockaddr *sa;
    char *buf, *lim, *next;
    size_t len;
    int ret, mib[6] = { CTL_NET, PF_ROUTE, 0, AF_INET,
                NET_RT_FLAGS, RTF_LLINFO };

    /* 第一次调用: 获取所需缓冲区大小 */
    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return (-1);

    if (len == 0)
        return (0);

    /* 分配缓冲区 */
    if ((buf = malloc(len)) == NULL)
        return (-1);

    /* 第二次调用: 获取实际数据 */
    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        free(buf);
        return (-1);
    }

    lim = buf + len;
    ret = 0;

    /* 遍历所有路由消息 */
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        sin = (struct sockaddr_in *)(rtm + 1);
        sa = (struct sockaddr *)(sin + 1);

        /* 解析IP和MAC地址 */
        if (addr_ston((struct sockaddr *)sin, &entry.arp_pa) < 0 ||
            addr_ston(sa, &entry.arp_ha) < 0)
            continue;

        /* 调用回调函数 */
        if ((ret = callback(&entry, arg)) != 0)
            break;
    }
    free(buf);

    return (ret);
}
#endif
```

**sysctl MIB数组解析：**
```c
int mib[6] = {
    CTL_NET,        // 网络子系统
    PF_ROUTE,       // 路由子系统
    0,              // 协议族(0表示所有)
    AF_INET,        // 地址族(IPv4)
    NET_RT_FLAGS,   // 获取指定标志的路由
    RTF_LLINFO      // 链路层信息(ARP)
};
```

**sysctl的优势：**
1. 不需要打开socket
2. 原子操作，一次获取所有数据
3. 性能高，无多次I/O开销
4. 不需要root权限

### 4.8 macOS/BSD平台特定问题

**sockaddr_dl结构：**
```c
/* 链路层地址结构 */
struct sockaddr_dl {
    uint8_t  sdl_len;      /* 地址长度 */
    uint8_t  sdl_family;  /* AF_LINK */
    uint16_t sdl_index;    /* 接口索引 */
    uint8_t  sdl_type;     /* 接口类型 */
    uint8_t  sdl_nlen;     /* 接口名称长度 */
    uint8_t  sdl_alen;     /* 链路层地址长度 */
    uint8_t  sdl_slen;     /* 选择器长度 */
    char     sdl_data[12]; /* 接口名称和地址 */
};
```

**MAC地址提取：**
```c
/* 从sockaddr_dl中提取MAC地址 */
if (sa->sa_family == AF_LINK) {
    struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;
    uint8_t *mac = LLADDR(sdl);  /* 指向MAC地址的宏 */
}
```

**权限要求：**
- 添加/删除ARP条目需要root权限
- 遍历ARP缓存普通用户即可

**FreeBSD vs macOS差异：**
1. FreeBSD支持sockaddr_len字段
2. macOS需要特殊处理某些标志位
3. 不同版本的sysctl实现略有差异

---

## 5. Windows平台实现

### 5.1 实现架构

Windows平台使用IP Helper API（iphlpapi.dll）管理ARP缓存，核心文件为`src/arp-win32.c`。

**句柄结构：**
```c
/* 文件: src/arp-win32.c 第20-22行 */
struct arp_handle {
    MIB_IPNETTABLE *iptable;  /* ARP表缓存指针 */
};
```

### 5.2 打开句柄

**实现源码：**
```c
/* 文件: src/arp-win32.c 第24-28行 */
arp_t *
arp_open(void)
{
    /* 简单分配内存，不打开任何系统资源 */
    return (calloc(1, sizeof(arp_t)));
}
```

**特点：**
- Windows实现不需要持久句柄
- 所有操作都直接调用IP Helper API
- 延迟加载ARP表

### 5.3 添加ARP条目

**实现源码：**
```c
/* 文件: src/arp-win32.c 第30-50行 */
int
arp_add(arp_t *arp, const struct arp_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;  /* 路由条目 */
    MIB_IPNETROW iprow;       /* ARP条目 */

    /* 查找最佳路由以确定接口索引 */
    if (GetBestRoute(entry->arp_pa.addr_ip,
        IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    /* 填充ARP条目 */
    iprow.dwIndex = ipfrow.dwForwardIfIndex;  /* 接口索引 */
    iprow.dwPhysAddrLen = ETH_ADDR_LEN;       /* MAC地址长度 */
    memcpy(iprow.bPhysAddr, &entry->arp_ha.addr_eth, ETH_ADDR_LEN);
    iprow.dwAddr = entry->arp_pa.addr_ip;      /* IP地址 */
    iprow.dwType = 4;  /* XXX - static 静态条目 */

    /* 创建ARP条目 */
    if (CreateIpNetEntry(&iprow) != NO_ERROR)
        return (-1);

    return (0);
}
```

**IP Helper API详解：**

1. **GetBestRoute:**
   - 查找到目标IP的最佳路由
   - 返回路由信息，包括接口索引
   - 原型: `DWORD GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute);`

2. **CreateIpNetEntry:**
   - 创建ARP表条目
   - 原型: `DWORD CreateIpNetEntry(PMIB_IPNETROW pArpEntry);`

3. **MIB_IPNETROW结构：**
```c
typedef struct _MIB_IPNETROW {
    DWORD    dwIndex;        /* 接口索引 */
    DWORD    dwPhysAddrLen;  /* 物理地址长度 */
    BYTE     bPhysAddr[MAXLEN_PHYSADDR];  /* 物理地址 */
    DWORD    dwAddr;         /* IP地址 */
    DWORD    dwType;         /* 类型: 1=其他, 2=无效, 3=动态, 4=静态 */
} MIB_IPNETROW, *PMIB_IPNETROW;
```

**ARP条目类型：**
- `1`: 其他
- `2`: 无效
- `3`: 动态（ARP协议学习）
- `4`: 静态（手动添加）

### 5.4 删除ARP条目

**实现源码：**
```c
/* 文件: src/arp-win32.c 第52-71行 */
int
arp_delete(arp_t *arp, const struct arp_entry *entry)
{
    MIB_IPFORWARDROW ipfrow;
    MIB_IPNETROW iprow;

    /* 查找最佳路由以确定接口索引 */
    if (GetBestRoute(entry->arp_pa.addr_ip,
        IP_ADDR_ANY, &ipfrow) != NO_ERROR)
        return (-1);

    /* 填充删除参数 */
    memset(&iprow, 0, sizeof(iprow));
    iprow.dwIndex = ipfrow.dwForwardIfIndex;
    iprow.dwAddr = entry->arp_pa.addr_ip;

    /* 删除ARP条目 */
    if (DeleteIpNetEntry(&iprow) != NO_ERROR) {
        errno = ENXIO;
        return (-1);
    }
    return (0);
}
```

**DeleteIpNetEntry API:**
- 原型: `DWORD DeleteIpNetEntry(PMIB_IPNETROW pArpEntry);`
- 只需要指定接口索引和IP地址
- 不需要MAC地址

### 5.5 获取ARP条目

**实现源码：**
```c
/* 文件: src/arp-win32.c 第73-94行 */
static int
_arp_get_entry(const struct arp_entry *entry, void *arg)
{
    struct arp_entry *e = (struct arp_entry *)arg;

    /* 比较IP地址 */
    if (addr_cmp(&entry->arp_pa, &e->arp_pa) == 0) {
        /* 找到匹配项，复制MAC地址 */
        memcpy(&e->arp_ha, &entry->arp_ha, sizeof(e->arp_ha));
        return (1);  /* 停止遍历 */
    }
    return (0);
}

int
arp_get(arp_t *arp, struct arp_entry *entry)
{
    /* 遍历ARP表查找匹配项 */
    if (arp_loop(arp, _arp_get_entry, entry) != 1) {
        errno = ENXIO;
        SetLastError(ERROR_NO_DATA);
        return (-1);
    }
    return (0);
}
```

**实现策略：**
1. 使用`arp_loop`遍历整个ARP表
2. 使用回调函数查找匹配的IP地址
3. 返回第一条匹配的MAC地址

### 5.6 遍历ARP缓存

**实现源码：**
```c
/* 文件: src/arp-win32.c 第96-132行 */
int
arp_loop(arp_t *arp, arp_handler callback, void *arg)
{
    struct arp_entry entry;
    ULONG len;
    int i, ret;

    /* 循环获取ARP表，直到缓冲区足够大 */
    for (len = sizeof(arp->iptable[0]); ; ) {
        if (arp->iptable)
            free(arp->iptable);

        arp->iptable = malloc(len);
        if (arp->iptable == NULL)
            return (-1);

        /* 获取ARP表 */
        ret = GetIpNetTable(arp->iptable, &len, FALSE);

        if (ret == NO_ERROR)
            break;  /* 成功 */
        else if (ret != ERROR_INSUFFICIENT_BUFFER)
            return (-1);  /* 其他错误 */
        /* 缓冲区不足，继续循环 */
    }

    entry.arp_pa.addr_type = ADDR_TYPE_IP;
    entry.arp_pa.addr_bits = IP_ADDR_BITS;

    entry.arp_ha.addr_type = ADDR_TYPE_ETH;
    entry.arp_ha.addr_bits = ETH_ADDR_BITS;

    /* 遍历所有ARP条目 */
    for (i = 0; i < (int)arp->iptable->dwNumEntries; i++) {
        /* 跳过MAC地址长度不正确的条目 */
        if (arp->iptable->table[i].dwPhysAddrLen != ETH_ADDR_LEN)
            continue;

        entry.arp_pa.addr_ip = arp->iptable->table[i].dwAddr;
        memcpy(&entry.arp_ha.addr_eth,
            arp->iptable->table[i].bPhysAddr, ETH_ADDR_LEN);

        /* 调用回调函数 */
        if ((ret = (*callback)(&entry, arg)) != 0)
            return (ret);
    }
    return (0);
}
```

**GetIpNetTable API详解：**

```c
/* 原型 */
DWORD GetIpNetTable(
    PMIB_IPNETTABLE pIpNetTable,  // 接收ARP表
    PULONG pdwSize,               // 缓冲区大小(输入/输出)
    BOOL bOrder                   // 是否排序
);

/* MIB_IPNETTABLE结构 */
typedef struct _MIB_IPNETTABLE {
    DWORD       dwNumEntries;           // 条目数量
    MIB_IPNETROW table[1];              // 条目数组(变长)
} MIB_IPNETTABLE, *PMIB_IPNETTABLE;
```

**缓冲区处理策略：**
1. 从小缓冲区开始
2. 调用`GetIpNetTable`
3. 如果返回`ERROR_INSUFFICIENT_BUFFER`，增大缓冲区
4. 重复直到成功或错误

**ARP条目过滤：**
- 只处理`dwPhysAddrLen == 6`的条目
- 跳过无效或未完成的条目

### 5.7 关闭句柄

**实现源码：**
```c
/* 文件: src/arp-win32.c 第134-143行 */
arp_t *
arp_close(arp_t *arp)
{
    if (arp != NULL) {
        /* 释放缓存的ARP表 */
        if (arp->iptable != NULL)
            free(arp->iptable);
        free(arp);
    }
    return (NULL);
}
```

### 5.8 Windows平台特定问题

**DLL依赖：**
- 需要链接`iphlpapi.lib`
- 运行时需要`iphlpapi.dll`
- Windows 2000及以上版本支持

**权限要求：**
- 添加/删除ARP条目需要管理员权限
- 读取ARP缓存普通用户即可

**API版本：**
- 传统API: `GetIpNetTable`, `CreateIpNetEntry`, `DeleteIpNetEntry`
- 新版API (Vista+): `GetIpNetTable2`, `CreateIpNetEntry2`
- libdnet使用传统API以保证兼容性

**IPv6支持：**
- 传统IP Helper API仅支持IPv4
- IPv6 ARP（NDP）需要使用`GetIpNetTable2`等新API
- libdnet ARP模块不支持IPv6

**错误处理：**
```c
/* Windows API返回值处理 */
if (GetIpNetTable(...) != NO_ERROR) {
    /* 设置errno和last error */
    errno = ENXIO;
    SetLastError(GetLastError());
    return (-1);
}
```

---

## 6. 跨平台对比分析

### 6.1 API设计对比

| 功能 | Linux | macOS/BSD | Windows | 无实现 |
|------|-------|-----------|---------|--------|
| 打开句柄 | socket(AF_INET, SOCK_DGRAM) | socket(PF_ROUTE, SOCK_RAW) | calloc | 返回NULL |
| 添加ARP | ioctl(SIOCSARP) | RTM_ADD | CreateIpNetEntry | ENOSYS |
| 删除ARP | ioctl(SIOCDARP) | RTM_DELETE | DeleteIpNetEntry | ENOSYS |
| 获取ARP | ioctl(SIOCGARP) | RTM_GET | arp_loop遍历 | ENOSYS |
| 遍历ARP | /proc/net/arp | sysctl | GetIpNetTable | ENOSYS |
| 关闭句柄 | close(fd) | close(fd) | free | free |

### 6.2 数据结构对比

**Linux: arpreq**
```c
struct arpreq {
    struct sockaddr arp_pa;      /* 协议地址(IP) */
    struct sockaddr arp_ha;      /* 硬件地址(MAC) */
    int arp_flags;              /* 标志 */
    struct sockaddr arp_netmask; /* 网络掩码 */
    char arp_dev[16];           /* 设备名称 */
};
```

**macOS/BSD: rt_msghdr + sockaddr**
```c
struct rt_msghdr {
    u_short rtm_msglen;         /* 消息长度 */
    u_char  rtm_version;        /* 版本 */
    u_char  rtm_type;           /* 消息类型 */
    u_short rtm_index;          /* 接口索引 */
    int     rtm_flags;          /* 标志 */
    int     rtm_addrs;          /* 地址掩码 */
    /* ... */
    char    rtm_data[256];      /* 地址数据 */
};
```

**Windows: MIB_IPNETROW**
```c
typedef struct _MIB_IPNETROW {
    DWORD dwIndex;              /* 接口索引 */
    DWORD dwPhysAddrLen;        /* MAC地址长度 */
    BYTE  bPhysAddr[MAXLEN_PHYSADDR];
    DWORD dwAddr;               /* IP地址 */
    DWORD dwType;               /* 类型 */
} MIB_IPNETROW;
```

### 6.3 操作方式对比

**Linux: ioctl方式**
```c
/* 优点 */
- 直接、简单
- 一次性操作
- 内核直接处理

/* 缺点 */
- 需要socket句柄
- 设备名称处理复杂
- 不同发行版差异大
```

**macOS/BSD: 路由消息方式**
```c
/* 优点 */
- 统一的路由管理接口
- 支持链路层地址
- 消息机制灵活
- sysctl遍历性能高

/* 缺点 */
- 实现复杂
- 需要处理序列号
- 两步操作(先GET再ADD)
- 消息格式复杂
```

**Windows: IP Helper API**
```c
/* 优点 */
- 高层API，易用
- 不需要句柄
- 自动处理接口索引
- 缓冲区自动扩展

/* 缺点 */
- 依赖Windows DLL
- 仅支持IPv4
- 遍历效率低(全表扫描)
- 需要管理员权限
```

### 6.4 性能对比

| 操作 | Linux | macOS/BSD | Windows |
|------|-------|-----------|---------|
| 单条获取 | 快(ioctl) | 中(消息交换) | 慢(全表扫描) |
| 单条添加 | 快(ioctl) | 中(消息交换) | 快(API) |
| 全表遍历 | 快(proc文件) | 最快(sysctl) | 中(API) |
| 批量操作 | 需多次ioctl | 需多次消息 | 需多次API |

**详细分析：**

**Linux遍历性能：**
```c
/* /proc/net/arp解析 */
- 文件I/O: 快
- 文本解析: 中等
- 系统调用: 少(fopen/fgets/fclose)
- 适用场景: 小到中等ARP表
```

**macOS/BSD遍历性能：**
```c
/* sysctl方式 */
- 系统调用: 1次
- 内存拷贝: 1次
- 数据格式: 二进制，无需解析
- 适用场景: 所有规模，最佳性能
```

**Windows遍历性能：**
```c
/* GetIpNetTable API */
- 系统调用: 1-2次(缓冲区调整)
- 数据格式: 二进制
- 缓冲区管理: 复杂
- 适用场景: 中小ARP表
```

### 6.5 权限要求对比

| 操作 | Linux | macOS/BSD | Windows |
|------|-------|-----------|---------|
| 打开句柄 | 普通用户 | 普通用户 | 普通用户 |
| 读取ARP | 普通用户 | 普通用户 | 普通用户 |
| 添加ARP | root/CAP_NET_ADMIN | root | 管理员 |
| 删除ARP | root/CAP_NET_ADMIN | root | 管理员 |
| 遍历ARP | 普通用户 | 普通用户 | 普通用户 |

### 6.6 错误处理对比

**Linux: errno**
```c
/* 常见错误 */
ESRCH    - 条目不存在
EEXIST   - 条目已存在
EPERM    - 权限不足
EINVAL   - 参数错误
ENODEV   - 设备不存在
```

**macOS/BSD: errno**
```c
/* 常见错误 */
ESRCH          - 条目不存在
EADDRNOTAVAIL  - 地址不可用
EADDRINUSE     - 地址已使用
EAFNOSUPPORT   - 地址族不支持
EPERM          - 权限不足
```

**Windows: GetLastError + errno**
```c
/* 常见错误 */
ERROR_NO_DATA      - 数据不存在
ERROR_ACCESS_DENIED - 访问被拒绝
ERROR_INVALID_PARAMETER - 参数无效
ERROR_NOT_SUPPORTED   - 不支持的操作

/* libdnet映射 */
errno = ENXIO  -> ERROR_NO_DATA
errno = EPERM  -> ERROR_ACCESS_DENIED
```

### 6.7 平台特性对比

| 特性 | Linux | macOS/BSD | Windows |
|------|-------|-----------|---------|
| IPv4支持 | ✅ | ✅ | ✅ |
| IPv6支持 | ❌ | ❌ | ❌(需新API) |
| 静态ARP | ✅ | ✅ | ✅ |
| 动态ARP | ✅ | ✅ | ✅ |
| 代理ARP | ✅ | ✅ | ✅ |
| ARP超时 | ✅ | ✅ | ✅ |
| 接口指定 | ✅(设备名) | ✅(索引) | ✅(索引) |
| 批量操作 | ❌ | ❌ | ❌ |
| 原子操作 | ✅ | ✅ | ✅ |

### 6.8 代码复杂度对比

| 文件 | 行数 | 复杂度 | 主要难点 |
|------|------|--------|----------|
| arp-ioctl.c | 490 | 高 | 多平台变体、proc文件解析 |
| arp-bsd.c | 324 | 高 | 路由消息、序列号匹配 |
| arp-win32.c | 144 | 低 | API简单、缓冲区管理 |

### 6.9 维护性分析

**Linux (arp-ioctl.c):**
```c
/* 维护难点 */
- 3种遍历方式(proc/streams/mib)
- 多种Unix变体(Solaris/HP-UX/IRIX)
- 设备名称处理复杂
- 需要持续跟进内核变化

/* 优点 */
- ioctl机制稳定
- /proc接口文档完善
```

**macOS/BSD (arp-bsd.c):**
```c
/* 维护难点 */
- 路由消息格式变化
- 不同BSD版本差异
- sockaddr_dl结构变化
- sysctl MIB参数变化

/* 优点 */
- 代码统一
- 逻辑清晰
```

**Windows (arp-win32.c):**
```c
/* 维护难点 */
- API版本更新频繁
- 新版API不向后兼容
- Windows版本差异

/* 优点 */
- 代码简单
- 微软文档完善
```

---

## 7. 实际应用示例

### 7.1 基本用法

**添加静态ARP条目：**
```c
#include <stdio.h>
#include <dnet.h>

int main(void)
{
    arp_t *arp;
    struct arp_entry entry;

    /* 打开ARP句柄 */
    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        return 1;
    }

    /* 设置ARP条目 */
    addr_pton("192.168.1.100", &entry.arp_pa);  /* IP地址 */
    addr_pton("00:11:22:33:44:55", &entry.arp_ha); /* MAC地址 */

    /* 添加到ARP表 */
    if (arp_add(arp, &entry) < 0) {
        perror("arp_add");
        arp_close(arp);
        return 1;
    }

    printf("ARP entry added successfully\n");

    /* 关闭句柄 */
    arp_close(arp);
    return 0;
}
```

**删除ARP条目：**
```c
int main(void)
{
    arp_t *arp;
    struct arp_entry entry;

    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        return 1;
    }

    /* 只需要IP地址 */
    addr_pton("192.168.1.100", &entry.arp_pa);

    /* 删除ARP条目 */
    if (arp_delete(arp, &entry) < 0) {
        perror("arp_delete");
        arp_close(arp);
        return 1;
    }

    printf("ARP entry deleted\n");
    arp_close(arp);
    return 0;
}
```

### 7.2 ARP扫描器

**局域网主机发现：**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dnet.h>

/* 回调函数: 打印每个ARP条目 */
static int print_arp_entry(const struct arp_entry *entry, void *arg)
{
    printf("IP: %-15s  MAC: %s\n",
           addr_ntoa(&entry->arp_pa),
           addr_ntoa(&entry->arp_ha));
    return 0;
}

int main(void)
{
    arp_t *arp;

    /* 打开ARP句柄 */
    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        return 1;
    }

    printf("ARP Cache:\n");
    printf("=========================================\n");

    /* 遍历并打印所有ARP条目 */
    if (arp_loop(arp, print_arp_entry, NULL) < 0) {
        perror("arp_loop");
        arp_close(arp);
        return 1;
    }

    printf("=========================================\n");

    /* 关闭句柄 */
    arp_close(arp);
    return 0;
}
```

### 7.3 ARP请求发送

**发送原始ARP请求：**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dnet.h>

int main(int argc, char *argv[])
{
    eth_t *eth;
    arp_t *arp;
    intf_t *intf;
    struct intf_entry if_entry;
    struct addr my_ip, my_mac, target_ip;
    u_char pkt[ETH_MTU];
    struct arp_hdr *arp_hdr;
    struct arp_ethip *arp_ethip;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <target_ip>\n", argv[0]);
        return 1;
    }

    /* 解析目标IP */
    if (addr_pton(argv[1], &target_ip) < 0) {
        fprintf(stderr, "Invalid IP address\n");
        return 1;
    }

    /* 获取默认网络接口信息 */
    if ((intf = intf_open()) == NULL) {
        perror("intf_open");
        return 1;
    }

    if_entry.intf_len = sizeof(if_entry);
    if (intf_get(intf, &if_entry) < 0) {
        perror("intf_get");
        intf_close(intf);
        return 1;
    }
    intf_close(intf);

    my_ip = if_entry.intf_addr;      /* 本机IP */
    my_mac = if_entry.intf_link_addr; /* 本机MAC */

    /* 打开Ethernet设备 */
    if ((eth = eth_open(if_entry.intf_name)) == NULL) {
        perror("eth_open");
        return 1;
    }

    /* 打开ARP句柄(用于封装) */
    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        eth_close(eth);
        return 1;
    }

    /* 构造ARP请求包 */
    memset(pkt, 0, sizeof(pkt));
    arp_pack_hdr_ethip(pkt,
        ARP_OP_REQUEST,           /* 请求 */
        my_mac.addr_eth,           /* 发送方MAC */
        my_ip.addr_ip,             /* 发送方IP */
        ETH_ADDR_BROADCAST,        /* 目标MAC(广播) */
        target_ip.addr_ip);        /* 目标IP */

    /* 发送ARP请求 */
    if (eth_send(eth, pkt, ARP_HDR_LEN + ARP_ETHIP_LEN) < 0) {
        perror("eth_send");
    } else {
        printf("ARP request sent to %s\n", argv[1]);
    }

    /* 清理资源 */
    arp_close(arp);
    eth_close(eth);
    return 0;
}
```

### 7.4 ARP监控工具

**实时监控ARP变化：**
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dnet.h>

/* ARP条目缓存 */
#define MAX_ENTRIES 1024
static struct arp_entry cache[MAX_ENTRIES];
static int cache_count = 0;

/* 查找条目 */
static int find_entry(const struct addr *pa)
{
    for (int i = 0; i < cache_count; i++) {
        if (addr_cmp(pa, &cache[i].arp_pa) == 0)
            return i;
    }
    return -1;
}

/* 添加条目到缓存 */
static void add_entry(const struct arp_entry *entry)
{
    if (cache_count < MAX_ENTRIES) {
        cache[cache_count] = *entry;
        cache_count++;
    }
}

/* 回调函数: 检测变化 */
static int detect_changes(const struct arp_entry *entry, void *arg)
{
    int idx = find_entry(&entry->arp_pa);
    time_t now = time(NULL);

    if (idx < 0) {
        /* 新条目 */
        printf("[%s] NEW: %s -> %s\n",
               ctime(&now),
               addr_ntoa(&entry->arp_pa),
               addr_ntoa(&entry->arp_ha));
        add_entry(entry);
    } else if (addr_cmp(&entry->arp_ha, &cache[idx].arp_ha) != 0) {
        /* MAC地址变化 */
        printf("[%s] CHANGE: %s: %s -> %s\n",
               ctime(&now),
               addr_ntoa(&entry->arp_pa),
               addr_ntoa(&cache[idx].arp_ha),
               addr_ntoa(&entry->arp_ha));
        cache[idx] = *entry;
    }

    return 0;
}

int main(void)
{
    arp_t *arp;

    /* 打开ARP句柄 */
    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        return 1;
    }

    printf("ARP Monitor (Ctrl+C to exit)\n");

    /* 定期检查 */
    while (1) {
        arp_loop(arp, detect_changes, NULL);
        sleep(2);  /* 2秒间隔 */
    }

    arp_close(arp);
    return 0;
}
```

### 7.5 使用dnet命令行工具

**查看ARP表：**
```bash
# 显示所有ARP条目
$ ./test/dnet/dnet arp show
192.168.1.1 at 00:11:22:33:44:55
192.168.1.254 at aa:bb:cc:dd:ee:ff
```

**添加ARP条目：**
```bash
# 添加静态ARP条目
$ sudo ./test/dnet/dnet arp add 192.168.1.100 00:11:22:33:44:55
192.168.1.100 added
```

**删除ARP条目：**
```bash
# 删除ARP条目
$ sudo ./test/dnet/dnet arp delete 192.168.1.100
192.168.1.100 deleted
```

**查询特定IP的MAC：**
```bash
# 查询ARP条目
$ ./test/dnet/dnet arp get 192.168.1.1
192.168.1.1 at 00:11:22:33:44:55
```

### 7.6 ARP欺骗检测

**简单的ARP欺骗检测：**
```c
#include <stdio.h>
#include <stdlib.h>
#include <dnet.h>

/* 已知网关的MAC地址 */
static struct addr gateway_mac;

/* 初始化 */
static int init(const char *gateway_ip, const char *gateway_mac_str)
{
    if (addr_pton(gateway_ip, &gateway_mac) < 0) {
        fprintf(stderr, "Invalid gateway IP\n");
        return -1;
    }
    if (addr_pton(gateway_mac_str, &gateway_mac) < 0) {
        fprintf(stderr, "Invalid gateway MAC\n");
        return -1;
    }
    return 0;
}

/* 检测回调 */
static int detect_arp_spoof(const struct arp_entry *entry, void *arg)
{
    if (addr_cmp(&entry->arp_pa, &gateway_mac) == 0) {
        /* 找到网关IP */
        if (addr_cmp(&entry->arp_ha, &gateway_mac) != 0) {
            /* MAC地址不匹配，可能是ARP欺骗 */
            printf("[ALERT] ARP Spoofing detected!\n");
            printf("  Gateway IP: %s\n", addr_ntoa(&entry->arp_pa));
            printf("  Expected MAC: %s\n", addr_ntoa(&gateway_mac));
            printf("  Actual MAC: %s\n", addr_ntoa(&entry->arp_ha));
            return 1;  /* 停止扫描 */
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    arp_t *arp;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <gateway_ip> <gateway_mac>\n", argv[0]);
        return 1;
    }

    /* 初始化网关信息 */
    if (init(argv[1], argv[2]) < 0)
        return 1;

    /* 打开ARP句柄 */
    if ((arp = arp_open()) == NULL) {
        perror("arp_open");
        return 1;
    }

    printf("Checking for ARP spoofing...\n");

    /* 检测ARP欺骗 */
    if (arp_loop(arp, detect_arp_spoof, NULL) < 0) {
        perror("arp_loop");
    } else {
        printf("No ARP spoofing detected\n");
    }

    arp_close(arp);
    return 0;
}
```

---

## 8. 常见问题与解决方案

### 8.1 权限问题

**问题：添加/删除ARP条目失败，errno=EPERM**

**原因：**
- Linux: 需要root权限或CAP_NET_ADMIN能力
- macOS/BSD: 需要root权限
- Windows: 需要管理员权限

**解决方案：**

```bash
# Linux: 使用sudo
sudo ./your_program

# 设置CAP_NET_ADMIN能力
sudo setcap cap_net_admin+ep ./your_program

# Windows: 以管理员身份运行
# 右键 -> 以管理员身份运行
```

**代码检查：**
```c
if (arp_add(arp, &entry) < 0) {
    if (errno == EPERM) {
        fprintf(stderr, "Error: Need root/administrator privileges\n");
        fprintf(stderr, "Please run with sudo or as administrator\n");
    }
    perror("arp_add");
    return -1;
}
```

### 8.2 设备未找到

**问题：Linux平台添加ARP失败，errno=ENODEV**

**原因：**
- 无法确定使用哪个网络接口
- IP地址不在任何接口的子网内

**解决方案：**

```c
/* 手动指定接口名称(需要修改arpreq) */
struct arpreq ar;
memset(&ar, 0, sizeof(ar));
addr_ntos(&entry->arp_pa, &ar.arp_pa);
addr_ntos(&entry->arp_ha, &ar.arp_ha);
ar.arp_flags = ATF_PERM | ATF_COM;
strncpy(ar.arp_dev, "eth0", sizeof(ar.arp_dev));  /* 指定接口 */

if (ioctl(fd, SIOCSARP, &ar) < 0) {
    perror("ioctl(SIOCSARP)");
    return -1;
}
```

### 8.3 条目不存在

**问题：arp_get失败，errno=ESRCH**

**原因：**
- ARP条目不存在
- 条目未完成(ATF_COM标志未设置)

**解决方案：**

```c
/* 先尝试ping触发ARP解析 */
system("ping -c 1 192.168.1.100 > /dev/null 2>&1");

/* 等待ARP解析完成 */
sleep(1);

/* 再次尝试获取 */
if (arp_get(arp, &entry) < 0 && errno == ESRCH) {
    fprintf(stderr, "ARP entry not found or incomplete\n");
    return -1;
}
```

### 8.4 macOS接口索引问题

**问题：macOS平台添加ARP失败，errno=EADDRNOTAVAIL**

**原因：**
- 无法确定接口索引
- IP地址不在路由表中

**解决方案：**

```c
/* 手动指定接口 */
intf_t *intf;
struct intf_entry if_entry;

if ((intf = intf_open()) == NULL) {
    perror("intf_open");
    return -1;
}

/* 获取指定接口信息 */
strncpy(if_entry.intf_name, "en0", sizeof(if_entry.intf_name));
if_entry.intf_len = sizeof(if_entry);

if (intf_get(intf, &if_entry) < 0) {
    perror("intf_get");
    intf_close(intf);
    return -1;
}

intf_close(intf);

/* 使用接口索引... */
```

### 8.5 Windows API错误

**问题：Windows平台操作失败，GetLastError返回错误码**

**常见错误码：**

| 错误码 | 含义 | 解决方案 |
|--------|------|----------|
| ERROR_ACCESS_DENIED | 权限不足 | 以管理员身份运行 |
| ERROR_INVALID_PARAMETER | 参数无效 | 检查IP/MAC地址格式 |
| ERROR_NOT_SUPPORTED | 操作不支持 | 检查Windows版本 |
| ERROR_NO_DATA | 条目不存在 | 检查IP地址 |

**调试代码：**
```c
#include <stdio.h>
#include <windows.h>

void print_last_error(const char *operation)
{
    DWORD error = GetLastError();
    if (error != NO_ERROR) {
        LPSTR msg = NULL;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPSTR)&msg, 0, NULL);

        fprintf(stderr, "%s failed: %s (Error %lu)\n",
                operation, msg, error);
        LocalFree(msg);
    }
}

/* 使用 */
if (CreateIpNetEntry(&iprow) != NO_ERROR) {
    print_last_error("CreateIpNetEntry");
    return -1;
}
```

### 8.6 ARP条目不持久

**问题：重启后添加的ARP条目丢失**

**原因：**
- 操作系统默认不保存静态ARP条目

**解决方案：**

**Linux (/etc/ethers):**
```bash
# 编辑/etc/ethers文件
192.168.1.100 00:11:22:33:44:55

# 配置启动脚本
sudo bash -c 'echo "arp -f /etc/ethers" >> /etc/rc.local'
```

**Windows (arp -s):**
```batch
REM 添加持久ARP条目
arp -s 192.168.1.100 00-11-22-33-44-55

REM 或使用netsh
netsh interface ip add neighbors "Ethernet" "192.168.1.100" "00-11-22-33-44-55"
```

### 8.7 性能问题

**问题：arp_loop遍历慢**

**原因：**
- Windows: 每次都全表扫描
- Linux: /proc文件解析效率低

**解决方案：**

**使用缓存：**
```c
/* 缓存ARP表 */
struct arp_cache {
    struct arp_entry *entries;
    int count;
    time_t timestamp;
    time_t ttl;  /* 缓存生存时间 */
};

static struct arp_cache cache = {NULL, 0, 0, 60}; /* 60秒缓存 */

/* 带缓存的遍历 */
int arp_loop_cached(arp_t *arp, arp_handler callback, void *arg)
{
    time_t now = time(NULL);

    /* 检查缓存是否有效 */
    if (cache.entries != NULL && (now - cache.timestamp) < cache.ttl) {
        /* 使用缓存 */
        for (int i = 0; i < cache.count; i++) {
            if (callback(&cache.entries[i], arg) != 0)
                break;
        }
        return 0;
    }

    /* 重建缓存 */
    if (cache.entries != NULL)
        free(cache.entries);

    /* 第一次调用获取数量 */
    struct arp_entry tmp[1];
    int count = 0;
    arp_loop(arp, count_callback, &count);

    /* 分配缓存 */
    cache.entries = malloc(sizeof(struct arp_entry) * count);
    cache.count = 0;

    /* 填充缓存 */
    struct loop_data data = {cache.entries, &cache.count};
    arp_loop(arp, fill_cache_callback, &data);

    cache.timestamp = now;

    /* 使用新缓存 */
    for (int i = 0; i < cache.count; i++) {
        if (callback(&cache.entries[i], arg) != 0)
            break;
    }

    return 0;
}
```

### 8.8 线程安全问题

**问题：多线程同时操作ARP表导致竞争条件**

**解决方案：**

```c
#include <pthread.h>

static pthread_mutex_t arp_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 线程安全的arp_add */
int arp_add_threadsafe(arp_t *arp, const struct arp_entry *entry)
{
    int ret;

    pthread_mutex_lock(&arp_mutex);
    ret = arp_add(arp, entry);
    pthread_mutex_unlock(&arp_mutex);

    return ret;
}

/* 线程安全的arp_get */
int arp_get_threadsafe(arp_t *arp, struct arp_entry *entry)
{
    int ret;

    pthread_mutex_lock(&arp_mutex);
    ret = arp_get(arp, entry);
    pthread_mutex_unlock(&arp_mutex);

    return ret;
}
```

### 8.9 IPv6支持

**问题：libdnet ARP模块不支持IPv6**

**原因：**
- IPv6使用NDP（Neighbor Discovery Protocol）
- 不使用ARP协议

**解决方案：**

**Windows:**
```c
/* 使用新版IP Helper API */
#include <iphlpapi.h>

DWORD GetIpNetTable2(
    ADDRESS_FAMILY Family,      /* AF_INET6 for IPv6 */
    PMIB_IPNET_TABLE2 *Table    /* 返回IPv6邻居表 */
);
```

**Linux:**
```c
/* 读取/proc/net/ndp */
FILE *fp = fopen("/proc/net/ndp", "r");
// 解析IPv6邻居发现表
```

**macOS/BSD:**
```c
/* 使用sysctl获取IPv6邻居表 */
int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_INET6, NET_RT_FLAGS, RTF_LLINFO};
sysctl(mib, 6, ...);
```

### 8.10 调试技巧

**启用详细日志：**
```c
#define ARP_DEBUG

#ifdef ARP_DEBUG
#define arp_log(fmt, ...) \
    fprintf(stderr, "[ARP] " fmt "\n", ##__VA_ARGS__)
#else
#define arp_log(fmt, ...) do {} while(0)
#endif

/* 使用 */
arp_log("Adding entry: IP=%s, MAC=%s",
        addr_ntoa(&entry->arp_pa),
        addr_ntoa(&entry->arp_ha));
```

**查看系统ARP表：**
```bash
# Linux/macOS
arp -an

# Linux: 查看详细信息
ip neigh show

# Windows
arp -a

# 查看/proc/net/arp(Linux)
cat /proc/net/arp
```

**抓包分析：**
```bash
# 使用tcpdump抓ARP包
sudo tcpdump -i eth0 -nn arp

# 使用Wireshark
# 过滤器: arp
```

---

## 附录A: 相关系统调用和API参考

### Linux ioctl命令

| 命令 | 描述 | 参数 |
|------|------|------|
| SIOCGARP | 获取ARP条目 | struct arpreq * |
| SIOCSARP | 设置ARP条目 | struct arpreq * |
| SIOCDARP | 删除ARP条目 | struct arpreq * |

### BSD路由消息类型

| 类型 | 值 | 描述 |
|------|----|------|
| RTM_ADD | 0x1 | 添加路由 |
| RTM_DELETE | 0x2 | 删除路由 |
| RTM_CHANGE | 0x3 | 修改路由 |
| RTM_GET | 0x4 | 获取路由 |

### Windows IP Helper API

| API | 描述 |
|-----|------|
| GetIpNetTable | 获取ARP表 |
| CreateIpNetEntry | 创建ARP条目 |
| DeleteIpNetEntry | 删除ARP条目 |
| GetBestRoute | 获取最佳路由 |

---

## 附录B: 参考资料

### RFC文档
- RFC 826: An Ethernet Address Resolution Protocol
- RFC 903: A Reverse Address Resolution Protocol

### 系统文档
- Linux: `man 7 arp`
- FreeBSD: `man 4 arp`
- macOS: `man 4 route`

### 相关工具
- Linux: `arp`, `ip neigh`, `tcpdump`
- macOS: `arp`, `ndp`, `tcpdump`
- Windows: `arp`, `netsh`, `Wireshark`

---

**文档版本:** 1.0
**最后更新:** 2026
**作者:** libdnet源码分析
**适用版本:** libdnet 1.13
