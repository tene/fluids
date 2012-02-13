CFLAGS := -Wall -Werror -g -ggdb -std=c99 -I. -ltinfo
OBJS := npraises.o
PROGS := test

all: $(OBJS) $(PROGS)

npraises.o: npraises.c npraises.h Makefile
	gcc -c $(CFLAGS) -fPIC -o npraises.o npraises.c

test: test.c npraises.h npraises.o Makefile
	gcc $(CFLAGS) -o test test.c npraises.o
