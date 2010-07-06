noinst_LIBRARIES=libwcrt.a

# Only program to build here
libwcrt_a_SOURCES=alloc.c alloc.h \
                  cluster.c cluster.h \
                  cycle_time.c cycle_time.h \
                  dump.c dump.h \
                  graphcoloring.c graphcoloring.h \
                  handler.c handler.h \
                  header.h \
                  knapsack.c knapsack.h \
                  parse.c parse.h \
                  selectcontent.c selectcontent.h \
                  slacks.c slacks.h \
                  timing.c timing.h \
                  timingMSG.c timingMSG.h \
                  topo.c topo.h \
                  util.c util.h \
                  wcrt.c wcrt.h

libwcrt_a_CPPFLAGS=-D_DEBUG

# Additionally remove the core dumps on 'make clean' if there are any
CLEANFILES=core

# Due to the special include system, we must list all indirect source 
# files as extra dist files to make them appear in the tar made by
# 'gmake dist'
