/*! This is a header file of the Chronos timing analyzer. */

/*
 * Loop detection functions.
 */

#ifndef __CHRONOS_LOOPDETECT_H
#define __CHRONOS_LOOPDETECT_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Recursive function.
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 */
int reachable( int src, int dest, block **bblist, char *visited );

/*
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 */
int isReachable( int src, int dest, block **bblist, int num_bb );

/*
 * Recursive function.
 * Returns 1 if inloopid is nested inside outloopid, 0 otherwise.
 */
int nested( int inloopid, int outloopid, loop **loops );

/*
 * Removes edge outid from bb.
 */
int removeEdge( block *bb, char outid );

/*
 * Removes loop exit edges.
 */
int removeExitEdges( procedure *p );

/*
 * Removes loop back-edges.
 */
void removeBackEdges( loop **loops, int num_loops );

/*
 * Returns the index if id is found in exits, -1 otherwise.
 */
int inExits( int id, block **exits, int num_exits );

/*
 * Prompts user to choose from a list of loopexits.
 * Returns the chosen loopexit.
 *
 * Uses shell script getsrc to print source code lines corresponding to the exit blocks.
 * Reads from [filename].mdis which contains the assembly code of the program,
 * along with the source code line number that each instruction corresponds to.
 */
block *promptLoopExit( loop *lp, block **exits, int num_exits );

/*
 * Reads loop exit info from file.
 * If a valid annotation is found for lp, sets lp's loopexit to the given id,
 * then returns 1. Returns 0 otherwise.
 */
char readLoopExit( loop *lp, block **exits, int num_exits );

/*
 * Finalise loopexits from a list of possibly multiple loopexits.
 * Exit blocks are recorded in the loop, then the exit edges are removed.
 */
int determineLoopExits( loop **loops, int num_loops, 
                        block **bblist, int num_bb );

/*
 * Reads loop annotation (loopbound and is_dowhile).
 */
int read_loop_annotation();

/*
 * Removes block with id bbid from loop lp.
 */
int removeLoopComponent( loop *lp, int bbid );

/*
 * Adds bb to lp's components, if it does not already exist there.
 * Check indicates if checking of existence should be done.
 */
int addToLoop( loop *lp, block *bb, char check );

/*
 * Checks if an edge from b to out is inside a loop.
 * The edge (b, out) is in a loop if b is reachable from out.
 * If the edge is in loop, both b and out are marked.
 * Returns 1 if a new loop is detected (i.e. b is not already in a loop of the same level),
 * 2 if a dummy block is added to the procedure, 0 otherwise.
 */
int checkLoop( procedure *p, block *bb, char outid, int level );

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
