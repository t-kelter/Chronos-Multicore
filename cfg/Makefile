CC=gcc
CFLAGS=-g
#CFLAGS=-pg

SRC=main.c common.c misc.c ss.c readfile.c prog.c cfg.c
OBJ=$(SRC:.c=.o)
TARGET=main

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

depend:
	makedepend -Y -- $(SRC)

# DO NOT DELETE

main.o: readfile.h ss.h ss.def prog.h cfg.h globals.h
misc.o: misc.h
ss.o: misc.h ss.h ss.def
readfile.o: ecoff.h readfile.h ss.h ss.def
prog.o: prog.h ss.h ss.def cfg.h globals.h
cfg.o: cfg.h ss.h ss.def prog.h common.h
