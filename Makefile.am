
# CFG_SNAPSHOT_DUMP is used to save each frames to JPEG pictures
# CFG_SNAPSHOT_RAW is used to save each frames to YUV files
# CFG_SNAPSHOT_RGB is used to save each frames to RGB files

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install
SUBDIRS = .


# _FILE_OFFSET_BITS=64 is used to support files over 2GB in 32-bit OS
DEFINES = -D_FILE_OFFSET_BITS=64

DEFS	+= $(SYSDEFS)
LIBS	+= $(POSTLIB)

if	XAM_MSWIN
if	XAM_GUI
bin_PROGRAMS		= ezthumb ezthumb_win
ezthumb_SOURCES		= ezthumb.c ezthumb.h id_lookup.c id_lookup.h ezutil.c
ezthumb_LDADD		= main_con.o ezthumb_icon.o
ezthumb_win_SOURCES	= ezthumb.c ezthumb.h ezgui.c ezgui.h ezicon.h \
			  ezutil.c id_lookup.c id_lookup.h
ezthumb_win_LDADD 	= main_win.o ezthumb_icon.o
else
bin_PROGRAMS		= ezthumb 
ezthumb_SOURCES		= ezthumb.c ezthumb.h id_lookup.c id_lookup.h ezutil.c
ezthumb_LDADD		= main_con.o ezthumb_icon.o
endif
else
bin_PROGRAMS	= ezthumb
if	XAM_GUI
ezthumb_SOURCES	= main.c ezthumb.c ezthumb.h ezgui.c ezgui.h ezicon.h \
		  ezutil.c id_lookup.c id_lookup.h
else
ezthumb_SOURCES	= main.c ezthumb.c ezthumb.h id_lookup.c id_lookup.h ezutil.c
endif
BUILT_SOURCES 	= $(PRELIBS)
endif

# This list of files for source release
EXTRA_DIST = aclocal.m4 ChangeLog config.h.in configure configure.ac COPYING \
	     depcomp external ezgui.c  ezgui.h  ezicon.h  ezqiz \
	     ezthumb.1  ezthumb.c  ezthumb.desktop ezthumb.h  ezthumb.ico  \
	     ezthumb_icon.rc ezthumb.lsm  ezthumb.nsi  ezthumb.pdf ezutil.c \
	     id_lookup.c id_lookup.h install-sh libmingw m4 main.c  \
	     Makefile.am Makefile.in missing Readme.txt  TODO

RELVERS	= `grep PACKAGE_VERSION config.h | cut -d\" -f 2`

var:
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo DEBUGS = $(DEBUGS)
	@echo IUPCFG = $(IUPCFG)
	@echo LIBS = $(LIBS)
	@echo SYSTOOL = $(SYSTOOL)
	@echo SYSGUI = $(SYSGUI)
	@echo COMPILE = $(COMPILE)
	@echo LINK = $(LINK)
	@echo TARGET = $(TARGET) : $(PROJECT) : $(POSTLIB)

main_con.o : main.c
	$(COMPILE) -DCFG_GUI_OFF -c -o $@ $<

main_win.o : main.c
	$(COMPILE) -DCFG_GUI_ON -c -o $@ $<

ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

libiup:
	$(IUPCFG) make -C ./external/iup do_all

libiup-clean:
	$(IUPCFG) make -C ./external/iup clean

libcsoup:
	make -C ./external/libcsoup all

libcsoup-clean:
	make -C ./external/libcsoup clean

myrelease:
	RELDIR=ezthumb-$(RELVERS)-win32-bin; \
	if test -d "$$RELDIR"; then \
	  rm -rf "$$RELDIR"; \
	fi; \
	mkdir "$$RELDIR"; \
	cp -af $(EXTRA_DIST) "$$RELDIR"; \
	7z a -tzip "$$RELDIR.zip" "$$RELDIR"


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
	@echo "OutFile \"ezthumb-$(RELVERS)-setup.exe\"" > nsis_version.txt
	makensis ezthumb.nsi
	@$(RM) nsis_version.txt 

extlib:
	$(IUPCFG) make -C external all

extclean:
	$(IUPCFG) make -C external clean

release: extclean dist-bzip2

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

