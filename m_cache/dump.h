/*! This is a header file of the Chronos timing analyzer. */

/*
 * Text dump for debugging.
 */

#ifndef __CHRONOS_DUMP_H
#define __CHRONOS_DUMP_H

#include "header.h"
#include "analysisDAG_common.h"
#include "infeasible.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int printBlock( const block * const bb );

int printProc( const procedure * const p );

int print_cfg();

int printLoop( const loop * const lp );

int print_loops();

int print_topo();

int printWCETLoop( const loop * const lp );

int printWCETProc( const procedure * const p );

int print_wcet();

int printInstr( FILE *fptr, const instr * const insn );

int printBlockInstr( FILE *fptr, const block * const bb );

int print_instrlist();

int printPath( const path * const pt );

int printBranch( const branch * const br, const char printcf );

int printAssign( const assign * const assg, const int id, const char printcf );

int print_effects();

int dump_loops();

int dump_callgraph();

void dumpCacheState( const cache_state * const cs );

void dumpCacheState_L2( const cache_state * const cs );

void dump_prog_info( const procedure * const proc );

/* This is for debugging. Dumped chmc info after preprocessing */
void dump_pre_proc_chmc( const procedure * const proc,
                         const enum AccessScenario scenario);


#endif
