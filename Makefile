# Compiler and flags
CC = gcc
CFLAGS = -std=c89 -Wall -pedantic -Werror -O2 #-DDEBUG_MODE -DTRACE_MODE
LIBS = -lfilebuffer

# Project diretories
IDIR = ./include
ODIR = ./obj
SDIR = ./src
LDIR = ./libs

# Get files
EXEC = pattern
SRCS = $(wildcard $(SDIR)/*.c)
OBJS = $(SRCS:$(SDIR)/%.c=$(ODIR)/%.o)
DEPS = $(wildcard $(IDIR)/*.h)

all: $(EXEC) $(OBJS)

# Main target
$(EXEC): $(OBJS)
	$(CC) -L$(LDIR) -o $@ $^ $(LIBS)

# To obtain object files
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR)

# To remove generated files
clean:
	rm -f $(EXEC)
	rm -f $(OBJS)
