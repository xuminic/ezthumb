
CC      = gcc
AR      = ar
CP      = cp
RM      = rm -f

ifndef	CFLAGS
CFLAGS	= -Wall -Wextra
endif
CFLAGS	+= -I..

ifdef	SLOG_SOCKET
EXLIBS	= -lpthread
endif

SRCS	= main.c fixtoken.c memdump.c slog.c crc.c fontpath.c config.c \
	  csc_cdll.c csc_cli.c smm.c

TARGET	= csoup


.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SRCS) main_define.h
	$(CC) $(CFLAGS) -L.. -o $@ $(SRCS) -lcsoup $(EXLIBS)

main_define.h: mkclicmd
	grep "extern.*struct.*clicmd" $(SRCS) | cut -d: -f2 |tr -d '\r' > $@
	./mkclicmd $@

mkclicmd: mkclicmd.c
	$(CC) $(CFLAGS) -L.. -o $@ $< -lcsoup

universal_crc: universal_crc.c
	$(CC) $(CFLAGS) -o $@ $<

crc: universal_crc
	universal_crc -a tab -b 32 -p 0x04c11db7 -i 0xffffffff -x 0xffffffff -r > crc32.c
	universal_crc -a tab -b 16 -p 0x1021 > crc16.c
	universal_crc -a tab -b 8 -p 0x7 > crc8.c

clean:
	$(RM) $(TARGET) main_define.h
	$(RM) universal_crc



