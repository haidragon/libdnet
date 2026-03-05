# Linux内核分支深度分析

## 目录
1. [Linux平台架构概述](#linux平台架构概述)
2. [内核网络机制](#内核网络机制)
3. [以太网模块实现](#以太网模块实现)
4. [IP模块实现](#ip模块实现)
5. [路由模块实现](#路由模块实现)
6. [ARP模块实现](#arp模块实现)
7. [接口管理模块实现](#接口管理模块实现)
8. [防火墙模块实现](#防火墙模块实现)
9. [TUN/TAP模块实现](#tuntap模块实现)
10. [各平台实现对比](#各平台实现对比)
11. [性能优化与最佳实践](#性能优化与最佳实践)
12. [附录：完整代码示例](#附录完整代码示例)

---

## Linux平台架构概述

### 1.1 平台特定文件清单

| 模块 | Linux实现文件 | 替代实现 |
|------|---------------|----------|
| 以太网 | `eth-linux.c` | `eth-bsd.c`, `eth-dlpi.c`, `eth-snoop.c`, `eth-nit.c` |
| IP | `ip.c` (通用) | `ip-win32.c` |
| 路由 | `route-linux.c` | `route-bsd.c`, `route-win32.c` |
| ARP | `arp-ioctl.c` (Linux使用ioctl) | `arp-bsd.c`, `arp-win32.c` |
| 接口 | `intf.c` (通用，含Linux路径) | `intf-win32.c` |
| 防火墙 | `fw-ipchains.c` | `fw-ipfw.c`, `fw-pf.c`, `fw-ipf.c`, `fw-pktfilter.c` |
| TUN/TAP | `tun-linux.c` | `tun-bsd.c` |

### 1.2 Linux网络架构图

```
┌─────────────────────────────────────────┐
│        libdnet用户态API                 │
├─────────────────────────────────────────┤
│  Linux Socket API                      │
│  - PF_PACKET (以太网层)                 │
│  - AF_INET (IP层)                      │
│  - AF_NETLINK (内核通信)                │
├─────────────────────────────────────────┤
│  /proc 文件系统                         │
│  - /proc/net/route                     │
│  - /proc/net/arp                       │
│  - /proc/net/dev                       │
├─────────────────────────────────────────┤
│  ioctl 系统调用                         │
│  - SIOC* 接口                          │
├─────────────────────────────────────────┤
│  Linux内核网络子系统                    │
│  - 网络驱动 (drivers/net/*)            │
│  - 协议栈 (net/ipv4, net/ipv6)         │
└─────────────────────────────────────────┘
```

### 1.3 核心设计理念

1. **直接内核访问**：通过`/proc`文件系统直接读取内核数据
2. **原始套接字**：支持完整的原始套接字操作（发送和接收）
3. **Netlink通信**：使用Netlink套接字与内核路由子系统通信
4. **ioctl接口**：使用ioctl进行接口和ARP配置
5. **权限模型**：基于Linux capabilities（CAP_NET_RAW、CAP_NET_ADMIN）

---

## 内核网络机制

### 2.1 PF_PACKET套接字

Linux提供`PF_PACKET`套接字直接访问数据链路层：

```c
// 特点：
// - 可以发送和接收完整的以太网帧
// - 支持`ETH_P_ALL`捕获所有协议类型
// - 绑定到特定接口或所有接口
```

### 2.2 Netlink套接字

Netlink是用户空间与内核通信的标准机制：

```c
// 特点：
// - 双向通信
// - 异步消息传递
// - 支持多种协议族（NETLINK_ROUTE、NETLINK_NETFILTER等）
```

### 2.3 /proc文件系统

Linux通过`/proc/net/`导出网络状态：

```bash
/proc/net/
├── arp              # ARP表
├── dev              # 接口统计
├── route            # IPv4路由表
├── ipv6_route       # IPv6路由表
├── if_inet6         # IPv6接口地址
└── ip_fwchains      # ipchains防火墙规则
```

---

## 以太网模块实现

### 3.1 完整源码分析

```c
// eth-linux.c
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

struct eth_handle {
    int fd;                   // PF_PACKET套接字
    struct ifreq ifr;         // 接口请求结构
    struct sockaddr_ll sll;   // 链路层地址
};
```

### 3.2 以太网套接字打开

```c
// eth-linux.c:42-67
eth_t *
eth_open(const char *device)
{
    eth_t *e;
    int n;

    if ((e = calloc(1, sizeof(*e))) != NULL) {
        // 创建PF_PACKET原始套接字
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
        // 设置接口名称
        strlcpy(e->ifr.ifr_name, device, sizeof(e->ifr.ifr_name));

        // 获取接口索引
        if (ioctl(e->fd, SIOCGIFINDEX, &e->ifr) < 0)
            return (eth_close(e));

        // 配置链路层地址
        e->sll.sll_family = AF_PACKET;
        e->sll.sll_ifindex = e->ifr.ifr_ifindex;
    }
    return (e);
}
```

**关键点分析**：

1. **PF_PACKET套接字**：`socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))`
   - `PF_PACKET`：数据包协议族
   - `SOCK_RAW`：原始套接字
   - `ETH_P_ALL`：捕获所有以太网协议类型

2. **SIOCGIFINDEX ioctl**：获取接口索引
   - 接口索引用于绑定套接字到特定接口

3. **sockaddr_ll结构**：链路层地址结构
   ```c
   struct sockaddr_ll {
       unsigned short sll_family;   // AF_PACKET
       unsigned short sll_protocol; // 以太网类型
       int            sll_ifindex;  // 接口索引
       unsigned short sll_hatype;   // 硬件类型
       unsigned char  sll_pkttype;  // 分组类型
       unsigned char  sll_halen;    // 硬件地址长度
       unsigned char  sll_addr[8];  // 硬件地址
   };
   ```

### 3.3 发送以太网帧

```c
// eth-linux.c:69-78
ssize_t
eth_send(eth_t *e, const void *buf, size_t len)
{
    struct eth_hdr *eth = (struct eth_hdr *)buf;

    // 设置协议类型
    e->sll.sll_protocol = eth->eth_type;

    // 发送数据帧
    return (sendto(e->fd, buf, len, 0, (struct sockaddr *)&e->sll,
        sizeof(e->sll)));
}
```

**发送流程**：

1. 从数据帧中提取以太网类型
2. 设置sockaddr_ll的协议字段
3. 使用sendto发送到指定接口

### 3.4 获取接口MAC地址

```c
// eth-linux.c:91-104
int
eth_get(eth_t *e, eth_addr_t *ea)
{
    struct addr ha;

    // 通过ioctl获取硬件地址
    if (ioctl(e->fd, SIOCGIFHWADDR, &e->ifr) < 0)
        return (-1);

    // 将sockaddr转换为libdnet地址
    if (addr_ston(&e->ifr.ifr_hwaddr, &ha) < 0)
        return (-1);

    memcpy(ea, &ha.addr_eth, sizeof(*ea));
    return (0);
}
```

**SIOCGIFHWADDR ioctl**：
- 获取接口的硬件地址（MAC地址）
- 返回的ifreq结构包含ifr_hwaddr字段

### 3.5 设置接口MAC地址

```c
// eth-linux.c:106-118
int
eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct addr ha;

    // 构造地址结构
    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);

    // 转换为sockaddr
    addr_ntos(&ha, &e->ifr.ifr_hwaddr);

    // 通过ioctl设置硬件地址
    return (ioctl(e->fd, SIOCSIFHWADDR, &e->ifr));
}
```

**注意**：设置MAC地址通常需要root权限或CAP_NET_ADMIN能力

---

## IP模块实现

### 4.1 IP套接字打开

```c
// ip.c:25-61
ip_t *
ip_open(void)
{
    ip_t *i;
    int n;
    socklen_t len;

    if ((i = calloc(1, sizeof(*i))) == NULL)
        return (NULL);

    // 创建原始套接字
    if ((i->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        return (ip_close(i));
    
#ifdef IP_HDRINCL
    // 设置IP头包含选项
    n = 1;
    if (setsockopt(i->fd, IPPROTO_IP, IP_HDRINCL, &n, sizeof(n)) < 0)
        return (ip_close(i));
#endif

#ifdef SO_SNDBUF
    // 优化发送缓冲区大小
    len = sizeof(n);
    if (getsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, &len) < 0)
        return (ip_close(i));

    for (n += 128; n < 1048576; n += 128) {
        if (setsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) {
            if (errno == ENOBUFS)
                break;
            return (ip_close(i));
        }
    }
#endif

#ifdef SO_BROADCAST
    // 允许广播
    n = 1;
    if (setsockopt(i->fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n)) < 0)
        return (ip_close(i));
#endif
    return (i);
}
```

**关键选项**：

1. **IP_HDRINCL**：
   - 允许应用层提供完整IP头
   - 内核不再自动添加IP头

2. **SO_SNDBUF优化**：
   - 逐步增加发送缓冲区大小
   - 直到达到1MB上限或系统限制

3. **SO_BROADCAST**：
   - 允许发送广播数据包

### 4.2 发送IP数据包

```c
// ip.c:63-93
ssize_t
ip_send(ip_t *i, const void *buf, size_t len)
{
    struct ip_hdr *ip;
    struct sockaddr_in sin;

    ip = (struct ip_hdr *)buf;

    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SOCKADDR_SA_LEN       
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip->ip_dst;
    
#ifdef HAVE_RAWIP_HOST_OFFLEN
    // 某些系统需要主机字节序
    ip->ip_len = ntohs(ip->ip_len);
    ip->ip_off = ntohs(ip->ip_off);

    len = sendto(i->fd, buf, len, 0,
        (struct sockaddr *)&sin, sizeof(sin));
    
    ip->ip_len = htons(ip->ip_len);
    ip->ip_off = htons(ip->ip_off);

    return (len);
#else
    return (sendto(i->fd, buf, len, 0,
        (struct sockaddr *)&sin, sizeof(sin)));
#endif
}
```

**发送流程**：

1. 从IP头提取目的地址
2. 构造sockaddr_in结构
3. 使用sendto发送数据包

**IP头结构**：
```c
struct ip_hdr {
    uint8_t  ip_vhl;      // 版本和头长度
    uint8_t  ip_tos;      // 服务类型
    uint16_t ip_len;      // 总长度
    uint16_t ip_id;       // 标识
    uint16_t ip_off;      // 分片偏移
    uint8_t  ip_ttl;      // 生存时间
    uint8_t  ip_p;        // 协议
    uint16_t ip_sum;      // 校验和
    uint32_t ip_src;      // 源地址
    uint32_t ip_dst;      // 目的地址
};
```

---

## 路由模块实现

### 5.1 路由模块架构

Linux路由模块使用两种机制：
1. **Netlink套接字**：用于查询和修改路由表
2. **/proc文件系统**：用于读取路由表

```c
// route-linux.c:41-44
struct route_handle {
    int fd;    // ioctl套接字（用于添加/删除路由）
    int nlfd;  // Netlink套接字（用于查询路由）
};
```

### 5.2 路由模块初始化

```c
// route-linux.c:46-69
route_t *
route_open(void)
{
    struct sockaddr_nl snl;
    route_t *r;

    if ((r = calloc(1, sizeof(*r))) != NULL) {
        r->fd = r->nlfd = -1;

        // 创建ioctl套接字
        if ((r->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            return (route_close(r));

        // 创建Netlink套接字
        if ((r->nlfd = socket(AF_NETLINK, SOCK_RAW,
             NETLINK_ROUTE)) < 0)
            return (route_close(r));

        // 绑定Netlink套接字
        memset(&snl, 0, sizeof(snl));
        snl.nl_family = AF_NETLINK;

        if (bind(r->nlfd, (struct sockaddr *)&snl, sizeof(snl)) < 0)
            return (route_close(r));
    }
    return (r);
}
```

**双套接字设计**：
- `fd`：用于ioctl操作（添加/删除路由）
- `nlfd`：用于Netlink通信（查询路由）

### 5.3 添加路由

```c
// route-linux.c:71-92
int
route_add(route_t *r, const struct route_entry *entry)
{
    struct rtentry rt;
    struct addr dst;

    memset(&rt, 0, sizeof(rt));
    rt.rt_flags = RTF_UP | RTF_GATEWAY;

    // 判断是否是主机路由
    if (ADDR_ISHOST(&entry->route_dst)) {
        rt.rt_flags |= RTF_HOST;
        memcpy(&dst, &entry->route_dst, sizeof(dst));
    } else
        addr_net(&entry->route_dst, &dst);

    // 填充路由条目
    if (addr_ntos(&dst, &rt.rt_dst) < 0 ||
        addr_ntos(&entry->route_gw, &rt.rt_gateway) < 0 ||
        addr_btos(entry->route_dst.addr_bits, &rt.rt_genmask) < 0)
        return (-1);

    // 通过ioctl添加路由
    return (ioctl(r->fd, SIOCADDRT, &rt));
}
```

**rtentry结构**：
```c
struct rtentry {
    unsigned long  rt_pad1;
    struct sockaddr rt_dst;      // 目的地址
    struct sockaddr rt_gateway;  // 网关
    struct sockaddr rt_genmask;   // 子网掩码
    unsigned short rt_flags;     // 路由标志
    short           rt_pad2;
    unsigned long   rt_pad3;
    unsigned char   rt_tos;
    unsigned char   rt_class;
};
```

### 5.4 删除路由

```c
// route-linux.c:94-114
int
route_delete(route_t *r, const struct route_entry *entry)
{
    struct rtentry rt;
    struct addr dst;

    memset(&rt, 0, sizeof(rt));
    rt.rt_flags = RTF_UP;

    // 判断是否是主机路由
    if (ADDR_ISHOST(&entry->route_dst)) {
        rt.rt_flags |= RTF_HOST;
        memcpy(&dst, &entry->route_dst, sizeof(dst));
    } else
        addr_net(&entry->route_dst, &dst);

    // 填充路由条目
    if (addr_ntos(&dst, &rt.rt_dst) < 0 ||
        addr_btos(entry->route_dst.addr_bits, &rt.rt_genmask) < 0)
        return (-1);

    // 通过ioctl删除路由
    return (ioctl(r->fd, SIOCDELRT, &rt));
}
```

### 5.5 查询路由（Netlink）

```c
// route-linux.c:116-221
int
route_get(route_t *r, struct route_entry *entry)
{
    static int seq;
    struct nlmsghdr *nmsg;
    struct rtmsg *rmsg;
    struct rtattr *rta;
    struct sockaddr_nl snl;
    struct iovec iov;
    struct msghdr msg;
    u_char buf[512];
    int i, af, alen;

    // 确定地址族和长度
    switch (entry->route_dst.addr_type) {
    case ADDR_TYPE_IP:
        af = AF_INET;
        alen = IP_ADDR_LEN;
        break;
    case ADDR_TYPE_IP6:
        af = AF_INET6;
        alen = IP6_ADDR_LEN;
        break;
    default:
        errno = EINVAL;
        return (-1);
    }
    memset(buf, 0, sizeof(buf));

    // 构造Netlink消息头
    nmsg = (struct nlmsghdr *)buf;
    nmsg->nlmsg_len = NLMSG_LENGTH(sizeof(*nmsg)) + RTA_LENGTH(alen);
    nmsg->nlmsg_flags = NLM_F_REQUEST;
    nmsg->nlmsg_type = RTM_GETROUTE;
    nmsg->nlmsg_seq = ++seq;

    // 填充路由消息
    rmsg = (struct rtmsg *)(nmsg + 1);
    rmsg->rtm_family = af;
    rmsg->rtm_dst_len = entry->route_dst.addr_bits;

    // 添加目的地址属性
    rta = RTM_RTA(rmsg);
    rta->rta_type = RTA_DST;
    rta->rta_len = RTA_LENGTH(alen);

    /* XXX - gross hack for default route */
    if (af == AF_INET && entry->route_dst.addr_ip == IP_ADDR_ANY) {
        i = htonl(0x60060606);
        memcpy(RTA_DATA(rta), &i, alen);
    } else
        memcpy(RTA_DATA(rta), entry->route_dst.addr_data8, alen);

    // 发送Netlink消息
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;

    iov.iov_base = nmsg;
    iov.iov_len = nmsg->nlmsg_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &snl;
    msg.msg_namelen = sizeof(snl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(r->nlfd, &msg, 0) < 0)
        return (-1);

    // 接收Netlink响应
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    if ((i = recvmsg(r->nlfd, &msg, 0)) <= 0)
        return (-1);

    if (nmsg->nlmsg_len < (int)sizeof(*nmsg) || 
        nmsg->nlmsg_len > i ||
        nmsg->nlmsg_seq != seq) {
        errno = EINVAL;
        return (-1);
    }
    if (nmsg->nlmsg_type == NLMSG_ERROR)
        return (-1);

    i -= NLMSG_LENGTH(sizeof(*nmsg));

    // 解析路由属性
    entry->route_gw.addr_type = ADDR_TYPE_NONE;
    entry->intf_name[0] = '\0';
    for (rta = RTM_RTA(rmsg); RTA_OK(rta, i); rta = RTA_NEXT(rta, i)) {
        if (rta->rta_type == RTA_GATEWAY) {
            entry->route_gw.addr_type = entry->route_dst.addr_type;
            memcpy(entry->route_gw.addr_data8, RTA_DATA(rta), alen);
            entry->route_gw.addr_bits = alen * 8;
        } else if (rta->rta_type == RTA_OIF) {
            char ifbuf[IFNAMSIZ];
            char *p;
            int intf_index;

            intf_index = *(int *) RTA_DATA(rta);
            p = if_indextoname(intf_index, ifbuf);
            if (p == NULL)
                return (-1);
            strlcpy(entry->intf_name, ifbuf, sizeof(entry->intf_name));
        }
    }
    if (entry->route_gw.addr_type == ADDR_TYPE_NONE) {
        errno = ESRCH;
        return (-1);
    }

    return (0);
}
```

**Netlink消息结构**：

```
nlmsghdr
├── nlmsg_len: 消息长度
├── nlmsg_type: 消息类型 (RTM_GETROUTE)
├── nlmsg_flags: 消息标志 (NLM_F_REQUEST)
└── nlmsg_seq: 序列号

rtmsg (路由消息)
├── rtm_family: 地址族 (AF_INET/AF_INET6)
└── rtm_dst_len: 目的地址长度

rtattr (路由属性)
├── rta_type: 属性类型 (RTA_DST, RTA_GATEWAY, RTA_OIF)
└── rta_data: 属性数据
```

### 5.6 遍历路由表（/proc）

#### IPv4路由表

```c
// route-linux.c:223-261
int
route_loop(route_t *r, route_handler callback, void *arg)
{
    FILE *fp;
    struct route_entry entry;
    char buf[BUFSIZ];
    char ifbuf[16];
    int ret = 0;

    if ((fp = fopen(PROC_ROUTE_FILE, "r")) != NULL) {
        int i, iflags, refcnt, use, metric, mss, win, irtt;
        uint32_t mask;

        while (fgets(buf, sizeof(buf), fp) != NULL) {
            // 解析/proc/net/route格式
            i = sscanf(buf, "%15s %X %X %X %d %d %d %X %d %d %d\n",
                ifbuf, &entry.route_dst.addr_ip,
                &entry.route_gw.addr_ip, &iflags, &refcnt, &use,
                &metric, &mask, &mss, &win, &irtt);

            if (i < 11 || !(iflags & RTF_UP))
                continue;

            strlcpy(entry.intf_name, ifbuf, sizeof(entry.intf_name));

            entry.route_dst.addr_type = entry.route_gw.addr_type =
                ADDR_TYPE_IP;

            // 转换子网掩码
            if (addr_mtob(&mask, IP_ADDR_LEN,
                &entry.route_dst.addr_bits) < 0)
                continue;

            entry.route_gw.addr_bits = IP_ADDR_BITS;
            entry.metric = metric;

            if ((ret = callback(&entry, arg)) != 0)
                break;
        }
        fclose(fp);
    }
    // ... IPv6路由表处理
    return (ret);
}
```

**/proc/net/route格式**：
```
Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT
eth0  00000000    0102A8C0 0003    0      0   0      FFFFFFFF ...
```

#### IPv6路由表

```c
// route-linux.c:262-296
if (ret == 0 && (fp = fopen(PROC_IPV6_ROUTE_FILE, "r")) != NULL) {
    char s[33], d[8][5], n[8][5];
    int i, iflags, metric;
    u_int slen, dlen;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        i = sscanf(buf, "%04s%04s%04s%04s%04s%04s%04s%04s %02x "
            "%32s %02x %04s%04s%04s%04s%04s%04s%04s%04s "
            "%x %*x %*x %x %15s",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
            &dlen, s, &slen,
            n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7],
            &metric, &iflags, ifbuf);

        if (i < 21 || !(iflags & RTF_UP))
            continue;

        strlcpy(entry.intf_name, ifbuf, sizeof(entry.intf_name));

        // 构造IPv6地址字符串
        snprintf(buf, sizeof(buf), "%s:%s:%s:%s:%s:%s:%s:%s/%d",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
            dlen);
        addr_aton(buf, &entry.route_dst);
        snprintf(buf, sizeof(buf), "%s:%s:%s:%s:%s:%s:%s:%s/%d",
            n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7],
            IP6_ADDR_BITS);
        addr_aton(buf, &entry.route_gw);
        entry.metric = metric;

        if ((ret = callback(&entry, arg)) != 0)
            break;
    }
    fclose(fp);
}
```

**/proc/net/ipv6_route格式**：
```
dest prefix src prefix metric ref use flags ifname
00000000000000000000000000000000 00 00000000000000000000000000000000 00 256 0 0 00000002 eth0
```

---

## ARP模块实现

### 6.1 ARP模块架构

Linux ARP模块使用两种机制：
1. **ioctl**：用于添加、删除、查询ARP条目
2. **/proc/net/arp**：用于遍历ARP表

```c
// arp-ioctl.c:47-52
struct arp_handle {
    int fd;  // ioctl套接字
#ifdef HAVE_ARPREQ_ARP_DEV
    intf_t *intf;  // 接口句柄（用于查找设备）
#endif
};
```

### 6.2 ARP模块初始化

```c
// arp-ioctl.c:54-74
arp_t *
arp_open(void)
{
    arp_t *a;

    if ((a = calloc(1, sizeof(*a))) != NULL) {
        // 创建ioctl套接字
        if ((a->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            return (arp_close(a));
#ifdef HAVE_ARPREQ_ARP_DEV
        if ((a->intf = intf_open()) == NULL)
            return (arp_close(a));
#endif
    }
    return (a);
}
```

### 6.3 添加ARP条目

```c
// arp-ioctl.c:100-163
int
arp_add(arp_t *a, const struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    // 设置IP地址
    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

    /* XXX - see arp(7) for details... */
#ifdef __linux__
    // Linux特殊处理
    if (addr_ntos(&entry->arp_ha, &ar.arp_ha) < 0)
        return (-1);
    ar.arp_ha.sa_family = ARP_HRD_ETH;
#else
    // 其他系统
    ar.arp_ha.sa_family = AF_UNSPEC;
    memcpy(ar.arp_ha.sa_data, &entry->arp_ha.addr_eth, ETH_ADDR_LEN);
#endif

#ifdef HAVE_ARPREQ_ARP_DEV
    // 查找设备名称
    if (intf_loop(a->intf, _arp_set_dev, &ar) != 1) {
        errno = ESRCH;
        return (-1);
    }
#endif
    // 设置ARP标志
    ar.arp_flags = ATF_PERM | ATF_COM;
    
#ifdef hpux
    /* XXX - screwy extended arpreq struct */
    {
        struct sockaddr_in *sin;

        ar.arp_hw_addr_len = ETH_ADDR_LEN;
        sin = (struct sockaddr_in *)&ar.arp_pa_mask;
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = IP_ADDR_BROADCAST;
    }
#endif
    // 通过ioctl添加ARP条目
    if (ioctl(a->fd, SIOCSARP, &ar) < 0)
        return (-1);

    return (0);
}
```

**arpreq结构**：
```c
struct arpreq {
    struct sockaddr arp_pa;      // 协议地址 (IP)
    struct sockaddr arp_ha;      // 硬件地址 (MAC)
    int             arp_flags;   // ARP标志
    struct sockaddr arp_netmask; // 子网掩码
    char            arp_dev[16]; // 设备名称
};
```

**ARP标志**：
- `ATF_COM`：已完成
- `ATF_PERM`：永久条目
- `ATF_PUBL`：发布条目

### 6.4 删除ARP条目

```c
// arp-ioctl.c:165-179
int
arp_delete(arp_t *a, const struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

    // 通过ioctl删除ARP条目
    if (ioctl(a->fd, SIOCDARP, &ar) < 0)
        return (-1);

    return (0);
}
```

### 6.5 查询ARP条目

```c
// arp-ioctl.c:181-205
int
arp_get(arp_t *a, struct arp_entry *entry)
{
    struct arpreq ar;

    memset(&ar, 0, sizeof(ar));

    if (addr_ntos(&entry->arp_pa, &ar.arp_pa) < 0)
        return (-1);

#ifdef HAVE_ARPREQ_ARP_DEV
    if (intf_loop(a->intf, _arp_set_dev, &ar) != 1) {
        errno = ESRCH;
        return (-1);
    }
#endif
    // 通过ioctl查询ARP条目
    if (ioctl(a->fd, SIOCGARP, &ar) < 0)
        return (-1);

    if ((ar.arp_flags & ATF_COM) == 0) {
        errno = ESRCH;
        return (-1);
    }
    return (addr_ston(&ar.arp_ha, &entry->arp_ha));
}
```

### 6.6 遍历ARP表（/proc）

```c
// arp-ioctl.c:207-240
int
arp_loop(arp_t *a, arp_handler callback, void *arg)
{
    FILE *fp;
    struct arp_entry entry;
    char buf[BUFSIZ], ipbuf[100], macbuf[100], maskbuf[100], devbuf[100];
    int i, type, flags, ret;

    if ((fp = fopen(PROC_ARP_FILE, "r")) == NULL)
        return (-1);

    ret = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        i = sscanf(buf, "%s 0x%x 0x%x %99s %99s %99s\n",
            ipbuf, &type, &flags, macbuf, maskbuf, devbuf);

        if (i < 4 || (flags & ATF_COM) == 0)
            continue;

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
```

**/proc/net/arp格式**：
```
IP address       HW type   Flags    HW address         Mask      Device
192.168.1.1      0x1       0x2      00:11:22:33:44:55  *        eth0
```

---

## 接口管理模块实现

### 7.1 接口句柄结构

```c
// intf.c:96-104
struct intf_handle {
    int fd;         // IPv4套接字
    int fd6;        // IPv6套接字
    struct ifconf ifc;   // 接口配置
#ifdef SIOCGLIFCONF
    struct lifconf lifc;  // IPv6接口配置
#endif
    u_char ifcbuf[4192];  // 缓冲区
};
```

### 7.2 接口模块初始化

```c
// intf.c:148-173
intf_t *
intf_open(void)
{
    intf_t *intf;
    int one = 1;

    if ((intf = calloc(1, sizeof(*intf))) != NULL) {
        intf->fd = intf->fd6 = -1;

        // 创建IPv4套接字
        if ((intf->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            return (intf_close(intf));

        // 允许广播
        setsockopt(intf->fd, SOL_SOCKET, SO_BROADCAST,
            (const char *) &one, sizeof(one));

        // 创建IPv6套接字（如果支持）
#if defined(SIOCGLIFCONF) || defined(SIOCGIFNETMASK_IN6) || defined(SIOCGIFNETMASK6)
        if ((intf->fd6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
#  ifdef EPROTONOSUPPORT
            if (errno != EPROTONOSUPPORT)
#  endif
                return (intf_close(intf));
        }
#endif
    }
    return (intf);
}
```

### 7.3 读取/proc/net/dev（Linux）

```c
// intf.c:944-1000+
int
intf_loop(intf_t *intf, intf_handler callback, void *arg)
{
    // ... ioctl遍历接口
    // ...

#ifdef HAVE_PROCFS
    // Linux特殊处理：读取/proc/net/dev
#define PROC_DEV_FILE  "/proc/net/dev"
    {
        FILE *f;
        char buf[256], name[INTF_NAME_LEN];
        struct intf_entry entry;
        u_int64_t rx_packets, rx_bytes, rx_errors, rx_dropped;
        u_int64_t tx_packets, tx_bytes, tx_errors, tx_dropped;

        if ((f = fopen(PROC_DEV_FILE, "r")) != NULL) {
            // 跳过头两行
            fgets(buf, sizeof(buf), f);
            fgets(buf, sizeof(buf), f);

            while (fgets(buf, sizeof(buf), f) != NULL) {
                char *p;

                // 解析接口名称
                p = strchr(buf, ':');
                if (p == NULL)
                    continue;
                *p = '\0';
                
                // 跳过空格
                for (name[0] = '\0'; buf[0] == ' '; buf++)
                    ;
                strlcpy(name, buf, sizeof(name));

                // 解析统计信息
                sscanf(p + 1,
                    "%Lu %Lu %Lu %Lu %*Lu %*Lu %*Lu %*Lu "
                    "%Lu %Lu %Lu %Lu",
                    &rx_bytes, &rx_packets, &rx_errors, &rx_dropped,
                    &tx_bytes, &tx_packets, &tx_errors, &tx_dropped);

                // 填充条目结构
                // ...
            }
            fclose(f);
        }
    }
#endif
    return (0);
}
```

**/proc/net/dev格式**：
```
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo: 2776770   14126    0    0    0     0          0         0  2776770   14126    0    0    0     0       0          0
  eth0: 1234567   56789    1    2    0     0          0         0  9876543   43210    0    0    0     0       0          0
```

### 7.4 读取IPv6地址（/proc/net/if_inet6）

```c
// intf.c:776-943
#ifdef HAVE_PROCFS
#define PROC_INET6_FILE    "/proc/net/if_inet6"
{
    FILE *f;
    char buf[256], s[8][5], name[INTF_NAME_LEN];
    struct intf_entry entry;

    if ((f = fopen(PROC_INET6_FILE, "r")) != NULL) {
        while (fgets(buf, sizeof(buf), f) != NULL) {
            int i, scope, prefix_len;
            u_int if_idx, flags;

            i = sscanf(buf,
                "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %15s\n",
                s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                &if_idx, &prefix_len, &scope, &flags, name);

            if (i < 13)
                continue;

            // 构造IPv6地址
            snprintf(buf, sizeof(buf),
                "%s:%s:%s:%s:%s:%s:%s:%s/%d",
                s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                prefix_len);
            addr_aton(buf, &entry.intf_addr);

            // 添加到别名地址列表
            // ...
        }
        fclose(f);
    }
}
#endif
```

**/proc/net/if_inet6格式**：
```
00000000000000000000000000000001 01 80 10 80       lo
fe80000000000000021122fffe334455 02 40 20 80       eth0
```

---

## 防火墙模块实现

### 8.1 IPChains防火墙

Linux 2.2内核使用ipchains防火墙：

```c
// fw-ipchains.c:40-42
struct fw_handle {
    int fd;  // 原始套接字
};
```

### 8.2 IPChains规则转换

```c
// fw-ipchains.c:44-78
static void
fr_to_fwc(const struct fw_rule *fr, struct ip_fwchange *fwc)
{
    memset(fwc, 0, sizeof(*fwc));

    strlcpy(fwc->fwc_rule.ipfw.fw_vianame, fr->fw_device, IFNAMSIZ);
    
    // 设置操作标签
    if (fr->fw_op == FW_OP_ALLOW)
        strlcpy(fwc->fwc_rule.label, IP_FW_LABEL_ACCEPT, 
            sizeof(fwc->fwc_rule.label));
    else
        strlcpy(fwc->fwc_rule.label, IP_FW_LABEL_BLOCK,
            sizeof(fwc->fwc_rule.label));

    // 设置方向标签
    if (fr->fw_dir == FW_DIR_IN)
        strlcpy(fwc->fwc_label, IP_FW_LABEL_INPUT,
            sizeof(fwc->fwc_label));
    else
        strlcpy(fwc->fwc_label, IP_FW_LABEL_OUTPUT,
            sizeof(fwc->fwc_label));
    
    // 设置规则参数
    fwc->fwc_rule.ipfw.fw_proto = fr->fw_proto;
    fwc->fwc_rule.ipfw.fw_src.s_addr = fr->fw_src.addr_ip;
    fwc->fwc_rule.ipfw.fw_dst.s_addr = fr->fw_dst.addr_ip;
    addr_btom(fr->fw_src.addr_bits, &fwc->fwc_rule.ipfw.fw_smsk.s_addr,
        IP_ADDR_LEN);
    addr_btom(fr->fw_dst.addr_bits, &fwc->fwc_rule.ipfw.fw_dmsk.s_addr,
        IP_ADDR_LEN);

    /* XXX - ICMP? */
    fwc->fwc_rule.ipfw.fw_spts[0] = fr->fw_sport[0];
    fwc->fwc_rule.ipfw.fw_spts[1] = fr->fw_sport[1];
    fwc->fwc_rule.ipfw.fw_dpts[0] = fr->fw_dport[0];
    fwc->fwc_rule.ipfw.fw_dpts[1] = fr->fw_dport[1];
}
```

### 8.3 添加防火墙规则

```c
// fw-ipchains.c:126-135
int
fw_add(fw_t *fw, const struct fw_rule *rule)
{
    struct ip_fwchange fwc;

    fr_to_fwc(rule, &fwc);
    
    // 通过setsockopt添加规则
    return (setsockopt(fw->fd, IPPROTO_IP, IP_FW_APPEND,
        &fwc, sizeof(fwc)));
}
```

**setsockopt与防火墙**：
- `IP_FW_APPEND`：追加规则
- `IP_FW_DELETE`：删除规则
- `IP_FW_INSERT`：插入规则

### 8.4 遍历防火墙规则（/proc）

```c
// fw-ipchains.c:148-216
int
fw_loop(fw_t *fw, fw_handler callback, void *arg)
{
    FILE *fp;
    struct ip_fwchange fwc;
    struct fw_rule fr;
    char buf[BUFSIZ];
    u_int phi, plo, bhi, blo, tand, txor;
    int ret;
    
    if ((fp = fopen(PROC_IPCHAINS_FILE, "r")) == NULL)
        return (-1);

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        // 解析/proc/net/ip_fwchains格式
        if (sscanf(buf,
            "%8s %X/%X->%X/%X %s %hX %hX %hu %u %u %u %u "
            "%hu-%hu %hu-%hu A%X X%X %hX %u %hu %s\n",
            fwc.fwc_label,
            &fwc.fwc_rule.ipfw.fw_src.s_addr,
            &fwc.fwc_rule.ipfw.fw_smsk.s_addr,
            &fwc.fwc_rule.ipfw.fw_dst.s_addr,
            &fwc.fwc_rule.ipfw.fw_dmsk.s_addr,
            fwc.fwc_rule.ipfw.fw_vianame,
            &fwc.fwc_rule.ipfw.fw_flg,
            &fwc.fwc_rule.ipfw.fw_invflg,
            &fwc.fwc_rule.ipfw.fw_proto,
            &phi, &plo, &bhi, &blo,
            &fwc.fwc_rule.ipfw.fw_spts[0],
            &fwc.fwc_rule.ipfw.fw_spts[1],
            &fwc.fwc_rule.ipfw.fw_dpts[0],
            &fwc.fwc_rule.ipfw.fw_dpts[1],
            &tand, &txor,
            &fwc.fwc_rule.ipfw.fw_redirpt,
            &fwc.fwc_rule.ipfw.fw_mark,
            &fwc.fwc_rule.ipfw.fw_outputsize,
            fwc.fwc_rule.label) != 23)
            break;

        // 转换字节序
        fwc.fwc_rule.ipfw.fw_src.s_addr =
            htonl(fwc.fwc_rule.ipfw.fw_src.s_addr);
        fwc.fwc_rule.ipfw.fw_dst.s_addr =
            htonl(fwc.fwc_rule.ipfw.fw_dst.s_addr);
        fwc.fwc_rule.ipfw.fw_smsk.s_addr =
            htonl(fwc.fwc_rule.ipfw.fw_smsk.s_addr);
        fwc.fwc_rule.ipfw.fw_dmsk.s_addr =
            htonl(fwc.fwc_rule.ipfw.fw_dmsk.s_addr);
        
        fwc_to_fr(&fwc, &fr);
        
        if ((ret = callback(&fr, arg)) != 0) {
            fclose(fp);
            return (ret);
        }
    }
    fclose(fp);
    
    return (0);
}
```

---

## TUN/TAP模块实现

### 9.1 TUN/TAP设备

Linux提供TUN/TAP虚拟网络设备：

```c
// tun-linux.c:29-33
struct tun {
    int fd;              // TUN/TAP设备文件描述符
    intf_t *intf;        // 接口句柄
    struct ifreq ifr;    // 接口请求结构
};
```

### 9.2 打开TUN设备

```c
// tun-linux.c:35-64
tun_t *
tun_open(struct addr *src, struct addr *dst, int mtu)
{
    tun_t *tun;
    struct intf_entry ifent;

    if ((tun = calloc(1, sizeof(*tun))) == NULL)
        return (NULL);

    // 打开/dev/net/tun
    if ((tun->fd = open("/dev/net/tun", O_RDWR, 0)) < 0 ||
        (tun->intf = intf_open()) == NULL)
        return (tun_close(tun));

    // 设置TUN设备标志
    tun->ifr.ifr_flags = IFF_TUN;

    // 创建TUN设备
    if (ioctl(tun->fd, TUNSETIFF, (void *) &tun->ifr) < 0)
        return (tun_close(tun));

    // 配置接口
    memset(&ifent, 0, sizeof(ifent));
    strlcpy(ifent.intf_name, tun->ifr.ifr_name, sizeof(ifent.intf_name));
    ifent.intf_flags = INTF_FLAG_UP|INTF_FLAG_POINTOPOINT;
    ifent.intf_addr = *src;
    ifent.intf_dst_addr = *dst;
    ifent.intf_mtu = mtu;

    if (intf_set(tun->intf, &ifent) < 0)
        return (tun_close(tun));

    return (tun);
}
```

**TUNSETIFF ioctl**：
- 创建TUN或TAP虚拟设备
- `IFF_TUN`：TUN设备（IP层）
- `IFF_TAP`：TAP设备（以太网层）
- `IFF_NO_PI`：不添加包信息头

### 9.3 发送数据

```c
// tun-linux.c:78-90
ssize_t
tun_send(tun_t *tun, const void *buf, size_t size)
{
    struct iovec iov[2];
    uint32_t etype = htonl(ETH_TYPE_IP);

    // TUN设备需要协议类型前缀
    iov[0].iov_base = &etype;
    iov[0].iov_len = sizeof(etype);
    iov[1].iov_base = (void *)buf;
    iov[1].iov_len = size;

    return (writev(tun->fd, iov, 2));
}
```

### 9.4 接收数据

```c
// tun-linux.c:92-104
ssize_t
tun_recv(tun_t *tun, void *buf, size_t size)
{
    struct iovec iov[2];
    uint32_t type;

    iov[0].iov_base = &type;
    iov[0].iov_len = sizeof(type);
    iov[1].iov_base = (void *)buf;
    iov[1].iov_len = size;

    // 返回实际数据长度（不包括协议类型）
    return (readv(tun->fd, iov, 2) - sizeof(type));
}
```

---

## 各平台实现对比

### 10.1 以太网模块对比

| 特性 | Linux (eth-linux.c) | BSD (eth-bsd.c) | Windows (eth-win32.c) |
|------|---------------------|-----------------|----------------------|
| 实现方式 | PF_PACKET套接字 | BPF | 存根实现 |
| 发送 | ✅ | ✅ | ❌ |
| 接收 | ✅ | ✅ | ❌ |
| MAC地址获取 | ✅ | ✅ | ⚠️ (通过intf) |
| MAC地址设置 | ✅ | ✅ | ❌ |
| 权限要求 | CAP_NET_RAW | root | 需要WinPcap |

### 10.2 IP模块对比

| 特性 | Linux (ip.c) | BSD (ip.c) | Windows (ip-win32.c) |
|------|-------------|------------|----------------------|
| 实现方式 | SOCK_RAW | SOCK_RAW | SOCK_RAW |
| IP_HDRINCL | ✅ | ✅ | ✅ |
| 发送 | ✅ | ✅ | ✅ |
| 接收 | ✅ | ✅ | ❌ (Windows 8+) |
| SO_SNDBUF优化 | ✅ | ✅ | ❌ |
| 权限要求 | CAP_NET_RAW | root | 管理员 |

### 10.3 路由模块对比

| 特性 | Linux (route-linux.c) | BSD (route-bsd.c) | Windows (route-win32.c) |
|------|----------------------|------------------|------------------------|
| 数据源 | /proc + Netlink | 路由套接字 | GetIpForwardTable |
| IPv4 | ✅ | ✅ | ✅ |
| IPv6 | ✅ | ✅ | ✅ (Vista+) |
| 动态更新 | ✅ | ✅ | ⚠️ |
| 添加路由 | ✅ | ✅ | ✅ |
| 删除路由 | ✅ | ✅ | ✅ |
| 权限要求 | CAP_NET_ADMIN | root | 管理员 |

### 10.4 ARP模块对比

| 特性 | Linux (arp-ioctl.c) | BSD (arp-bsd.c) | Windows (arp-win32.c) |
|------|---------------------|----------------|----------------------|
| 数据源 | /proc + ioctl | sysctl | GetIpNetTable |
| 添加条目 | ✅ | ✅ | ✅ |
| 删除条目 | ✅ | ✅ | ✅ |
| 静态条目 | ✅ | ✅ | ✅ |
| 动态更新 | ✅ | ✅ | ⚠️ |
| 权限要求 | CAP_NET_ADMIN | root | 管理员 |

### 10.5 接口管理对比

| 特性 | Linux (intf.c) | BSD (intf-bsd.c) | Windows (intf-win32.c) |
|------|---------------|------------------|------------------------|
| 数据源 | ioctl + /proc | ioctl + sysctl | GetAdaptersAddresses |
| IPv4 | ✅ | ✅ | ✅ |
| IPv6 | ✅ | ✅ | ✅ |
| MAC地址 | ✅ | ✅ | ✅ |
| MTU | ✅ | ✅ | ✅ |
| 接口标志 | ✅ | ✅ | ✅ |
| 统计信息 | ✅ | ✅ | ⚠️ |

---

## 性能优化与最佳实践

### 11.1 发送缓冲区优化

libdnet自动优化发送缓冲区大小：

```c
// ip.c:42-54
#ifdef SO_SNDBUF
len = sizeof(n);
if (getsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, &len) < 0)
    return (ip_close(i));

for (n += 128; n < 1048576; n += 128) {
    if (setsockopt(i->fd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) {
        if (errno == ENOBUFS)
            break;
        return (ip_close(i));
    }
}
#endif
```

**优化策略**：
- 从当前缓冲区大小开始
- 每次增加128字节
- 直到达到1MB或系统限制

### 11.2 /proc文件读取优化

**缓冲区大小**：
```c
// 使用足够大的缓冲区
#define BUFSIZ 8192
char buf[BUFSIZ];
```

**解析效率**：
- 使用sscanf一次性解析整行
- 跳过无效条目
- 及时关闭文件句柄

### 11.3 权限管理

**Capabilities模型**：

```bash
# 获取原始套接字权限
sudo setcap cap_net_raw+ep /path/to/program

# 获取路由管理权限
sudo setcap cap_net_admin+ep /path/to/program
```

**最小权限原则**：
- 仅请求必要的capabilities
- 避免以root身份运行
- 使用capabilities替代sudo

### 11.4 错误处理

**统一错误处理**：

```c
// 检查返回值
if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
    if (errno == EPERM)
        fprintf(stderr, "需要CAP_NET_ADMIN权限\n");
    else if (errno == ENODEV)
        fprintf(stderr, "接口不存在\n");
    else
        perror("ioctl");
    return (-1);
}
```

---

## 附录：完整代码示例

### A.1 Linux网络工具完整示例

```c
/*
 * libdnet Linux网络工具完整示例
 * 功能：发送/接收以太网帧、IP包，查询路由表、ARP表
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "dnet.h"

// 接口回调函数
static int print_interface(const struct intf_entry *entry, void *arg) {
    printf("\n[%s]\n", entry->intf_name);
    printf("  Flags: 0x%04X", entry->intf_flags);
    if (entry->intf_flags & INTF_FLAG_UP) printf(" UP");
    if (entry->intf_flags & INTF_FLAG_LOOPBACK) printf(" LOOPBACK");
    if (entry->intf_flags & INTF_FLAG_BROADCAST) printf(" BROADCAST");
    if (entry->intf_flags & INTF_FLAG_MULTICAST) printf(" MULTICAST");
    printf("\n");
    
    if (entry->intf_mtu != 0)
        printf("  MTU: %d\n", entry->intf_mtu);
    
    if (entry->intf_addr.addr_type == ADDR_TYPE_IP)
        printf("  IP: %s/%d\n", addr_ntoa(&entry->intf_addr), 
            entry->intf_addr.addr_bits);
    
    if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH)
        printf("  MAC: %s\n", addr_ntoa(&entry->intf_link_addr));
    
    return 0;
}

// 路由回调函数
static int print_route(const struct route_entry *entry, void *arg) {
    printf("%-18s %-18s %-10s %5d\n",
        addr_ntoa(&entry->route_dst),
        addr_ntoa(&entry->route_gw),
        entry->intf_name,
        entry->metric);
    return 0;
}

// ARP回调函数
static int print_arp(const struct arp_entry *entry, void *arg) {
    printf("%-18s %-18s\n",
        addr_ntoa(&entry->arp_pa),
        addr_ntoa(&entry->arp_ha));
    return 0;
}

int main(void) {
    intf_t *intf;
    route_t *route;
    arp_t *arp;
    eth_t *eth;
    ip_t *ip;

    printf("=== libdnet Linux网络工具 ===\n");

    // 1. 枚举接口
    printf("\n1. 网络接口:\n");
    if ((intf = intf_open()) != NULL) {
        intf_loop(intf, print_interface, NULL);
        intf_close(intf);
    }

    // 2. 枚举路由表
    printf("\n2. 路由表:\n");
    printf("%-18s %-18s %-10s %5s\n", "目标", "网关", "接口", "度量");
    printf("------------------ ------------------ ---------- -----\n");
    if ((route = route_open()) != NULL) {
        route_loop(route, print_route, NULL);
        route_close(route);
    }

    // 3. 枚举ARP表
    printf("\n3. ARP表:\n");
    printf("%-18s %-18s\n", "IP地址", "MAC地址");
    printf("------------------ ------------------\n");
    if ((arp = arp_open()) != NULL) {
        arp_loop(arp, print_arp, NULL);
        arp_close(arp);
    }

    // 4. 发送以太网帧
    printf("\n4. 以太网帧发送:\n");
    if ((eth = eth_open("eth0")) != NULL) {
        u_char frame[128];
        struct eth_addr_t src_mac, dst_mac;
        
        // 获取源MAC
        eth_get(eth, &src_mac);
        printf("  源MAC: %s\n", eth_ntoa(&src_mac));
        
        // 构造以太网帧
        eth_addr_aton("ff:ff:ff:ff:ff:ff", &dst_mac);
        memcpy(frame, dst_mac.data, 6);
        memcpy(frame + 6, src_mac.data, 6);
        *(uint16_t *)(frame + 12) = htons(0x1234);
        memset(frame + 14, 0xAA, 64 - 14);
        
        // 发送
        eth_send(eth, frame, 64);
        printf("  发送完成\n");
        eth_close(eth);
    }

    // 5. 发送IP数据包
    printf("\n5. IP数据包发送:\n");
    if ((ip = ip_open()) != NULL) {
        u_char pkt[128];
        struct ip_hdr *iph;
        
        // 构造IP头
        iph = (struct ip_hdr *)pkt;
        iph->ip_v = 4;
        iph->ip_hl = 5;
        iph->ip_tos = 0;
        iph->ip_len = htons(64);
        iph->ip_id = htons(0x1234);
        iph->ip_off = 0;
        iph->ip_ttl = 64;
        iph->ip_p = IP_PROTO_ICMP;
        iph->ip_sum = 0;
        iph->ip_src = inet_addr("192.168.1.100");
        iph->ip_dst = inet_addr("192.168.1.1");
        
        // 填充数据
        memset(pkt + 20, 'X', 44);
        
        // 发送
        ip_send(ip, pkt, 64);
        printf("  发送完成\n");
        ip_close(ip);
    }

    printf("\n=== 完成 ===\n");
    return 0;
}
```

### A.2 跨平台抽象示例

```c
/*
 * 平台特定头文件
 */

#ifdef __linux__
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <net/if.h>
    #include <netpacket/packet.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <linux/if_ether.h>
    #define PLATFORM_LINUX
#elif defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #include <sys/socket.h>
    <sys/sockio.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define PLATFORM_BSD
#endif

/*
 * 平台特定宏
 */

#ifdef PLATFORM_LINUX
    #define SOCKET_INVALID -1
    #define socket_close close
    #define socket_errno errno
#elif defined(PLATFORM_WINDOWS)
    #define SOCKET_INVALID INVALID_SOCKET
    #define socket_close closesocket
    #define socket_errno WSAGetLastError()
#elif defined(PLATFORM_BSD)
    #define SOCKET_INVALID -1
    #define socket_close close
    #define socket_errno errno
#endif

/*
 * 初始化函数
 */

#ifdef PLATFORM_WINDOWS
int network_init(void) {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void network_cleanup(void) {
    WSACleanup();
}
#else
int network_init(void) {
    return 0;
}

void network_cleanup(void) {
    // 无需清理
}
#endif
```

---

## 总结

### Linux平台优势

1. **完整的网络访问**：
   - 支持以太网层完整功能
   - 原始套接字发送和接收
   - 完整的路由和ARP管理

2. **高效的实现**：
   - 直接访问/proc文件系统
   - Netlink双向通信
   - ioctl直接操作

3. **灵活的权限模型**：
   - Linux capabilities
   - 不需要root权限
   - 最小权限原则

### Linux与其他平台对比

| 特性 | Linux | BSD/macOS | Windows |
|------|-------|-----------|---------|
| 以太网功能 | 完整 | 完整 | 有限 |
| 原始套接字 | 完整 | 完整 | 受限 |
| 路由管理 | 完整 | 完整 | 完整 |
| ARP管理 | 完整 | 完整 | 完整 |
| 性能 | 高 | 高 | 中 |
| 权限 | 灵活 | 严格 | 严格 |
| 零依赖 | ✅ | ✅ | ❌ |

### 最佳实践

1. **使用capabilities**：避免root权限
2. **错误处理**：检查所有返回值
3. **资源管理**：及时关闭文件描述符
4. **性能优化**：利用缓冲区优化
5. **安全性**：最小权限原则
