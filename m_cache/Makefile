SOURCES=main.c
EXECUTABLE=opt
DEBUG= #Set to -D_DEBUG for debugging mode
CCFLAGS=-g -D_PRINT -lm

.PHONY: all clean

all:
	$(CC) $(CCFLAGS) $(DEBUG) -o $(EXECUTABLE) $(SOURCES);

clean:
	rm -rf $(EXECUTABLE) core;
