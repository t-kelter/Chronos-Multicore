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
#include "analysisEnum.h"
#include "DAG_WCET.h"
#include "handler.h"
#include "block.h"


char enum_in_seq_bef( int bbid, int target, ushort *bb_seq, ushort bb_len ) {

  int i;
  // bb_seq has reverse order
  for( i = bb_len - 1; i >= 0 && bb_seq[i] != target; i-- )
    if( bb_seq[i] == bbid )
      return 1;
  return 0;
}


int enum_effectCancelled( branch *br, assign *assg, ushort *bb_seq, ushort len, block **bblist, int num_bb ) {

  int i, ln, id;

  for( i = 0; i < num_bb; i++ ) {
    id = bblist[i]->bbid;

    if( id == br->bb->bbid || enum_in_seq_bef( id, br->bb->bbid, bb_seq, len )) {
	    
      for( ln = 0; ln < num_assign[br->bb->pid][id]; ln++ ) {
	if( assg != NULL && assignlist[br->bb->pid][id][ln] == assg )
	  continue;
	if( strstr( br->deri_tree, assignlist[br->bb->pid][id][ln]->deri_tree ) != NULL )
	  return id;
      }
    }
  }
  return -1;
}


int enum_findBranch( branch *br, ushort *bb_seq, ushort len ) {

  int i;
  for( i = 0; i < len; i++ )
    if( bb_seq[i] == br->bb->bbid )
      return i;
  return -1;
}


char enum_BBconflictInPath( branch *bru, char direction, block *bv, 
			    ushort *bb_seq, ushort len, block **bblist, int num_bb ) {

  int  cf, id, idx;
  char res;
  branch *br;

  // check for each branch conflicting with bru, whether it occurs in this path
  for( cf = 0; cf < bru->num_conflicts; cf++ ) {
    br = bru->conflicts[cf];

    id = enum_findBranch( br, bb_seq, len );
    if( id == -1 )
      continue;

    idx = getblock( bb_seq[id-1], bblist, 0, num_bb-1 );
    if( idx == -1 )
      continue;

    // direction taken by br to its successor in bb_seq
    res = detectDirection( br, bblist[idx] );
    if( ( direction == bru->jump_cond && bru->conflictdir_jump[cf] == res ) ||
        ( direction == neg( bru->jump_cond ) && bru->conflictdir_fall[cf] == res ) ) {

      // check cancellation of effect by assignment
      id = enum_effectCancelled( br, NULL, bb_seq, len, bblist, num_bb );

      if( id == -1 ) {
        return 1;
      }
    }
  }
  return 0;
}


char enum_BAconflictInPath( block *bu, ushort *bb_seq, ushort len, block **bblist, int num_bb ) {

  int  cf, ln, id, idx;
  char res;
  assign *assg;
  branch *br;
	    
  for( ln = 0; ln < num_assign[bu->pid][bu->bbid]; ln++ ) {
    assg = assignlist[bu->pid][bu->bbid][ln];
    
    for( cf = 0; cf < assg->num_conflicts; cf++ ) {
      br = assg->conflicts[cf];
      
      id = enum_findBranch( br, bb_seq, len );
      if( id == -1 )
	continue;

      idx = getblock( bb_seq[id-1], bblist, 0, num_bb-1 );
      if( idx == -1 )
	continue;
      
      // direction taken by br to its successor in bb_seq
      res = detectDirection( br, bblist[idx] );
      if( assg->conflictdir[cf] == res ) {

	// check cancellation of effect by assignment
	id = enum_effectCancelled( br, assg, bb_seq, len, bblist, num_bb );

	if( id == -1 ) {
	  return 1;
	}
      }
    }
  }
  return 0;
}


int analyseEnumDAG( char objtype, void *obj ) {

  procedure *p;
  loop  *lp;
  block *bb;

  block **topo;
  int num_topo;

  int  i, j, k, m, id;
  int  num, len;
  char dir, feasible;

  branch *br;

  ull  *pathcounts;
  char *num_incoming;

  FILE *fptr;


  if( objtype == PROC ) {
    p            = (procedure*) obj;
    topo         = p->topo;
    num_topo     = p->num_topo;
  }
  else if( objtype == LOOP ) {
    lp           = (loop*) obj;
    p            = procs[ lp->pid ];
    topo         = lp->topo;
    num_topo     = lp->num_topo;
  } else {
    fprintf( stderr, "Invalid objtype passed to analysisDAG: %d\n", objtype );
    exit(1);
  }

   CALLOC( pathcounts, ull*, p->num_bb, sizeof(ull), "pathcounts" );

  if( infeas ) {
    // trace number of incoming blocks, so we can free those that are no longer needed
     CALLOC( num_incoming, char*, p->num_bb, sizeof(char), "num_incoming" );
    for( i = 0; i < p->num_bb; i++ )
      num_incoming[i] = 0;

    for( i = 0; i < num_topo; i++ ) {
      bb = topo[i];
      for( j = 0; j < bb->num_outgoing; j++ )
	num_incoming[ bb->outgoing[j] ]++;
    }
  }

  for( i = 0; i < num_topo; i++ ) {
    bb = topo[i];

    if( infeas ) {
      enum_pathlen[p->pid][bb->bbid]  = NULL;
      enum_pathlist[p->pid][bb->bbid] = NULL;
    }

    if( !bb->num_outgoing ) {
      // sink
      pathcounts[ bb->bbid ] = 1;

      if( infeas ) {
	 MALLOC( enum_pathlen[p->pid][bb->bbid], ushort*, sizeof(ushort), "enum_pathlen[p][b]" );
	enum_pathlen[p->pid][bb->bbid][0] = 1;

	 MALLOC( enum_pathlist[p->pid][bb->bbid], ushort**, sizeof(ushort*), "enum_pathlist[p][b]" );
	 MALLOC( enum_pathlist[p->pid][bb->bbid][0], ushort*, sizeof(ushort), "enum_pathlist[p][b][0]" );
	enum_pathlist[p->pid][bb->bbid][0][0] = bb->bbid;
      }
      continue;
    }

    for( j = 0; j < bb->num_outgoing; j++ ) {

      if( !infeas ) {
	pathcounts[ bb->bbid ] += pathcounts[ bb->outgoing[j] ];
	continue;
      }
      id = getblock( bb->outgoing[j], topo, 0, i-1 );

      for( k = 0; k < pathcounts[ bb->outgoing[j] ]; k++ ) {
	feasible = 1;

	// check for BB conflict
	br = branchlist[p->pid][bb->bbid];

	if( br != NULL && id != -1 ) {
	  if( j == 1 )
	    dir = br->jump_cond;
	  else
	    dir = neg( br->jump_cond );
	  
	  if( enum_BBconflictInPath( br, dir, topo[id], enum_pathlist[p->pid][ bb->outgoing[j] ][k],
				     enum_pathlen[p->pid][ bb->outgoing[j] ][k], topo, num_topo ))
	    feasible = 0;
	}
	if( feasible ) {
	  // check for BA conflict
	  if( enum_BAconflictInPath( bb, enum_pathlist[p->pid][ bb->outgoing[j] ][k],
				     enum_pathlen[p->pid][ bb->outgoing[j] ][k], topo, num_topo ))
	    feasible = 0;
	}
      
	if( feasible ) {
	  pathcounts[ bb->bbid ]++;
	
	  num = pathcounts[ bb->bbid ];
	  len = enum_pathlen[p->pid][ bb->outgoing[j] ][k] + 1;
	
	  // extend
	   REALLOC( enum_pathlen[p->pid][bb->bbid], ushort*, num * sizeof(ushort), "enum_pathlist[p][b]" );
	  enum_pathlen[p->pid][bb->bbid][num-1] = len;
	
	   REALLOC( enum_pathlist[p->pid][bb->bbid], ushort**, num * sizeof(ushort*), "enum_pathlist[p][b]" );
	   MALLOC( enum_pathlist[p->pid][bb->bbid][num-1], ushort*, len * sizeof(ushort), "enum_pathlist[p][b][n]" );
	
	  // copy from child's bb sequence
	  for( m = 0; m < len - 1; m++ )
	    enum_pathlist[p->pid][bb->bbid][num-1][m] = enum_pathlist[p->pid][ bb->outgoing[j] ][k][m];
	
	  // add bb
	  enum_pathlist[p->pid][bb->bbid][num-1][len-1] = bb->bbid;
	}
      } // end for child's path

      num_incoming[ bb->outgoing[j] ]--;
      if( !num_incoming[ bb->outgoing[j] ] ) {
	
	// free memory usage
	for( m = 0; m < pathcounts[ bb->outgoing[j] ]; m++ )
	  free( enum_pathlist[p->pid][ bb->outgoing[j] ][m] );
	
	free( enum_pathlist[p->pid][ bb->outgoing[j] ] );
	free( enum_pathlen[p->pid][ bb->outgoing[j] ] );
      }
    } // end for bb's children

    if( do_inline ) {

      // nested loop
      if( bb->is_loophead && ( objtype != LOOP || bb->loopid != lp->lpid ))
	pathcounts[ bb->bbid ] *= enum_paths_loop[ bb->loopid ];
      
      // procedure call
      if( bb->callpid != -1 )
	pathcounts[ bb->bbid ] *= enum_paths_proc[ bb->callpid ];
    }
    printf( "pathcount at %d-%d: %Lu\n", p->pid, bb->bbid, pathcounts[ bb->bbid ] );
  }

  if( objtype == PROC )
    enum_paths_proc[ p->pid ] = pathcounts[ topo[num_topo-1]->bbid ];

  else if( objtype == LOOP )
    enum_paths_loop[ lp->lpid ] = pathcounts[ topo[num_topo-1]->bbid ];

  if( infeas ) {
    // print paths to file
    if( p->pid == 1 ) {
      fptr = openfile( "paths", "w" );
      for( i = 0; i < pathcounts[ topo[num_topo-1]->bbid ]; i++ ) {
	for( j = enum_pathlen[p->pid][ topo[num_topo-1]->bbid ][i] - 1; j >= 0; j-- )
	  fprintf( fptr, "%d ", enum_pathlist[p->pid][ topo[num_topo-1]->bbid ][i][j] );
	fprintf( fptr, "\n" );
      }
    }

    // free memory usage: only the source has not been freed
    for( j = 0; j < pathcounts[ topo[num_topo-1]->bbid ]; j++ )
      free( enum_pathlist[p->pid][ topo[num_topo-1]->bbid ][j] );

    free( enum_pathlist[p->pid][ topo[num_topo-1]->bbid ] );
    free( enum_pathlen[p->pid][ topo[num_topo-1]->bbid ] );
  }

  free( pathcounts );

  return 0;
}


int analyseEnumProc( procedure *p ) {

  int  i;
  loop *lp;

   CALLOC( enum_paths_loop, ull*, p->num_loops, sizeof(ull), "enum_paths_loop" );

  if( infeas ) {
     MALLOC( enum_pathlist[p->pid], ushort***, p->num_bb * sizeof(ushort**), "enum_pathlist[p]" );
     MALLOC( enum_pathlen[p->pid], ushort**, p->num_bb * sizeof(ushort*), "enum_pathlen[p]" );
  }

  // analyse each loop from the inmost (reverse order from detection)
  for( i = p->num_loops - 1; i >= 0; i-- ) {
    lp = p->loops[i];
    analyseEnumDAG( LOOP, lp );
    printf( "#paths in loop %d-%d: %Lu\n", p->pid, lp->lpid, enum_paths_loop[lp->lpid] );
  }

  // analyse procedure
  analyseEnumDAG( PROC, p );

  free( enum_paths_loop );
  
  if( infeas ) {
    free( enum_pathlist[p->pid] );
    free( enum_pathlen[p->pid] );
  }

  return 0;
}


int analysis_enum()
{
  DSTART( "analysis_enum" );

  int i;

   CALLOC( enum_paths_proc, ull*, num_procs, sizeof(ull), "enum_paths_proc" );

  if( infeas ) {
     MALLOC( enum_pathlist, ushort****, num_procs * sizeof(ushort***), "enum_pathlist" );
     MALLOC( enum_pathlen, ushort***, num_procs * sizeof(ushort**), "enum_pathlen" );
  }

  // analyse each procedure in reverse topological order of call graph
  for( i = 0; i < num_procs; i++ ) {
    analyseEnumProc( procs[ proc_cg[i] ] );
    DOUT( "#paths in proc %d: %Lu\n", proc_cg[i], enum_paths_proc[proc_cg[i]] );
  }

  if( do_inline )
    DOUT( "\nTotal number of paths: %Lu\n", enum_paths_proc[main_id] );

  free( enum_paths_proc );

  if( infeas ) {
    free( enum_pathlist );
    free( enum_pathlen );
  }

  DRETURN( 0 );
}
