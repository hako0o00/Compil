CC = gcc
CFLAGS = -Wall -Wno-unused-function -Isrc
LDFLAGS = -mwindows -lgdi32

FLEX = flex
BISON = bison

SRCDIR = src

all: cpix

$(SRCDIR)/lexer.c: $(SRCDIR)/lexer.l $(SRCDIR)/parser.tab.h
	$(FLEX) -o $(SRCDIR)/lexer.c $(SRCDIR)/lexer.l

$(SRCDIR)/parser.c $(SRCDIR)/parser.tab.h: $(SRCDIR)/parser.y
	$(BISON) -d --defines=$(SRCDIR)/parser.tab.h -o $(SRCDIR)/parser.c $(SRCDIR)/parser.y

cpix: $(SRCDIR)/parser.c $(SRCDIR)/lexer.c
	$(CC) $(CFLAGS) -o cpix \
		$(SRCDIR)/main.c $(SRCDIR)/quad.c $(SRCDIR)/vm.c $(SRCDIR)/lexer.c $(SRCDIR)/parser.c \
		$(LDFLAGS)

run: cpix
	./cpix tests/hello.Cpix

clean:
	rm -f cpix $(SRCDIR)/lexer.c $(SRCDIR)/parser.c $(SRCDIR)/parser.tab.h tests/*.quads.txt

.PHONY: all run clean
