/*! This is a header file of the Chronos timing analyzer. */

/*
 * Block and instruction manipulation functions.
 */

#ifndef __CHRONOS_BLOCK_H
#define __CHRONOS_BLOCK_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Returns the index of the instruction 'i' in the instruction list
 * 'ilist', or -1 if not found. Searches the list from 'start' to 'end'.
 */
int getinstruction( const instr *i, const instr **ilist, int start, int end );

/*
 * Returns the index of bbid in bblist, -1 if not found.
 * Searches bblist from index start to end, both inclusive.
 * Mostly used to search for successor, which is usually near the end.
 */
int getblock( int bbid, block **bblist, int start, int end );

/*
 * Returns:
 *  0 if addr is in the address range of bb;
 *  1 if addr is larger than the range;
 * -1 if addr is smaller than the range.
 */
int testBlockRange( int addr, block *bb );

/*
 * Returns 1 if addr is in the address range of p, 0 otherwise.
 */
int inProcRange( int addr, procedure *p );

/*
 * Recursive binary search that searches bblist for a block 
 * with address range that includes addr.
 */
block* binSearch( int addr, block **bblist, int start, int end );

/*
 * Searches bblist for a block with address range that includes addr.
 * Returns the block pointer, NULL if none found.
 */
block* searchBBlist( int addr, block **bblist, int len );

/*
 * Returns pointer to the block containing addr, NULL if not found.
 */
block* findBlock( int addr );

/*
 * Returns 1 if insn is a branch instruction, 0 otherwise. Currently unused.
 */
char isBranchInstr( instr *insn );

char isAssignInstr( instr *insn );

char *getJumpDest( instr *insn );

int hexValue( char *hexStr );

#endif
