PREFIX = /usr

CC = $(shell xcrun -find clang)
CFLAGS = -Wall -Werror -O2

OBJS := \
	ini.o \
	xcrun.o

PROG := xcrun

%c.o:
	$(CC) $(CFLAGS) -c $< -o @@

all: $(OBJS)
	$(CC) $(OBJS) -o $(PROG)

install: $(PROG)
	install -d $(PREFIX)/bin
	install -s -m 755 $(PROG) $(PREFIX)/bin/$(PROG)

.PHONY: clean
clean:
	rm -rf $(OBJS) $(PROG)
