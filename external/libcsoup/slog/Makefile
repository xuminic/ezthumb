
CFLAGS	+= -I..

#SRCS   := $(shell echo *.c)
ifdef	SLOG_SOCKET
SRCS	:= slog_core.c slog_csoup.c slog_socket.c
else
SRCS	:= slog_core.c slog_csoup.c
endif


OBJS	:= $(SRCS:.c=.o)

.PHONY: all clean
	
all: $(OBJS)

clean:
	$(RM) $(OBJS)



