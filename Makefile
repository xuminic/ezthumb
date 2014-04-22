
ifndef	SYSTOOL		# Options: cygwin, unix, mingw, cygmingw
ifeq	($(MSYSTEM),MINGW32)
export	SYSTOOL = mingw
else
export	SYSTOOL	= unix
endif
endif

ifndef	SYSAPI		# Options: CFG_WIN32_API, CFG_UNIX_API
ifeq	($(MSYSTEM),MINGW32)
export	SYSAPI	= CFG_WIN32_API
else
export	SYSAPI	= CFG_UNIX_API
endif
endif

ifndef	SYSGUI		# Options: CFG_GUI_ON, CFG_GUI_OFF
export	SYSGUI	= CFG_GUI_ON
endif

CSOUP	= ../libcsoup
IUP	= ../iup/iup



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

ifeq	($(SYSTOOL),unix)
CC	= gcc
AR	= ar
CP	= cp
RM	= rm -f

EXDIR	= 
FFMPEG  = /usr/include/ffmpeg
EXINC	= -I$(FFMPEG) -I$(CSOUP) -I$(IUP)/include

GUIINC	= `pkg-config gtk+-2.0 --cflags`
GUILIB	= `pkg-config gtk+-2.0 --libs` -lX11
SYSFLAG	= $(EXINC)
LIBDIR	= -L$(CSOUP) -L$(IUP)/lib/Linux26g4_64
endif


PREFIX	= /usr/local
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/man1

DEBUG	= -g -DDEBUG
DEFINES = -D_FILE_OFFSET_BITS=64 -D$(SYSGUI)
CFLAGS	= -Wall -Wextra -O3 $(DEBUG) $(DEFINES) $(SYSFLAG) 

RELDIR	= ./release-bin


ifeq   ($(SYSGUI),CFG_GUI_OFF)
OBJDIR = objc
else
# CFG_GUI_ON and CFG_GUI_GTK
OBJDIR = objw
endif

LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	 -lfreetype -lpng -ljpeg -lz -lm -lcsoup $(WGLIB)

OBJS	= $(OBJDIR)/main.o	\
	  $(OBJDIR)/ezthumb.o	\
	  $(OBJDIR)/ezutil.o	\
	  $(OBJDIR)/id_lookup.o

ifeq	($(SYSGUI),CFG_GUI_GTK)
OBJS	+= $(OBJDIR)/ezgui.o
LIBS	+= $(GUILIB)
CFLAGS	+= $(GUIINC)
endif
ifeq	($(SYSGUI),CFG_GUI_ON)
OBJS	+= $(OBJDIR)/eziup.o
LIBS	+= -liup $(GUILIB)
CFLAGS	+= $(GUIINC)
endif

RELDATE	= $(shell date  +%Y%m%d)
RELDIR	= ezthumb-$(shell version.sh)


.PHONY: all

all: objdir ezthumb ezthumb.pdf

ifeq	($(SYSTOOL),unix)
ezthumb: $(OBJS) 
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS)
else
ezthumb:
	SYSGUI=CFG_GUI_OFF make objdir ezthumb.exe
	SYSGUI=CFG_GUI_ON  make objdir ezthumb_win.exe
endif

# internal rules, do not use it
ezthumb.exe: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS)

# internal rules, do not use it
ezthumb_win.exe: $(OBJDIR)/ezthumb_icon.o $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) 

ezthumb.pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - $@

objdir:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

showdll:
	@if [ -f ezthumb.exe ]; then \
		echo "[ezthumb.exe]:"; \
		objdump -p ezthumb.exe | grep 'DLL Name:'; \
	fi
	@if [ -f ezthumb_win.exe ]; then \
		echo "[ezthumb_win.exe]:"; \
		objdump -p ezthumb_win.exe | grep 'DLL Name:'; \
	fi

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

ifeq	($(SYSTOOL),unix)
install: ezthumb
	install -s ezthumb $(BINDIR)
	install ezthumb.1 $(MANDIR)
else
install: ezthumb 
	if [ -d $(RELDIR)-win-bin ]; then $(RM) -r $(RELDIR)-win-bin; fi
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.pdf ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXDLL) $(RELDIR)-win-bin
endif

debug: ezthumb
	$(CP) ezthumb ~/bin

cleanobj:
	$(RM) -r $(OBJDIR)

ifeq	($(SYSTOOL),unix)
clean: cleanobj
	$(RM) ezthumb 
else
clean:
	SYSGUI=CFG_GUI_OFF make cleanobj
	SYSGUI=CFG_GUI_ON  make cleanobj
	$(RM) ezthumb.exe ezthumb_win.exe 
endif

cleanall: clean
	$(RM) ezthumb.pdf

rel_source:
	if [ -d $(RELDIR) ]; then $(RM) -r $(RELDIR); fi
	-mkdir $(RELDIR)
	RELCS=`pwd`/$(RELDIR)/libcsoup make -C $(CSOUP) release
	-mkdir $(RELDIR)/ezthumb
	-$(CP) *.c *.h *.1 *.pdf *.txt *.ico Make* $(RELDIR)/ezthumb
	-$(CP) COPYING ChangeLog SMirC-thumbsup.svg $(RELDIR)/ezthumb
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-$(RM) -r $(RELDIR)

rel_win_dev:
	-tar czf ezthumb-libmingw-$(RELDATE).tar.gz libmingw

rel_win_bin: install
	-tar czf $(RELDIR)-win-bin.tar.gz $(RELDIR)-win-bin
	-$(RM) -r $(RELDIR)-win-bin

ifeq	($(SYSTOOL),unix)
release: rel_source
else
release: rel_source rel_win_dev rel_win_bin
endif
	
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

