CCOMP=gcc
BUILD_TOOL=
default: all

all:
		$(BUILD_TOOL)$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o aesdsocket aesdsocket.c -Wall -Werror $(LDFLAGS) -pthread -g

clean:
		@rm -f aesdsocket
