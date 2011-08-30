# older ffmpeg doesn't have swscale. you might need to build a newer 
# ffmpeg from source
PRJDIR	= .

FFMPEG  = $(PRJDIR)/ffmpeg-0.6.1
LIBGD   = $(PRJDIR)/gd-2.0.35
FREETYPE= $(PRJDIR)/freetype-2.4.3/objs/.libs
LIBJPEG = $(PRJDIR)/jpeg-6b
LIBBZ2  = $(PRJDIR)/bzip2-1.0.6

INCDIR	= -I.
LIBDIR	= -L.

CFLAGS = -Wall -O2 -s -g -D_FILE_OFFSET_BITS=64 $(INCDIR) $(LIBDIR)
LDFLAGS= -lavcodec -lavformat -lavcodec -lswscale -lavutil -lgd \
	 -lfreetype -lbz2 -lm

OBJS	= main.o  fixtoken.o
LIBOBJ	= ezthumb.o cliopt.o eznotify.o \
	  id_lookup.o id_codec.o id_codec_flag.o \
	  id_codec_type.o id_pix_fmt.o id_pict_type.o id_sam_format.o

all: ezthumb install

libezthumb.a : $(LIBOBJ)
	ar crus $@ $^

ezthumb : $(OBJS) libezthumb.a
	$(CC) $(CFLAGS) -o $@ $(OBJS)  $(LDFLAGS) -lezthumb

vidlen : vidlen.c
	$(CC) -o $@ $^ $(CFLAGS) -lavformat

%.o : %.c
	$(CC) $(CFLAGS) -c $<

install:
	cp -f ezthumb ~/bin

clean:
	rm -f ezthumb libezthumb.a $(OBJS) $(LIBOBJ)


