
CFLAGS	+= -I..

#SRCS	:= $(shell echo *.c)
SRCS	:= csc_cli_command.c	\
	   csc_cli_getopt.c	\
	   csc_cli_make_list.c	\
	   csc_cli_make_table.c \
	   csc_cli_mkargs.c	\
	   csc_cli_print.c	\
	   csc_cli_private.c	\
	   csc_cli_qopt.c

OBJS	:= $(SRCS:.c=.o)

.PHONY: all clean
	
all: $(OBJS)

clean:
	$(RM) $(OBJS)



