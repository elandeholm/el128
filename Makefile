CC=clang
RM=rm
CC-DEFINES=
CC-FLAGS=$(CC-DEFINES) -Wall -std=c99
CC-DFLAGS=$(CC-FLAGS) -g -DDEBUG
CC-OFLAGS=$(CC-FLAGS) -O2 -march=native
LD-FLAGS=
RM-FLAGS=-f
LD-LIBS=

OBJS=el128.o
EXES=el128

el128.o: el128.c 
	$(CC) $(CC-OFLAGS) -o el128.o -c el128.c
el128: el128.o el128.o
	$(CC)  -o el128 el128.o 
all:	$(EXES)
	
clean:
	$(RM) $(RM-FLAGS) $(OBJS) $(EXES)
	
