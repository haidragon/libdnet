# libdnet Windows 平台编译完整指南

## 目录

1. [概述](#概述)
2. [系统要求](#系统要求)
3. [环境准备](#环境准备)
4. [configure 修改说明](#configure-修改说明)
5. [编译步骤](#编译步骤)
6. [依赖库说明](#依赖库说明)
7. [编译测试程序](#编译测试程序)
8. [常见问题](#常见问题)

---

## 概述

libdnet (libdumbnet) 是一个简化版的网络工具库，提供网络接口、ARP、路由、IP、以太网等功能的跨平台访问。

### Windows 平台特点

- 使用 Windows IP Helper API (`iphlpapi.dll`)
- 使用 Winsock 2.2 (`ws2_32.dll`)
- 某些功能需要第三方库（如 WinPcap）
- 需要 MSYS2 或 MinGW-w64 编译工具链

---

## 系统要求

### 操作系统

- Windows 7 或更高版本
- Windows 10/11（推荐）
- Windows Server 2012 及以上

### 必需软件

- **MSYS2 MinGW64**（推荐）或 MinGW-w64
- GCC 7.x 或更高版本
- Make、Autoconf、Automake、Libtool

### 可选软件

- WinPcap（用于原始以太网帧访问）
- PktFilter（用于防火墙功能）

---

## 环境准备

### 方案 1：安装 MSYS2（推荐）

#### 1.1 下载安装 MSYS2

访问 https://www.msys2.org/ 下载安装程序，默认安装到 `C:\msys64`

#### 1.2 安装编译工具链

打开 **"MSYS2 MinGW64"** 终端（不是普通的 MSYS2 终端）：

```bash
# 更新包数据库
pacman -Syu

# 安装编译工具链
pacman -S --noconfirm --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-make \
  mingw-w64-x86_64-autoconf \
  mingw-w64-x86_64-automake \
  mingw-w64-x86_64-libtool \
  make \
  autoconf \
  automake \
  libtool
```

#### 1.3 验证安装

```bash
# 检查 GCC
gcc --version

# 检查 Make
make --version
```

### 方案 2：使用独立的 MinGW-w64

#### 2.1 下载 MinGW-w64

从以下任一源下载：
- https://github.com/niXman/mingw-builds-binaries/releases
- https://winlibs.com/

选择 `x86_64-posix-seh` 版本，解压到 `C:\mingw64`

#### 2.2 配置环境变量

添加到系统 PATH：
```
C:\mingw64\bin
```

#### 2.3 验证安装

打开 CMD 或 PowerShell：

```cmd
gcc --version
```

---

## configure 修改说明

### 问题描述

在 MSYS2/MINGW64 环境下运行 `./configure` 时，可能会遇到以下错误：

```
checking for Python... configure: error: need MingW32 package to build under Cygwin
```

### 原因分析

configure 脚本中的第 12177-12269 行检测到 `CYGWIN=yes` 时，会检查是否存在 mingw 目录：

```bash
if test "$CYGWIN" = yes ; then
    if test -d /usr/include/mingw || test -d /usr/i686-w64-mingw32; then
        # 使用 mingw 编译
        ...
    else
        # 报错
        as_fn_error $? "need MingW32 package to build under Cygwin" "$LINENO" 5
    fi
fi
```

MSYS2 环境虽然被检测为类似 Cygwin，但它是原生 MinGW，不需要 Cygwin 的 mingw 包。

### 修改方案

#### 原始代码（第 12177-12182 行）

```bash
if test "$CYGWIN" = yes ; then
	if test -d /usr/include/mingw || test -d /usr/i686-w64-mingw32; then
		if test "$MINGW" = no ; then
			CPPFLAGS="$CPPFLAGS -mno-cygwin"
			CFLAGS="$CFLAGS -mno-cygwin"
		fi

		$as_echo "#define WIN32_LEAN_AND_MEAN 1" >>confdefs.h

		{ $as_echo "$as_me:${as_lineno-$LINENO}: checking for main in -lws2_32" >&5
		...
```

#### 错误触发代码（第 12267-12269 行）

```bash
	else
		as_fn_error $? "need MingW32 package to build under Cygwin" "$LINENO" 5
	fi
fi
```

#### 修改后的代码

注释掉 mingw 目录检查和错误提示：

```bash
if test "$CYGWIN" = yes ; then
	# Skip the mingw check for MSYS2/MINGW64 - MSYS2 is native MinGW
	# if test -d /usr/include/mingw || test -d /usr/i686-w64-mingw32; then
	#	if test "$MINGW" = no ; then
	#		CPPFLAGS="$CPPFLAGS -mno-cygwin"
	#		CFLAGS="$CFLAGS -mno-cygwin"
	#	fi

	$as_echo "#define WIN32_LEAN_AND_MEAN 1" >>confdefs.h

	{ $as_echo "$as_me:${as_lineno-$LINENO}: checking for main in -lws2_32" >&5
	...

	#else
	#	as_fn_error $? "need MingW32 package to build under Cygwin" "$LINENO" 5
	#fi
```

### 修改步骤

1. 打开 `configure` 文件
2. 定位到第 12177 行
3. 按上述注释方式修改代码
4. 保存文件

### 自动化修改脚本

```bash
# 在 libdnet 根目录下运行
sed -i '12178s/^/#/' configure
sed -i '12179s/^/#/' configure
sed -i '12180s/^/#/' configure
sed -i '12181s/^/#/' configure
sed -i '12182s/^/#/' configure
sed -i '12267s/^/#/' configure
sed -i '12268s/^/#/' configure
sed -i '12269s/^/#/' configure
```

---

## 编译步骤

### 在 MSYS2 MinGW64 终端中编译

```bash
# 进入 libdnet 目录
cd /c/Users/Administrator/libev/libdnet-1.13

# 清理旧的编译文件
make clean 2>/dev/null || true
rm -f config.h config.status libtool

# 配置项目
./configure --host=x86_64-w64-mingw32 --prefix=/usr/local

# 编译
make

# 可选：安装
make install
```

### 配置选项说明

| 选项 | 说明 |
|------|------|
| `--host=x86_64-w64-mingw32` | 指定目标平台为 Windows 64 位 |
| `--prefix=/usr/local` | 安装路径 |
| `--disable-shared` | 只编译静态库 |
| `--enable-debug` | 启用调试符号 |
| `--with-python` | 包含 Python 绑定 |

### 只编译静态库

```bash
./configure --host=x86_64-w64-mingw32 --disable-shared
make
```

### 清理编译产物

```bash
make clean
```

---

## 依赖库说明

### 系统依赖（必需）

#### 1. Winsock 2.2 (`ws2_32.dll`)

- **用途**：网络套接字 API
- **链接选项**：`-lws2_32`
- **系统自带**：Windows XP 及以上
- **头文件**：
  - `winsock2.h`
  - `ws2tcpip.h`

#### 2. IP Helper API (`iphlpapi.dll`)

- **用途**：网络接口、路由、ARP 表操作
- **链接选项**：`-liphlpapi`
- **系统自带**：Windows 2000 及以上
- **头文件**：
  - `iphlpapi.h`
  - `ipexport.h`
  - `iptypes.h`

### 可选依赖

#### 3. WinPcap

- **用途**：原始以太网帧捕获和发送
- **下载**：https://www.winpcap.org/
- **头文件**：`pcap.h`
- **链接选项**：`-lwpcap`
- **开发包**：WinPcap Developer's Pack

libdnet 的 Windows 实现：
- `eth-win32.c`：空实现，依赖 WinPcap

#### 4. PktFilter

- **用途**：防火墙规则管理
- **下载**：http://www.hsc.fr/ressources/outils/pktfilter/index.html.en
- **通信方式**：命名管道 `\\.\pipe\PktFltPipe`

libdnet 的 Windows 实现：
- `fw-pktfilter.c`：通过命名管道与 PktFilter 服务通信

### 编译时自动链接的库

configure 会自动检测并链接以下库：

```c
// 自动添加的宏定义
#define WIN32_LEAN_AND_MEAN 1
#define HAVE_LIBWS2_32 1
#define HAVE_LIBIPHLPAPI 1
#define snprintf _snprintf
```

### 链接命令示例

#### 使用动态库编译

```bash
gcc test.c -Iinclude -Lsrc/.libs -ldnet -liphlpapi -lws2_32 -o test.exe
```

#### 使用静态库编译

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -liphlpapi -lws2_32 -static -o test.exe
```

#### 完全静态链接（无运行时依赖）

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -liphlpapi -lws2_32 -static -static-libgcc -o test.exe
```

---

## 编译测试程序

### 示例代码

创建 `test.c`：

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
    printf("网络接口列表：\n");
    printf("====================================\n");

    if (intf_loop(intf, [](struct intf_entry *entry, void *arg) -> int {
        printf("接口: %s\n", entry->intf_name);
        printf("  类型: 0x%04x\n", entry->intf_type);
        printf("  标志: 0x%08x\n", entry->intf_flags);
        printf("  MTU: %u\n", entry->intf_mtu);

        if (entry->intf_addr.addr_type != ADDR_TYPE_NONE) {
            printf("  IP: %s/%d\n",
                   addr_ntoa(&entry->intf_addr),
                   entry->intf_addr.addr_bits);
        }

        if (entry->intf_link_addr.addr_type == ADDR_TYPE_ETH) {
            printf("  MAC: %s\n",
                   addr_ntoa(&entry->intf_link_addr));
        }

        printf("\n");
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

#### 在 MSYS2 MinGW64 终端中

```bash
# 动态链接
gcc test.c -Iinclude -Lsrc/.libs -ldnet -liphlpapi -lws2_32 -o test.exe

# 静态链接（推荐）
gcc test.c -Iinclude src/.libs/libdnet.a -liphlpapi -lws2_32 -static -o test.exe
```

#### 在 Windows CMD/PowerShell 中

```cmd
rem 静态链接（需要添加 MinGW64 bin 到 PATH）
gcc test.c -Iinclude src\.libs\libdnet.a -liphlpapi -lws2_32 -static -o test.exe
```

### 运行测试程序

```bash
# 需要管理员权限
./test.exe
```

或在 Windows CMD 中：

```cmd
test.exe
```

---

## 常见问题

### Q1: configure 错误：need MingW32 package to build under Cygwin

**原因**：configure 检测到 CYGWIN=yes，但没有找到 mingw 目录。

**解决方案**：

方案 1：修改 configure 文件（见上文 [configure 修改说明](#configure-修改说明)）

方案 2：手动跳过检查，直接编译 Windows 源文件

### Q2: 找不到 iphlpapi.h 或 ws2tcpip.h

**原因**：Windows SDK 路径未正确配置。

**解决方案**：

- 确保 MSYS2 MinGW64 已正确安装
- 检查头文件是否存在：
  ```bash
  ls /mingw64/x86_64-w64-mingw32/include/iphlpapi.h
  ls /mingw64/x86_64-w64-mingw32/include/ws2tcpip.h
  ```

### Q3: undefined reference to `GetAdaptersAddresses`

**原因**：缺少 iphlpapi 库。

**解决方案**：链接时添加 `-liphlpapi`

```bash
gcc test.c -ldnet -liphlpapi -lws2_32 -o test.exe
```

### Q4: 编译出的程序在别的电脑上运行报错

**原因**：使用了动态链接，目标机器缺少 libdnet.dll 或 MSVC 运行时。

**解决方案**：使用静态链接

```bash
gcc test.c -Iinclude src/.libs/libdnet.a -liphlpapi -lws2_32 -static -o test.exe
```

或复制必要的 DLL：
- libdnet.dll
- libgcc_s_seh-1.dll
- libstdc++-6.dll

### Q5: 运行时提示 Permission denied

**原因**：某些操作需要管理员权限。

**解决方案**：以管理员身份运行

- 右键点击程序 → "以管理员身份运行"
- 或在管理员权限的 CMD/PowerShell 中运行

### Q6: 找不到 libdnet.so 或 libdnet.dll

**原因**：库路径未设置或编译失败。

**解决方案**：

检查编译产物：
```bash
ls -lh src/.libs/libdnet.a
ls -lh src/.libs/libdnet.so*
ls -lh src/.libs/libdnet.dll*
```

如果是 `.so` 文件，说明是在 Linux 下编译的，需要重新配置为 Windows 平台：
```bash
make clean
./configure --host=x86_64-w64-mingw32
make
```

### Q7: 如何查看编译日志？

**方法 1**：查看 config.log

```bash
cat config.log | grep error
```

**方法 2**：使用详细的 make 输出

```bash
make V=1
```

---

## Windows 平台特性

### 编译产物

在 `.libs` 目录下生成：

```
libdnet.a              # 静态库
libdnet.dll           # 动态库（如果启用）
libdnet.dll.a        # DLL 的导入库
```

### 库文件大小参考

```
libdnet.a            约 370 KB
libdnet.dll          约 200 KB
```

### 支持的功能模块

| 模块 | Windows 实现 | 说明 |
|------|-------------|------|
| `intf` | `intf-win32.c` | 网络接口操作 |
| `arp` | `arp-win32.c` | ARP 表操作 |
| `route` | `route-win32.c` | 路由表操作 |
| `ip` | `ip-win32.c` | IP 层数据包发送 |
| `eth` | `eth-win32.c` | 空实现（需要 WinPcap） |
| `fw` | `fw-pktfilter.c` | 防火墙（需要 PktFilter） |
| `tun` | `tun-none.c` | 隧道设备（空实现） |

---

## 参考资料

### 官方资源

- libdnet GitHub: https://github.com/dugsong/libdnet
- libdnet Wiki: https://github.com/dugsong/libdnet/wiki

### 依赖库

- MSYS2: https://www.msys2.org/
- MinGW-w64: https://www.mingw-w64.org/
- WinPcap: https://www.winpcap.org/
- PktFilter: http://www.hsc.fr/ressources/outils/pktfilter/index.html.en

### Windows API 文档

- IP Helper API: https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/
- Winsock 2.2: https://docs.microsoft.com/en-us/windows/win32/winsock/

---

## 完整编译脚本

### 一键编译脚本 (build_all.bat)

```batch
@echo off
setlocal EnableDelayedExpansion

echo ================================================
echo libdnet Windows 一键编译脚本
echo ================================================
echo.

REM 检查 MSYS2
if not exist "C:\msys64\mingw64.exe" (
    echo [错误] 找不到 MSYS2 MinGW64
    pause
    exit /b 1
)

echo [1/4] 启动编译...
echo.

"C:\msys64\mingw64.exe" --login -c ^
  "cd /c/Users/Administrator/libev/libdnet-1.13 && ^
   make clean 2^>/dev/null || true && ^
   rm -f config.h config.status && ^
   ./configure --host=x86_64-w64-mingw32 --prefix=/usr/local && ^
   make"

echo.
echo [2/4] 检查编译结果...
if exist "src\.libs\libdnet.a" (
    echo ✓ 编译成功！
    dir "src\.libs\libdnet.a"
) else (
    echo ✗ 编译失败
)

echo.
echo ================================================
echo 编译完成！
echo ================================================
echo.

echo 编译测试程序示例：
echo   gcc test.c -Iinclude -Lsrc/.libs -ldnet -liphlpapi -lws2_32 -o test.exe
echo.
pause
```

---

## 总结

编译 libdnet 在 Windows 平台需要：

1. ✅ 安装 MSYS2 MinGW64 编译环境
2. ✅ 修改 configure 文件（注释 mingw 检查）
3. ✅ 运行 `./configure --host=x86_64-w64-mingw32`
4. ✅ 运行 `make`
5. ✅ 链接时添加 `-liphlpapi -lws2_32`

生成的库文件位于 `src/.libs/libdnet.a`，可以直接用于编译 Windows 应用程序。
