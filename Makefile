CC = gcc
CCXX = g++
YY_DEBUG = -DYY_DEBUG_MODE -DYY_TRACE_MODE
DEBUG = -DDEBUG_MODE #-DTRACE_MODE
CFLAGS = -std=gnu99 -Wall -pedantic -O3 -D_GNU_SOURCE
CCXXFLAGS = -std=c++11 -Wall -O3
LEX = flex
YACC = bison

PROJECT_DIR = $(shell pwd)

IDIR = $(PROJECT_DIR)/include
ODIR = $(PROJECT_DIR)/obj
SDIR = $(PROJECT_DIR)/src
LDIR = $(PROJECT_DIR)/libs
MDIR = $(PROJECT_DIR)/main
TDIR = $(PROJECT_DIR)/tests

EXEC = compiler.out
TEST = test.out
SRCS = $(wildcard $(SDIR)/*.c)
OBJS = $(SRCS:$(SDIR)/%.c=$(ODIR)/%.o)
DEPS = $(wildcard $(IDIR)/*.h)

LIBS = -lfl -lm -lgmp -lavl -ldarray -lfilebuffer -larraylist -lstack

MYLIBS_SDIR = $(PROJECT_DIR)/external/mylibs/src
MYLIBS_ODIR = $(PROJECT_DIR)/libs
MY_LIBS = $(MYLIBS_ODIR)/libavl.a $(MYLIBS_ODIR)/libdarray.a \
 		  $(MYLIBS_ODIR)/libfilebuffer.a $(MYLIBS_ODIR)/libarraylist.a \
		  $(MYLIBS_ODIR)/libstack.a

INTERPRETER_SDIR = $(PROJECT_DIR)/external/interpreter

DOBJS = $(SRCS:$(SDIR)/%.c=$(ODIR)/%_dbg.o)
DEXEC = compiler_dbg.out

all: libs compiler interpreter test
compiler: $(MY_LIBS) $(EXEC)
test: $(MY_LIBS) $(EXEC) interpreter $(TEST)
compiler_dbg: $(MY_LIBS) $(DEXEC)
libs: $(MY_LIBS)
interpreter: interpreter.out interpreter-cln.out

#### NORMAL COMPILER #####

# Create YACC files
$(ODIR)/parser.tab.c $(IDIR)/parser.tab.h: $(SDIR)/parser.y
	$(YACC) --defines=$(IDIR)/parser.tab.h $(SDIR)/parser.y -o $(ODIR)/parser.tab.c

# Create LEX files
$(ODIR)/parser_lex.yy.c: $(SDIR)/parser.l $(IDIR)/parser.tab.h
	$(LEX) -o $@ $(SDIR)/parser.l

# To obtain object files#
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

# Compile and link all together
$(EXEC): $(ODIR)/parser_lex.yy.c $(ODIR)/parser.tab.c $(IDIR)/parser.tab.h $(OBJS) $(MDIR)/main.c
	$(CC) $(CFLAGS) $(YY_DEBUG) -L$(LDIR) -I$(IDIR) $(MDIR)/main.c $(ODIR)/parser.tab.c $(ODIR)/parser_lex.yy.c $(OBJS) $(LIBS) -o $@


##### COMPILER DBG ######

# To obtain dbg object files#
$(ODIR)/%_dbg.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@ -I$(IDIR)

$(DEXEC): $(ODIR)/parser_lex.yy.c $(ODIR)/parser.tab.c $(IDIR)/parser.tab.h $(DOBJS) $(MDIR)/main.c
	$(CC) $(CFLAGS) $(DEBUG) -L$(LDIR) -I$(IDIR) $(MDIR)/main.c $(ODIR)/parser.tab.c $(ODIR)/parser_lex.yy.c $(DOBJS) $(LIBS) -o $@

##### TESTS #####

$(TEST): $(ODIR)/parser_lex.yy.c $(ODIR)/parser.tab.c $(IDIR)/parser.tab.h $(OBJS) $(TDIR)/test.c
	$(CC) $(CFLAGS) -L${LDIR} -I${IDIR} $(TDIR)/test.c $(ODIR)/parser.tab.c $(ODIR)/parser_lex.yy.c $(OBJS) $(LIBS) -o $@ && \
	echo "\033[1m\033[34mRUNNING TESTS\033[0m" && ./$(TEST)

##### MY LIBS #####

# avl
$(MYLIBS_ODIR)/libavl.a: $(MYLIBS_SDIR)/avl/*
	cd $(MYLIBS_SDIR)/avl && \
	$(CC) $(CFLAGS) *.c -c && ar rcs $@ *.o && \
	rm -f *.o && cp avl.h $(IDIR) && \
	cd $(PROJECT_DIR)

# darray
$(MYLIBS_ODIR)/libdarray.a: $(MYLIBS_SDIR)/darray/*
	cd $(MYLIBS_SDIR)/darray && \
	$(CC) $(CFLAGS) *.c -c && ar rcs $@ *.o && \
	rm -f *.o && cp darray.h $(IDIR) && \
	cd $(PROJECT_DIR)

# arraylist
$(MYLIBS_ODIR)/libarraylist.a: $(MYLIBS_SDIR)/arraylist/*
	cd $(MYLIBS_SDIR)/arraylist && \
	$(CC) $(CFLAGS) *.c -c && ar rcs $@ *.o && \
	rm -f *.o && cp arraylist.h $(IDIR) && \
	cd $(PROJECT_DIR)

# filebuffer
$(MYLIBS_ODIR)/libfilebuffer.a: $(MYLIBS_SDIR)/filebuffer/*
	cd $(MYLIBS_SDIR)/filebuffer && \
	$(CC) $(CFLAGS) *.c -c && ar rcs $@ *.o && \
	rm -f *.o && cp filebuffer.h $(IDIR) && \
	cd $(PROJECT_DIR)

# stack
$(MYLIBS_ODIR)/libstack.a: $(MYLIBS_SDIR)/stack/*
	cd $(MYLIBS_SDIR)/stack && \
	$(CC) $(CFLAGS) *.c -c && ar rcs $@ *.o && \
	rm -f *.o && cp stack.h $(IDIR) && \
	cd $(PROJECT_DIR)

##### INTERPRETER #####
interpreter.out: $(INTERPRETER_SDIR)/interpreter.cc
	$(CCXX) $(CCXXFLAGS) $< -o $@

interpreter-cln.out: $(INTERPRETER_SDIR)/interpreter-cln.cc
	$(CCXX) $(CCXXFLAGS) $< -lcln -o $@

clean:
	rm -rf $(ODIR)/*
	rm -rf $(IDIR)/parser.tab.h
	rm -f $(EXEC)
	rm -f $(TEST)
	rm -f $(DEXEC)
	rm -f interpreter.out
	rm -f interpreter-cln.out

clean_libs:
	rm -rf $(LDIR)/*
	rm -f $(IDIR)/avl.h
	rm -f $(IDIR)/darray.h
	rm -f $(IDIR)/arraylist.h
	rm -f $(IDIR)/filebuffer.h
	rm -f $(IDIR)/stack.h

help:
	@echo "TASKS"
	@echo "make all            -->     libs, compiler, interpreter, tests"
	@echo "make libs           -->     build my libs"
	@echo "make compiler       -->     build main compiler binary"
	@echo "make compiler_dbg   -->     build compiler debug version"
	@echo "make test           -->     build and run tests"
	@echo "make interpreter    -->     build both interpreters (Author: Maciej Gebala)"
	@echo "make clean          -->     delete files from tasks: compiler, compiler_dbg test and interpreter"
	@echo "make clean_libs     -->     delete libs files"
