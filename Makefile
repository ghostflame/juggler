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
	@gzip -c dist/juggler.1 > $(MANDIR)/man1/juggler.1.gz
	@chmod 644 $(MANDIR)/man1/juggler.1.gz

uninstall:
	@cd src && $(MAKE) $(MFLAGS) uninstall
	@rm -f $(MANDIR)/man1/juggler.1.gz

clean:
	@cd src && $(MAKE) $(MFLAGS) clean


