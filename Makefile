CC = gcc
CFLALGS = -g -O0 -lwrappmem -lpmem -lpmemobj
TARGET = a.out
RUN_SRCS = libpmemtest.c libpmemtest2.c
SRCS = wraplibpmem.c wraplibpmemobj.c
OBJS = $(SRCS:.c=.o)
SHARED = libwrappmem.so

$(TARGET): $(SHARED)
	$(CC) -o $@ $(RUN_SRCS) $(CFLALGS)

$(SHARED): $(OBJS)
	$(CC) -shared -o $(SHARED) $(OBJS) -ldl -g -O0

wraplibpmem.o: wraplibpmem.c wraplibpmem.h
	$(CC) -c -fPIC wraplibpmem.c -o wraplibpmem.o -g -O0

wraplibpmemobj.o: wraplibpmemobj.c wraplibpmem.h wraplibpmemobj.h
	$(CC) -c -fPIC wraplibpmemobj.c -o wraplibpmemobj.o -g -O0

clean:
	-rm -f $(OBJS) $(SHARED) $(TARGET)

nowrap:
	$(CC) -o $(TARGET) $(RUN_SRCS) -lpmem -lpmemobj

#$@は:の左 $^は:の右