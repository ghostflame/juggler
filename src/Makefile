CC      = /usr/bin/gcc -std=c11 $(WFLAGS)

HEADS   = juggler
FILES   = thread main

VERS    = $(shell sed -rn 's/Version:\t(.*)/\1/p' ../juggler.spec)

OBJS    = $(FILES:%=%.o)
HDRS    = $(HEADS:%=%.h)

WFLAGS  = -Wall -Wshadow -Wpedantic -Wextra -Wno-unused-parameter
IFLAGS  = -I.
TFLAGS  = -pthread
DFLAGS  = -g -pg -ggdb3 -DDEBUG
CFLAGS  = $(TFLAGS) $(IFLAGS) -DJUGGLER_VERSION='"$(VERS)"'
LFLAGS  = $(TFLAGS) $(IFLAGS)

BINDIR ?= $(DESTDIR)/usr/bin
BIN     = juggler

all:    CFLAGS += -O2
all:    $(BIN)

debug:  CFLAGS += $(DFLAGS)
debug:  $(BIN)

$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LFLAGS)
	@echo "Juggler sighted!"

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Juggler installed."

uninstall:
	@echo "Evicting the juggler."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@rm -f core* *.core $(OBJS) $(BIN)

