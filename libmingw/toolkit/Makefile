

CFLAGS	= -Wall -g

TARGET	= hentai mkweb bin2c blackip

all: $(TARGET)

hentai: hentai.c
	gcc $(CFLAGS) -o $@ $^

mkweb: mkweb.c
	gcc $(CFLAGS) -o $@ $^

bin2c: bin2c.c
	gcc $(CFLAGS) -o $@ $^

blackip: blackip.c
	gcc $(CFLAGS) -DCFG_LIBCURL -o $@ $^ -lz -lcurl

install:
	cp $(TARGET) ~/bin

debug: $(TARGET)
	cp $(TARGET) ~/bin

clean:
	rm $(TARGET)
