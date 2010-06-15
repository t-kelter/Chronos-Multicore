SOURCES=main.c
EXECUTABLE=opt
DEBUG= #Set to -D_DEBUG for debugging mode
CCFLAGS=-g -D_PRINT -lm
SUBDIRS=wcrt

.PHONY: all $(SUBDIRS) clean

all: $(SUBDIRS)
	$(CC) $(CCFLAGS) $(DEBUG) -o $(EXECUTABLE) $(SOURCES);

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -rf $(EXECUTABLE) core;
