# libdnet macOS与Linux平台编译使用指南

## 目录

1. [概述](#概述)
2. [系统要求](#系统要求)
3. [环境准备](#环境准备)
4. [编译步骤](#编译步骤)
5. [依赖库说明](#依赖库说明)
6. [编译测试程序](#编译测试程序)
7. [常见问题](#常见问题)
8. [平台特性](#平台特性)
9. [参考资料](#参考资料)

---

## 概述

libdnet (libdumbnet) 是一个简化版的网络工具库，提供网络接口、ARP、路由、IP、以太网等功能的跨平台访问。

### macOS 与 Linux 平台特点

- **macOS**: 基于 BSD 内核，使用 BPF (Berkeley Packet Filter) 进行数据包捕获
- **Linux**: 使用 Netlink 和 /proc 文件系统进行网络操作
- 两者都支持完整的网络功能，包括原始套接字、接口管理等
- 使用标准的 configure && make 编译流程

---

## 系统要求

### 操作系统

#### macOS
- macOS 10.10 (Yosemite) 或更高版本
- macOS 10.13 (High Sierra) 及以上推荐

#### Linux
- 任何主流 Linux 发行版 (Ubuntu, Debian, CentOS, Fedora, Arch 等)
- 内核版本 3.10 或更高推荐

### 必需软件

- **GCC** 或 **Clang** 编译器
- **Make** 构建工具
- **Autoconf** 和 **Automake** (如果从源码仓库编译)
- **Libtool** (用于构建共享库)

### 可选软件

- **Check** (用于运行测试套件)
- **Python** (用于构建 Python 绑定)

---

## 环境准备

### macOS 环境准备

#### 安装 Xcode 命令行工具

```bash
# 安装 Xcode 命令行工具
xcode-select --install
```

#### 使用 Homebrew 安装依赖

```bash
# 安装 Homebrew (如果尚未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install autoconf automake libtool
```

#### 验证安装

```bash
# 检查编译器
gcc --version
# 或
clang --version

# 检查 Make
make --version
```

### Linux 环境准备

#### Ubuntu/Debian

```bash
# 更新包列表
sudo apt-get update

# 安装编译工具
sudo apt-get install build-essential autoconf automake libtool

# 可选：安装测试工具
sudo apt-get install check

# 可选：安装 Python 绑定依赖
sudo apt-get install python3-dev
```

#### CentOS/RHEL/Fedora

```bash
# 安装编译工具
sudo yum groupinstall "Development Tools"
sudo yum install autoconf automake libtool

# 或者在 Fedora 上使用 dnf
sudo dnf install gcc make autoconf automake libtool

# 可选：安装测试工具
sudo yum install check

# 可选：安装 Python 绑定依赖
sudo yum install python3-devel
```

#### Arch Linux

```bash
# 安装编译工具
sudo pacman -S base-devel autoconf automake libtool

# 可选：安装测试工具
sudo pacman -S check

# 可选：安装 Python 绑定依赖
sudo pacman -S python
```

#### 验证安装

```bash
# 检查 GCC
gcc --version

# 检查 Make
make --version
```

---

## 编译步骤

### 基本编译

在 macOS 和 Linux 上，libdnet 的编译过程基本相同：

```bash
# 进入 libdnet 目录
cd /path/to/libdnet-1.13

# 清理旧的编译文件
make clean 2>/dev/null || true
rm -f config.h config.status libtool

# 配置项目
./configure

# 编译
make

# 可选：安装到系统
sudo make install
```

### 配置选项说明

| 选项 | 说明 |
|------|------|
| `--prefix=PATH` | 指定安装路径 (默认: /usr/local) |
| `--disable-shared` | 只编译静态库 |
| `--enable-debug` | 启用调试符号 |
| `--with-python=DIR` | 包含 Python 绑定 (使用 DIR 中的 python) |
| `--with-check=DIR` | 使用 Check 测试框架 (在 DIR 中查找) |

### 自定义安装路径

```bash
# 安装到自定义路径
./configure --prefix=/opt/libdnet
make
sudo make install
```

### 只编译静态库

```bash
./configure --disable-shared
make
```

### 编译带 Python 绑定

```bash
# 使用系统默认 Python
./configure --with-python

# 或指定 Python 路径
./configure --with-python=/usr/bin/python3

make
sudo make install
```

### 清理编译产物

```bash
make clean
```

### 完全清理

```bash
make distclean
```

---

## 依赖库说明

### 系统依赖 (必需)

#### 1. 标准 C 库 (libc)

- **用途**: 基本系统调用和库函数
- **链接选项**: 自动链接
- **头文件**: 标准系统头文件

#### 2. Socket 库

- **用途**: 网络套接字 API
- **链接选项**: 自动链接
- **头文件**:
  - `<sys/socket.h>`
  - `<netinet/in.h>`
  - `<arpa/inet.h>`

#### 3. 平台特定库

**macOS**:
- **BPF (Berkeley Packet Filter)**: 数据包捕获和发送
- **System Frameworks**: CoreFoundation, SystemConfiguration 等

**Linux**:
- **Netlink**: 内核通信 (路由表操作)
- **/proc 文件系统**: 网络状态信息

### 可选依赖

#### Check 测试框架

- **用途**: 单元测试
- **下载**: https://libcheck.github.io/check/
- **头文件**: `<check.h>`
- **链接选项**: `-lcheck`

#### Python

- **用途**: Python 绑定
- **头文件**: `<Python.h>`

### 编译时自动检测的库

configure 会自动检测并配置以下功能：

```c
// macOS 特定功能
#define HAVE_BPF 1
#define HAVE_SYS_SYSCTL_H 1

// Linux 特定功能
#define HAVE_NETLINK 1
#define HAVE_PROCFS 1
```

### 链接命令示例

#### 使用动态库编译

```bash
gcc test.c -Iinclude -Lsrc/.libs -ldnet -o test
```

#### 使用静态库编译

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -o test
```

#### macOS 链接框架

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -framework CoreFoundation -o test
```

---

## 编译测试程序

### 示例代码

创建 `test.c`:

```c
#include <dnet.h>
#include <stdio.h>

int main(void) {
    intf_t *intf;
    struct intf_entry entry;
    int ret = 0;

    // 打开网络接口
    if ((intf = intf_open()) == NULL) {
        perror("intf_open");
        return 1;
    }

    // 遍历所有网络接口
    entry.intf_len = sizeof(entry);
    printf("网络接口列表：
");
    printf("====================================
");

    if (intf_loop(intf, [](struct intf_entry *entry, void *arg) -> int {
        printf("接口: %s
", entry->intf_name);
        printf("  类型: 0x%04x
", entry->intf_type);
        printf("  标志: 0x%08x
", entry->intf_flags);
        printf("  MTU: %u
", entry->intf_mtu);

        if (entry->intf_addr.addr_type != ADDR_TYPE_NONE) {
            printf("  IP: %s/%d
",
                   addr_ntoa(&entry->intf_addr),
                   entry->intf_addr.addr_bits);
        }

        if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
            printf("  MAC: %s
",
                   addr_ntoa(&entry->intf_link_addr));
        }

        printf("
");
        return 0;
    }, NULL) < 0) {
        perror("intf_loop");
        ret = 1;
    }

    intf_close(intf);
    return ret;
}
```

### 编译测试程序

#### macOS

```bash
# 动态链接
gcc test.c -Iinclude -Lsrc/.libs -ldnet -o test

# 静态链接 (推荐)
gcc test.c -Iinclude src/.libs/libdnet.a -o test
```

#### Linux

```bash
# 动态链接
gcc test.c -Iinclude -Lsrc/.libs -ldnet -o test

# 静态链接 (推荐)
gcc test.c -Iinclude src/.libs/libdnet.a -o test
```

### 运行测试程序

```bash
# macOS 和 Linux 都需要 root 权限
sudo ./test
```

---

## 常见问题

### Q1: configure 错误: "C compiler cannot create executables"

**原因**: 缺少编译器或编译器未正确安装。

**解决方案**:

macOS:
```bash
# 安装 Xcode 命令行工具
xcode-select --install
```

Linux:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
```

### Q2: 找不到 <net/bpf.h> 或 <linux/if_packet.h>

**原因**: 缺少系统头文件或开发包。

**解决方案**:

macOS:
```bash
# 安装 Xcode 命令行工具
xcode-select --install
```

Linux:
```bash
# Ubuntu/Debian
sudo apt-get install linux-headers-$(uname -r)

# CentOS/RHEL
sudo yum install kernel-devel
```

### Q3: 编译出的程序在其他机器上运行报错

**原因**: 使用了动态链接，目标机器缺少 libdnet.so 或系统库不兼容。

**解决方案**: 使用静态链接

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -o test
```

### Q4: 运行时提示 "Operation not permitted" 或 "Permission denied"

**原因**: 某些操作需要 root 权限或特定能力。

**解决方案**: 

macOS/Linux:
```bash
# 使用 sudo 运行
sudo ./test
```

Linux 特定:
```bash
# 或使用 capabilities (Linux)
sudo setcap cap_net_raw+ep ./test
sudo setcap cap_net_admin+ep ./test
```

### Q5: 找不到 libdnet.so 或 libdnet.dylib

**原因**: 库路径未设置或编译失败。

**解决方案**:

检查编译产物:
```bash
ls -lh src/.libs/libdnet.a
ls -lh src/.libs/libdnet.so*
ls -lh src/.libs/libdnet.dylib*
```

如果是动态链接，设置库路径:

macOS:
```bash
export DYLD_LIBRARY_PATH=/path/to/libdnet/src/.libs:$DYLD_LIBRARY_PATH
```

Linux:
```bash
export LD_LIBRARY_PATH=/path/to/libdnet/src/.libs:$LD_LIBRARY_PATH
```

### Q6: macOS 10.6+ 上 BPF 设备打开失败

**原因**: macOS 10.6 有一个 bug，使用 O_WRONLY 打开 BPF 设备会阻止其他进程接收流量。

**解决方案**: libdnet 已经在 eth-bsd.c 中修复了这个问题，使用 O_RDWR 打开 BPF 设备。确保使用最新版本的源代码。

---

## 平台特性

### 编译产物

在 `.libs` 目录下生成：

**macOS**:
```
libdnet.a              # 静态库
libdnet.dylib          # 动态库
libdnet.la            # Libtool 库文件
```

**Linux**:
```
libdnet.a              # 静态库
libdnet.so            # 动态库
libdnet.la            # Libtool 库文件
```

### 库文件大小参考

```
libdnet.a            约 200-300 KB
libdnet.so/dylib     约 150-200 KB
```

### 支持的功能模块

| 模块 | macOS 实现 | Linux 实现 | 说明 |
|------|-----------|-----------|------|
| `intf` | `intf.c` (通用) | `intf.c` (通用) | 网络接口操作 |
| `arp` | `arp-bsd.c` | `arp-ioctl.c` | ARP 表操作 |
| `route` | `route-bsd.c` | `route-linux.c` | 路由表操作 |
| `ip` | `ip.c` (通用) | `ip.c` (通用) | IP 层数据包发送 |
| `eth` | `eth-bsd.c` | `eth-linux.c` | 以太网帧操作 |
| `fw` | `fw-pf.c` | `fw-ipchains.c` | 防火墙规则 |
| `tun` | `tun-bsd.c` | `tun-linux.c` | 隧道设备 |

### 平台差异

#### 以太网操作

| 特性 | macOS (BPF) | Linux (PF_PACKET) |
|------|------------|------------------|
| 发送 | ✅ | ✅ |
| 接收 | ✅ | ✅ |
| MAC地址获取 | ✅ | ✅ |
| MAC地址设置 | ✅ | ✅ |
| 权限要求 | root | CAP_NET_RAW |

#### 路由操作

| 特性 | macOS (sysctl) | Linux (Netlink + /proc) |
|------|--------------|----------------------|
| IPv4 | ✅ | ✅ |
| IPv6 | ✅ | ✅ |
| 添加路由 | ✅ | ✅ |
| 删除路由 | ✅ | ✅ |
| 动态更新 | ✅ | ✅ |
| 权限要求 | root | CAP_NET_ADMIN |

#### ARP 操作

| 特性 | macOS (sysctl) | Linux (ioctl + /proc) |
|------|--------------|---------------------|
| 添加条目 | ✅ | ✅ |
| 删除条目 | ✅ | ✅ |
| 静态条目 | ✅ | ✅ |
| 动态更新 | ✅ | ✅ |
| 权限要求 | root | CAP_NET_ADMIN |

---

## 参考资料

### 官方资源

- libdnet GitHub: https://github.com/dugsong/libdnet
- libdnet Wiki: https://github.com/dugsong/libdnet/wiki

### 平台文档

**macOS**:
- Apple Developer Documentation: https://developer.apple.com/documentation/
- BPF 文档: https://www.freebsd.org/cgi/man.cgi?query=bpf

**Linux**:
- Netlink 文档: https://man7.org/linux/man-pages/man7/netlink.7.html
- PF_PACKET 文档: https://man7.org/linux/man-pages/man7/packet.7.html
- Capabilities 文档: https://man7.org/linux/man-pages/man7/capabilities.7.html

### 相关文档

- Windows 编译指南: `01-Windows平台编译完整指南.md`
- Linux 内核分支分析: `14-Linux内核分支深度分析.md`

---

## 总结

在 macOS 和 Linux 上编译和使用 libdnet 需要：

1. ✅ 安装编译工具链 (GCC/Clang, Make, Autoconf, Automake, Libtool)
2. ✅ 运行 `./configure` 配置项目
3. ✅ 运行 `make` 编译库
4. ✅ 运行 `sudo make install` 安装到系统 (可选)
5. ✅ 链接时指定正确的库路径和库名称

macOS 和 Linux 上的主要区别在于：
- macOS 使用 BPF 进行数据包捕获和发送
- Linux 使用 Netlink 和 /proc 文件系统进行网络操作
- 两者都支持完整的网络功能，但实现方式不同

使用 libdnet 可以简化跨平台网络编程，避免直接处理平台特定的 API 和系统调用。
