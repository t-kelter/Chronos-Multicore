noinst_LIBRARIES=libwcrt.a

# Only program to build here
libwcrt_a_SOURCES=wcrt.c \
                  cycle_time.c cycle_time.h

libwcrt_a_CPPFLAGS=-D_DEBUG

# Additionally remove the core dumps on 'make clean' if there are any
CLEANFILES=core

# Due to the special include system, we must list all indirect source 
# files as extra dist files to make them appear in the tar made by
# 'gmake dist'
EXTRA_DIST=alloc.c \
           analysis.c \
           cluster.c \
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
           util.c \
           wcrt.h
