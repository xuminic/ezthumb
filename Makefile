
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


RELDIR	= ezthumb-`./ezthumb --vernum`
RELDATE	= `date  +%Y%m%d`


all: smm ezthumb


ifeq	($(SYSTOOL),unix)
ezthumb: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm
else
ezthumb:
	make clean
	SYSGUI=CFG_GUI_OFF make ezthumb.exe
	make clean
	SYSGUI=CFG_GUI_ON make ezthumb_win.exe
endif

ezthumb.exe: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm

ezthumb_win.exe: $(OBJS)
	$(CC) $(CFLAGS) -mwindows $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm

ezicon.h : SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

version: version.c
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $<

smm:
	make -C libsmm

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

install:
ifeq	($(SYSTOOL),unix)
	install -s ezthumb $(BINDIR)
	install ezthumb.1 $(MANDIR)
else
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXDLL) $(RELDIR)-win-bin
endif


clean:
ifeq	($(SYSTOOL),unix)
	$(RM) ezthumb version $(OBJS)
else
	$(RM) ezthumb.exe ezthumb_win.exe version $(OBJS)
endif

cleanall: clean
	make -C libsmm clean

rel_source:
	-mkdir $(RELDIR)
	-cp *.c *.h *.1 *.txt *.ico Make* COPYING ChangeLog $(RELDIR)
	-cp SMirC-thumbsup.svg $(RELDIR)
	-cp -a libsmm $(RELDIR)
	-tar czf $(RELDIR).tar.gz $(RELDIR)
	-rm -rf $(RELDIR)

rel_win_dev:
	-tar czf ezthumb-libmingw-$(RELDATE).tar.gz libmingw

rel_win_bin: install
	-tar czf $(RELDIR)-win-bin.tar.gz $(RELDIR)-win-bin
	-rm -rf $(RELDIR)-win-bin

ifeq	($(SYSTOOL),unix)
release: rel_source
else
release: rel_source rel_win_dev rel_win_bin
endif
	

