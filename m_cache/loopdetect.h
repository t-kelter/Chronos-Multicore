/*! This is a header file of the Chronos timing analyzer. */

/*
 * Loop detection functions.
 */

#ifndef __CHRONOS_LOOPDETECT_H
#define __CHRONOS_LOOPDETECT_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Determines whether the given block is a loop exit block. The block is a
 * loop exit block when all predecessors are inside a single loop. If such
 * a loop could be found, it is returned, else NULL is returned.
 *
 * This only works if loop exit edges have not been removed.
 */
loop *isLoopExit( const block * const bb, const procedure * const proc );

/*
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 */
int isReachable( int src, int dest, block **bblist, int num_bb );

/*
 * Recursive function.
 * Returns 1 if inloopid is nested inside outloopid, 0 otherwise.
 */
int isNested( int inloopid, int outloopid, loop **loops );

/* 
 * Identify loops in each procedure, level by level.
 * After each layer, remove all back-edges and re-run the detection:
 * this will identify the second nested level, and so on.
 *
 * For each loop:
 * - there is a unique loophead
 * - there may be multiple loopsinks (due to 'continue' statements),
 *   in which case a dummy sink is constructed
 * - there may be multiple loopexits (due to 'break', 'return' or 'goto'),
 *   in which case the user is prompted to identify the 'normal' exit
 *   (via loop condition, as opposed to via the above statements)
 */
int detect_loops();


#endif
