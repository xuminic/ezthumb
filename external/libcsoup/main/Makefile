
CC      = gcc
AR      = ar
#CC      = i686-w64-mingw32-gcc
#AR      = i686-w64-mingw32-ar

CP      = cp
RM      = rm -f

ifndef	CFLAGS
CFLAGS	= -Wall -Wextra -O3 -DDEBUG -DCFG_CDLL_SAFE -DCFG_UNIT_TEST
endif
CFLAGS	+= -I.. -I.

ifdef	SLOG_SOCKET
EXLIBS	= -lpthread
endif

SRCS	= main.c fixtoken.c memdump.c slog.c crc.c fontpath.c config.c \
	  csc_cdll.c csc_cli.c smm.c strings.c packhex.c dummy.c mem.c

TARGET	= csoup


.PHONY: all clean
all: $(TARGET)

csoup: $(SRCS) main_define.h packhex.h libcsoup
	$(CC) $(CFLAGS) -L.. -o $@ $(SRCS) -lcsoup $(EXLIBS)

main_define.h: mkclicmd
	grep "extern.*struct.*clicmd" $(SRCS) | cut -d: -f2 |tr -d '\r' > $@
	./mkclicmd $@

mkclicmd: mkclicmd.c
	$(CC) $(CFLAGS) -L.. -o $@ $< -lcsoup

packhex.h: packhex
	packhex -i packed_hex *.c > $@

packhex: ../soup/csc_pack_hex.c
	$(CC) $(CFLAGS) -L.. -DEXECUTABLE -o $@ $< -lcsoup

libcsoup:
	(cd ..; make libcsoup.a)

universal_crc: universal_crc.c
	$(CC) $(CFLAGS) -o $@ $<

crc: universal_crc
	universal_crc -a tab -b 32 -p 0x04c11db7 -i 0xffffffff -x 0xffffffff -r > crc32.c
	universal_crc -a tab -b 16 -p 0x1021 > crc16.c
	universal_crc -a tab -b 8 -p 0x7 > crc8.c

clean:
	$(RM) $(TARGET) main_define.h packhex.h universal_crc mkclicmd packhex
	$(RM) $(TARGET).exe universal_crc.exe mkclicmd.exe packhex.exe



