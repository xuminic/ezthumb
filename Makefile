
CSOUP	= ../libcsoup

include $(CSOUP)/Make.conf


ifndef	SYSGUI		# Options: CFG_GUI_ON, CFG_GUI_OFF
export	SYSGUI	= CFG_GUI_ON
endif


ifeq    ($(SYSTOOL),cygwin)
EXDIR	= ..
FFMPEG  = $(EXDIR)/ffmpeg-0.6.1
LIBGD   = $(EXDIR)/gd-2.0.35
FREETYPE= $(EXDIR)/freetype-2.4.3
LIBJPEG = $(EXDIR)/jpeg-6b
LIBBZ2  = $(EXDIR)/bzip2-1.0.6
EXINC	= -I$(FFMPEG) -I$(LIBGD) -I$(FREETYPE) -I$(LIBJPEG) -I$(LIBBZ2)
EXLIB	= -L$(FREETYPE)/objs/.libs -L$(LIBJPEG) -L$(LIBBZ2) \
	  -L$(FFMPEG)/libavcodec        \
	  -L$(FFMPEG)/libavdevice       \
	  -L$(FFMPEG)/libavfilter       \
	  -L$(FFMPEG)/libavformat       \
	  -L$(FFMPEG)/libavutil         \
	  -L$(FFMPEG)/libswscale        \
	  -L$(LIBGD)/.libs
EXDLL	= /bin/cyggd-2.dll	\
	  /bin/cyggcc_s-1.dll	\
	  /bin/cygwin1.dll	\
	  /bin/cygXpm-4.dll	\
	  /bin/cygX11-6.dll	\
	  /bin/cygxcb-1.dll	\
	  /bin/cygXau-6.dll	\
	  /bin/cygXdmcp-6.dll	\
	  /bin/cygfontconfig-1.dll	\
	  /bin/cygexpat-1.dll	\
	  /bin/cygfreetype-6.dll	\
	  /bin/cygz.dll		\
	  /bin/cygiconv-2.dll	\
	  /bin/cygjpeg-7.dll	\
	  /bin/cygpng12.dll
endif

ifeq	($(SYSTOOL),mingw)
EXDIR	= ./libmingw
FFMPEG  = $(EXDIR)/ffmpeg-git-41bf67d-win32
LIBGD   = $(EXDIR)/gd-2.0.33-1
GTK2	= $(EXDIR)/gtk-2.24.8
EXINC	= -I$(FFMPEG)/include -I$(LIBGD)/include 
EXLIB	= -L$(FFMPEG)/lib -L$(LIBGD)/lib
GTKINC	= -I$(GTK2)/include 		\
	  -I$(GTK2)/include/atk-1.0	\
	  -I$(GTK2)/include/gtk-2.0 	\
	  -I$(GTK2)/include/gdk-pixbuf-2.0 \
	  -I$(GTK2)/include/cairo 	\
	  -I$(GTK2)/include/pango-1.0	\
	  -I$(GTK2)/include/glib-2.0	\
	  -I$(GTK2)/lib/gtk-2.0/include \
	  -I$(GTK2)/lib/glib-2.0/include
GTKLIB	= -L$(GTK2)/lib -lpthread -lgtk-win32-2.0 -lgdk-win32-2.0 -latk-1.0 \
	  -lgobject-2.0 -lgdk_pixbuf-2.0 -lglib-2.0 -lcairo -lpango-1.0
EXDLL	= $(FFMPEG)/bin/*.dll				\
	  $(EXDIR)/freetype-2.3.5-1/lib/freetype6.dll	\
	  $(LIBGD)/lib/libgd2.dll			\
	  $(EXDIR)/libpng-1.2.37/lib/libpng13.dll	\
	  $(EXDIR)/jpeg-6b-4/lib/jpeg62.dll		\
	  $(EXDIR)/libiconv-1.9.2-1/lib/*.dll		\
	  $(EXDIR)/zlib-1.2.3/lib/zlib1.dll		\
	  $(GTK2)/bin/intl.dll				\
	  $(GTK2)/bin/libatk-1.0-0.dll			\
	  $(GTK2)/bin/libcairo-2.dll			\
	  $(GTK2)/bin/libexpat-1.dll			\
	  $(GTK2)/bin/libfontconfig-1.dll		\
	  $(GTK2)/bin/libgdk-win32-2.0-0.dll		\
	  $(GTK2)/bin/libgdk_pixbuf-2.0-0.dll		\
	  $(GTK2)/bin/libgio-2.0-0.dll			\
	  $(GTK2)/bin/libglib-2.0-0.dll			\
	  $(GTK2)/bin/libgmodule-2.0-0.dll		\
	  $(GTK2)/bin/libgobject-2.0-0.dll		\
	  $(GTK2)/bin/libgthread-2.0-0.dll		\
	  $(GTK2)/bin/libgtk-win32-2.0-0.dll		\
	  $(GTK2)/bin/libpango-1.0-0.dll		\
	  $(GTK2)/bin/libpangocairo-1.0-0.dll		\
	  $(GTK2)/bin/libpangoft2-1.0-0.dll		\
	  $(GTK2)/bin/libpangowin32-1.0-0.dll		\
	  $(GTK2)/bin/libpng14-14.dll
endif

ifeq	($(SYSTOOL),unix)
CC	= gcc
AR	= ar
CP	= cp
RM	= rm -f

EXDIR	= 
FFMPEG  = /usr/include/ffmpeg
EXINC	= -I$(FFMPEG)
GTKINC	= `pkg-config gtk+-2.0 --cflags`
GTKLIB	= `pkg-config gtk+-2.0 --libs`
endif


RELDIR	= ./release-bin


ifeq   ($(SYSGUI),CFG_GUI_ON)
OBJDIR = objw
else
OBJDIR = objc
endif

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


INCDIR	= $(EXINC) -I$(CSOUP)
LIBDIR	= $(EXLIB) -L$(CSOUP)

CFLAGS	+= -D_FILE_OFFSET_BITS=64 -D$(SYSGUI) $(INCDIR)


ifeq	($(SYSGUI),CFG_GUI_ON)
CFLAGS	+= $(GTKINC)
endif


OBJS	= $(OBJDIR)/main.o	\
	  $(OBJDIR)/ezthumb.o	\
	  $(OBJDIR)/eznotify.o	\
	  $(OBJDIR)/id_lookup.o

ifeq	($(SYSGUI),CFG_GUI_ON)
OBJS	+= $(OBJDIR)/ezgui.o
endif

LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd
#	 -lfreetype -lbz2 -lm

ifeq	($(SYSGUI),CFG_GUI_ON)
LIBS	+= $(GTKLIB)
endif


ifeq	($(SYSTOOL),unix)
RELDIR	= ezthumb-`./version`
else
RELDIR	= ezthumb-`./version.exe`
endif
RELDATE	= `date  +%Y%m%d`



.PHONY: all 

all: objdir ezthumb ezthumb.pdf

ifeq	($(SYSTOOL),unix)
ezthumb: $(OBJS) 
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lcsoup
else
ezthumb:
	SYSGUI=CFG_GUI_OFF make objdir ezthumb.exe
	SYSGUI=CFG_GUI_ON  make objdir ezthumb_win.exe
endif

# internal rules, do not use it
ezthumb.exe: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) -lcsoup

# internal rules, do not use it
ezthumb_win.exe: $(OBJDIR)/ezthumb_icon.o $(OBJS)
	$(CC) $(CFLAGS) -mwindows $(LIBDIR) -o $@ $^ $(LIBS) -lcsoup

ezthumb.pdf: ezthumb.1
	man -l -Tps $< |ps2pdf - $@

objdir:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

version: version.c ezthumb.h
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $<

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

ifeq	($(SYSTOOL),unix)
install: ezthumb
	install -s ezthumb $(BINDIR)
	install ezthumb.1 $(MANDIR)
else
install: ezthumb version
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
	$(RM) ezthumb version
else
clean:
	SYSGUI=CFG_GUI_OFF make cleanobj
	SYSGUI=CFG_GUI_ON  make cleanobj
	$(RM) ezthumb.exe ezthumb_win.exe version.exe
endif

cleanall: clean
	$(RM) ezthumb.pdf

rel_source:
	if [ -d $(RELDIR) ]; then $(RM) -r $(RELDIR); fi
	-mkdir $(RELDIR)
	-$(CP) *.c *.h *.1 *.pdf *.txt *.ico Make* COPYING ChangeLog $(RELDIR)
	-$(CP) SMirC-thumbsup.svg $(RELDIR)
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-$(RM) -r $(RELDIR)

rel_win_dev:
	-tar czf ezthumb-libmingw-$(RELDATE).tar.gz libmingw

rel_win_bin: install
	-tar czf $(RELDIR)-win-bin.tar.gz $(RELDIR)-win-bin
	-$(RM) -r $(RELDIR)-win-bin

ifeq	($(SYSTOOL),unix)
release: version rel_source
else
release: version rel_source rel_win_dev rel_win_bin
endif
	

