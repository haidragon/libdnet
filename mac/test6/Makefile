CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = ../../src/.libs/libdnet.a
LIBS = -lpthread
INCLUDES = -I../../include
TARGET = udp_packet

LIBDNET_STATIC = ../../src/.libs/libdnet.a
LIBDNET_MAKEFILE = ../../Makefile

all: check_libdnet $(TARGET)
check_libdnet:
	@if [ ! -f $(LIBDNET_STATIC) ]; then \
		(cd ../.. && $(MAKE) clean > /dev/null 2>&1); \
		(cd ../.. && ./configure --quiet); \
		(cd ../.. && $(MAKE) --quiet); \
	fi

$(TARGET): $(TARGET).c $(LIBDNET_STATIC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET) *.o

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run check_libdnet
