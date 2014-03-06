PREFIX ?= /usr

CC = clang
CFLAGS = -Wall -Werror -O2

OBJS := \
	xcode-select.o

PROG := xcode-select

%c.o:
	$(CC) $(CFLAGS) -c $< -o @@

all: $(OBJS)
	$(CC) $(OBJS) -o $(PROG)

install: $(PROG)
	install -d $(DSTROOT)/$(PREFIX)/bin
	install -s -m 755 $(PROG) $(DSTROOT)/$(PREFIX)/bin/$(PROG)

.PHONY: clean
clean:
	rm -rf $(OBJS) $(PROG)
