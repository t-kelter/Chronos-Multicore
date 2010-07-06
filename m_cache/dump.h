/*! This is a header file of the Chronos timing analyzer. */

/*
 * Text dump for debugging.
 */

#ifndef __CHRONOS_DUMP_H
#define __CHRONOS_DUMP_H

#include "header.h"
#include "infeasible.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int printBlock( block *bb );

int printProc( procedure *p );

int print_cfg();

int printLoop( loop *lp );

int print_loops();

int print_topo();

int printWCETLoop( loop *lp );

int printWCETProc( procedure *p );

int print_wcet();

int printInstr( FILE *fptr, instr *insn );

int printBlockInstr( FILE *fptr, block *bb );

int print_instrlist();

int printPath( path *pt );

int printBranch( branch *br, char printcf );

int printAssign( assign *assg, int id, char printcf );

int print_effects();

int dump_loops();

int dump_callgraph();

static void
dumpCacheState(cache_state *cs);

static void
dumpCacheState_L2(cache_state *cs);

static void
dumpProcCopy(task_t *task);


#endif
