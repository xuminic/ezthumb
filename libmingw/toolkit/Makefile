

CFLAGS	= -Wall -O3

TARGET	= hentai mkweb

all: $(TARGET)

hentai: hentai.c
	gcc $(CFLAGS) -o $@ $^

mkweb: mkweb.c
	gcc $(CFLAGS) -o $@ $^

install:
	cp $(TARGET) ~/bin

debug: $(TARGET)
	cp $(TARGET) ~/bin

clean:
	rm $(TARGET)
