
include Make.conf


INCDIR	= $(EXINC) -I./libsmm
LIBDIR	= $(EXLIB) -L./libsmm

CFLAGS	+= -D_FILE_OFFSET_BITS=64 $(INCDIR)


ifeq	($(SYSGUI),CFG_GUI_ON)
CFLAGS	+= $(GTKINC)
endif

OBJS	= main.o fixtoken.o ezthumb.o cliopt.o eznotify.o id_lookup.o
ifeq	($(SYSGUI),CFG_GUI_ON)
OBJS	+= ezgui.o
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



.PHONY: ezthumb

all: ezthumb


ifeq	($(SYSTOOL),unix)
ezthumb: smm $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm
else
ezthumb:
	make cleanobj smm
	SYSGUI=CFG_GUI_OFF make ezthumb.exe
	make cleanobj smm
	SYSGUI=CFG_GUI_ON make ezthumb_win.exe
endif

ezthumb.exe: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) -lsmm

ezthumb_win.exe: ezthumb_icon.o $(OBJS)
	$(CC) $(CFLAGS) -mwindows $(LIBDIR) -o $@ $^ $(LIBS) -lsmm

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

version: version.c
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $<

smm:
	make -C libsmm

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

install: ezthumb
ifeq	($(SYSTOOL),unix)
	install -s ezthumb $(BINDIR)
	install ezthumb.1 $(MANDIR)
else
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXDLL) $(RELDIR)-win-bin
endif

debug: ezthumb
	cp ezthumb ~/bin

cleanobj:
	$(RM) $(OBJS)

clean: cleanobj
ifeq	($(SYSTOOL),unix)
	$(RM) ezthumb version
else
	$(RM) ezthumb.exe ezthumb_win.exe version.exe
endif

cleanall: clean
	make -C libsmm clean

rel_source:
	-mkdir $(RELDIR)
	-cp *.c *.h *.1 *.txt *.ico Make* COPYING ChangeLog $(RELDIR)
	-cp SMirC-thumbsup.svg $(RELDIR)
	-mkdir $(RELDIR)/libsmm
	-cp libsmm/*.c libsmm/*.h libsmm/Makefile $(RELDIR)/libsmm
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-rm -rf $(RELDIR)

rel_win_dev:
	-tar czf ezthumb-libmingw-$(RELDATE).tar.gz libmingw

rel_win_bin: install
	-tar czf $(RELDIR)-win-bin.tar.gz $(RELDIR)-win-bin
	-rm -rf $(RELDIR)-win-bin

ifeq	($(SYSTOOL),unix)
release: version rel_source
else
release: version rel_source rel_win_dev rel_win_bin
endif
	

