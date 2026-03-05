# Makefile for test2 - 地址操作示例
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = ../../src/.libs/libdnet.a
LIBS = -lpthread

INCLUDES = -I../../include
TARGET = addr_operations

LIBDNET_STATIC = ../../src/.libs/libdnet.a
LIBDNET_SO = ../../src/.libs/libdnet.dylib
LIBDNET_MAKEFILE = ../../Makefile

all: check_libdnet $(TARGET)

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

$(TARGET): $(TARGET).c $(LIBDNET_STATIC)
	@echo "编译 $(TARGET) (静态链接)..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LIBS)
	@echo "✓ $(TARGET) 编译完成"

clean:
	@echo "清理编译产物..."
	rm -f $(TARGET) *.o
	@echo "✓ 清理完成"

run: $(TARGET)
	@echo "运行 $(TARGET)..."
	./$(TARGET)

.PHONY: all clean run check_libdnet
