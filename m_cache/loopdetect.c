// Include standard library headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "loopdetect.h"
#include "handler.h"
#include "block.h"
#include "parseCFG.h"
#include "dump.h"


/*
 * Determines whether the given block is a loop exit block. The block is a
 * loop exit block when all predecessors are inside a single loop. If such
 * a loop could be found, it is returned, else NULL is returned.
 *
 * This only works if loop exit edges have not been removed.
 */
loop *isLoopExit( const block * const bb, const procedure * const proc )
{
  loop *loopToExit = NULL;

  // Scan all predecessors
  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {
    const block * const pred = proc->bblist[bb->incoming[i]];

    // Is predecessor in another loop?
    if ( pred->loopid >= 0 &&
         pred->loopid != bb->loopid ) {
      loop * const pred_loop = proc->loops[pred->loopid];

      // Are all predecessors in the same loop?
      if ( loopToExit == NULL ) {
        loopToExit = pred_loop;
      } else {
        if ( loopToExit == pred_loop ) {
          continue;
        } else {
          return NULL;
        }
      }
    } else {
      return NULL;
    }
  }

  return loopToExit;
}


/*
 * Recursive function.
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 */
static int reachable( int src, int dest, block **bblist, char *visited )
{
  if ( src == dest )
    return 1;

  visited[src] = 1;

  block *bb = bblist[src];
  if ( !bb )
    printf( "reachable: Null basic block at %d.\n", src ), exit( 1 );

  int i;
  for ( i = 0; i < bb->num_outgoing; i++ ) {
    if ( !visited[bb->outgoing[i]] )
      if ( reachable( bb->outgoing[i], dest, bblist, visited ) )
        return 1;
  }

  return 0;
}

/*
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 */
int isReachable( int src, int dest, block **bblist, int num_bb )
{
  char *visited; // bit array to mark visited nodes
  CALLOC( visited, char*, num_bb, sizeof(char), "visited" );
  int res = reachable( src, dest, bblist, visited );
  free( visited );

  return res;
}

/*
 * Recursive function.
 * Returns 1 if inloopid is nested inside outloopid, 0 otherwise.
 */
int isNested( int inloopid, int outloopid, loop **loops )
{
  if ( inloopid == -1 )
    return 0;

  loop *in = loops[inloopid];
  if ( !in )
    printf( "Null loop at %d\n", inloopid ), exit( 1 );

  if ( in->nest == -1 )
    return 0;

  if ( in->nest == outloopid )
    return 1;

  return isNested( in->nest, outloopid, loops );
}

/*
 * Removes edge outid from bb.
 */
/*
static int removeEdge( block *bb, char outid )
{
  bb->num_outgoing--;

  // if the removed edge is the first edge and there is a second edge, then shift
  if ( bb->num_outgoing && outid == 0 )
    bb->outgoing[0] = bb->outgoing[1];

  return 0;
}
*/

/*
 * Removes loop exit edges.
 */
/*
static int removeExitEdges( procedure *p )
{
  int i;
  for ( i = 0; i < p->num_loops; i++ ) {
    loop *lp = p->loops[i];

    int j;
    for ( j = 0; j < lp->num_topo; j++ ) {
      block *bb = lp->topo[j];

      int k;
      for ( k = 0; k < bb->num_outgoing; k++ ) {
        block *e = p->bblist[bb->outgoing[k]];

        if ( e->loopid != lp->lpid && !isNested( e->loopid, lp->lpid, p->loops ) ) {
          removeEdge( bb, k );
        }
      }
    }
  }

  return 0;
}
*/

/*
 * Removes loop back-edges.
 */
static void removeBackEdges( loop **loops, int num_loops )
{
  int i;
  for ( i = 0; i < num_loops; i++ ) {
    loop *lp = loops[i];
    block *bb = lp->loopsink;

    if ( bb->num_outgoing == 1 && bb->outgoing[0] == lp->loophead->bbid )
      bb->num_outgoing--;
    else if ( bb->num_outgoing == 2 ) {

      if ( bb->outgoing[0] == lp->loophead->bbid ) {
        bb->outgoing[0] = bb->outgoing[1];
        bb->num_outgoing--;
      } else if ( bb->outgoing[1] == lp->loophead->bbid )
        bb->num_outgoing--;
    }

    bb->num_outgoing_copy = bb->num_outgoing;
  }
}

/*
 * Returns the index if id is found in exits, -1 otherwise.
 */
static int inExits( int id, block **exits, int num_exits )
{
  int i;
  for ( i = 0; i < num_exits; i++ )
    if ( exits[i]->bbid == id )
      return i;

  return -1;
}

/*
 * Prompts user to choose from a list of loopexits.
 * Returns the chosen loopexit.
 *
 * Uses shell script getsrc to print source code lines corresponding to the exit blocks.
 * Reads from [filename].mdis which contains the assembly code of the program,
 * along with the source code line number that each instruction corresponds to.
 */
static block *promptLoopExit( loop *lp, block **exits, int num_exits )
{
  printf( "%d exits detected for loop %d-%d:\n", num_exits, lp->pid, lp->lpid );

  int i;
  for ( i = 0; i < num_exits; i++ ) {

    // print source code line

    // The value of shell variable y is the line number 
    // of the last assembly instruction of the exit block.
    // getsrc traces the source code line corresponding to this line and prints it out.

    printf( "Exit[%d]: %3d\n", i, exits[i]->bbid );
    fflush( stdout );

    //intr *insn = exits[i]->instrlist[ exits[i]->num_instr - 1];
    //printf( "%3d: %s: ", exits[i]->bbid, insn->addr ); fflush( stdout );

    //char proc[100];
    //sprintf( proc, "y=`cat -n %s.mdis | awk '$2 ~ /%s/ {print $1}'`; getsrc %s $y", 
    //     filename, insn->addr, filename );

    // printf( "%s\n", proc ); fflush( stdout );
    //system( proc );
  }

  /*
   printf( "\nPlease identify the normal exit: " );

   scanf( "%d", &res ); fflush( stdin );
   */

  int res = exits[0]->bbid;

  // verify answer
  for ( i = 0; i < num_exits; i++ ) {
    if ( res == exits[i]->bbid )
      return exits[i];
  }

  // invalid choice: repeat
  printf( "Sorry, that was an invalid choice.\n" );
  return promptLoopExit( lp, exits, num_exits );
}

/*
 * Reads loop exit info from file.
 * If a valid annotation is found for lp, sets lp's loopexit to the given id,
 * then returns 1. Returns 0 otherwise.
 */
static char readLoopExit( loop *lp, block **exits, int num_exits )
{
  FILE *fptr = openfile( "ex", "r" );

  int pid, lpid, ex, id;
  while ( fscanf( fptr, "%d %d %d", &pid, &lpid, &ex ) != EOF ) {

    if ( pid == lp->pid && lpid == lp->lpid ) {
      id = inExits( ex, exits, num_exits );

      if ( id != -1 ) {
        lp->loopexit = exits[id];
        fclose( fptr );
        return 1;
      }
    }
  }
  fclose( fptr );

  return 0;
}

/*
 * Finalise loopexits from a list of possibly multiple loopexits.
 * Exit blocks are recorded in the loop, then the exit edges are removed.
 */
static int determineLoopExits( loop **loops, int num_loops, block **bblist, int num_bb )
{
  int i;
  for ( i = 0; i < num_loops; i++ ) {
    loop *lp = loops[i];

    block **exits = NULL;
    int num_exits = 0;

    // check all loop components: any successor not in the same loop is a loopexit
    int j;
    for ( j = 0; j < lp->num_topo; j++ ) {
      block *b = lp->topo[j];

      int k;
      for ( k = 0; k < b->num_outgoing; k++ ) {

        if ( getblock( b->outgoing[k], lp->topo, 0, lp->num_topo - 1 ) == -1 ) {

          // add only if not already in list
          if ( inExits( b->outgoing[k], exits, num_exits ) == -1 ) {

            num_exits++;
            REALLOC( exits, block**, num_exits * sizeof(block*), "exits" );
            exits[num_exits - 1] = bblist[b->outgoing[k]];
          }

          // remove exit edge
          // removeEdge( b, k );
        }
      }
    }
    if ( !num_exits )
      prerr( "Loopexit not detected for %d\n", lp->lpid );

    // Set standard exit
    if ( num_exits == 1 ) {
      lp->loopexit = exits[0];
    } else {
      // try to read from file
      int success = 0;
      char proc[100];
      sprintf( proc, "ls %s.ex 2> /dev/null", filename );

      if ( !system( proc ) )
        // file exists
        success = readLoopExit( lp, exits, num_exits );

      if ( !success )
        // file does not exist or no valid annotation for this loop: prompt user to choose
        lp->loopexit = promptLoopExit( lp, exits, num_exits );
    }

    free( exits );
  }

  return 0;
}

/*
 * Reads loop annotation (loopbound and is_dowhile).
 */
static int read_loop_annotation()
{
  char proc[100];
  sprintf( proc, "ls %s.lb 2> /dev/null", filename );
  if ( system( proc ) ) {
    // file does not exist yet

    printf( "Loops detected:\n" );
    print_loops();

    printf( "Please provide loop annotations in file %s.lb in the following format:\n", filename );
    printf( "<proc_id> <loop_id> <loopbound> <is_dowhile>\n" );
    printf( "Press 'y' when ready: " );

    char c;
    do {
      scanf( "%c", &c );
    } while ( c != 'y' );

  }

  int pid, lpid, lb, dw;
  int scan_result;

  /* For verification. Gives the number of read in loopbounds per procedure. */
  int *loopbounds_read;
  CALLOC( loopbounds_read, int*, num_procs, sizeof( int ), "loopbounds_read" );

  /* Read the loopbounds. */
  FILE * const fptr = openfile( "lb", "r" );
  while ( scan_result = fscanf( fptr, "%d %d %d %d", &pid, &lpid, &lb, &dw ), scan_result != EOF ) {

    if ( scan_result != 4 ) {
      prerr( "Loopbound file had wrong format!" );
    }

    procedure * const p = procs[pid];
    if ( !p || pid >= num_procs )
      printf( "Invalid procedure id %d\n", pid ), exit( 1 );

    loop * const lp = p->loops[lpid];
    if ( !lp || lpid >= p->num_loops )
      printf( "Invalid loop id [%d] %d\n", pid, lpid ), exit( 1 );

    if ( lb < 0 )
      printf( "Invalid loop bound [%d][%d] %d (must be non-negative)\n", pid, lpid, lb ), exit( 1 );
    lp->loopbound = lb;

    if ( dw != 0 && dw != 1 )
      printf( "Invalid is_dowhile [%d][%d] %d (must be 0/1)\n", pid, lpid, dw ), exit( 1 );
    lp->is_dowhile = dw;

    loopbounds_read[p->pid]++;
  }
  fclose( fptr );

  /* Verify that all loopbounds were given. */
  int i;
  for ( i = 0; i < num_procs; i++ ) {
    if ( loopbounds_read[i] != procs[i]->num_loops ) {
      prerr( "Missing loopbounds in procedure %d: Only %d of %d given.",
          i, loopbounds_read[i], procs[i]->num_loops );
    }
  }
  free( loopbounds_read );

  return 0;
}

/*
 * Removes block with id bbid from loop lp.
 */
static int removeLoopComponent( loop *lp, int bbid )
{
  int i;
  for ( i = 0; i < lp->num_topo; i++ ) {
    if ( lp->topo[i]->bbid == bbid ) {

      // shift
      int j;
      for ( j = i + 1; j < lp->num_topo; j++ )
        lp->topo[j - 1] = lp->topo[j];

      lp->num_topo--;
      // REALLOC( lp->topo, block**, lp->num_topo * sizeof(block*), "loop topo" );
      return 0;
    }
  }
  return -1;
}

/*
 * Adds bb to lp's components, if it does not already exist there.
 * Check indicates if checking of existence should be done.
 */
static int addToLoop( loop *lp, block *bb, char check )
{
  char found = 0;

  if ( check ) {
    int i;
    for ( i = lp->num_topo - 1; !found && i >= 0; i-- )
      if ( lp->topo[i]->bbid == bb->bbid )
        found = 1;
  }
  if ( found )
    return -1;

  lp->num_topo++;
  REALLOC( lp->topo, block**, lp->num_topo * sizeof(block*), "loop topo" );
  lp->topo[lp->num_topo - 1] = bb;
  return 0;
}

/*
 * Checks if an edge from b to out is inside a loop.
 * The edge (b, out) is in a loop if b is reachable from out.
 * If the edge is in loop, both b and out are marked.
 * Returns 1 if a new loop is detected (i.e. b is not already in a loop of the same level),
 * 2 if a dummy block is added to the procedure, 0 otherwise.
 */
static int checkLoop( procedure *p, block *bb, char outid, int level )
{
  block *out = p->bblist[bb->outgoing[(int) outid]];

  // case: out is already known to be in same loop detected in this level
  if ( out->loopid != -1 && p->loops[out->loopid]->level == level &&
       out->loopid == bb->loopid ) {

    // case: out is loophead --> b is loopsink
    if ( out->is_loophead ) {

      loop *lp = p->loops[out->loopid];

      // case: single loopsink so far
      if ( lp->loopsink == NULL ) {
        lp->loopsink = bb;

      // case: multiple loopsinks
      } else {

        // case: existing loopsink is a dummy
        if ( lp->loopsink->startaddr == -1 ) {
          // alter the outgoing of bb which is originally to out, to this dummy
          bb->outgoing[(int) outid] = lp->loopsink->bbid;

        // case: existing loopsink not a dummy
        } else {

          // construct a new block to act as dummy sink
          block *dummy;
          MALLOC( dummy, block*, sizeof(block), "dummy sink" );
          memset( dummy, 0, sizeof(block) );
          createBlock( dummy, p->pid, p->num_bb, -1, lp->loophead->bbid,
                       -1, -1, bb->loopid );

          p->num_bb++;
          REALLOC( p->bblist, block**, p->num_bb * sizeof(block*), "bblist" );
          p->bblist[p->num_bb - 1] = dummy;

          // modify outgoing edge for the original sinks
          p->bblist[bb->bbid]->outgoing[(int) outid] = dummy->bbid;

          if ( lp->loopsink->num_outgoing &&
               lp->loopsink->outgoing[0] == lp->loophead->bbid )
            lp->loopsink->outgoing[0] = dummy->bbid;
          else if ( lp->loopsink->num_outgoing > 1 &&
                    lp->loopsink->outgoing[1] == lp->loophead->bbid )
            lp->loopsink->outgoing[1] = dummy->bbid;

          // add dummy node to loop
          addToLoop( lp, dummy, 0 );
          lp->loopsink = dummy;

          return 2;
        }
      }
    }
    // case: out not loophead --> do nothing

    return 0;
  }

  // case: edge is in a loop
  if ( isReachable( out->bbid, bb->bbid, p->bblist, p->num_bb ) ) {

    // case: new loop with bb as loophead
    if ( bb->loopid == -1 || // not in any existing loop
        p->loops[bb->loopid]->level < level ) { // part of loop in previous level

      loop *lp;
      MALLOC( lp, loop*, sizeof(loop), "loop" );
      createLoop( lp, p->pid, p->num_loops, level, bb );

      p->num_loops++;
      REALLOC( p->loops, loop**, p->num_loops * sizeof(loop*), "loop list" );
      p->loops[p->num_loops - 1] = lp;

      if ( bb->loopid != -1 ) { // nested
        lp->nest = bb->loopid;

        // remove out from the outer loop (loophead should remain there)
        removeLoopComponent( p->loops[bb->loopid], out->bbid );
      }

      // mark bb and out
      bb->is_loophead = 1;
      bb->loopid = lp->lpid;
      out->loopid = lp->lpid;

      // insert out to lp
      addToLoop( lp, out, 1 );

      return 1;
    }

    // case: extending from existing loop
    else {

      if ( out->loopid != -1 ) // nested; remove from outer loop
        removeLoopComponent( p->loops[out->loopid], out->bbid );

      out->loopid = bb->loopid;

      // insert out to loop
      addToLoop( p->loops[bb->loopid], out, 1 );
    }
  }

  return 0;
}

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
int detect_loops()
{
  int i;
  for ( i = 0; i < num_procs; i++ ) {
    procedure *p = procs[i];
    p->num_loops = 0;

    // store backtrack points in DFS traversal
    int *bt;
    int btsize;
    // mark nodes in DFS traversal (0: unvisited, 1: visited, 2: finished)
    char *markfin;

    int level = -1; // nesting level (0 is outmost)
    int lpcount = 0; // counts new loops detected in current level

    // per loop level
    do {

      // remove back-edges of loops detected in previous level
      int k = p->num_loops - lpcount;
      removeBackEdges( &( p->loops[k] ), lpcount );

      // update level, reset lpcount
      level++;
      lpcount = 0;

      // DFS-traverse basic blocks
      CALLOC( markfin, char*, p->num_bb, sizeof(char), "markfin array" );
      bt = NULL;
      btsize = 0;

      // assume entry node is bb 0
      // traversal stops when no more edge to explore and bt is empty
      int id = 0;

      while ( 1 ) {
        block *bb = p->bblist[id];

        // if dummy node, ignore
        if ( bb->startaddr == -1 ) {
          if ( !btsize )
            break;
          id = bt[--btsize];
          continue;
        }

        // if no outgoing edge, mark finished
        if ( bb->num_outgoing == 0 )
          markfin[id] = 2;

        // if finished, go to last backtrack point
        if ( markfin[id] == 2 ) {
          if ( !btsize )
            break;
          id = bt[--btsize];
          continue;
        }

        // mark this visit
        markfin[id]++;

        // if just visited, check first edge
        if ( markfin[id] == 1 ) {

          int res = checkLoop( p, bb, 0, level );
          if ( res == 1 )
            lpcount++;
          else if ( res == 2 ) {
            REALLOC( markfin, char*, p->num_bb * sizeof(char), "markfin array" );
            markfin[p->num_bb - 1] = 0;
          }

          // if branch add to backtrack point, else mark finished
          if ( bb->num_outgoing == 2 ) {
            REALLOC( bt, int*, ++btsize * sizeof(int), "backtrack array" );
            bt[btsize - 1] = id;
          } else
            markfin[id] = 2;

          // if first successor not visited, proceed there; else backtrack
          id = bb->outgoing[0];
          if ( markfin[id] > 0 ) {
            if ( !btsize )
              break;
            id = bt[--btsize];
          }
          continue;
        }

        // if 'returning' branch node, check second edge
        if ( bb->num_outgoing < 2 )
          printf( "Unusual case at [%d] %d\n", bb->pid, bb->bbid ), exit( 1 );

        if ( checkLoop( p, bb, 1, level ) )
          lpcount++;

        // if second successor not visited, proceed there; else backtrack
        id = bb->outgoing[1];
        if ( markfin[id] > 0 ) {
          if ( !btsize )
            break;
          id = bt[--btsize];
        }
      }

      free( markfin );
      free( bt );

      k = p->num_loops - lpcount;
      determineLoopExits( &( p->loops[k] ), lpcount, p->bblist, p->num_bb );

    } while ( lpcount );

    // removeExitEdges( p );
  }

  read_loop_annotation();

  return 0;
}
