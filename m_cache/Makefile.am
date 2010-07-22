SUBDIRS=wcrt

noinst_PROGRAMS=opt opt_debug

# Settings for both programs
AM_CPPFLAGS=-D_PRINT

# First program to build: The analyzer/optimizer for the user
opt_SOURCES=analysisCache.c analysisCache.h \
						analysisCacheL2.c analysisCacheL2.h \
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
						parseCFG.c parseCFG.h \
						path.c path.h \
						pathDAG.c pathDAG.h \
						topo.c topo.h \
						updateCacheL2.c updateCacheL2.h
opt_LDADD=wcrt/libwcrt.a

# Second program to build: The analyzer/optimizer for the developer to play around
opt_debug_SOURCES=analysisCache.c analysisCache.h \
								  analysisCacheL2.c analysisCacheL2.h \
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
								  main_debug.c \
								  loopdetect.c loopdetect.h \
								  parseCFG.c parseCFG.h \
								  path.c path.h \
								  pathDAG.c pathDAG.h \
								  topo.c topo.h \
								  updateCacheL2.c updateCacheL2.h
opt_debug_LDADD=wcrt/libwcrt.a
