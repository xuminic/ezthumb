
# CFG_SNAPSHOT_DUMP is used to save each frames to JPEG pictures
# CFG_SNAPSHOT_RAW is used to save each frames to YUV files
# CFG_SNAPSHOT_RGB is used to save each frames to RGB files

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install
SUBDIRS = .


# _FILE_OFFSET_BITS=64 is used to support files over 2GB in 32-bit OS
DEFINES = -D_FILE_OFFSET_BITS=64 -D$(SYSGUI)
#CFLAGS	= -Wall -Wextra $(DEBUG) $(DEFINES) $(SYSINC) $(SYSFLAG) 

DEFS	+= $(DEBUGS)

RELDATE := $(shell date +%Y%m%)
RELVERS := $(shell grep EZTHUMB_VERSION ezthumb.h | cut -d\" -f 2)
RELDIR  = $(PROJECT)-$(RELVERS)
RELWIN  = $(RELDIR)-win32-bin

#OBJS	= $(OBJDIR)/main.o $(OBJDIR)/ezthumb.o  $(OBJDIR)/ezutil.o \
#	  $(OBJDIR)/id_lookup.o
DEFLIB	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	  -lfreetype -lpng -ljpeg -lz -lm -lcsoup



bin_PROGRAMS	= ezthumb
ezthumb_SOURCES	= main.c ezthumb.c ezthumb.h ezgui.c ezgui.h ezicon.h ezutil.c id_lookup.c id_lookup.h


# This list of files for source release
RELLIST	= ChangeLog  conftest.c	 COPYING  ezgui.c  ezgui.h  ezicon.h  ezqiz \
	  ezthumb.1  ezthumb.c  ezthumb.h  ezthumb.ico  ezthumb_icon.rc \
	  ezthumb.lsm  ezthumb.nsi  ezthumb.pdf	 ezutil.c  id_lookup.c	\
	  id_lookup.h  main.c  Makefile	 mkconfig.sh  Readme.txt  TODO


#all: $(TARGET)
#	@echo GTK=$(GTKVER) $(IUPCFG)

#$(TARGET): objdir ezconfig.h $(OBJS)
#	$(IUPCFG) make -C external all
#	$(CC) $(CFLAGS) $(SYSLDD) -o $@ $(OBJS) $(LIBS)

var:
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo DEBUG = $(DEBUG)
	@echo IUPCFG = $(IUPCFG)
	@echo LIBS = $(LIBS)
	@echo SYSTOOL = $(SYSTOOL)
	@echo SYSGUI = $(SYSGUI)
	@echo COMPILE = $(COMPILE)
	@echo LINK = $(LINK)


$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@


ezconfig.h: conftest.c
	./mkconfig.sh > ezconfig.h

CONTEST = DUMMY
conftest: conftest.c
	$(CC) $(CFLAGS) $(SYSLDD) -D$(CONTEST) -o $@ $< $(DEFLIB)

#clean:
#	$(RM) -f $(OBJDIR)/*
#	$(RM) $(TARGET)
#	$(RM) -f conftest conftest.exe conftest.log ezconfig.h

extlib:
	$(IUPCFG) make -C external all

extclean:
	$(IUPCFG) make -C external clean


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

gnome-install:
	$(INSTALL_DATA) ezthumb.desktop /usr/share/applications
	@for res in 16 22 24 32 48 256; do \
	 $(INSTALL_DATA) -T ./external/icons/ezthumb"$$res".png \
	    /usr/share/icons/hicolor/"$$res"x"$$res"/apps/ezthumb.png; \
	done
	gtk-update-icon-cache -f /usr/share/icons/hicolor

gnome-uninstall:
	rm -f /usr/share/applications/ezthumb.desktop
	@for res in 16 22 24 32 48 256; do \
	  rm -f /usr/share/icons/hicolor/"$$res"x"$$res"/apps/ezthumb.png; \
	done
	gtk-update-icon-cache -f /usr/share/icons/hicolor

# supportive rule for generating files 
# since special tools are involved so they can be only internally used.
ezthumb.pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - $@

gtkicons:
	@for res in 16 22 24 32 48 128 256; do \
	  convert -background transparent ./external/icons/SMirC-thumbsup.svg \
	    -resize "$$res"x"$$res" ./external/icons/ezthumb"$$res".png; \
	done

installer_win: release-win
	@echo "OutFile \"$(RELDIR)-setup.exe\"" > nsis_version.txt
	makensis ezthumb.nsi
	@$(RM) nsis_version.txt 


# show DLLs linked to ezthumb in MSWindows
showdll:
	@if [ -f ezthumb.exe ]; then \
		echo "[ezthumb.exe]:"; \
		objdump -p ezthumb.exe | grep 'DLL Name:'; \
	fi
	@if [ -f ezthumb_win.exe ]; then \
		echo "[ezthumb_win.exe]:"; \
		objdump -p ezthumb_win.exe | grep 'DLL Name:'; \
	fi

