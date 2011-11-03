
include Make.conf

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
else
EXDIR	= ..
FFMPEG  = $(EXDIR)/ffmpeg-0.6.1
EXINC	= -I$(FFMPEG)
endif

INCDIR	= $(EXINC) -I./libsmm
LIBDIR	= $(EXLIB) -L./libsmm

CFLAGS	+= -D_FILE_OFFSET_BITS=64 $(INCDIR)

OBJS	= main.o fixtoken.o ezthumb.o cliopt.o eznotify.o id_lookup.o 

LIBS	= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	 -lfreetype -lbz2 -lm


ifeq	($(SYSTOOL),unix)
	TARGET	= ezthumb
else
	TARGET	= ezthumb.exe
endif

all: smm $(TARGET) install

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $(OBJS) $(LIBS) -lsmm

smm:
	make -C libsmm

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

install:
	cp -f $(TARGET) ~/bin

clean:
	make -C libsmm clean
	rm -f $(TARGET) $(OBJS)

release:
	mkdir $(RELDIR)
	cp $(TARGET) ezthumb.1 $(RELDIR)
ifeq	($(SYSTOOL),cygwin)
	cp /bin/cyggd-2.dll $(RELDIR)
	cp /bin/cyggcc_s-1.dll $(RELDIR)
	cp /bin/cygwin1.dll $(RELDIR)
	cp /bin/cygXpm-4.dll $(RELDIR)
	cp /bin/cygX11-6.dll $(RELDIR)
	cp /bin/cygxcb-1.dll $(RELDIR)
	cp /bin/cygXau-6.dll $(RELDIR)
	cp /bin/cygXdmcp-6.dll $(RELDIR)
	cp /bin/cygfontconfig-1.dll $(RELDIR)
	cp /bin/cygexpat-1.dll $(RELDIR)
	cp /bin/cygfreetype-6.dll $(RELDIR)
	cp /bin/cygz.dll $(RELDIR)
	cp /bin/cygiconv-2.dll $(RELDIR)
	cp /bin/cygjpeg-7.dll $(RELDIR)
	cp /bin/cygpng12.dll $(RELDIR)
endif

