
CFLAGS	+= -I..

#SRCS	:= $(shell echo *.c)
SRCS	:= csc_cdll.c		\
	   csc_cli_alloc_list.c	\
	   csc_cli_alloc_table.c \
	   csc_cli_getopt_alloc.c \
	   csc_cli_getopt.c	\
	   csc_cli_option.c	\
	   csc_cmp_file_extargs.c \
	   csc_cmp_file_extlist.c \
	   csc_cmp_file_extname.c \
	   csc_config.c		\
	   csc_crc16.c		\
	   csc_crc16c.c		\
	   csc_crc32.c		\
	   csc_crc8.c		\
	   csc_cuttoken.c	\
	   csc_extname_filter_match.c \
	   csc_extname_filter_open.c \
	   csc_file_load.c	\
	   csc_file_store.c	\
	   csc_strbody.c	\
	   csc_strcmp_list.c	\
	   csc_path_basename.c	\
	   csc_path_path.c	\
	   csc_fixtoken.c	\
	   csc_gettoken.c	\
	   csc_isdelim.c	\
	   csc_iso639.c		\
	   csc_memdump.c	\
	   csc_mkargs.c		\
	   csc_strcpy_alloc.c	\
	   csc_strlcpy.c	\
	   csc_ziptoken.c

OBJS	:= $(SRCS:.c=.o)

.PHONY: all clean
	
all: $(OBJS)

clean:
	$(RM) $(OBJS)



