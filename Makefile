CC = gcc
CFLALGS = -lwrappmem -lpmem -lpmemobj
TARGET = a.out
RUN_SRCS = libpmemtest.c
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

#$@は:の左 $^は:の右