SOURCES=main.c
EXECUTABLE=timing
DEBUG= #Set to -D_DEBUG for debugging mode
CCFLAGS=-g -lm

.PHONY: all clean

all:
	$(CC) $(CCFLAGS) $(DEBUG) -o $(EXECUTABLE) $(SOURCES);

clean:
	rm -rf $(EXECUTABLE) core;
