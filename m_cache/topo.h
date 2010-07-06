/*! This is a header file of the Chronos timing analyzer. */

/*
 * Functions for constructing topological ordering.
 */

#ifndef __CHRONOS_TOPO_H
#define __CHRONOS_TOPO_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Topological sort with dfs principle:
 * topological order is in reverse order of finishing
 * (loosely, finished means that all outgoing nodes are already visited).
 *
 * For loops, the block pointers are already stored in topo array (unsorted).
 * First make a copy of the block pointers, then re-fill topo with sorted sequence.
 *
 * When nested loopheads is encountered, construct a 'blackbox' 
 * with outgoing edge directly to the loopexit.
 */
int topo_sort_loop( loop *lp );

int topo_sort_proc( procedure *p );

/*
 * Determines topological ordering for procedures and loops.
 */
int topo_sort();


#endif
