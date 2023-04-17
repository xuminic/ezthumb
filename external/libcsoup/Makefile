
export	CC	= gcc
export	AR	= ar crs
export	CP	= cp
export	RM	= rm -f

PREFIX	= /usr/local
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/man1

#export	SLOG_SOCKET = -DCFG_SLOG_SOCKET

# Options: CFG_WIN32_API, CFG_UNIX_API CFG_SLOG_SOCKET
SYSAPI	= 
DEBUG	= -DDEBUG -DCFG_CDLL_SAFE -DCFG_UNIT_TEST
DEFINES = -DUNICODE -D_UNICODE -D_FILE_OFFSET_BITS=64 $(SLOG_SOCKET)

export	CFLAGS	= -Wall -O3 $(DEBUG) $(DEFINES) $(SYSAPI)

ifndef	RELCS
RELCS	= libcsoup-$(shell version.sh)
endif

TARGET	= libcsoup.a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: all clean cli soup slog smm main doc
	
all: main

main: $(TARGET)
	make -C main all

$(TARGET) : cli slog smm soup
	$(RM) $(TARGET)
	$(AR) $(TARGET) cli/*.o slog/*.o smm/*.o soup/*.o 

cli:
	make -C cli all

slog:
	make -C slog all

smm:
	make -C smm all

soup:
	make -C soup all

doc:
	doxygen doc/Doxyfile

cleandoc:
	$(RM) -r doc/html  doc/latex

clean:
	make -C cli clean
	make -C main clean
	make -C slog clean
	make -C smm clean
	make -C soup clean
	$(RM) $(TARGET) packhex

release:
	if [ -d $(RELCS) ]; then $(RM) -r $(RELCS); fi
	-mkdir $(RELCS) $(RELCS)/doc
	$(CP) ChangeLog Makefile *.h *.sh $(RELCS)
	$(CP) -a cli main slog smm soup $(RELCS)
	$(CP) doc/Doxyfile $(RELCS)/doc
	make -C $(RELCS) clean
	$(RM) $(RELCS).tar.gz
	tar czf $(RELCS).tar.gz $(RELCS)

