/*! This is a header file of the Chronos timing analyzer. */

/*! Functions for solving the Knapsack problem. */

#ifndef __CHRONOS_KNAPSACK_H
#define __CHRONOS_KNAPSACK_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Knapsack solution via Dynamic Programming.
 * Returns the optimal gain value, and updates the array alloc with the solution.
 * If only interested in the objective value, pass NULL as alloc.
 *
 * DP table: need to keep only the current row and the last one row.
 * For each cell we keep the gain and the allocation (as bit array).
 */
int DP_knapsack( int capacity, int num_items, int *gain, int *weight, char
    *alloc );


#endif
