# libdnet 编译安装指南

## 项目简介

libdnet (libdumbnet) 是一个简化了网络操作的跨平台 C 语言库，提供了统一的接口来访问底层网络设备和服务。

## 系统要求

### Linux/macOS/*BSD
- GCC 编译器
- Make 工具
- automake, autoconf, libtool

### Windows
- Cygwin + MinGW (推荐)
  或
- Microsoft Visual C++
- WinPcap (可选，用于原始以太网发送)
- PktFilter (可选，用于防火墙支持)

## 编译安装步骤

### Linux/macOS/*BSD 系统

```bash
# 1. 配置
./configure

# 2. 编译
make

# 3. 安装 (需要 root 权限)
sudo make install

# 4. 清理 (可选)
make clean
```

快速编译:
```bash
./configure && make
```

### Windows 系统

#### 方式一：Cygwin + MinGW (推荐)

```bash
# 1. 确保已安装 Cygwin 和 MinGW 包
# 2. 配置并编译
./configure && make
```

#### 方式二：Microsoft Visual C++

```bash
# 1. 配置
./configure

# 2. 构建 Python 模块
cd python
C:/Python23/python.exe setup.py build

# 3. 构建库
cd ../src
lib /out:dnet.lib *.obj
```

### Windows 前置依赖

#### WinPcap (原始以太网支持)
- 下载地址: http://winpcap.polito.it/install/default.htm
- 需要安装驱动和 DLL，并提取开发包到构建目录

#### PktFilter (防火墙支持)
- 下载地址: http://www.hsc.fr/ressources/outils/pktfilter/index.html.en

#### 注意
大多数 Windows 开发者建议直接使用预编译的 libdnet developer's pack（包含 MinGW 和 MSVC++ 库）。

## Solaris/IRIX/BSD/OS/HP-UX/Tru64 系统

### Solaris
- **防火墙支持**: 安装 Darren Reed 的 IP Filter
  - 地址: http://coombs.anu.edu.au/~avalon/
- **隧道支持**: 安装 Universal TUN/TAP Driver
  - 地址: http://vtun.sourceforge.net/tun/

然后执行:
```bash
./configure && make
```

### 其他 Unix 系统
参考具体系统的文档，可能需要额外的系统特定依赖。

## 可选配置参数

```bash
# 指定 Python 路径
./configure --with-python=/path/to/python

# 使用 Check 测试框架
./configure --with-check=/path/to/check

# 指定安装前缀
./configure --prefix=/usr/local

# 显示所有配置选项
./configure --help
```

## 测试

如果已安装 Check 测试框架:
```bash
cd test
make check
```

## 卸载

```bash
make uninstall
```

## 常见问题

### Windows 下缺少 MingW32 包
错误: `need MingW32 package to build under Cygwin`
解决: 通过 Cygwin 安装程序安装 MingW32 包

### 找不到 Python
使用 `--with-python` 参数指定 Python 路径

### 权限问题
安装时使用 `sudo` (Linux/macOS) 或以管理员身份运行 (Windows)

## 相关文档

- 官方文档: 查看 `man/` 目录下的手册文件
- 示例代码: 查看 `test/` 目录下的测试文件
- Python 绑定: 查看 `python/` 目录

## 许可证

查看 LICENSE 文件了解许可信息。
