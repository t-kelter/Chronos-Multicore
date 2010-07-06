SUBDIRS=wcrt

noinst_PROGRAMS=opt optdebug

# First program to build: The analyzer/optimizer for the user
opt_SOURCES=main.c
opt_CPPFLAGS=-D_PRINT
opt_LDADD=wcrt/libwcrt.a

# Second program to build: The analyzer/optimizer for the developer to play around
optdebug_SOURCES=main_debug.c
optdebug_CPPFLAGS=-D_DEBUG -D_PRINT
optdebug_LDADD=wcrt/libwcrt.a

# Due to the special include system, we must list all indirect source 
# files as extra dist files to make them appear in the tar made by
# 'gmake dist'
EXTRA_DIST=analysisCache.c \
           analysisCacheL2.c \
           analysisDAG_BCET.c \
           analysisDAG_WCET.c \
           analysisEnum.c \
           analysisILP.c \
           block.c \
           busSchedule.c \
           DAG_WCET.c \
           dump.c \
           findConflicts.c \
           handler.c \
           header.h \
           infeasible.c \
           infeasible.h \
           loopdetect.c \
           parseCFG.c \
           path.c \
           pathDAG.c \
           pathTest.c \
           topo.c \
           updateCacheL2.c
