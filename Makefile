
include Make.conf


INCDIR	= $(EXINC) -I./libsmm
LIBDIR	= $(EXLIB) -L./libsmm

CFLAGS	+= -D_FILE_OFFSET_BITS=64 $(INCDIR)


ifeq	($(SYSGUI),CFG_GUI_ON)
CFLAGS	+= $(GTKINC)
endif


OBJS	= $(OBJDIR)/main.o	\
	  $(OBJDIR)/fixtoken.o	\
	  $(OBJDIR)/ezthumb.o	\
	  $(OBJDIR)/cliopt.o	\
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



.PHONY: ezthumb

all: ezthumb


ifeq	($(SYSTOOL),unix)
ezthumb: smm $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm
else
ezthumb: smm
	SYSGUI=CFG_GUI_OFF make ezthumb.exe
	SYSGUI=CFG_GUI_ON  make ezthumb_win.exe
endif

# internal rules, do not use it
ezthumb.exe: $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) -lsmm

# internal rules, do not use it
ezthumb_win.exe: $(OBJDIR)/ezthumb_icon.o $(OBJS)
	$(CC) $(CFLAGS) -mwindows $(LIBDIR) -o $@ $^ $(LIBS) -lsmm

ezicon.h: SMirC-thumbsup.svg
	gdk-pixbuf-csource --name=ezicon_pixbuf --raw $< > $@

$(OBJDIR)/ezthumb_icon.o: ezthumb_icon.rc
	windres $< -o $@

version: version.c
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $< -lm

smm:
	make -C libsmm

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

ifeq	($(SYSTOOL),unix)
install: ezthumb
	install -s ezthumb $(BINDIR)
	install ezthumb.1 $(MANDIR)
else
install: ezthumb version
	-mkdir $(RELDIR)-win-bin
	-$(CP) ezthumb*.exe ezthumb.1 ezthumb.ico $(RELDIR)-win-bin
	-$(CP) $(EXDLL) $(RELDIR)-win-bin
endif

debug: ezthumb
	cp ezthumb ~/bin

cleanobj:
	$(RM) $(OBJS)
ifeq	($(SYSGUI),CFG_GUI_ON)
	$(RM) $(OBJDIR)/ezthumb_icon.o
endif

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
	

