CC = gcc
CFLAGS = -std=c89 -pedantic -O2
LEX = flex

IDIR = ./include
ODIR = ./obj
SDIR = ./src
LDIR = ./libs

all: ex1 ex2 ex3 ex4

ex1: $(SDIR)/ex1.l
	$(LEX) -o $(ODIR)/$@_lex.yy.c $^
	$(CC) $(CFLAGS) $(ODIR)/$@_lex.yy.c -lfl -o $@

ex2: $(SDIR)/ex2.l
	$(LEX) -o $(ODIR)/$@_lex.yy.c $^
	$(CC) $(CFLAGS) $(ODIR)/$@_lex.yy.c -lfl -o $@

ex3: $(SDIR)/ex3.l
	$(LEX) -o $(ODIR)/$@_lex.yy.c $^
	$(CC) $(CFLAGS) $(ODIR)/$@_lex.yy.c -lfl -o $@

ex4: $(SDIR)/ex4.l
	$(LEX) -o $(ODIR)/$@_lex.yy.c $^
	$(CC) $(CFLAGS) $(ODIR)/$@_lex.yy.c -I$(IDIR) -L$(LDIR) -lfl -lm -lstack -o $@


clean:
	rm -rf $(ODIR)/*
	rm -f ex*
