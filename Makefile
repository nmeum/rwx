X11INC ?= /usr/include/X11
X11LIB ?= /usr/lib/X11

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS ?= -O0 -ggdb -pedantic -Wall -Wextra
CFLAGS += -D_BSD_SOURCE -std=c99 -I$(X11INC)

CC      ?= cc
LDFLAGS += -L$(X11LIB) -lX11

.c.o:
	$(CC) $(CFLAGS) -c $<

rwx: rwx.o
	$(CC) -o rwx $< $(LDFLAGS)

install: rwx rwx.1
	install -Dm755 rwx "$(DESTDIR)$(BINDIR)/rwx"
	install -Dm644 rwx.1 "$(DESTDIR)$(MANDIR)/man1/rwx.1"
