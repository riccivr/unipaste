# unipaste - suckless universal rich text converter and stream formatter
# See LICENSE file for copyright and license details.

include config.mk

# Plugin support (default: none, zero dependencies)
# Build with sanitizer: make SANITIZE=builtin
PLUGIN_SRC = plugin_none.c
ifeq ($(SANITIZE),builtin)
PLUGIN_SRC = plugin_builtin.c
CFLAGS += -DSANITIZE_BUILTIN
endif

SRC = unipaste.c parser.c table.c entity.c strbuf.c $(PLUGIN_SRC)
OBJ = $(SRC:.c=.o)

all: unipaste

.c.o:
	$(CC) -c $(CFLAGS) $<

unipaste: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f unipaste $(OBJ) plugin_none.o plugin_builtin.o unipaste-$(VERSION).tar.gz

dist: clean
	mkdir -p unipaste-$(VERSION)/tests unipaste-$(VERSION)/packaging
	cp -R LICENSE Makefile README.md config.mk unipaste.1 arg.h unipaste.h plugin.h $(SRC) plugin_none.c plugin_builtin.c tests fuzz packaging unipaste-$(VERSION)
	tar -cf unipaste-$(VERSION).tar unipaste-$(VERSION)
	gzip unipaste-$(VERSION).tar
	rm -rf unipaste-$(VERSION)

deb: all
	@T=$$(mktemp -d); \
	mkdir -p $$T/DEBIAN $$T/usr/bin $$T/usr/share/man/man1; \
	cp unipaste $$T/usr/bin/; \
	sed "s/VERSION/$(VERSION)/g" < unipaste.1 > $$T/usr/share/man/man1/unipaste.1; \
	chmod 755 $$T/usr/bin/unipaste; \
	chmod 644 $$T/usr/share/man/man1/unipaste.1; \
	printf "Package: unipaste\nVersion: $(VERSION)\nSection: utils\nPriority: optional\nArchitecture: amd64\nMaintainer: riccivr <riccivr@users.noreply.github.com>\nDescription: Suckless universal rich text and clipboard stream converter\n" > $$T/DEBIAN/control; \
	chmod 755 $$T/DEBIAN; \
	dpkg-deb --root-owner-group --build $$T unipaste_$(VERSION)_amd64.deb; \
	rm -rf $$T; \
	echo "Built unipaste_$(VERSION)_amd64.deb"

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
	sh tests/test_fixtures.sh

fixtures: unipaste
	sh tests/test_fixtures.sh

stress: unipaste
	sh tests/test_stress.sh

sanitize: clean
	$(CC) $(CFLAGS) -g -fsanitize=address,undefined $(SRC) -o unipaste $(LDFLAGS) -fsanitize=address,undefined
	sh tests/test_unipaste.sh
	sh tests/test_fixtures.sh
	sh tests/test_stress.sh

valgrind: unipaste
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./unipaste < tests/fixtures/slack_message.html > /dev/null
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./unipaste -m markdown < tests/fixtures/github_code.html > /dev/null
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./unipaste -u < tests/fixtures/teams_checklist.html > /dev/null
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./unipaste -l footnote < tests/fixtures/docs_nested_outline.html > /dev/null
	@echo "[PASS] Valgrind memory leak checks clean (0 leaks, 0 errors)"

fuzz: clean
	clang -std=c99 -pedantic -Wall -Wextra $(CPPFLAGS) -I. -fsanitize=fuzzer,address,undefined fuzz/fuzz_unipaste.c parser.c table.c entity.c strbuf.c $(PLUGIN_SRC) -o fuzz_unipaste
	./fuzz_unipaste tests/fixtures -max_total_time=5 -runs=20000
	rm -f fuzz_unipaste

.PHONY: all clean dist install uninstall test fixtures stress sanitize valgrind fuzz
