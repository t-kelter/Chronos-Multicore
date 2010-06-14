/*
 * Block and instruction manipulation functions.
 */

#include <string.h>
#include <stdio.h>


/*
 * Returns the index of bbid in bblist, -1 if not found.
 * Searches bblist from index start to end, both inclusive.
 * Mostly used to search for successor, which is usually near the end.
 */
int getblock( int bbid, block **bblist, int start, int end ) {

  int i;
  for( i = end; i >= start; i-- )
    if( bblist[i]->bbid == bbid )
      return i;
  return -1;
}


/*
 * Returns:
 *  0 if addr is in the address range of bb;
 *  1 if addr is larger than the range;
 * -1 if addr is smaller than the range.
 */
int testBlockRange( int addr, block *bb ) {

  if( !bb )
    printf( "Null basic block encountered in search.\n" ), exit(1);

  if( addr < bb->startaddr )
    return -1;

  if( addr >= bb->startaddr + bb->size )
    return 1;

  return 0;
}


/*
 * Returns 1 if addr is in the address range of p, 0 otherwise.
 */
int inProcRange( int addr, procedure *p ) {

  block *lastbb;
  int lastid;

  if( !p || !p->num_bb )
    return 0;

  if( addr < p->bblist[0]->startaddr )
    return 0;

  // there may be a dummy block
  lastid = p->num_bb - 1;
  lastbb = p->bblist[lastid];
  while( lastbb->startaddr == -1 && lastid > 0 )
    lastbb = p->bblist[ --lastid ];

  if( lastbb->startaddr == -1 )
    return 0;

  if( addr >= lastbb->startaddr + lastbb->size )
    return 0;

  return 1;
}


/*
 * Recursive binary search that searches bblist for a block 
 * with address range that includes addr.
 */
block* binSearch( int addr, block **bblist, int start, int end ) {

  int mid, rel;

  if( start > end )
    return NULL;

  mid = (start + end) / 2;
  rel = testBlockRange( addr, bblist[mid] );

  if( rel == 0 )
    return bblist[mid];

  if( rel < 0 )
    return binSearch( addr, bblist, start, mid - 1 );

  return binSearch( addr, bblist, mid + 1, end );
}


/*
 * Searches bblist for a block with address range that includes addr.
 * Returns the block pointer, NULL if none found.
 */
block* searchBBlist( int addr, block **bblist, int len ) {

  /*
  // linear search
  int i;
  for( i = 0; i < len; i++ ) {
    if( testBlockRange( addr, bblist[i] ) == 0 )
      return bblist[i];
  }
  */

  // recursive binary search
  return binSearch( addr, bblist, 0, len - 1 );
}


/*
 * Returns pointer to the block containing addr, NULL if not found.
 */
block* findBlock( int addr ) {

  int i, lastid;
  procedure *p;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];
    if( inProcRange( addr, p )) {
      // exclude dummy block
      lastid = p->num_bb - 1;
      while( p->bblist[lastid]->startaddr == -1 && lastid > 0 )
	lastid--;
      return searchBBlist( addr, p->bblist, lastid + 1 );
    }
  }
  return NULL;
}


/*
 * Returns 1 if insn is a branch instruction, 0 otherwise. Currently unused.
 */
char isBranchInstr( instr *insn ) {

  return ( strcmp( insn->op, "bgez" ) == 0 || 
	   strcmp( insn->op, "bgtz" ) == 0 ||
	   strcmp( insn->op, "blez" ) == 0 ||
	   strcmp( insn->op, "bne"  ) == 0 || 
	   strcmp( insn->op, "beq"  ) == 0 ||
	   strcmp( insn->op, "bc1f" ) == 0 || 
	   strcmp( insn->op, "bc1t" ) == 0 );
}


char isAssignInstr( instr *insn ) {

  return ( strcmp( insn->op, "sw"  ) == 0 ||
	   strcmp( insn->op, "sb"  ) == 0 ||
	   strcmp( insn->op, "sbu" ) == 0 ||
	   strcmp( insn->op, "sh"  ) == 0 );
}


char *getJumpDest( instr *insn ) {

  // The jump destination is given in r2 ( bgez, bgtz, blez ) or r3 ( bne, beq ).

  if( strcmp( insn->op, "bgez" ) == 0 || 
      strcmp( insn->op, "bgtz" ) == 0 || 
      strcmp( insn->op, "blez" ) == 0 )
    return insn->r2;

  if( strcmp( insn->op, "bne" ) == 0 || 
      strcmp( insn->op, "beq" ) == 0 )
    return insn->r3;

  printf( "Unrecognized or not a jump instruction: %s\n", insn->op );
  return NULL;
}


int hexValue( char *hexStr ) {

  int val;

  sscanf( hexStr, "%x", &val );
  return val;
}
