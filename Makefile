
include Make.conf


INCDIR	= $(EXINC) -I./libsmm
LIBDIR	= $(EXLIB) -L./libsmm

CFLAGS	+= -D_FILE_OFFSET_BITS=64 $(INCDIR)

OBJS	= main.o fixtoken.o ezthumb.o cliopt.o eznotify.o id_lookup.o 

LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd
#	 -lfreetype -lbz2 -lm


ifeq	($(SYSTOOL),unix)
	TARGET	= ezthumb
else
	TARGET	= ezthumb.exe
endif

all: version smm $(TARGET) install

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm

version: version.c
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $<

smm:
	make -C libsmm

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

install:
	$(CP) $(TARGET) ~/bin

clean:
	make -C libsmm clean
	$(RM) $(TARGET) version $(OBJS)

release:
ifeq	($(SYSTOOL),unix)
	./release.sh
else
	mkdir $(RELDIR)
	$(CP) $(TARGET) ezthumb.1 $(RELDIR)
	$(CP) $(EXDLL) $(RELDIR)
	./release.sh win
endif

