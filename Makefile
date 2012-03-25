CFLAGS := -Wall -Werror -g -ggdb -fvisibility=hidden -std=c99 -I. -ltinfo
OBJS := npraises.o gridfluid.o
PROGS := test

all: $(OBJS) $(PROGS)

npraises.o: npraises.c npraises.h Makefile
	gcc -c $(CFLAGS) -fPIC -o npraises.o npraises.c

gridfluid.o: gridfluid.c gridfluid.h Makefile
	gcc -c $(CFLAGS) -fPIC -o gridfluid.o gridfluid.c

test: test.c npraises.h npraises.o gridfluid.o gridfluid.h Makefile
	gcc $(CFLAGS) -o test test.c npraises.o gridfluid.o
	find . -name 'core*' -exec rm {} \;
