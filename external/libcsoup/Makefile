
export	CC	= gcc
export	AR	= ar
export	CP	= cp
export	RM	= rm -f

PREFIX	= /usr/local
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/man1

# Options: CFG_WIN32_API, CFG_UNIX_API
SYSAPI	= 
DEBUG	= -g -DDEBUG
DEFINES = 

export	CFLAGS	= -Wall -Wextra -O3 $(DEBUG) $(DEFINES) $(SYSAPI) 

ifndef	RELCS
RELCS	= libcsoup-$(shell version.sh)
endif

TARGET	= libcsoup.a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: all clean soup slog smm main doc
	
all: main

main: $(TARGET)
	make -C main all

$(TARGET) : soup slog smm
	$(RM) $(TARGET)
	$(AR) crus $(TARGET) soup/*.o slog/*.o smm/*.o

soup:
	make -C soup all

slog:
	make -C slog all

smm:
	make -C smm all

doc:
	doxygen doc/Doxyfile

clean:
	make -C soup clean
	make -C slog clean
	make -C smm clean
	make -C main clean
	$(RM) $(TARGET)

release:
	if [ -d $(RELCS) ]; then $(RM) -r $(RELCS); fi
	-mkdir $(RELCS)
	$(CP) *.h Make* $(RELCS)
	$(CP) -a soup slog smm main $(RELCS)
	make -C $(RELCS) clean
	make -C $(RELCS)/main clean


