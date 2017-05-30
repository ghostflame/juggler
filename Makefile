# default some environment variables
BINDIR ?= $(DESTDIR)/usr/bin
MANDIR ?= $(DESTDIR)/usr/share/man

TARGET  = all
VERS    = $(shell sed -rn 's/^Version:\t(.*)/\1/p' juggler.spec)

all:    code

debug:  TARGET = debug
debug:  code

MAN1DIR = $(MANDIR)/man1
MANPAGE = $(MAN1DIR)/juggler.1.gz

code:
	@cd src && $(MAKE) $(MFLAGS) $(TARGET)

install:
	@echo "Installing juggler."
	@mkdir -p $(BINDIR)
	@mkdir -p $(MAN1DIR)
	@gzip -c dist/juggler.1 > $(MANPAGE)
	@chmod 644 $(MANPAGE)
	@cd src && $(MAKE) $(MFLAGS) install

uninstall:
	@cd src && $(MAKE) $(MFLAGS) uninstall
	@rm -f $(MANPAGE)

clean:
	@cd src && $(MAKE) $(MFLAGS) clean


