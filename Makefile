CC = gcc
CFLALGS = -ggdb -lwrappmem -lpmem -lpmemobj
TARGET = a.out
RUN_SRCS = libpmemtest.c libpmemtest2.c
SRCS = wraptest.c
OBJS = $(SRCS:.c=.o)
SHARED = libwrappmem.so
#OBJS = $(SRCS:.c=.o)

$(TARGET): $(SHARED)
	$(CC) -o $@ $(RUN_SRCS) $(CFLALGS)

$(SHARED): $(OBJS)
	$(CC) -shared -o $(SHARED) $(OBJS)

$(OBJS): $(SRCS)
	$(CC) -c -fPIC $(SRCS) -o $(OBJS)

clean:
	-rm -f $(OBJS) $(SHARED) $(TARGET)

nowrap:
	$(CC) -o $(TARGET) $(RUN_SRCS) -lpmem -lpmemobj

#$@は:の左 $^は:の右