
IUPLIBP	= $(shell echo ./iup/lib/*)
INSTALL	= ../libmingw

ifdef	USE_GTK2
IUPCFG	= USE_GTK2=$(USE_GTK2)
else
IUPCFG	=
endif

.PHONY: all libcsoup libiup libgd libgd_clean clean \
	mingw_all mingw_clean mingw_install

ifeq	($(MSYSTEM),MINGW32)
all:
else
all: libcsoup libiup
endif

libcsoup:
	if [ ! -f ./libcsoup/libcsoup.a ]; then \
		make -C ./libcsoup all; \
	fi

libiup:
	if [ ! -f $(IUPLIBP)/libiup.a ]; then \
		$(IUPCFG) make -C ./iup do_all; \
	fi

libgd:
	if [ ! -f ./libgd/Makefile ]; then \
		(cd ./libgd && ./configure --disable-shared && cd ..); \
	fi
	if [ ! -f ./libgd/.libs/libgd.a ]; then \
		make -C ./libgd all; \
	fi

libgd_clean:
	if [ -f ./libgd/Makefile ]; then \
		make -C ./libgd distclean; \
	fi

clean:
	$(IUPCFG) make -C ./iup clean
	make -C ./libcsoup clean

mingw_all: libcsoup libiup libgd

mingw_clean: clean libgd_clean

mingw_install:
	cp -f  ./libcsoup/libcsoup.a $(INSTALL)/lib
	cp -f  ./libcsoup/*.h $(INSTALL)/include
	cp -af ./iup/include/* $(INSTALL)/include/iup
	cp -f  ./iup/lib/mingw4/*.a $(INSTALL)/lib
	cp -f  ./libgd/gd.h $(INSTALL)/include
	cp -f  ./libgd/.libs/libgd.a $(INSTALL)/lib
							        


