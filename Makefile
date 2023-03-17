CC = gcc
# CFLALGS = -g -O0 -lwrappmem -lpmem -lpmemobj
TARGET = libwrappmem.so
SRCS = wraplibpmem.c wraplibpmemobj.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET) PmemWrap_memcpy.out

$(TARGET): $(OBJS)
	$(CC) -shared -o $(TARGET) $(OBJS) -ldl -pthread -g -O0

wraplibpmem.o: wraplibpmem.c wraplibpmem.h
	$(CC) -c -fPIC wraplibpmem.c -o wraplibpmem.o -pthread -g -O0

wraplibpmemobj.o: wraplibpmemobj.c wraplibpmem.h wraplibpmemobj.h
	$(CC) -c -fPIC wraplibpmemobj.c -o wraplibpmemobj.o -g -O0

clean:
	-rm -f $(OBJS) $(TARGET) PmemWrap_memcpy.out

PmemWrap_memcpy.out: PmemWrap_memcpy.c
	gcc -o $@ $^ -pthread

#$@は:の左 $^は:の右