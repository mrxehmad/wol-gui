# wol-gui - simple Wake-on-LAN GUI (GTK3)
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

BIN      = wol-gui
VERSION  = 0.1.0
SRCS     = src/main.c src/ui.c src/hosts.c src/wol.c
OBJS     = $(SRCS:.c=.o)
DEPS     = $(OBJS:.o=.d)

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

src/version.h: src/version.h.in
	sed 's/@VERSION@/$(VERSION)/' $< > $@.tmp && mv -f $@.tmp $@

src/%.o: src/%.c src/version.h
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -f $(BIN) $(OBJS) $(DEPS) src/version.h

install: $(BIN)
	install -Dm755 $(BIN) $(BINDIR)/$(BIN)
	install -Dm644 data/wol-gui.desktop $(APPSDIR)/wol-gui.desktop
	install -Dm644 data/wol-gui.svg $(ICONSDIR)/wol-gui.svg

uninstall:
	rm -f $(BINDIR)/$(BIN)
	rm -f $(APPSDIR)/wol-gui.desktop
	rm -f $(ICONSDIR)/wol-gui.svg
