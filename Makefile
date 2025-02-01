CC = gcc
# CFLALGS = -g -O0 -lwrappmem -lpmem -lpmemobj
TARGET = lib/libwrappmem.so
SRCS = wraplibpmem.c wraplibpmemobj.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET) bin/PmemWrap_memcpy.out

$(TARGET): $(OBJS)
	mkdir -p lib
	$(CC) -shared -o $(TARGET) $(OBJS) -ldl -pthread -g -O0

wraplibpmem.o: wraplibpmem.c wraplibpmem.h
	$(CC) -c -fPIC wraplibpmem.c -o wraplibpmem.o -pthread -g -O0

wraplibpmemobj.o: wraplibpmemobj.c wraplibpmem.h wraplibpmemobj.h
	$(CC) -c -fPIC wraplibpmemobj.c -o wraplibpmemobj.o -g -O0

clean:
	rm -f $(OBJS) $(TARGET) bin/PmemWrap_memcpy.out

bin/PmemWrap_memcpy.out: PmemWrap_memcpy.c
	mkdir -p bin
	gcc -o $@ $^ -pthread

#$@は:の左 $^は:の右