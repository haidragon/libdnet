# libdnet 数据包捕获示例 Makefile

CC = cc
CFLAGS = -Wall -O2 -I../../include
LDFLAGS = ../../src/.libs/libdnet.a

TARGET = packet_capture
SOURCE = packet_capture.c

# 检查并编译 libdnet 库
check_libdnet:
	@echo "检查 libdnet 库..."
	@if [ ! -f ../../src/.libs/libdnet.a ]; then \
		echo "libdnet 库未编译，开始编译..."; \
		(cd ../.. && make clean); \
		(cd ../.. && ./configure); \
		(cd ../.. && make); \
	else \
		echo "libdnet 库已存在"; \
	fi

# 默认目标
all: check_libdnet $(TARGET)

# 编译目标文件
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

# 运行程序 (需要 root 权限)
run: $(TARGET)
	@echo "注意: 数据包捕获需要 root 权限"
	@echo "请使用: sudo ./packet_capture"
	@read -p "是否现在运行? [y/N] " answer; \
	if [ "$$answer" = "y" ] || [ "$$answer" = "Y" ]; then \
		sudo ./$(TARGET); \
	fi

# 清理
clean:
	rm -f $(TARGET)

# 重新编译
rebuild: clean all

.PHONY: all clean rebuild run check_libdnet
