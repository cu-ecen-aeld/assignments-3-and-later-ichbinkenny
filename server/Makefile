CCOMP=gcc
BUILD_TOOL=
default: all

all:
		$(BUILD_TOOL)$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o aesdsocket aesdsocket.c -Wall -Werror $(LDFLAGS)

clean:
		@rm -f aesdsocket
