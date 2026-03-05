# libdnet 源码中的 WinPcap 和 libpcap 操作深度分析

## 概述

libdnet（libdumbnet）是一个跨平台的网络工具库，提供了对网络层、传输层和数据链路层的底层访问。本文档深入分析了 libdnet 1.18.2 源码中使用的 WinPcap/Npcap 和 libpcap/BPF 相关操作、函数调用及其业务逻辑。

**关键发现**：libdnet 在 Windows 平台上直接使用 **WinPcap/Npcap 的 Packet.dll API**，而不是通过 pcap 接口。在 Unix 系统上使用 **BPF（Berkeley Packet Filter）** 或其他平台特定的数据包捕获机制。

---

## 一、Windows 平台 - WinPcap/Npcap 操作

### 1.1 相关源文件

- `src/eth-win32.c` - Windows 平台以太网操作
- `src/intf-win32.c` - Windows 平台网络接口操作
- `src/ip-win32.c` - Windows 平台 IP 层操作
- `src/fw-pktfilter.c` - Windows 防火墙操作（使用 PktFilter）
- `src/route-win32.c` - Windows 路由操作

### 1.2 使用的 WinPcap/Npcap API 列表

#### 1.2.1 Packet.dll 核心函数

| 函数名 | 功能 | 调用位置 |
|--------|------|----------|
| `PacketGetAdapterNames` | 获取网络适配器名称列表 | `eth-win32.c:49` |
| `PacketOpenAdapter` | 打开指定的网络适配器 | `eth-win32.c:65` |
| `PacketCloseAdapter` | 关闭网络适配器 | `eth-win32.c:74,99` |
| `PacketSetBuff` | 设置接收缓冲区大小 | `eth-win32.c:70` |
| `PacketAllocatePacket` | 分配数据包结构 | `eth-win32.c:71` |
| `PacketFreePacket` | 释放数据包结构 | `eth-win32.c:97` |
| `PacketInitPacket` | 初始化数据包 | `eth-win32.c:87` |
| `PacketSendPacket` | 发送数据包 | `eth-win32.c:88` |
| `PacketRequest` | 发送 OID 请求（获取/设置适配器信息） | `eth-win32.c:115,133` |

#### 1.2.2 Windows 网络 API（辅助函数）

| 函数名 | 功能 | 调用位置 |
|--------|------|----------|
| `GetAdaptersInfo` | 获取适配器信息 | `fw-pktfilter.c:327` |
| `GetIfTable` | 获取接口表 | `intf-win32.c:178` |
| `GetIfEntry` | 获取特定接口条目 | `intf-win32.c:238` |
| `GetIpAddrTable` | 获取 IP 地址表 | `intf-win32.c:189` |
| `GetBestInterface` | 获取最佳输出接口 | `route-win32.c:38` |
| `GetIpForwardTable` | 获取 IP 路由表 | `route-win32.c:123` |

### 1.3 详细函数调用分析

#### 1.3.1 `eth_open()` - 打开以太网设备

**文件**: `src/eth-win32.c:27-82`

**业务逻辑**:

1. **获取接口信息**:
   ```c
   intf_t *intf = intf_open();
   strlcpy(ifent.intf_name, device, sizeof(ifent.intf_name));
   intf_get(intf, &ifent);
   ```
   - 调用 `intf_open()` 打开接口
   - 通过 `intf_get()` 获取接口的 MAC 地址

2. **枚举 Packet 适配器**:
   ```c
   buf = NULL;
   PacketGetAdapterNames(buf, &len);  // 第一次调用获取所需缓冲区大小
   buf = malloc(len);
   PacketGetAdapterNames(buf, &len);  // 第二次调用获取实际数据
   ```
   - 使用 `PacketGetAdapterNames()` 获取系统中所有 WinPcap 可见的适配器
   - 返回的数据是多字符串格式，以 `\0\0` 结尾

3. **查找匹配的适配器**:
   ```c
   for (p = buf; *p != '\0'; p += strlen(p) + 1) {
       eth->lpa = PacketOpenAdapter(p);
       if (eth->lpa->hFile != INVALID_HANDLE_VALUE &&
           eth_get(eth, &ea) == 0 &&
           memcmp(&ea, &ifent.intf_link_addr.addr_eth, ETH_ADDR_LEN) == 0) {
           PacketSetBuff(eth->lpa, 512000);
           eth->pkt = PacketAllocatePacket();
           break;
       }
   }
   ```
   - 遍历所有适配器名称
   - 使用 `PacketOpenAdapter()` 打开每个适配器
   - 通过 `eth_get()` 读取适配器 MAC 地址
   - 与目标接口的 MAC 地址比较，找到匹配项
   - 设置 512KB 的内核缓冲区 (`PacketSetBuff`)
   - 分配数据包结构 (`PacketAllocatePacket`)

4. **清理不匹配的适配器**:
   ```c
   PacketCloseAdapter(eth->lpa);
   ```

**使用的 WinPcap 函数**:
- `PacketGetAdapterNames()` - 枚举适配器
- `PacketOpenAdapter()` - 打开适配器
- `PacketSetBuff()` - 设置缓冲区
- `PacketAllocatePacket()` - 分配包结构
- `PacketCloseAdapter()` - 关闭适配器

---

#### 1.3.2 `eth_send()` - 发送以太网帧

**文件**: `src/eth-win32.c:84-90`

**业务逻辑**:
```c
PacketInitPacket(eth->pkt, (void *)buf, len);
PacketSendPacket(eth->lpa, eth->pkt, TRUE);
return (len);
```

1. **初始化数据包**:
   - `PacketInitPacket()` 将用户数据缓冲区和长度绑定到 PACKET 结构

2. **发送数据包**:
   - `PacketSendPacket()` 实际发送数据包
   - 第三个参数 `TRUE` 表示同步发送（等待发送完成）
   - 返回 `len` 表示成功发送的字节数

**使用的 WinPcap 函数**:
- `PacketInitPacket()` - 初始化包结构
- `PacketSendPacket()` - 发送包

---

#### 1.3.3 `eth_close()` - 关闭以太网设备

**文件**: `src/eth-win32.c:92-103`

**业务逻辑**:
```c
if (eth->pkt != NULL)
    PacketFreePacket(eth->pkt);
if (eth->lpa != NULL)
    PacketCloseAdapter(eth->lpa);
free(eth);
```

1. **释放数据包结构**:
   - `PacketFreePacket()` 释放之前分配的 PACKET 结构

2. **关闭适配器**:
   - `PacketCloseAdapter()` 关闭适配器句柄

3. **释放内存**:
   - 释放 eth_t 结构本身

**使用的 WinPcap 函数**:
- `PacketFreePacket()` - 释放包结构
- `PacketCloseAdapter()` - 关闭适配器

---

#### 1.3.4 `eth_get()` - 获取接口 MAC 地址

**文件**: `src/eth-win32.c:105-120`

**业务逻辑**:
```c
PACKET_OID_DATA *data = (PACKET_OID_DATA *)buf;
data->Oid = OID_802_3_CURRENT_ADDRESS;
data->Length = ETH_ADDR_LEN;

if (PacketRequest(eth->lpa, FALSE, data) == TRUE) {
    memcpy(ea, data->Data, ETH_ADDR_LEN);
    return (0);
}
```

1. **构造 OID 请求**:
   - `OID_802_3_CURRENT_ADDRESS` 是标准的 NDIS OID，用于查询当前 MAC 地址
   - 设置请求的数据长度为 6 字节（ETH_ADDR_LEN）

2. **发送查询请求**:
   - `PacketRequest()` 发送 ODI 请求
   - 第二个参数 `FALSE` 表示这是查询操作（GET），不是设置（SET）

3. **复制结果**:
   - 将返回的 MAC 地址复制到用户提供的缓冲区

**使用的 WinPcap 函数**:
- `PacketRequest()` - 发送 OID 查询请求

**相关常量**:
- `OID_802_3_CURRENT_ADDRESS` (0x01010102) - IEEE 802.3 当前物理地址

---

#### 1.3.5 `eth_set()` - 设置接口 MAC 地址

**文件**: `src/eth-win32.c:122-137`

**业务逻辑**:
```c
PACKET_OID_DATA *data = (PACKET_OID_DATA *)buf;
data->Oid = OID_802_3_CURRENT_ADDRESS;
memcpy(data->Data, ea, ETH_ADDR_LEN);
data->Length = ETH_ADDR_LEN;

if (PacketRequest(eth->lpa, TRUE, data) == TRUE)
    return (0);
```

1. **构造 OID 请求**:
   - 使用相同的 `OID_802_3_CURRENT_ADDRESS` OID
   - 将用户提供的 MAC 地址复制到请求的数据区

2. **发送设置请求**:
   - `PacketRequest()` 发送 ODI 请求
   - 第二个参数 `TRUE` 表示这是设置操作（SET），不是查询（GET）

**使用的 WinPcap 函数**:
- `PacketRequest()` - 发送 OID 设置请求

---

### 1.4 Windows 平台编译配置

#### 1.4.1 configure.ac 中的 WinPcap 配置

**文件**: `configure.ac:92-157`

```autoconf
# Set defaults for WPDPACK_DIR
WPDPACK_DIR='../../WPdpack'

if test "$CYGWIN" = yes ; then
    AC_MSG_CHECKING(for WinPcap developer's pack)
    AC_ARG_WITH(wpdpack,
        [  --with-wpdpack=DIR      use WinPcap developer's pack in DIR],
        [ AC_MSG_RESULT($withval)
          if test -f $withval/include/packet32.h -a -f $withval/lib/packet.lib; then
             WPDPACK_DIR=$withval
             CFLAGS="$CFLAGS -I$withval/include"
             LIBS="$LIBS -L$withval/lib -lpacket"
          else
             AC_MSG_ERROR(packet32.h or packet.lib not found in $withval)
          fi ],
        [ for dir in ${prefix} ${HOME}/WPdpack ; do
             if test -f ${dir}/include/packet32.h -a -f ${dir}/lib/packet.lib; then
                WPDPACK_DIR=$dir
                CFLAGS="$CFLAGS -I${dir}/include"
                LIBS="$LIBS -L${dir}/lib -lpacket"
                have_pcap=yes
                break;
             fi
          done
          if test "$have_pcap" != yes; then
             AC_MSG_ERROR(WinPcap developer's pack not found)
          fi ])
fi
```

**配置项说明**:
- 检查 `packet32.h` 头文件是否存在
- 检查 `packet.lib` 库文件是否存在
- 自动搜索 `${prefix}` 或 `${HOME}/WPdpack` 目录
- 将 WinPcap 的 include 和 lib 路径添加到编译选项
- 链接 `packet.lib` 库

#### 1.4.2 CMakeLists.txt 中的 Npcap 配置

**文件**: `CMakeLists.txt:40-48`

```cmake
if (NPCAP_SDK)
    get_filename_component(NPCAP_SDK "${NPCAP_SDK}" ABSOLUTE)
    set(NPCAP_SDK_INCLUDE "${NPCAP_SDK}/Include")
    set(CMAKE_REQUIRED_INCLUDES ${NPCAP_SDK_INCLUDE})
    check_include_file(Packet32.h HAVE_PACKET32_H)
    set(CMAKE_REQUIRED_INCLUDES )
else()
    message(WARNING "NPCAP_SDK not specified; building without Npcap support")
endif()
```

**文件**: `CMakeLists.txt:203-219`

```cmake
if (HAVE_PACKET32_H)
    list(APPEND PLATFORM_SOURCES src/eth-win32.c)
elseif(HAVE_NET_PFILT_H)
    list(APPEND PLATFORM_SOURCES src/eth-pfilt.c)
elseif(HAVE_LINUX_PF_PACKET)
    list(APPEND PLATFORM_SOURCES src/eth-linux.c)
...
endif()
```

**文件**: `CMakeLists.txt:339-344`

```cmake
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC Iphlpapi ws2_32)
    if (HAVE_PACKET32_H)
        target_include_directories(${PROJECT_NAME} PUBLIC  ${NPCAP_SDK_INCLUDE})
        target_link_directories(${PROJECT_NAME} PUBLIC ${NPCAP_SDK}/Libs/)
        target_link_libraries(${PROJECT_NAME} PUBLIC Packet)
    endif()
endif()
```

**配置说明**:
- 支持 Npcap SDK（WinPcap 的现代替代品）
- 通过 `NPCAP_SDK` 环境变量指定 SDK 路径
- 检查 `Packet32.h` 头文件是否存在
- 链接 `Packet.lib`（Npcap 提供的 Packet.dll 导入库）
- 同时链接 `Iphlpapi` 和 `ws2_32` 等系统库

---

## 二、Unix 平台 - libpcap/BPF 操作

### 2.1 相关源文件

| 平台 | 文件 | 说明 |
|------|------|------|
| BSD/macOS | `src/eth-bsd.c` | 使用 BPF（Berkeley Packet Filter） |
| Linux | `src/eth-linux.c` | 使用 PF_PACKET 套接字 |
| Solaris | `src/eth-dlpi.c` | 使用 DLPI（Data Link Provider Interface） |
| HP-UX | `src/eth-ndd.c` | 使用 NDD（Network Device Driver） |
| 其他 | `src/eth-none.c` | 空实现（不支持） |

### 2.2 BSD/macOS - BPF 操作

#### 2.2.1 使用的 BPF ioctl 命令

| ioctl 命令 | 功能 | 调用位置 |
|------------|------|----------|
| `BIOCSETIF` | 绑定到指定网络接口 | `eth-bsd.c:53` |
| `BIOCSHDRCMPLT` | 设置是否由内核补全以太网头部 | `eth-bsd.c:57` |
| `BIOCGIFLIST` | 获取接口列表（用于获取 MAC） | 通过 sysctl 间接使用 |

#### 2.2.2 `eth_open()` - 使用 BPF 打开设备

**文件**: `src/eth-bsd.c:39-63`

```c
if ((e = calloc(1, sizeof(*e))) != NULL) {
    if ((e->fd = open("/dev/bpf0", O_WRONLY)) < 0)
        return (eth_close(e));

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

    if (ioctl(e->fd, BIOCSETIF, (char *)&ifr) < 0)
        return (eth_close(e));
#ifdef BIOCSHDRCMPLT
    i = 1;
    if (ioctl(e->fd, BIOCSHDRCMPLT, &i) < 0)
        return (eth_close(e));
#endif
    strlcpy(e->device, device, sizeof(e->device));
}
```

**业务逻辑**:

1. **打开 BPF 设备**:
   - 尝试打开 `/dev/bpf0`，如果失败会尝试 `/dev/bpf1` 等（由内核自动分配）
   - 使用 `O_WRONLY` 表示只写（只用于发送）

2. **绑定到网络接口**:
   - `BIOCSETIF` ioctl 将 BPF 设备绑定到指定的网络接口
   - 通过 `ifr.ifr_name` 指定接口名称（如 "em0"）

3. **设置头部补全标志**:
   - `BIOCSHDRCMPLT` 设置为 1，表示需要用户提供完整的以太网头部
   - 如果不设置，内核会自动补全源 MAC 地址

**使用的 BPF 函数**:
- `open()` - 打开 BPF 设备文件
- `ioctl()` - 配置 BPF 设备
- `BIOCSETIF` - 绑定接口
- `BIOCSHDRCMPLT` - 设置头部补全模式

---

#### 2.2.3 `eth_send()` - 使用 BPF 发送帧

**文件**: `src/eth-bsd.c:65-69`

```c
ssize_t
eth_send(eth_t *e, const void *buf, size_t len)
{
    return (write(e->fd, buf, len));
}
```

**业务逻辑**:
- 直接使用 `write()` 系统调用将完整的以太网帧写入 BPF 设备
- 内核会将帧发送到绑定的网络接口
- 缓冲区必须包含完整的以太网头部（14 字节）

**使用的 BPF 函数**:
- `write()` - 向 BPF 设备写入数据

---

#### 2.2.4 `eth_get()` - 获取 MAC 地址（通过 sysctl）

**文件**: `src/eth-bsd.c:82-135`

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

    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return (-1);

    if ((buf = malloc(len)) == NULL)
        return (-1);

    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        free(buf);
        return (-1);
    }
    for (p = buf; p < buf + len; p += ifm->ifm_msglen) {
        ifm = (struct if_msghdr *)p;
        sdl = (struct sockaddr_dl *)(ifm + 1);

        if (ifm->ifm_type != RTM_IFINFO ||
            (ifm->ifm_addrs & RTA_IFP) == 0)
            continue;

        if (sdl->sdl_family != AF_LINK || sdl->sdl_nlen == 0 ||
            memcmp(sdl->sdl_data, e->device, sdl->sdl_nlen) != 0)
            continue;

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

**业务逻辑**:

1. **准备 sysctl 请求**:
   ```c
   int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 };
   ```
   - `CTL_NET` - 网络子系统
   - `AF_ROUTE` - 路由套接字
   - `AF_LINK` - 链路层地址
   - `NET_RT_IFLIST` - 获取接口列表

2. **第一次调用获取缓冲区大小**:
   - `sysctl(mib, 6, NULL, &len, NULL, 0)` - 获取所需缓冲区大小

3. **分配缓冲区并第二次调用获取实际数据**:
   - `sysctl(mib, 6, buf, &len, NULL, 0)` - 获取接口列表数据

4. **解析返回的数据**:
   - 遍历 `if_msghdr` 结构数组
   - 查找 `RTM_IFINFO` 类型的消息
   - 检查接口名称是否匹配
   - 提取 `sockaddr_dl`（链路层地址）结构中的 MAC 地址

5. **复制 MAC 地址**:
   - 将找到的 MAC 地址复制到用户缓冲区

**使用的系统函数**:
- `sysctl()` - 查询内核网络接口信息
- `malloc()` / `free()` - 内存管理

---

#### 2.2.5 `eth_set()` - 设置 MAC 地址（通过 ioctl）

**文件**: `src/eth-bsd.c:137-161`

```c
#if defined(SIOCSIFLLADDR)
int
eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct ifreq ifr;
    struct addr ha;

    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, e->device, sizeof(ifr.ifr_name));
    addr_ntos(&ha, &ifr.ifr_addr);

    return (ioctl(e->fd, SIOCSIFLLADDR, &ifr));
}
#else
int
eth_set(eth_t *e, const eth_addr_t *ea)
{
    errno = ENOSYS;
    return (-1);
}
#endif
```

**业务逻辑**:

1. **检查系统支持**:
   - 如果定义了 `SIOCSIFLLADDR`，表示系统支持设置链路层地址
   - 否则返回 `ENOSYS` 错误

2. **构造 ifreq 结构**:
   - 设置接口名称
   - 将 MAC 地址转换为 `sockaddr` 格式

3. **发送设置请求**:
   - 使用 `ioctl()` 发送 `SIOCSIFLLADDR` 命令
   - 修改接口的 MAC 地址

**使用的系统函数**:
- `ioctl()` - 配置网络接口
- `SIOCSIFLLADDR` - 设置链路层地址

---

### 2.3 Linux - PF_PACKET 操作

#### 2.3.1 `eth_open()` - 使用 PF_PACKET 套接字

**文件**: `src/eth-linux.c:42-67`

```c
if ((e = calloc(1, sizeof(*e))) != NULL) {
    if ((e->fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
        return (eth_close(e));
#ifdef SO_BROADCAST
    n = 1;
    if (setsockopt(e->fd, SOL_SOCKET, SO_BROADCAST, &n,
        sizeof(n)) < 0)
        return (eth_close(e));
#endif
    strlcpy(e->ifr.ifr_name, device, sizeof(e->ifr.ifr_name));

    if (ioctl(e->fd, SIOCGIFINDEX, &e->ifr) < 0)
        return (eth_close(e));

    e->sll.sll_family = AF_PACKET;
    e->sll.sll_ifindex = e->ifr.ifr_ifindex;
}
```

**业务逻辑**:

1. **创建原始套接字**:
   ```c
   socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
   ```
   - `PF_PACKET` - 数据包层套接字
   - `SOCK_RAW` - 原始套接字（可以发送完整的以太网帧）
   - `ETH_P_ALL` - 接收所有协议类型的帧

2. **启用广播**:
   ```c
   setsockopt(e->fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n))
   ```
   - 允许发送广播帧

3. **获取接口索引**:
   ```c
   ioctl(e->fd, SIOCGIFINDEX, &e->ifr)
   ```
   - 使用 `SIOCGIFINDEX` 获取接口的索引号

4. **设置套接字地址**:
   ```c
   e->sll.sll_family = AF_PACKET;
   e->sll.sll_ifindex = e->ifr.ifr_ifindex;
   ```
   - 准备用于 `sendto()` 的目标地址结构

**使用的系统函数**:
- `socket()` - 创建 PF_PACKET 套接字
- `setsockopt()` - 设置套接字选项
- `ioctl()` - 获取接口信息

---

#### 2.3.2 `eth_send()` - 使用 sendto() 发送帧

**文件**: `src/eth-linux.c:69-78`

```c
ssize_t
eth_send(eth_t *e, const void *buf, size_t len)
{
    struct eth_hdr *eth = (struct eth_hdr *)buf;

    e->sll.sll_protocol = eth->eth_type;

    return (sendto(e->fd, buf, len, 0, (struct sockaddr *)&e->sll,
        sizeof(e->sll)));
}
```

**业务逻辑**:

1. **提取以太网类型**:
   - 从以太网头部提取 `eth_type` 字段
   - 设置到 `sockaddr_ll` 的 `sll_protocol` 字段

2. **发送数据包**:
   - 使用 `sendto()` 发送完整的以太网帧
   - 目标地址是 `sockaddr_ll` 结构（链路层地址）

**使用的系统函数**:
- `sendto()` - 发送数据包到指定接口

---

#### 2.3.3 `eth_get()` - 获取 MAC 地址

**文件**: `src/eth-linux.c:91-104`

```c
int
eth_get(eth_t *e, eth_addr_t *ea)
{
    struct addr ha;

    if (ioctl(e->fd, SIOCGIFHWADDR, &e->ifr) < 0)
        return (-1);

    if (addr_ston(&e->ifr.ifr_hwaddr, &ha) < 0)
        return (-1);

    memcpy(ea, &ha.addr_eth, sizeof(*ea));
    return (0);
}
```

**业务逻辑**:

1. **查询硬件地址**:
   ```c
   ioctl(e->fd, SIOCGIFHWADDR, &e->ifr)
   ```
   - `SIOCGIFHWADDR` - 获取接口硬件地址（MAC 地址）

2. **转换地址格式**:
   - 将 `sockaddr` 格式转换为内部 `addr_t` 格式

3. **复制 MAC 地址**:
   - 复制到用户提供的缓冲区

**使用的系统函数**:
- `ioctl()` - 获取硬件地址

---

#### 2.3.4 `eth_set()` - 设置 MAC 地址

**文件**: `src/eth-linux.c:106-118`

```c
int
eth_set(eth_t *e, const eth_addr_t *ea)
{
    struct addr ha;

    ha.addr_type = ADDR_TYPE_ETH;
    ha.addr_bits = ETH_ADDR_BITS;
    memcpy(&ha.addr_eth, ea, ETH_ADDR_LEN);

    addr_ntos(&ha, &e->ifr.ifr_hwaddr);

    return (ioctl(e->fd, SIOCSIFHWADDR, &e->ifr));
}
```

**业务逻辑**:

1. **构造地址结构**:
   - 设置地址类型、位数和 MAC 地址值

2. **转换地址格式**:
   - 将内部 `addr_t` 格式转换为 `sockaddr` 格式

3. **设置硬件地址**:
   ```c
   ioctl(e->fd, SIOCSIFHWADDR, &e->ifr)
   ```
   - `SIOCSIFHWADDR` - 设置接口硬件地址

**使用的系统函数**:
- `ioctl()` - 设置硬件地址

---

### 2.4 Solaris - DLPI 操作

#### 2.4.1 使用的 DLPI 原语

| DLPI 原语 | 功能 | 调用位置 |
|-----------|------|----------|
| `DL_INFO_REQ` | 请求提供者信息 | `eth-dlpi.c:169` |
| `DL_INFO_ACK` | 提供者信息确认 | `eth-dlpi.c:172` |
| `DL_ATTACH_REQ` | 附加到 PPA | `eth-dlpi.c:178` |
| `DL_OK_ACK` | 操作成功确认 | `eth-dlpi.c:182` |
| `DL_BIND_REQ` | 绑定到 SAP | `eth-dlpi.c:186` |
| `DL_BIND_ACK` | 绑定确认 | `eth-dlpi.c:194` |
| `DL_PHYS_ADDR_REQ` | 请求物理地址 | `eth-dlpi.c:274` |
| `DL_PHYS_ADDR_ACK` | 物理地址确认 | `eth-dlpi.c:278` |
| `DL_SET_PHYS_ADDR_REQ` | 设置物理地址 | `eth-dlpi.c:293` |

#### 2.4.2 `eth_open()` - 使用 DLPI 打开设备

**文件**: `src/eth-dlpi.c:132-202`

```c
if ((e = calloc(1, sizeof(*e))) == NULL)
    return (NULL);

#ifdef HAVE_SYS_DLPIHDR_H
if ((e->fd = open("/dev/streams/dlb", O_RDWR)) < 0)
    return (eth_close(e));

if ((ppa = eth_match_ppa(e, device)) < 0) {
    errno = ESRCH;
    return (eth_close(e));
}
#else
e->fd = -1;
snprintf(dev, sizeof(dev), "/dev/%s", device);
if ((p = dev_find_ppa(dev)) == NULL) {
    errno = EINVAL;
    return (eth_close(e));
}
ppa = atoi(p);
*p = '\0';

if ((e->fd = open(dev, O_RDWR)) < 0) {
    snprintf(dev, sizeof(dev), "/dev/%s", device);
    if ((e->fd = open(dev, O_RDWR)) < 0)
        return (eth_close(e));
}
#endif
```

**业务逻辑**:

1. **打开 DLPI 设备**:
   - 打开 `/dev/streams/dlb` 或 `/dev/<device>` 设备文件
   - 获取 PPA（Physical Point of Attachment，物理附加点）

2. **发送 DL_INFO_REQ**:
   ```c
   dlp->info_req.dl_primitive = DL_INFO_REQ;
   dlpi_msg(e->fd, dlp, DL_INFO_REQ_SIZE, RS_HIPRI,
       DL_INFO_ACK, DL_INFO_ACK_SIZE, sizeof(buf))
   ```
   - 查询提供者（driver）信息
   - 获取 SAP（Service Access Point）长度

3. **检查提供者风格**:
   ```c
   if (dlp->info_ack.dl_provider_style == DL_STYLE2) {
       dlp->attach_req.dl_primitive = DL_ATTACH_REQ;
       dlp->attach_req.dl_ppa = ppa;
       dlpi_msg(e->fd, dlp, DL_ATTACH_REQ_SIZE, 0,
           DL_OK_ACK, DL_OK_ACK_SIZE, sizeof(buf))
   }
   ```
   - 如果是 STYLE2，需要先附加到 PPA

4. **绑定到 SAP**:
   ```c
   dlp->bind_req.dl_primitive = DL_BIND_REQ;
   dlp->bind_req.dl_sap = DL_ETHER;
   dlp->bind_req.dl_service_mode = DL_CLDLS;
   dlpi_msg(e->fd, dlp, DL_BIND_REQ_SIZE, 0,
       DL_BIND_ACK, DL_BIND_ACK_SIZE, sizeof(buf))
   ```
   - 绑定到以太网 SAP
   - 使用 CLDLS（无连接数据链路服务）

5. **进入原始模式**:
   ```c
   #ifdef DLIOCRAW
   strioctl(e->fd, DLIOCRAW, 0, NULL)
   #endif
   ```
   - 启用原始模式，发送完整的以太网帧

**使用的系统函数**:
- `open()` - 打开 DLPI 设备
- `putmsg()` / `getmsg()` - 发送/接收 STREAMS 消息
- `ioctl()` - DLIOCRAW ioctl

---

#### 2.4.3 `eth_send()` - 使用 DLPI 发送帧

**文件**: `src/eth-dlpi.c:204-254`

```c
#ifdef DLIOCRAW
return (write(e->fd, buf, len));
#else
dlp->unitdata_req.dl_primitive = DL_UNITDATA_REQ;
dlp->unitdata_req.dl_dest_addr_length = ETH_ADDR_LEN;
dlp->unitdata_req.dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
dlp->unitdata_req.dl_priority.dl_min =
    dlp->unitdata_req.dl_priority.dl_max = 0;

eth = (struct eth_hdr *)buf;
*(uint16_t *)sap = ntohs(eth->eth_type);

ctl.maxlen = 0;
ctl.len = dlen + ETH_ADDR_LEN + abs(e->sap_len);
ctl.buf = (char *)ctlbuf;

if (e->sap_len >= 0) {
    memcpy(ctlbuf + dlen, sap, e->sap_len);
    memcpy(ctlbuf + dlen + e->sap_len,
        eth->eth_dst.data, ETH_ADDR_LEN);
} else {
    memcpy(ctlbuf + dlen, eth->eth_dst.data, ETH_ADDR_LEN);
    memcpy(ctlbuf + dlen + ETH_ADDR_LEN, sap, abs(e->sap_len));
}
data.maxlen = 0;
data.len = len;
data.buf = (char *)buf;

if (putmsg(e->fd, &ctl, &data, 0) < 0)
    return (-1);

return (len);
#endif
```

**业务逻辑**:

1. **检查是否在原始模式**:
   - 如果已启用 `DLIOCRAW`，直接使用 `write()` 发送
   - 否则需要使用 DLPI 消息格式

2. **构造 DL_UNITDATA_REQ 消息**:
   - 设置原语类型
   - 设置目的地址（MAC 地址）
   - 设置以太网类型（SAP）

3. **发送消息**:
   - 使用 `putmsg()` 发送控制消息和数据

**使用的系统函数**:
- `write()` - 直接写入（原始模式）
- `putmsg()` - 发送 STREAMS 消息

---

#### 2.4.4 `eth_get()` - 获取物理地址

**文件**: `src/eth-dlpi.c:267-284`

```c
dlp->physaddr_req.dl_primitive = DL_PHYS_ADDR_REQ;
dlp->physaddr_req.dl_addr_type = DL_CURR_PHYS_ADDR;

if (dlpi_msg(e->fd, dlp, DL_PHYS_ADDR_REQ_SIZE, 0,
    DL_PHYS_ADDR_ACK, DL_PHYS_ADDR_ACK_SIZE, sizeof(buf)) < 0)
    return (-1);

memcpy(ea, buf + dlp->physaddr_ack.dl_addr_offset, sizeof(*ea));
```

**业务逻辑**:

1. **构造 DL_PHYS_ADDR_REQ**:
   - 设置原语类型为获取物理地址
   - 设置地址类型为当前物理地址

2. **发送请求**:
   - 等待 `DL_PHYS_ADDR_ACK` 确认

3. **提取地址**:
   - 从确认消息的偏移位置复制 MAC 地址

**使用的系统函数**:
- `putmsg()` / `getmsg()` - 发送/接收 DLPI 消息

---

#### 2.4.5 `eth_set()` - 设置物理地址

**文件**: `src/eth-dlpi.c:286-301`

```c
dlp->set_physaddr_req.dl_primitive = DL_SET_PHYS_ADDR_REQ;
dlp->set_physaddr_req.dl_addr_length = ETH_ADDR_LEN;
dlp->set_physaddr_req.dl_addr_offset = DL_SET_PHYS_ADDR_REQ_SIZE;

memcpy(buf + DL_SET_PHYS_ADDR_REQ_SIZE, ea, sizeof(*ea));

return (dlpi_msg(e->fd, dlp, DL_SET_PHYS_ADDR_REQ_SIZE + ETH_ADDR_LEN,
    0, DL_OK_ACK, DL_OK_ACK_SIZE, sizeof(buf)));
```

**业务逻辑**:

1. **构造 DL_SET_PHYS_ADDR_REQ**:
   - 设置原语类型
   - 设置地址长度和偏移

2. **复制地址数据**:
   - 将新 MAC 地址复制到消息中

3. **发送请求**:
   - 等待 `DL_OK_ACK` 确认

**使用的系统函数**:
- `putmsg()` / `getmsg()` - 发送/接收 DLPI 消息

---

## 三、平台对比总结

### 3.1 数据包发送机制对比

| 平台 | 实现方式 | 核心函数 | 特点 |
|------|----------|----------|------|
| **Windows** | WinPcap/Npcap Packet.dll | `PacketSendPacket()` | 需要预初始化包结构，同步发送 |
| **BSD/macOS** | BPF | `write()` 到 `/dev/bpf*` | 需要绑定接口，简单直接 |
| **Linux** | PF_PACKET | `sendto()` | 需要构造 `sockaddr_ll` 地址 |
| **Solaris** | DLPI | `putmsg()` 或 `write()` | 复杂的流式消息接口 |

### 3.2 MAC 地址获取方式对比

| 平台 | 获取方式 | 核心函数/OID/IOCTL |
|------|----------|-------------------|
| **Windows** | NDIS OID 查询 | `PacketRequest()` + `OID_802_3_CURRENT_ADDRESS` |
| **BSD/macOS** | sysctl 查询 | `sysctl()` + `NET_RT_IFLIST` |
| **Linux** | ioctl 查询 | `ioctl()` + `SIOCGIFHWADDR` |
| **Solaris** | DLPI 查询 | `DL_PHYS_ADDR_REQ` 原语 |

### 3.3 MAC 地址设置方式对比

| 平台 | 设置方式 | 核心函数/OID/IOCTL |
|------|----------|-------------------|
| **Windows** | NDIS OID 设置 | `PacketRequest()` + `OID_802_3_CURRENT_ADDRESS` |
| **BSD/macOS** | ioctl 设置 | `ioctl()` + `SIOCSIFLLADDR` |
| **Linux** | ioctl 设置 | `ioctl()` + `SIOCSIFHWADDR` |
| **Solaris** | DLPI 设置 | `DL_SET_PHYS_ADDR_REQ` 原语 |

### 3.4 设备枚举方式对比

| 平台 | 枚举方式 | 核心函数 |
|------|----------|----------|
| **Windows** | WinPcap 适配器枚举 | `PacketGetAdapterNames()` |
| **BSD/macOS** | sysctl 查询 | `sysctl()` + `NET_RT_IFLIST` |
| **Linux** | /proc/net/dev | 读取 `/proc` 文件系统 |
| **Solaris** | DLPI 设备 | 打开 `/dev/*` 设备文件 |

---

## 四、业务逻辑总结

### 4.1 核心业务流程

libdnet 的以太网操作遵循以下通用流程：

1. **打开设备** (`eth_open()`):
   - 枚举可用的网络接口
   - 查找与指定名称匹配的接口
   - 打开底层设备（Packet.dll/BPF/PF_PACKET/DLPI）
   - 配置设备参数（缓冲区大小、模式等）
   - 分配必要的资源

2. **发送数据包** (`eth_send()`):
   - 准备完整的以太网帧（包含 14 字节头部）
   - 调用平台特定的发送函数
   - 等待发送完成（同步模式）或立即返回（异步模式）

3. **获取/设置 MAC 地址** (`eth_get()`/`eth_set()`):
   - 构造查询/设置请求
   - 发送到底层驱动
   - 等待响应
   - 处理返回的数据

4. **关闭设备** (`eth_close()`):
   - 释放所有分配的资源
   - 关闭底层设备句柄
   - 清理内存

### 4.2 跨平台抽象

libdnet 通过以下机制实现跨平台抽象：

1. **编译时选择**:
   - 在 `configure.ac` 或 `CMakeLists.txt` 中检测平台特性
   - 只编译对应平台的源文件（如 `eth-win32.c` 或 `eth-bsd.c`）

2. **统一接口**:
   - 所有平台实现相同的函数签名：
     ```c
     eth_t *eth_open(const char *device);
     int eth_get(eth_t *e, eth_addr_t *ea);
     int eth_set(eth_t *e, const eth_addr_t *ea);
     ssize_t eth_send(eth_t *e, const void *buf, size_t len);
     eth_t *eth_close(eth_t *e);
     ```

3. **统一数据结构**:
   - 使用相同的 `eth_t` 句柄类型（平台相关实现）
   - 使用相同的 `eth_addr_t` MAC 地址类型

### 4.3 WinPcap/Npcap 特定优化

libdnet 在 Windows 平台上使用了以下 WinPcap/Npcap 特性：

1. **内核缓冲区优化**:
   ```c
   PacketSetBuff(eth->lpa, 512000);
   ```
   - 设置 512KB 的内核缓冲区，减少丢包

2. **预分配包结构**:
   ```c
   eth->pkt = PacketAllocatePacket();
   ```
   - 在打开设备时预先分配，避免重复分配开销

3. **OID 查询机制**:
   - 使用标准的 NDIS OIDs（Object Identifiers）
   - 支持查询和设置各种适配器参数

4. **同步发送模式**:
   ```c
   PacketSendPacket(eth->lpa, eth->pkt, TRUE);
   ```
   - 使用同步发送确保数据包发送完成

---

## 五、关键技术点

### 5.1 WinPcap Packet.dll API 说明

#### 5.1.1 核心数据结构

```c
// LPADAPTER - 适配器句柄
typedef struct _ADAPTER  *LPADAPTER;

// LPPACKET - 数据包结构
typedef struct _PACKET {
    HANDLE  hEvent;         // 事件句柄
    OVERLAPPED Overlapped; // 重叠结构
    PVOID   Buffer;         // 缓冲区指针
    UINT    Length;        // 缓冲区长度
}  LPPACKET, *LPPACKET;

// PACKET_OID_DATA - OID 请求/响应数据
typedef struct _PACKET_OID_DATA {
    ULONG Oid;             // OID 代码
    ULONG Length;          // 数据长度
    UCHAR Data[1];         // 数据（变长）
} PACKET_OID_DATA, *PPACKET_OID_DATA;
```

#### 5.1.2 常用 NDIS OIDs

| OID 名称 | 值 | 用途 |
|----------|-----|------|
| `OID_802_3_CURRENT_ADDRESS` | 0x01010102 | 获取/设置当前 MAC 地址 |
| `OID_802_3_PERMANENT_ADDRESS` | 0x01010101 | 获取永久（出厂）MAC 地址 |
| `OID_GEN_VENDOR_DESCRIPTION` | 0x00010107 | 获取厂商描述 |
| `OID_GEN_VENDOR_ID` | 0x00010106 | 获取厂商 ID |

### 5.2 BPF ioctl 命令说明

```c
// 绑定到指定网络接口
#define BIOCSETIF     _IOW('B', 2, struct ifreq)

// 设置内核缓冲区大小
#define BIOCSBLEN     _IOWR('B', 102, u_int)

// 设置是否补全以太网头部
#define BIOCSHDRCMPLT _IOW('B', 25, u_int)

// 获取链路层头部类型
#define BIOCGDLT      _IOR('B', 27, u_int)

// 设置读超时
#define BIOCSRTIMEOUT _IOW('B', 6, struct timeval)
```

### 5.3 Linux PF_PACKET 套接字

```c
// 创建 PF_PACKET 套接字
int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

// 链路层地址结构
struct sockaddr_ll {
    unsigned short sll_family;   // AF_PACKET
    unsigned short sll_protocol; // 以太网类型
    int            sll_ifindex; // 接口索引
    unsigned short sll_hatype;  // 硬件类型
    unsigned char  sll_pkttype; // 数据包类型
    unsigned char  sll_halen;   // 硬件地址长度
    unsigned char  sll_addr[8]; // 硬件地址
};
```

### 5.4 DLPI 原语类型

```c
// 信息请求
typedef struct {
    t_uscalar_t dl_primitive;   // DL_INFO_REQ
} dl_info_req_t;

// 附加请求
typedef struct {
    t_uscalar_t dl_primitive;   // DL_ATTACH_REQ
    t_uscalar_t dl_ppa;         // PPA 值
} dl_attach_req_t;

// 绑定请求
typedef struct {
    t_uscalar_t dl_primitive;   // DL_BIND_REQ
    t_uscalar_t dl_sap;          // SAP 值
    t_uscalar_t dl_max_conind;   // 最大并发连接数
    t_uscalar_t dl_service_mode; // 服务模式
    t_uscalar_t dl_conn_mgmt;    // 连接管理
    t_uscalar_t dl_xidtest;      // XID 测试
} dl_bind_req_t;
```

---

## 六、编译和依赖

### 6.1 Windows 平台依赖

#### 6.1.1 必需的库

| 库名称 | 用途 | 来源 |
|--------|------|------|
| `Packet.lib` | WinPcap/Npcap Packet.dll 导入库 | WinPcap/Npcap Developer's Pack |
| `ws2_32.lib` | Windows Sockets 2 | Windows SDK |
| `Iphlpapi.lib` | IP Helper API | Windows SDK |
| `Advapi32.lib` | 高级 API（某些功能需要） | Windows SDK |

#### 6.1.2 必需的头文件

| 头文件 | 用途 | 来源 |
|--------|------|------|
| `Packet32.h` | Packet.dll API 声明 | WinPcap/Npcap Developer's Pack |
| `Ntddndis.h` | NDIS 定义 | Windows Driver Kit |
| `ws2tcpip.h` | Winsock 2 | Windows SDK |
| `Iphlpapi.h` | IP Helper API | Windows SDK |

#### 6.1.3 配置环境变量

```bash
# 设置 Npcap SDK 路径
set NPCAP_SDK=C:\npcap-sdk

# 或使用 CMake
cmake -DNPCAP_SDK=C:\npcap-sdk ..
```

### 6.2 Unix 平台依赖

#### 6.2.1 BSD/macOS

- **内核配置**: 需要 BPF 支持（默认启用）
- **设备文件**: `/dev/bpf*` 设备文件
- **权限**: 需要适当权限访问 BPF 设备

#### 6.2.2 Linux

- **内核配置**: 需要 `CONFIG_PACKET` 和 `CONFIG_PACKET_MMAP`
- **系统调用**: `socket()` + `PF_PACKET`
- **权限**: 需要 `CAP_NET_RAW` 能力或 root 权限

#### 6.2.3 Solaris

- **内核配置**: 需要 DLPI 支持（默认启用）
- **设备文件**: `/dev/*` DLPI 设备文件
- **权限**: 需要适当权限访问设备文件

---

## 七、使用示例

### 7.1 Windows 平台示例

```c
#include <dnet.h>

int main() {
    eth_t *eth;
    eth_addr_t ea;
    struct eth_hdr *eth_hdr;
    char packet[1500];

    // 1. 打开以太网设备
    eth = eth_open("\\Device\\NPF_{GUID}");
    if (!eth) {
        perror("eth_open");
        return 1;
    }

    // 2. 获取 MAC 地址
    if (eth_get(eth, &ea) < 0) {
        perror("eth_get");
    } else {
        printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               ea.data[0], ea.data[1], ea.data[2],
               ea.data[3], ea.data[4], ea.data[5]);
    }

    // 3. 构造以太网帧
    eth_hdr = (struct eth_hdr *)packet;
    eth_pack_hdr(eth_hdr,
                 "\xff\xff\xff\xff\xff\xff",  // 广播 MAC
                 ea,                           // 源 MAC
                 ETH_TYPE_IP);                 // 以太网类型

    // 4. 发送数据包
    if (eth_send(eth, packet, 64) < 0) {
        perror("eth_send");
    }

    // 5. 关闭设备
    eth_close(eth);

    return 0;
}
```

### 7.2 Linux 平台示例

```c
#include <dnet.h>

int main() {
    eth_t *eth;
    eth_addr_t ea;
    struct eth_hdr *eth_hdr;
    char packet[1500];

    // 1. 打开以太网设备
    eth = eth_open("eth0");
    if (!eth) {
        perror("eth_open");
        return 1;
    }

    // 2. 获取 MAC 地址
    if (eth_get(eth, &ea) < 0) {
        perror("eth_get");
    } else {
        printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               ea.data[0], ea.data[1], ea.data[2],
               ea.data[3], ea.data[4], ea.data[5]);
    }

    // 3. 构造以太网帧
    eth_hdr = (struct eth_hdr *)packet;
    eth_pack_hdr(eth_hdr,
                 "\xff\xff\xff\xff\xff\xff",  // 广播 MAC
                 ea,                           // 源 MAC
                 ETH_TYPE_IP);                 // 以太网类型

    // 4. 发送数据包
    if (eth_send(eth, packet, 64) < 0) {
        perror("eth_send");
    }

    // 5. 关闭设备
    eth_close(eth);

    return 0;
}
```

---

## 八、总结

### 8.1 关键发现

1. **libdnet 不直接使用 libpcap API**:
   - 在 Windows 上使用 WinPcap/Npcap 的底层 Packet.dll API
   - 在 Unix 上使用系统原生机制（BPF、PF_PACKET、DLPI）
   - 没有使用 `pcap_open_live()`、`pcap_sendpacket()` 等 libpcap 函数

2. **专注于数据链路层操作**:
   - 主要提供以太网帧的发送功能
   - 支持获取和设置 MAC 地址
   - 不提供数据包捕获功能（这是 libpcap 的职责）

3. **跨平台抽象设计优秀**:
   - 统一的接口定义
   - 编译时选择平台实现
   - 最小化平台相关代码

### 8.2 WinPcap/Npcap 的使用特点

1. **直接访问 Packet.dll**:
   - 使用更底层的 API，绕过 libpcap 的抽象层
   - 更细粒度的控制（缓冲区、OID 查询等）

2. **OID 机制**:
   - 通过标准的 NDIS OIDs 查询和设置适配器属性
   - 与 Windows 网络栈紧密集成

3. **同步发送模式**:
   - 确保数据包发送完成后再返回
   - 简化错误处理

### 8.3 与 libpcap 的关系

libdnet 和 libpcap 在功能上互补：

| 功能 | libdnet | libpcap |
|------|---------|---------|
| 发送数据包 | ✅ | ✅ |
| 捕获数据包 | ❌ | ✅ |
| 数据包过滤 | ❌ | ✅ |
| MAC 地址管理 | ✅ | ❌ |
| 接口枚举 | ✅ | ✅ |
| IP 路由操作 | ✅ | ❌ |

### 8.4 应用场景

libdnet 适用于需要以下功能的应用：

1. **构造和发送自定义网络数据包**
2. **管理网络接口（MAC 地址、MTU 等）**
3. **配置 IP 路由**
4. **防火墙规则管理**
5. **网络工具开发（ping、traceroute 等）**

典型应用包括：
- Nmap（网络扫描器）
- Scapy（数据包构造工具）
- 各种网络安全工具

---

## 九、参考文献

1. **WinPcap/Npcap 文档**:
   - https://npcap.com/guide/npcap-devguide.html
   - WinPcap Developer's Pack

2. **libdnet 官方网站**:
   - https://github.com/ofalk/libdnet

3. **BPF 文档**:
   - BSD Packet Filter (BPF) Man Pages

4. **Linux PF_PACKET**:
   - Linux Packet Socket Man Pages (packet(7))

5. **DLPI 文档**:
   - Data Link Provider Interface Specification

---

**文档版本**: 1.0
**生成日期**: 2026-03-05
**分析的源码版本**: libdnet 1.18.2
