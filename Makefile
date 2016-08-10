
ifndef	SYSTOOL		# Options: cygwin, unix, mingw, cygmingw
  ifeq	($(MSYSTEM),MINGW32)
    SYSTOOL = mingw
  else
    SYSTOOL = unix
  endif
endif

ifndef	SYSGUI		# Options: CFG_GUI_ON, CFG_GUI_OFF 
  SYSGUI = CFG_GUI_ON
endif

ifeq	($(SYSTOOL),mingw)
CC	= gcc -mms-bitfields
AR	= ar
CP	= cp
RM	= rm -f

SYSINC  = -I./libmingw/include -I./libmingw/include/iup \
	  -I./libmingw/ffmpeg/include
SYSLDD  = -L./libmingw/lib -L./libmingw/ffmpeg/lib
SYSFLAG = -DUNICODE -D_UNICODE -D_WIN32_IE=0x0500 -DWINVER=0x500 \
	  -DNONDLL #For linking static libgd
SYSLIB  = -liconv

# Options: -mwindows, -mconsole -mwindows, -Wl,--subsystem,windows
ifeq    ($(SYSGUI),CFG_GUI_OFF)
  SYSLIB += -mconsole
else
  SYSLIB += -mwindows -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 \
            -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32
endif
endif

# This setting is used for POSIX environment with the following libraries
# installed: GTK+, FFMPEG, FreeType and libgd
ifeq	($(SYSTOOL),unix)
CC	= gcc
AR	= ar
CP	= cp
RM	= rm -f

SYSINC  = -I./external/libcsoup -I/usr/include/ffmpeg
SYSLDD  = -L./external/libcsoup
SYSLIB  =
SYSFLAG =

ifeq	($(SYSGUI),CFG_GUI_ON)
  include MakeGTK.mk
  SYSINC += -I./external/iup/include $(GTKINC)
  SYSLIB += $(GTKLIB) -lX11
  SYSLDD += -L`echo ./external/iup/lib/*`
endif
endif


PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
MANDIR	= $(PREFIX)/share/man/man1
APPDIR	= $(PREFIX)/share/applications
P_ICON  = /usr/share/icons/hicolor
M_ICON  = apps/ezthumb.png

OBJDIR  = ./objs

CONVERT	= convert -background transparent ./external/icons/SMirC-thumbsup.svg

# CFG_SNAPSHOT_DUMP is used to save each frames to JPEG pictures
# CFG_SNAPSHOT_RAW is used to save each frames to YUV files
# CFG_SNAPSHOT_RGB is used to save each frames to RGB files
DEBUG	= -g -DDEBUG -O0

# _FILE_OFFSET_BITS=64 is used to support files over 2GB in 32-bit OS
DEFINES = -D_FILE_OFFSET_BITS=64 -D$(SYSGUI)
CFLAGS	= -Wall -Wextra $(DEBUG) $(DEFINES) $(SYSINC) $(SYSFLAG) 

PROJECT	= ezthumb

ifeq	($(SYSTOOL),unix)
  TARGET = $(PROJECT)
else
  ifeq  ($(SYSGUI),CFG_GUI_OFF)
    TARGET = $(PROJECT).exe
  else
    TARGET = $(PROJECT)_win.exe
  endif
endif

RELDATE := $(shell date +%Y%m%)
RELVERS := $(shell grep EZTHUMB_VERSION ezthumb.h | cut -d\" -f 2)
RELDIR  = $(PROJECT)-$(RELVERS)
RELWIN  = $(RELDIR)-win32-bin

OBJS	= $(OBJDIR)/main.o $(OBJDIR)/ezthumb.o  $(OBJDIR)/ezutil.o \
	  $(OBJDIR)/id_lookup.o
DEFLIB	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	  -lfreetype -lpng -ljpeg -lz -lm -lcsoup
LIBS	= $(DEFLIB)

ifeq ($(SYSGUI),CFG_GUI_ON)
  OBJS += $(OBJDIR)/ezgui.o
  ifeq ($(SYSTOOL),mingw)
    OBJS += $(OBJDIR)/ezthumb_icon.o
  endif
  LIBS += -liup -liupimglib
endif
LIBS += $(SYSLIB)


# This list of files for source release
RELLIST	= ChangeLog	\
	  conftest.c	\
	  COPYING	\
	  ezgui.c	\
	  ezgui.h	\
	  ezicon.h	\
	  ezqiz		\
	  ezthumb.1	\
	  ezthumb.c	\
	  ezthumb.h	\
	  ezthumb.ico	\
	  ezthumb_icon.rc	\
	  ezthumb.lsm	\
	  ezthumb.nsi	\
	  ezthumb.pdf	\
	  ezutil.c	\
	  id_lookup.c	\
	  id_lookup.h	\
	  main.c	\
	  Makefile	\
	  mkconfig.sh	\
	  Readme.txt	\
	  TODO

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: objs all clean cleanobj extlib extclean release installer_win \
	release-win showdll install uninstall

all: $(TARGET)
	@echo GTK=$(GTKVER) $(IUPCFG)

$(TARGET): objdir ezconfig.h $(OBJS)
	$(IUPCFG) make -C external all
	$(CC) $(CFLAGS) $(SYSLDD) -o $@ $(OBJS) $(LIBS)

ezthumb.pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - $@

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

objdir:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi


ezconfig.h: conftest.c
	./mkconfig.sh > ezconfig.h

CONTEST = DUMMY
conftest: conftest.c
	$(CC) $(CFLAGS) $(SYSLDD) -D$(CONTEST) -o $@ $< $(DEFLIB)

clean:
	$(RM) -f $(OBJDIR)/*
	$(RM) $(TARGET)
	$(RM) -f conftest conftest.exe conftest.log ezconfig.h

extlib:
	$(IUPCFG) make -C external all

extclean:
	$(IUPCFG) make -C external clean


ifeq	($(SYSTOOL),unix)
release: extclean
else
release: extclean installer_win
endif
	-if [ -d $(RELDIR) ]; then $(RM) -r $(RELDIR); fi
	-mkdir $(RELDIR)
	-$(CP) $(RELLIST) $(RELDIR)
	-$(CP) -a external libmingw $(RELDIR)
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-$(RM) -r $(RELDIR)

installer_win: release-win
	-echo "OutFile \"$(RELDIR)-setup.exe\"" > nsis_version.txt
	makensis ezthumb.nsi
	-$(RM) nsis_version.txt 

release-win:
	-if [ -d $(RELWIN) ]; then $(RM) -r $(RELWIN); fi
	-mkdir $(RELWIN)
	-$(CP) ezthumb.1 ezthumb.pdf ezthumb.ico $(RELWIN)
	-$(CP) ./libmingw/ffmpeg/bin/*.dll ./libmingw/lib/*.dll $(RELWIN)
	SYSGUI=CFG_GUI_OFF make clean
	SYSGUI=CFG_GUI_OFF make
	-$(CP) $(PROJECT).exe $(RELWIN)
	SYSGUI=CFG_GUI_ON make clean
	SYSGUI=CFG_GUI_ON make
	-$(CP) $(PROJECT)_win.exe $(RELWIN)
	-7z a -tzip $(RELWIN).zip $(RELWIN)

install:
	install -s ezthumb $(BINDIR)
	cp -f ezthumb.1 $(MANDIR)
	cp -f ezthumb.desktop $(APPDIR)
	$(CONVERT) -resize 256x256 $(P_ICON)/256x256/$(M_ICON)
	$(CONVERT) -resize 128x128 $(P_ICON)/128x128/$(M_ICON)
	$(CONVERT) -resize 32x32   $(P_ICON)/32x32/$(M_ICON)
	$(CONVERT) -resize 24x24   $(P_ICON)/24x24/$(M_ICON)
	$(CONVERT) -resize 22x22   $(P_ICON)/22x22/$(M_ICON)
	$(CONVERT) -resize 16x16   $(P_ICON)/16x16/$(M_ICON)
	gtk-update-icon-cache -f $(P_ICON)

uninstall:
	rm -f $(BINDIR)/ezthumb
	rm -f $(MANDIR)/ezthumb.1
	rm -f $(APPDIR)/ezthumb.desktop
	rm -f $(P_ICON)/256x256/$(M_ICON)
	rm -f $(P_ICON)/128x128/$(M_ICON)
	rm -f $(P_ICON)/48x48/$(M_ICON)
	rm -f $(P_ICON)/32x32/$(M_ICON)
	rm -f $(P_ICON)/24x24/$(M_ICON)
	rm -f $(P_ICON)/22x22/$(M_ICON)
	rm -f $(P_ICON)/16x16/$(M_ICON)

showdll:
	@if [ -f ezthumb.exe ]; then \
		echo "[ezthumb.exe]:"; \
		objdump -p ezthumb.exe | grep 'DLL Name:'; \
	fi
	@if [ -f ezthumb_win.exe ]; then \
		echo "[ezthumb_win.exe]:"; \
		objdump -p ezthumb_win.exe | grep 'DLL Name:'; \
	fi

