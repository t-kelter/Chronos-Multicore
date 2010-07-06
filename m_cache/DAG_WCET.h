/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based infeasible path detection.
 */

#ifndef __CHRONOS_DAG_WCET_H
#define __CHRONOS_DAG_WCET_H

#include "header.h"
#include "infeasible.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int detectDirection( branch *bru, block *bv );

/*
 * Restores the count of incoming conflicts for each branch.
 */
int restoreIncomingConflicts();

/*
 * Returns 1 if bbid appears in bb_seq before target, 0 otherwise.
 */
char in_seq_bef( int bbid, int target, int *bb_seq, int bb_len );

int effectCancelled( branch *br, assign *assg, path *pv, 
                     block **bblist, int num_bb );

char BBconflictInPath( branch *bru, char direction, block *bv, 
                       path *pv, block **bblist, int num_bb );

char BAconflictInPath( block *bu, block *bv, path *pv, 
                       block **bblist, int num_bb );

/*
 * Returns 1 if bb contains an assignment to a component of addr
 * (thus cancelling any effect before it), returns 0 otherwise.
 */
char assignsTo( block *bb, char *addr );

/*
 * Checks incoming conflicts of br, returns 1 if there exists 
 * incoming conflict that clashes with direction dir.
 */
char hasIncomingConflict( branch *br, char dir, block **bblist, 
                          int start, int num_bb );

/*
 * Traverse CFG in reverse topological order (already given in bblist)
 * to collect cost, eliminating infeasible paths.
 */
int traverse( int pid, block **bblist, int num_bb, int *in_degree, uint *cost );

path* find_WCETPath( int pid, block **bblist, int num_bb, 
                     int *in_degree, uint *cost );


#endif
