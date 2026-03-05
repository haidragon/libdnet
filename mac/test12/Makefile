# libdnet 接口操作示例 Makefile

CC = cc
CFLAGS = -Wall -O2 -I../../include
LDFLAGS = ../../src/.libs/libdnet.a

TARGET = interface_ops
SOURCE = interface_ops.c
OBJECT = $(SOURCE:.c=.o)

# 检查并编译 libdnet 库
check_libdnet:
	@echo "检查 libdnet 库..."
	@if [ ! -f ../../src/.libs/libdnet.a ]; then \
		echo "libdnet 库未编译，开始编译..."; \
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
	@echo "编译成功：$(TARGET)"

# 运行程序
run: $(TARGET)
	@echo "运行接口操作示例..."
	./$(TARGET)

# 清理
clean:
	rm -f $(TARGET) $(OBJECT)
	@echo "清理完成"

# 重新编译
rebuild: clean all

# 调试版本
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

.PHONY: all clean rebuild run check_libdnet debug
