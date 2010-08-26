SUBDIRS=wcrt

noinst_PROGRAMS=opt

# Settings
AM_CPPFLAGS=-D_GNU_SOURCE -I@chronos_path@

# First program to build: The analyzer/optimizer for the user
opt_SOURCES=analysisCache_L1.c analysisCache_L1.h \
						analysisCache_L2.c analysisCache_L2.h \
						analysisCache_common.c analysisCache_common.h \
						analysisDAG_BCET_structural.c analysisDAG_BCET_structural.h \
						analysisDAG_BCET_unroll.c analysisDAG_BCET_unroll.h \
						analysisDAG_ET_alignment.c analysisDAG_ET_alignment.h \
						analysisDAG_WCET_structural.c analysisDAG_WCET_structural.h \
						analysisDAG_WCET_unroll.c analysisDAG_WCET_unroll.h \
						analysisDAG_common.c analysisDAG_common.h \
						analysisEnum.c analysisEnum.h \
						analysisILP.c analysisILP.h \
						block.c block.h \
						busSchedule.c busSchedule.h \
						DAG_WCET.c DAG_WCET.h \
						dump.c dump.h \
						findConflicts.c findConflicts.h \
						handler.c handler.h \
						header.h \
						infeasible.c infeasible.h \
						main.c \
						loopdetect.c loopdetect.h \
						offsetData.c offsetData.h \
						offsetGraph.c offsetGraph.h \
						parseCFG.c parseCFG.h \
						path.c path.h \
						pathDAG.c pathDAG.h \
						topo.c topo.h \
						updateCacheL2.c updateCacheL2.h
						
# This forces Automake to use g++ to link the application.
nodist_opt_SOURCES = dummy.cpp

# We generate the dummy C++ source file so we don't have  to think about it.
dummy.cpp:
	echo 'void dummy();' > $@

CLEANFILES = dummy.cpp
						
opt_LDADD=wcrt/libwcrt.a ../debugmacros/libdebugmacros.a -lrt
