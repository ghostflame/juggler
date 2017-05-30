# default some environment variables
BINDIR ?= $(DESTDIR)/usr/bin
MANDIR ?= $(DESTDIR)/usr/share/man

TARGET  = all
VERS    = $(shell sed -rn 's/^Version:\t(.*)/\1/p' juggler.spec)

all:    code

debug:  TARGET = debug
debug:  code


code:
	@cd src && $(MAKE) $(MFLAGS) $(TARGET)

install:
	@echo "Installing juggler."
	@mkdir -p $(BINDIR)
	@cd src && $(MAKE) $(MFLAGS) install

uninstall:
	@cd src && $(MAKE) $(MFLAGS) uninstall

clean:
	@cd src && $(MAKE) $(MFLAGS) clean


