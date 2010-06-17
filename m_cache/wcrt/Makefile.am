noinst_PROGRAMS=timing

# Only program to build here
timing_SOURCES=main.c

# Additionally remove the core dumps on 'make clean' if there are any
CLEANFILES=core

# Due to the special include system, we must list all indirect source 
# files as extra dist files to make them appear in the tar made by
# 'gmake dist'
EXTRA_DIST=alloc.c \
           analysis.c \
           cluster.c \
           cycle_time.c \
           cycle_time.h \
           dp.c \
           dump.c \
           graphcoloring.c \
           handler.c \
           header.h \
           knapsack.c \
           main.c \
           parse.c \
           selectcontent.c \
           slacks.c \
           test.c \
           timing.c \
           timingMSG.c \
           topo.c \
           util.c
