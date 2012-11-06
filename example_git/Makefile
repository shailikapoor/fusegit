.PHONY: all

CC = gcc
CFLAGS = #-g -I../include -I../src
#LFLAGS = -L../build -lgit2 -lz
LFLAGS = -lgit2
APPS = example

all: clean $(APPS)

% : %.c
	$(CC) -o $@ $(CFLAGS) $< $(LFLAGS)

clean:
	$(RM) $(APPS)
	$(RM) -r *.dSYM
	$(RM) -r repo*
