
IUPDIR	= ..
INCDIR	= -I$(IUPDIR)/include `pkg-config gtk+-2.0 --cflags`
LIBDIR	= -L$(IUPDIR)/lib/Linux26g4_64 
LIBS	= -liup `pkg-config gtk+-2.0 --libs` -lX11
CFLAGS	= -Wall -O3 -g $(INCDIR) $(LIBDIR)

TARGET	= list button toggle sample dialog

%: %.c
	gcc $(CFLAGS) -o $@ $< $(LIBS)

all: $(TARGET)

clean:
	rm $(TARGET)
