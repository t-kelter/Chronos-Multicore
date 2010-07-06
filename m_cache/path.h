/*! This is a header file of the Chronos timing analyzer. */

/*
 * Path manipulation functions. Support for DAG-based infeasible path detection.
 */

#ifndef __CHRONOS_PATH_H
#define __CHRONOS_PATH_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int freePath( int bbid, int ptid );

int freePathsInNode( int bbid );

/*
 * Returns 1 if p1 and p2 have exactly the same set of branches, 0 otherwise.
 */
char identicalConflict( path *p1, path *p2 );

/*
 * Returns 1 if p contains branch br taking direction dir, 0 otherwise.
 */
char hasEdge( path *p, branch *br, char dir );

/*
 * Returns 1 if p1's conflict list is a subset of p2's conflict list, 0 otherwise.
 * Note that the identical relation is included.
 */
char subsetConflict( path *p1, path *p2 );

/*
 * Sorts pathlist according to increasing cost, then decreasing number of branches.
 */
int sortPath( path **pathlist, int num_paths );

/*
 * Copies p2's block and branch sequences into p1.
 */
int copySeq( path *p1, path *p2 );

/*
 * Insert edge <br,dir> into pt's branch sequence,
 * maintaining increasing order of branch index.
 */
int sortedInsertBranch( path *pt, branch *br, char dir );

/*
 * Removes the branch at index bri of pt's branch sequence.
 */
int removeBranch( path *pt, int bri );

/*
 * Returns the index at which br is found in the list branch_eff,
 * -1 if not found.
 */
int findBranch( branch *br, branch **branch_eff, int branch_len );


#endif
