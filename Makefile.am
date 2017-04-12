
# The line from 387, "am--refresh: Makefile" to 429, 
# "$(srcdir)/config.h.in:  $(am__configure_deps)" need to be manually
# removed after every 'automake' to prevent the automatic checking the 
# irrelevant files# like m4, configure.ac, etc while type-in make commands.
#
# CFG_SNAPSHOT_DUMP is used to save each frames to JPEG pictures
# CFG_SNAPSHOT_RAW is used to save each frames to YUV files
# CFG_SNAPSHOT_RGB is used to save each frames to RGB files

AUTOMAKE_OPTIONS = no-dependencies foreign
ACLOCAL_AMFLAGS = -I m4 --install
SUBDIRS = .


DEFS	+= $(SYSDEFS)
LIBS	+= $(POSTLIB)

if	XAM_MSWIN
bin_PROGRAMS	= ezthumb 
ezthumb_SOURCES	= ezthumb.c ezthumb.h id_lookup.c id_lookup.h ezutil.c
ezthumb_LDADD	= main_con.o ezthumb_icon.o
if	XAM_GUI
BUILT_SOURCES 	= $(PREFLIB)
WIN_OBJS	= main_win.o ezthumb.o id_lookup.o ezutil.o \
		  ezgui.o ezthumb_icon.o
endif
else
bin_PROGRAMS	= ezthumb
if	XAM_GUI
ezthumb_SOURCES	= main.c ezthumb.c ezthumb.h ezgui.c ezgui.h ezicon.h \
		  ezutil.c id_lookup.c id_lookup.h
else
ezthumb_SOURCES	= main.c ezthumb.c ezthumb.h id_lookup.c id_lookup.h ezutil.c
endif
BUILT_SOURCES 	= $(PREFLIB)
endif

# This list of files for source release
EXTRA_DIST = ChangeLog config.h.in configure configure.ac config.guess \
	     config.sub COPYING external ezgui.c  ezgui.h  ezicon.h  ezqiz \
	     ezthumb.1  ezthumb.c  ezthumb.desktop ezthumb.h  ezthumb.ico  \
	     ezthumb_icon.rc ezthumb.lsm  ezthumb.nsi  ezthumb.pdf ezutil.c \
	     id_lookup.c id_lookup.h install-sh libmingw main.c  \
	     Makefile.am Makefile.in missing Readme.txt  TODO

RELVERS	= `grep PACKAGE_VERSION config.h | cut -d\" -f 2`

var:
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo LIBS = $(LIBS)
	@echo IUPCFG = $(IUPCFG)
	@echo COMPILE = $(COMPILE)
	@echo LINK = $(LINK)
	@echo RELEASE = $(RELEASE) : $(PREFLIB) : $(POSTLIB)

help:
	@echo "Useful targets other than 'all'"
	@echo " clean             clean the object files of ezthumb only"
	@echo " distclean         clean the object files and configure files of ezthumb"
	@echo " extclean          clean the external libraries, i.e IUP and libcsoup"
	@echo " install           install the ezthumb to PREFIX/bin"
	@echo " gnome-install     also install the support files to GNOME desktop"
	@echo " gnome-uninstall   remove the support files of GNOME desktop"
	@echo " dist-gzip         create the source tar ball with .tar.gz"
	@echo " release           create the source tar ball compressed by bzip2"
	@echo "                   as well as an installer in Windows (nsis required)"
	@echo " installer-win     create an installer in Windows (nsis required)"
	@echo " showdll           display the linked DLLs in Windows"

main_con.o : main.c
	$(COMPILE) -DCFG_GUI_OFF -c -o $@ $<

main_win.o : main.c
	$(COMPILE) -DCFG_GUI_ON -c -o $@ $<

ezthumb_icon.o: ezthumb_icon.rc
	$(WINDRES) $< -o $@

ezthumb_win: $(WIN_OBJS)
	$(LINK) $^ $(WIN32GUI)

clean-local:
	-test -e "ezthumb_win$(EXEEXT)" && rm -f ezthumb_win$(EXEEXT)
	-test -e "$(distdir)-setup.exe" && rm -f $(distdir)-setup.exe
	-test -d "$(distdir)-win32-bin" && rm -rf "$(distdir)-win32-bin"

libiup:
	$(IUPCFG) make -C ./external/iup do_all

libiup-clean:
	$(IUPCFG) make -C ./external/iup clean

libcsoup:
	make -C ./external/libcsoup all

libcsoup-clean:
	make -C ./external/libcsoup clean

release: $(RELEASE)

if	XAM_MSWIN
all-local:
	if test -d "$(distdir)-win32-bin"; then \
	  rm -rf "$(distdir)-win32-bin"; \
	fi; \
	mkdir "$(distdir)-win32-bin"; \
	cp -a ezthumb.1 ezthumb.pdf ezthumb.ico "$(distdir)-win32-bin"; \
	cp -a ./libmingw/ffmpeg/bin/*.dll "$(distdir)-win32-bin"; \
	cp -a ./libmingw/lib/*.dll "$(distdir)-win32-bin"; \
	cp -a ezthumb*.exe ezqiz "$(distdir)-win32-bin"; \
	cp -a COPYING ChangeLog Readme.txt TODO "$(distdir)-win32-bin"
endif

gnome-install: install
	$(INSTALL_DATA) ezthumb.desktop /usr/share/applications
	@for res in 16 22 24 32 48 256; do \
	 $(INSTALL_DATA) -T ./external/icons/ezthumb"$$res".png \
	    /usr/share/icons/hicolor/"$$res"x"$$res"/apps/ezthumb.png; \
	done
	gtk-update-icon-cache -f /usr/share/icons/hicolor

gnome-uninstall: uninstall
	rm -f /usr/share/applications/ezthumb.desktop
	@for res in 16 22 24 32 48 256; do \
	  rm -f /usr/share/icons/hicolor/"$$res"x"$$res"/apps/ezthumb.png; \
	done
	gtk-update-icon-cache -f /usr/share/icons/hicolor

installer-win: all
	@echo "OutFile \"$(distdir)-setup.exe\"" > nsis_version.txt
	$(NSIS) ezthumb.nsi
	@$(RM) nsis_version.txt 

# supportive rule for generating files 
# since special tools are involved so they can be only internally used.
ezthumb-pdf: ezthumb.1
	man -l -Tps $< | ps2pdf - ezthumb.pdf

gtkicons:
	@for res in 16 22 24 32 48 128 256; do \
	  convert -background transparent ./external/icons/SMirC-thumbsup.svg \
	    -resize "$$res"x"$$res" ./external/icons/ezthumb"$$res".png; \
	done

extlib:
	$(IUPCFG) make -C external all

extclean:
	$(IUPCFG) make -C external clean

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

