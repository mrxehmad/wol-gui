# wolbox - simple Wake-on-LAN GUI (GTK3)
# SPDX-License-Identifier: MIT

PREFIX ?= /usr
DESTDIR ?=

BINDIR     = $(DESTDIR)$(PREFIX)/bin
DATADIR    = $(DESTDIR)$(PREFIX)/share
APPSDIR    = $(DATADIR)/applications
ICONSDIR   = $(DATADIR)/icons/hicolor/scalable/apps

PKG_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
PKG_LIBS   := $(shell pkg-config --libs gtk+-3.0)

CC       ?= cc
CFLAGS   += -std=c11 -O2 -Wall -Wextra $(PKG_CFLAGS)
LDFLAGS  +=
LIBS     += $(PKG_LIBS)

BIN      = wolbox
SRCS     = src/main.c src/ui.c src/hosts.c src/wol.c
OBJS     = $(SRCS:.c=.o)
DEPS     = $(OBJS:.o=.d)

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -f $(BIN) $(OBJS) $(DEPS)

install: $(BIN)
	install -Dm755 $(BIN) $(BINDIR)/$(BIN)
	install -Dm644 data/wolbox.desktop $(APPSDIR)/wolbox.desktop
	install -Dm644 data/wolbox.svg $(ICONSDIR)/wolbox.svg

uninstall:
	rm -f $(BINDIR)/$(BIN)
	rm -f $(APPSDIR)/wolbox.desktop
	rm -f $(ICONSDIR)/wolbox.svg
