CC = gcc
CFLAGS = -std=c89 -Wall -pedantic -O2 -D_GNU_SOURCE
LEX = flex
YACC = bison

IDIR = ./include
ODIR = ./obj
SDIR = ./src
LDIR = ./libs

EXEC = calc

LIBS = -lfl -lm -ldarray

all: $(EXEC)

# Create YACC files
$(ODIR)/$(EXEC).tab.c $(IDIR)/$(EXEC).tab.h: $(SDIR)/$(EXEC).y
	$(YACC) --defines=$(IDIR)/$(EXEC).tab.h $^ -o $(ODIR)/$(EXEC).tab.c

# Create LEX files
$(ODIR)/$(EXEC)_lex.yy.c: $(SDIR)/$(EXEC).l $(IDIR)/$(EXEC).tab.h
	$(LEX) -o $@ $(SDIR)/$(EXEC).l

# Compile and link all together
$(EXEC): $(ODIR)/$(EXEC)_lex.yy.c $(ODIR)/$(EXEC).tab.c $(IDIR)/$(EXEC).tab.h
	$(CC) $(CFLAGS) -L${LDIR} -I${IDIR} $(ODIR)/$@.tab.c $(ODIR)/$@_lex.yy.c $(LIBS) -o $@

clean:
	rm -rf $(ODIR)/*
	rm -rf $(IDIR)/$(EXEC).tab.h
	rm -f $(EXEC)
