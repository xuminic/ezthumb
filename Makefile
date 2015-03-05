
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
ifeq 	($(SYSGUI),CFG_GUI_ON)
GUILIB	= -liup
endif

# Options: -mwindows, -mconsole -mwindows, -Wl,--subsystem,windows
ifndef	WINCON		
WINCON	= -mwindows
endif

CSOUP	= ./external/libcsoup
IUP	= ./external/iup


ifeq	($(SYSTOOL),mingw)
CC	= gcc -mms-bitfields
AR	= ar
CP	= cp
RM	= rm -f

IUPLIB	= $(IUP)/lib/mingw4
EXTDIR	= ./libmingw
EXTLIB	= $(CSOUP)/libcsoup.a
ifneq 	($(SYSGUI),CFG_GUI_OFF)
GUILIB	+= $(WINCON) -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 \
	  -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32
endif
SYSINC	= -I$(EXTDIR)/ffmpeg/include -I$(EXTDIR)/include
LIBDIR	= -L$(EXTDIR)/ffmpeg/lib -L$(EXTDIR)/lib -L$(CSOUP)
SYSFLAG	= -DUNICODE -D_UNICODE -DNONDLL #For linking static libgd 
endif

# This setting is used for POSIX environment with the following libraries
# installed: GTK+, FFMPEG, FreeType and libgd
ifeq	($(SYSTOOL),unix)
CC	= gcc
AR	= ar
CP	= cp
RM	= rm -f

IUPLIB	= $(shell echo $(IUP)/lib/*)
EXTLIB	= $(CSOUP)/libcsoup.a $(IUPLIB)/libiup.a
ifneq 	($(SYSGUI),CFG_GUI_OFF)
GUILIB	+= `pkg-config gtk+-2.0 --libs` -lX11
endif
SYSINC	= -I/usr/include/ffmpeg `pkg-config gtk+-2.0 --cflags`
LIBDIR	= -L$(IUPLIB) -L$(CSOUP)
SYSFLAG	= 
endif


PREFIX	= /usr/local
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/man1
RELDIR	= ./release-bin
OBJDIR  = ./objs

# CFG_SNAPSHOT_DUMP is used to save each frames to JPEG pictures
# CFG_SNAPSHOT_RAW is used to save each frames to YUV files
# CFG_SNAPSHOT_RGB is used to save each frames to RGB files
DEBUG	= -g -DDEBUG

# _FILE_OFFSET_BITS=64 is used to support files over 2GB in 32-bit OS
DEFINES = -D_FILE_OFFSET_BITS=64

INCDIR	= -I$(CSOUP) -I$(IUP)/include $(SYSINC)
CFLAGS	= -Wall -Wextra -O3 $(DEBUG) $(DEFINES) $(INCDIR) $(SYSFLAG) 


LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	  -lfreetype -lpng -ljpeg -lz -lm -lcsoup


ifeq	($(SYSGUI),CFG_GUI_GTK)
OBJGUI	= $(OBJDIR)/main_gui.o $(OBJDIR)/ezgui.o
else
ifeq ($(SYSGUI),CFG_GUI_ON)
OBJGUI	= $(OBJDIR)/main_gui.o $(OBJDIR)/eziup.o
else
OBJGUI	= $(OBJDIR)/main_con.o
endif
endif
OBJCON	= $(OBJDIR)/main_con.o

OBJS	= $(OBJDIR)/ezthumb.o  $(OBJDIR)/ezutil.o  $(OBJDIR)/id_lookup.o \
	  $(OBJDIR)/ezthumb_debug.o


RELDATE	= $(shell date  +%Y%m%d)
RELDIR	= ezthumb-$(shell version.sh)

ifeq	($(SYSTOOL),unix)
TGTGUI	= ezthumb
TGTCON	= ezthumb_con
TARGET	= $(TGTGUI)
else
LIBS	+= -liconv
OBJGUI	+= $(OBJDIR)/ezthumb_icon.o
TGTGUI	= ezthumb_win.exe
TGTCON	= ezthumb.exe
TARGET	= $(TGTCON) $(TGTGUI)
endif

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: objs

all: objdir $(EXTLIB) $(TARGET)

$(TGTGUI): ezconfig.h $(OBJGUI) $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJGUI) $(OBJS) $(LIBS) $(GUILIB)

$(TGTCON): ezconfig.h $(OBJCON) $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJCON) $(OBJS) $(LIBS)

$(OBJDIR)/main_con.o: main.c
	$(CC) $(CFLAGS) -DCFG_GUI_OFF -c -o $@ $<

$(OBJDIR)/main_gui.o: main.c
	$(CC) $(CFLAGS) -D$(SYSGUI) -c -o $@ $<

ezthumb.pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - $@

$(IUPLIB)/libiup.a:
	make -C $(IUP) do_all

$(CSOUP)/libcsoup.a: csoup

csoup:
	make -C $(CSOUP) all

objdir:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

ezconfig.h: conftest.c
	./mkconfig.sh > ezconfig.h

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

install: all
	install -s $(TARGET) $(BINDIR)
	install ezthumb.1 $(MANDIR)

CONTEST = DUMMY
conftest: conftest.c
	$(CC) $(CFLAGS) $(LIBDIR) -D$(CONTEST) -o $@ $< $(LIBS)

clean:
	$(RM) -f $(OBJDIR)/*
	$(RM) $(TGTGUI) $(TGTCON)
	$(RM) -f conftest conftest.exe conftest.log ezconfig.h

cleanall: clean
	make -C $(IUP) clean
	make -C $(CSOUP) clean

quickclean:
	make -C $(CSOUP) clean
	$(RM) $(TGTGUI) $(TGTCON)

link: quickclean all

ifeq	($(SYSTOOL),unix)
release: rel_source
else
release: rel_source rel_win_bin
endif
	
ifeq	($(SYSTOOL),unix)
debug: all
	$(CP) $(TARGET) ~/bin
else
debug: install_win
	$(RM) -r ~/bin/ezthumb
	mv $(RELDIR)-win-bin ~/bin/ezthumb
endif

rel_source: cleanall
	if [ -d $(RELDIR) ]; then $(RM) -r $(RELDIR); fi
	-mkdir $(RELDIR)
	-$(CP) *.c *.h *.1 *.pdf *.txt *.ico *.nsi *.sh *.rc *.lsm $(RELDIR)
	-$(CP) COPYING ChangeLog Makefile TODO ezqiz $(RELDIR)
	-$(CP) SMirC-thumbsup.svg $(RELDIR)
	-$(CP) -a external libmingw $(RELDIR)
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-$(RM) -r $(RELDIR)

install_win: all
	if [ -d $(RELDIR)-win-bin ]; then $(RM) -r $(RELDIR)-win-bin; fi
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.pdf ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXTDIR)/ffmpeg/bin/*.dll $(EXTDIR)/lib/*.dll $(RELDIR)-win-bin

rel_win_bin: all
	-echo "OutFile \"ezthumb-$(shell version.sh)-setup.exe\"" > nsis_version.txt
	makensis ezthumb.nsi
	-$(RM) nsis_version.txt 

showdll:
	@if [ -f ezthumb.exe ]; then \
		echo "[ezthumb.exe]:"; \
		objdump -p ezthumb.exe | grep 'DLL Name:'; \
	fi
	@if [ -f ezthumb_win.exe ]; then \
		echo "[ezthumb_win.exe]:"; \
		objdump -p ezthumb_win.exe | grep 'DLL Name:'; \
	fi

