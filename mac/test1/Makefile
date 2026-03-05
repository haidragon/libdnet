# Makefile for libdnet macOS examples
# 基于 libdnet-1.13 的示例程序编译配置

# 编译器和标志
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = ../../src/.libs/libdnet.a
LIBS = -lpthread

# 头文件路径
INCLUDES = -I../../include

# 目标程序
TARGETS = interface_scanner packet_builder

# libdnet 库文件路径
LIBDNET_STATIC = ../../src/.libs/libdnet.a
LIBDNET_SO = ../../src/.libs/libdnet.dylib
LIBDNET_CONFIGURE = ../../configure
LIBDNET_MAKEFILE = ../../Makefile

# 默认目标
all: check_libdnet $(TARGETS)

# 检查并编译 libdnet 库（如果需要）
check_libdnet:
	@echo "检查 libdnet 库状态..."
	@if [ ! -f $(LIBDNET_STATIC) ] && [ ! -f $(LIBDNET_SO) ]; then \
		echo "✗ libdnet 库未找到，开始编译..."; \
		(cd ../.. && $(MAKE) clean > /dev/null 2>&1); \
		if [ ! -f $(LIBDNET_MAKEFILE) ]; then \
			echo "配置 libdnet..."; \
			(cd ../.. && ./configure --quiet) || { echo "✗ 配置失败"; exit 1; }; \
		fi; \
		echo "编译 libdnet..."; \
		(cd ../.. && $(MAKE) --quiet) || { echo "✗ 编译失败"; exit 1; }; \
		echo "✓ libdnet 库编译完成"; \
	else \
		echo "✓ libdnet 库已存在"; \
	fi

# 编译 interface_scanner
interface_scanner: interface_scanner.c $(LIBDNET_STATIC)
	@echo "编译 interface_scanner (静态链接)..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LIBS)
	@echo "✓ interface_scanner 编译完成"

# 编译 packet_builder
packet_builder: packet_builder.c $(LIBDNET_STATIC)
	@echo "编译 packet_builder (静态链接)..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LIBS)
	@echo "✓ packet_builder 编译完成"

# 清理
clean:
	@echo "清理编译产物..."
	rm -f $(TARGETS) *.o
	@echo "✓ 清理完成"

# 安装 (复制到 /usr/local/bin)
install: all
	@echo "安装程序到 /usr/local/bin..."
	install -m 755 interface_scanner /usr/local/bin/
	install -m 755 packet_builder /usr/local/bin/
	@echo "✓ 安装完成"

# 卸载
uninstall:
	@echo "卸载程序..."
	rm -f /usr/local/bin/interface_scanner
	rm -f /usr/local/bin/packet_builder
	@echo "✓ 卸载完成"

# 帮助
help:
	@echo "libdnet macOS 示例程序 Makefile"
	@echo ""
	@echo "可用目标:"
	@echo "  all              - 编译所有程序 (默认，自动检查和编译库)"
	@echo "  check_libdnet    - 检查 libdnet 库状态，如需要则编译"
	@echo "  libdnet_rebuild  - 强制重新编译 libdnet 库"
	@echo "  clean            - 清理编译产物"
	@echo "  install          - 安装到 /usr/local/bin (需要 sudo)"
	@echo "  uninstall        - 从 /usr/local/bin 卸载 (需要 sudo)"
	@echo "  help             - 显示此帮助信息"
	@echo ""
	@echo "示例:"
	@echo "  make                # 编译所有程序（会自动编译库）"
	@echo "  make check_libdnet  # 仅检查并编译库"
	@echo "  make clean          # 清理"
	@echo "  sudo make install   # 安装"
	@echo ""
	@echo "运行程序:"
	@echo "  sudo ./interface_scanner"
	@echo "  sudo ./packet_builder"

# 重新编译 libdnet 库
libdnet_rebuild:
	@echo "重新编译 libdnet 库..."
	@if [ -f $(LIBDNET_MAKEFILE) ]; then \
		(cd ../.. && $(MAKE) clean > /dev/null 2>&1); \
	fi
	@if [ ! -f $(LIBDNET_MAKEFILE) ]; then \
		echo "配置 libdnet..."; \
		(cd ../.. && ./configure --quiet) || { echo "✗ 配置失败"; exit 1; }; \
	fi
	@echo "编译 libdnet..."
	(cd ../.. && $(MAKE) --quiet) || { echo "✗ 编译失败"; exit 1; }
	@echo "✓ libdnet 库重新编译完成"

.PHONY: all clean install uninstall help check_libdnet libdnet_rebuild
