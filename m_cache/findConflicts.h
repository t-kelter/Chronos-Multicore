/*! This is a header file of the Chronos timing analyzer. */

/*! Functions to detect conflicting branches. */

#ifndef __CHRONOS_FIND_CONFLICTS_H
#define __CHRONOS_FIND_CONFLICTS_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Returns 1 if deri_tree is modified inside lp, 0 otherwise.
 * Potentially recursive.
 */
char modifiedInLoop( loop *lp, char *deri_tree );

/*
 * Recursive function.
 * Returns 1 if there is a path from bblist[srcid] to bblist[destid] in the CFG without
 * passing through a nested loophead nor an assignment to a component of deri_tree
 * (other than assg, which is the one being tested, if any).
 * Returns 0 otherwise.
 */
char reachableNoCancel( int pid, int srcid, int destid, char *deri_tree,
			assign *assg, block **bblist, int num_bb, char *visited );

/*
 * Returns 1 if there is a path from bblist[srcid] to bblist[destid] in the CFG without
 * passing through a nested loophead nor an assignment to a component of deri_tree
 * (other than assg, which is the one being tested, if any).
 * Returns 0 otherwise.
 */
char isReachableNoCancel( int pid, int srcid, int destid, char *deri_tree,
			  assign *assg, block **bblist, int num_bb );


int addAssign( char deri_tree[], block *bb, int lineno, int rhs, char rhs_var );

int addBranch( char deri_tree[], block *bb, int rhs, char rhs_var, char
    jump_cond );

int setReg2Mem( int pos, char mem_addr[], int value, int instr );

/*
 * Go through the entire list of instructions and collect effects (assignments, branches).
 * Calculate and store register values (in the form of deri_tree) for each effect.
 */
int execute();

/*
 * Records conflict between assg (at index id of assignlist) and br taking direction dir.
 */
int setBAConflict( assign *assg, branch *br, int id, char dir );

/*
 * Records conflict between br1 taking direction dir1 and br2 taking direction dir2.
 */
int setBBConflict( branch *br1, branch *br2, char dir1, char dir2, char new );

/*
 * Checks pairwise effects to identify conflicts.
 * Special handling of loopheads:
 * - when detecting for the loop, consider only the iteration edge (taken branch)
 * - when detecting for its nesting procedure/loop (treated as black box),
 *   consider only the exit edge (non-taken branch)
 * lpid is given to identify the loop being analyzed.
 */
int detectConflictTopo( procedure *p, block **bblist, int num_bb, int lpid );

/*
 * Checks pairwise effects to identify conflicts.
 */
int detectConflicts();


#endif
