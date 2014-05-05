
CFLAGS	+= -I..

#SRCS   := $(shell echo *.c)
SRCS	:= slog_bind_file.c	\
	   slog_bind_socket.c	\
	   slog_bind_stderr.c	\
	   slog_bind_stdout.c	\
	   slog_bind_window.c	\
	   slog.c		\
	   slogc.c		\
	   slog_control_word_read.c \
	   slog_level_read.c	\
	   slog_open.c		\
	   slog_output.c	\
	   slogs.c		\
	   slogz.c

OBJS	:= $(SRCS:.c=.o)

.PHONY: all clean
	
all: $(OBJS)

clean:
	$(RM) $(OBJS)



