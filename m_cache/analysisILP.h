/*! This is a header file of the Chronos timing analyzer. */

/*
 * ILP-based WCET analysis functions.
 */

#ifndef __CHRONOS_ANALYSIS_ILP_H
#define __CHRONOS_ANALYSIS_ILP_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Recursive function.
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 * Searches bblist from index start to end.
 * Used for bblist whose indexing does not correspond with bbid.
 */
int reachableTopo( block *src, int dest, block **bblist, int start, int end,
    char *visited );

/*
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 * Searches bblist from index start to end.
 * Used for bblist whose indexing does not correspond with bbid.
 */
int isReachableTopo( block *src, int dest, block **bblist, int start, int end );

int getBlockExecCount( block *bb );

int ILPconstDAG( char objtype, void *obj );

int ILPconstProc( procedure *p );

int analysis_ilp();


#endif
