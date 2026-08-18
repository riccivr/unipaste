# unipaste - suckless universal rich text converter and stream formatter
# See LICENSE file for copyright and license details.

include config.mk

SRC = unipaste.c parser.c table.c entity.c strbuf.c
OBJ = $(SRC:.c=.o)

all: unipaste

.c.o:
	$(CC) -c $(CFLAGS) $<

unipaste: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f unipaste $(OBJ) unipaste-$(VERSION).tar.gz

dist: clean
	mkdir -p unipaste-$(VERSION)/tests
	cp -R LICENSE Makefile README.md config.mk unipaste.1 arg.h unipaste.h $(SRC) tests unipaste-$(VERSION)
	tar -cf unipaste-$(VERSION).tar unipaste-$(VERSION)
	gzip unipaste-$(VERSION).tar
	rm -rf unipaste-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f unipaste $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/unipaste
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < unipaste.1 > $(DESTDIR)$(MANPREFIX)/man1/unipaste.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/unipaste.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/unipaste
	rm -f $(DESTDIR)$(MANPREFIX)/man1/unipaste.1

test: unipaste
	sh tests/test_unipaste.sh

sanitize: clean
	$(CC) $(CFLAGS) -g -fsanitize=address,undefined $(SRC) -o unipaste $(LDFLAGS) -fsanitize=address,undefined
	sh tests/test_unipaste.sh

.PHONY: all clean dist install uninstall test sanitize
