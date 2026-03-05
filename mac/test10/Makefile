CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = ../../src/.libs/libdnet.a
LIBS = -lpthread
INCLUDES = -I../../include
TARGET = blob_ops

all: check_libdnet $(TARGET)
check_libdnet:
	@if [ ! -f ../../src/.libs/libdnet.a ]; then \
		(cd ../.. && $(MAKE) clean > /dev/null 2>&1); \
		(cd ../.. && ./configure --quiet); \
		(cd ../.. && $(MAKE) --quiet); \
	fi

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET) *.o

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run check_libdnet
