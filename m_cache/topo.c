#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "topo.h"
#include "block.h"
#include "parseCFG.h"

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
int topo_sort_loop( loop *lp ) {

  int   i, k, exid;
  block *bb, *nb;
  loop  *lpn;

  block **unsorted;   // a copy of current unsorted blocks
  block **comefrom;   // keep the bb from which we came, to return to
  int   comefromsize;
  char  *markfin;     // markfin: 0: unvisited, 1: visited but unfinished, 2: finished
  char  fin;

  int countdone;

  comefrom     = NULL;
  comefromsize = 0;
  markfin = (char*) CALLOC( markfin, lp->num_topo, sizeof(char), "loop topo fin array" );

  // copy topo's unsorted contents
  unsorted = (block**) MALLOC( unsorted, lp->num_topo * sizeof(block*), "loop topo copy" );
  for( i = 0; i < lp->num_topo; i++ )
    unsorted[i] = lp->topo[i];

  countdone = 0;

  // printf( "Loop %d\n", lp->lpid );

  // start from loop source
  bb = unsorted[0];

  while( countdone < lp->num_topo ) {

    /*
    printBlock( bb );
    for( i = 0; i < lp->num_topo; i++ )
      printf( " %d", markfin[i] );
    printf( "\n" );
    */

    // (nested) loopheads
    if( bb->is_loophead && bb->loopid != lp->lpid ) {

      k = getblock( bb->bbid, unsorted, 0, lp->num_topo-1 );

      if( !markfin[k] ) {
	lpn = procs[ lp->pid ]->loops[ bb->loopid ];

	// construct blackbox
	nb = (block*) MALLOC( nb, sizeof(block), "blackbox" );
	memset(nb, 0, sizeof(block));
	createBlock( nb, lp->pid, bb->bbid, bb->startaddr, lpn->loopexit->bbid, -1, -1, lpn->lpid );
	nb->is_loophead = 1;
	nb->size        = bb->size;
	//nb->cost        = bb->cost;
	//nb->regid       = bb->regid;
	nb->num_instr   = bb->num_instr;
	nb->instrlist   = bb->instrlist;  // just the reference
	
	// printf( "loop %d-%d: added blackbox for loophead %d:\n", lp->pid, lp->lpid, lpn->lpid );
	// printf( "original block: " ); printBlock( bb );
	// printf( "blackbox block: " ); printBlock( nb );

	// replace bb with nb in unsorted
	unsorted[k] = nb;

	bb = nb;
      }

      // consider finished if loopexit is done
      exid = getblock( lpn->loopexit->bbid, unsorted, 0, lp->num_topo-1 );
      if( exid == -1 || markfin[exid] == 2 ) {

	markfin[k] = 2;
	lp->topo[ countdone++ ] = bb;

	// returning
	if( !comefromsize )
	  break;
	bb = comefrom[ --comefromsize ];
      }
      else {
	markfin[k] = 1;
	comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
	comefrom[ comefromsize++ ] = bb;

	// directly go to loop exit
	bb = lpn->loopexit;
      }
      continue;
    }

    // otherwise, consider finished if all children in the loop is finished
    fin = 1;
    for( i = 0; fin && i < bb->num_outgoing; i++ ) {
      k = getblock( bb->outgoing[i], unsorted, 0, lp->num_topo-1 );
      if( k != -1 && markfin[k] != 2 )
	fin = 0;
    }

    k = getblock( bb->bbid, unsorted, 0, lp->num_topo-1 );

    if( bb->bbid == lp->loopsink->bbid || !bb->num_outgoing || fin ) {  // finished
      // printf( "fin %d\n", bb->bbid );
      markfin[k] = 2;
      lp->topo[ countdone++ ] = bb;

      // returning
      if( !comefromsize )
	break;
      bb = comefrom[ --comefromsize ];
    }
    else if( !markfin[k] ) {  // just visited
      markfin[k] = 1;
      comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
      comefrom[ comefromsize++ ] = bb;
      
      // if first outgoing edge in loop, go there
      k = getblock( bb->outgoing[0], unsorted, 0, lp->num_topo-1 );
      if( k != -1 )
	bb = unsorted[k];
      else {
	// returning
	if( !comefromsize )
	  break;
	bb = comefrom[ --comefromsize ];
      }
    }
    else if( bb->num_outgoing > 1 ) {  // must be branch
      comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
      comefrom[ comefromsize++ ] = bb;
      
      // first branch must have finished
      // if second outgoing edge in loop, go there
      k = getblock( bb->outgoing[1], unsorted, 0, lp->num_topo-1 );
      if( k != -1 )
	bb = unsorted[k];
      else {
	// returning
	if( !comefromsize )
	  break;
	bb = comefrom[ --comefromsize ];
      }
    }
    else
      printf( "Unusual case at %d.\n", bb->bbid ), exit(1);

  } // end while( countdone < lp->num_blocks )

  // if nested loops have multiple exits, there may be ignored blocks
  lp->num_topo = countdone;
  // lp->topo     = (int*) REALLOC( lp->topo, lp->num_topo * sizeof(int), "loop topo array" );

  free( unsorted );
  free( comefrom );
  free( markfin );

  return 0;
}


int topo_sort_proc( procedure *p ) {

  int   i;
  block *bb, *nb;
  loop  *lp;

  block **comefrom;   // keep the bb from which we came, to return to
  int   comefromsize;
  char  *markfin;     // markfin: 0: unvisited, 1: visited but unfinished, 2: finished
  char  fin;

  int countdone;

  comefrom     = NULL;
  comefromsize = 0;
  markfin = (char*) CALLOC( markfin, p->num_bb, sizeof(char), "proc topo fin array" );
  p->topo = (block**) REALLOC( p->topo, p->num_bb * sizeof(block*), "proc topo array" );

  countdone = 0;

  // printf( "Proc %d\n", p->pid );

  // start from first bb
  bb = p->bblist[0];

  while( countdone < p->num_bb ) {

    /*
    // printf( "%d(%d)\n", bb->bbid, markfin[ bb->bbid ] );
    printBlock( bb );
    for( i = 0; i < p->num_bb; i++ )
      printf( " %d", markfin[i] );
    printf( "\n" );
    */

    // (nested) loopheads
    if( bb->is_loophead ) {

      if( !markfin[ bb->bbid ] ) {
	lp = p->loops[ bb->loopid ];

	// construct blackbox
	nb = (block*) MALLOC( nb, sizeof(block), "blackbox" );
	memset(nb, 0, sizeof(block));
	createBlock( nb, p->pid, bb->bbid, bb->startaddr, lp->loopexit->bbid, -1, -1, lp->lpid );

	nb->is_loophead = 1;
	nb->size        = bb->size;
	//nb->cost        = bb->cost;
	//nb->regid       = bb->regid;
	nb->num_instr   = bb->num_instr;
	nb->instrlist   = bb->instrlist;  // just the reference
	
	// printf( "proc %d: added blackbox for loophead %d:\n", p->pid, lp->lpid );
	// printf( "original block: " ); printBlock( bb );
	// printf( "blackbox block: " ); printBlock( nb );

	bb = nb;
      }

      // consider finished if loopexit is done
      if( markfin[ lp->loopexit->bbid ] == 2 ) {

	markfin[ bb->bbid ] = 2;
	p->topo[ countdone++ ] = bb;

	// returning
	if( !comefromsize )
	  break;
	bb = comefrom[ --comefromsize ];
      }
      else {
	markfin[ bb->bbid ] = 1;
	comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
	comefrom[ comefromsize++ ] = bb;

	// directly go to loop exit
	bb = lp->loopexit;
      }
      continue;
    }

    // if in loop and not loophead, ignore (just for safety; shouldn't reach this)
    if( bb->loopid != -1 ) {
      // returning
      if( !comefromsize )
	break;
      bb = comefrom[ --comefromsize ];
      continue;
    }

    // otherwise, consider finished if all children finished
    fin = 1;
    for( i = 0; fin && i < bb->num_outgoing; i++ )
      if( markfin[ bb->outgoing[i] ] != 2 )
	fin = 0;

    if( !bb->num_outgoing || fin ) { // finished
      // printf( "fin %d\n", bb->bbid );
      markfin[ bb->bbid ] = 2;
      p->topo[ countdone++ ] = bb;

      // returning
      if( !comefromsize )
	break;
      bb = comefrom[ --comefromsize ];
    }
    else if( !markfin[ bb->bbid ] ) {  // just visited
      markfin[ bb->bbid ] = 1;
      comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
      comefrom[ comefromsize++ ] = bb;

      // go to first outgoing edge
      bb = p->bblist[ bb->outgoing[0] ];
    }
    else if( bb->num_outgoing > 1 ) {  // must be branch
      comefrom = (block**) REALLOC( comefrom, (comefromsize + 1) * sizeof(block*), "topo comefrom array" );
      comefrom[ comefromsize++ ] = bb;

      // first branch must have finished
      bb = p->bblist[ bb->outgoing[1] ];
    }
    else
      printf( "Unusual case at %d.\n", bb->bbid ), exit(1);

  } // end while( countdone < p->num_bb )

  // since we skip loops, contents of topo is subset of bblist
  p->num_topo = countdone;
  // p->topo     = (int*) REALLOC( p->topo, p->num_topo * sizeof(int), "proc topo array" );

  free( comefrom );
  free( markfin );

  return 0;
}


/*
 * Determines topological ordering for procedures and loops.
 */
int topo_sort() {

  int i, j;
  procedure *p;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];

    // loops
    for( j = 0; j < p->num_loops; j++ )
      topo_sort_loop( p->loops[j] );

    // procedure
    topo_sort_proc( p );
  }

  return 0;
}
