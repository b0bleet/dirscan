appname := dirscan
CC := gcc
DIR := src/
SRCS := $(wildcard *.c)
OBJS := $(SRCS:c=o)
INCLUDES = -I./
.PHONY: all clean
all: $(appname)
UNAME_S := $(shell uname -s)
CFLAGS+= -g -std=c99
LDFLAGS+= -lcurl
ifneq ($(UNAME_S),Darwin)
	LDFLAGS+= -lpthread
	CFLAGS+= -fno-common
endif
#-fsanitize=address
$(appname): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) -g
	@rm -rf $(OBJS)

$(OBJS): %.o: %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES)

clean:
	@rm -rf $(appname) $(OBJS)
