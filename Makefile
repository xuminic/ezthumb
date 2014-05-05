
ifndef	SYSTOOL		# Options: cygwin, unix, mingw, cygmingw
ifeq	($(MSYSTEM),MINGW32)
SYSTOOL = mingw
else
SYSTOOL	= unix
endif
endif

ifndef	SYSGUI		# Options: CFG_GUI_ON, CFG_GUI_OFF, CFG_GUI_GTK
SYSGUI	= CFG_GUI_ON
endif

CSOUP	= external/libcsoup
IUP	= external/iup



ifeq	($(SYSTOOL),mingw)
CC	= gcc -mms-bitfields
AR	= ar
CP	= cp
RM	= rm -f

EXDIR	= ./libmingw
FFMPEG  = $(EXDIR)/ffmpeg
EXINC	= -I$(FFMPEG)/include -I$(EXDIR)/include -I$(CSOUP)

#WGLIB	= -lcomctl32 -lcomdlg32 -lgdi32 -Wl,--subsystem,windows
#WGLIB	= -lcomctl32 -mconsole -mwindows
WGLIB	= -liconv
EXDLL	= $(FFMPEG)/bin/*.dll $(EXDIR)/lib/*.dll

GUIINC	= -I$(EXDIR)/include/iup
#GUILIB	= -mwindows -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 \
#	  -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32
GUILIB	= -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 \
	  -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32
# linking static libgd requires defining NONDLL 
SYSFLAG	= -DUNICODE -D_UNICODE -DNONDLL $(EXINC)
LIBDIR	= -L$(FFMPEG)/lib -L$(EXDIR)/lib -L$(CSOUP)
endif


# This setting is used for POSIX environment with the following libraries
# installed: GTK+, FFMPEG, FreeType and libgd
ifeq	($(SYSTOOL),unix)
CC	= gcc
AR	= ar
CP	= cp
RM	= rm -f

GUIINC	= `pkg-config gtk+-2.0 --cflags`
GUILIB	= `pkg-config gtk+-2.0 --libs` -lX11
SYSINC	= -I/usr/include/ffmpeg $(GUIINC)
SYSLIB	= -L$(IUP)/lib/Linux26g4_64
EXLIB	= $(CSOUP)/libcsoup.a $(IUP)/lib/Linux26g4_64/libiup.a
ifneq 	($(SYSGUI),CFG_GUI_GTK)
GUILIB	+= -liup
endif
endif


PREFIX	= /usr/local
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/man1

DEBUG	= -g -DDEBUG
DEFINES = -D_FILE_OFFSET_BITS=64
INCDIR	= -I$(CSOUP) -I$(IUP)/include $(SYSINC)
LIBDIR  = -L$(CSOUP) $(SYSLIB)
CFLAGS	= -Wall -Wextra -O3 $(DEBUG) $(DEFINES) $(INCDIR) $(SYSFLAG) 

RELDIR	= ./release-bin
OBJDIR  = ./objs

LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	  -lfreetype -lpng -ljpeg -lz -lm -lcsoup $(WGLIB)

ifeq	($(SYSGUI),CFG_GUI_GTK)
OBJGUI	= $(OBJDIR)/main_gui.o $(OBJDIR)/ezgui.o
else
OBJGUI	= $(OBJDIR)/main_gui.o $(OBJDIR)/eziup.o
endif
OBJCON	= $(OBJDIR)/main_con.o

OBJS	= $(OBJDIR)/ezthumb.o  $(OBJDIR)/ezutil.o  $(OBJDIR)/id_lookup.o


RELDATE	= $(shell date  +%Y%m%d)
RELDIR	= ezthumb-$(shell version.sh)

ifeq	($(SYSTOOL),unix)
TGTGUI	= ezthumb
TGTCON	= ezthumb_con
TARGET	= $(TGTGUI)
else
TGTGUI	= ezthumb_win.exe
TGTCON	= ezthumb.exe
TARGET	= $(TGTGUI) $(TGTCON)
endif

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: objs

all: objdir $(EXLIB) $(TARGET)

$(TGTGUI): $(OBJGUI) $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) $(GUILIB)

$(TGTCON): $(OBJCON) $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS)

$(OBJDIR)/main_con.o: main.c
	$(CC) $(CFLAGS) -DCFG_GUI_OFF -c -o $@ $<

$(OBJDIR)/main_gui.o: main.c
	$(CC) $(CFLAGS) -D$(SYSGUI) -c -o $@ $<

ezthumb.pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - $@

$(IUP)/lib/Linux26g4_64/libiup.a:
	make -C $(IUP) do_all

$(CSOUP)/libcsoup.a:
	make -C $(CSOUP) all

objdir:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

install: all
	install -s $(TARGET) $(BINDIR)
	install ezthumb.1 $(MANDIR)

debug: all
	$(CP) $(TARGET) ~/bin

clean:
	$(RM) -r $(OBJDIR)
	$(RM) $(TGTGUI) $(TGTCON)

cleanall: clean
	make -C $(IUP) clean
	make -C $(CSOUP) clean

ifeq	($(SYSTOOL),unix)
release: rel_source
else
release: rel_source rel_win_bin
endif
	
rel_source: cleanall
	if [ -d $(RELDIR) ]; then $(RM) -r $(RELDIR); fi
	-mkdir $(RELDIR)
	-$(CP) *.c *.h *.1 *.pdf *.txt *.ico Make* $(RELDIR)
	-$(CP) COPYING ChangeLog SMirC-thumbsup.svg $(RELDIR)
	-$(CP) -a external libmingw $(RELDIR)
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-$(RM) -r $(RELDIR)

install_win: all
	if [ -d $(RELDIR)-win-bin ]; then $(RM) -r $(RELDIR)-win-bin; fi
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.pdf ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXDLL) $(RELDIR)-win-bin

rel_win_bin: install_win
	-tar czf $(RELDIR)-win-bin.tar.gz $(RELDIR)-win-bin
	-$(RM) -r $(RELDIR)-win-bin

showdll:
	@if [ -f ezthumb.exe ]; then \
		echo "[ezthumb.exe]:"; \
		objdump -p ezthumb.exe | grep 'DLL Name:'; \
	fi
	@if [ -f ezthumb_win.exe ]; then \
		echo "[ezthumb_win.exe]:"; \
		objdump -p ezthumb_win.exe | grep 'DLL Name:'; \
	fi

