# libdnet 网关查询示例 Makefile

CC = cc
CFLAGS = -Wall -O2 -I../../include
LDFLAGS = ../../src/.libs/libdnet.a

TARGET = gateway_query
SOURCE = gateway_query.c

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

# 运行程序
run: $(TARGET)
	@echo "运行网关查询示例..."
	./$(TARGET)

# 清理
clean:
	rm -f $(TARGET)

# 重新编译
rebuild: clean all

.PHONY: all clean rebuild run check_libdnet
