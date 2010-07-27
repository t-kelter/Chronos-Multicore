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
#include "analysisILP.h"
#include "infeasible.h"
#include "block.h"
#include "handler.h"
#include "wcrt/cycle_time.h"


static FILE *ilpf;

/*
 * Recursive function.
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 * Searches bblist from index start to end.
 * Used for bblist whose indexing does not correspond with bbid.
 */
int reachableTopo( block *src, int dest, block **bblist, int start, int end, char *visited ) {

  int i, id;

  if( src->bbid == dest )
    return 1;

  visited[src->bbid] = 1;

  for( i = 0; i < src->num_outgoing; i++ ) {
    id = getblock( src->outgoing[i], bblist, start, end );

    if( id != -1 && !visited[src->outgoing[i]] )
      if( reachableTopo( bblist[id], dest, bblist, start, end, visited ))
	return 1;
  }
  return 0;
}


/*
 * Returns 1 if there is a path from src to dest in the CFG, 0 otherwise.
 * Searches bblist from index start to end.
 * Used for bblist whose indexing does not correspond with bbid.
 */
int isReachableTopo( block *src, int dest, block **bblist, int start, int end ) {

  char *visited;  // bit array to mark visited nodes
  int res;

  if( src->bbid == dest )
    return 1;

  visited = (char*) CALLOC( visited, procs[src->pid]->num_bb, sizeof(char), "visited" );
  res = reachableTopo( src, dest, bblist, start, end, visited );

  free( visited );
  return res;
}


int getBlockExecCount( block *bb ) {

  int count;
  loop *lp, *lpn;

  count = 1;
  if( bb->loopid != -1 ) {
    lp = procs[bb->pid]->loops[bb->loopid];
	
    if( bb->is_loophead && !lp->is_dowhile )
      // loophead executed once extra
      count *= (lp->loopbound + 1);
    else
      count *= lp->loopbound;

    // nesting
    lpn = lp;
    while( lpn->nest != -1 ) {
      lpn = procs[bb->pid]->loops[lpn->nest];
      count *= lpn->loopbound;
    }
  }
  return count;
}


int ILPconstDAG( char objtype, void *obj ) {

  int i, j, k, ln, m, cid;

  procedure *p;
  loop  *lp;
  block *bb;

  block **topo;
  int   num_topo;

  int  **incoming;      // incoming edges needed for flow constraints
  char *num_incoming;

  assign *A;
  branch *B, *C;

  char dir;
  int  jump, fall;

  char temp[16];
  char str[1000];


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


  // collect incoming
  num_incoming = (char*) CALLOC( num_incoming, p->num_bb, sizeof(char), "num_incoming" );
  incoming     = (int**) MALLOC( incoming, p->num_bb * sizeof(int*), "incoming" );
  for( i = 0; i < p->num_bb; i++ ) {
    num_incoming[i] = 0;
    incoming[i]     = NULL;
  }
  for( i = 0; i < num_topo; i++ ) {
    bb = topo[i];
    for( j = 0; j < bb->num_outgoing; j++ ) {
      num_incoming[ bb->outgoing[j] ]++;
      incoming[ bb->outgoing[j] ] = (int*)
        REALLOC( incoming[ bb->outgoing[j] ], num_incoming[bb->outgoing[j]] * sizeof(int), "incoming elm" );
      incoming[ bb->outgoing[j] ][ num_incoming[bb->outgoing[j]] - 1 ] = bb->bbid;
    }
  }

  // start constraint collection
  for( i = 0; i < num_topo; i++ ) {
    bb = topo[i];

    // flow constraints

    // equals incoming
    if( num_incoming[bb->bbid] ) {

      fprintf( ilpf, "Y%d", bb->bbid );
      for( j = 0; j < num_incoming[bb->bbid]; j++ )
	fprintf( ilpf, " - Y%d~%d", incoming[bb->bbid][j], bb->bbid );
      fprintf( ilpf, " = 0\n" );
      fflush( ilpf );
    }

    // equals outgoing
    if( bb->num_outgoing ) {

      fprintf( ilpf, "Y%d", bb->bbid );
      for( j = 0; j < bb->num_outgoing; j++ )
	fprintf( ilpf, " - Y%d~%d", bb->bbid, bb->outgoing[j] );
      fprintf( ilpf, " = 0\n" );
      fflush( ilpf );
    }

    // conflicts constraints
    if( infeas ) {

      // BB conflict
      B = branchlist[bb->pid][bb->bbid];
      if( B != NULL ) {

	fall = getblock( bb->outgoing[0], topo, 0, i-1 );
	jump = getblock( bb->outgoing[1], topo, 0, i-1 );

	// special treatment for loophead
	if( bb->is_loophead ) {
	  if( objtype == LOOP && bb->loopid == lp->lpid )
	    // own loophead: consider only iteration edge
	    fall = -1;
	  else
	    // loop treated as black box: consider only exit edge
	    jump = -1;
	}

	// printf( "check BB conflict: " ); printBranch( B, 0 );

	for( k = 0; k < B->num_conflicts; k++ ) {
	  C = B->conflicts[k];

	  cid = getblock( C->bb->bbid, topo, 0, i-1 );
	  if( cid == -1 )
	    continue;

	  // printf( "against: " ); printBranch( C, 0 );

	  // check conflict when B jumps
	  dir = B->conflictdir_jump[k];
	  if( jump != -1 && dir != -1 ) {

	    // check cancellation of effect by assignment
	    str[0]  = '\0';
	    temp[0] = '\0';

	    for( ln = i - 1; ln >= cid; ln-- ) {

	      if( isReachableTopo( topo[jump], topo[ln]->bbid, topo, ln, i ) &&
		  isReachableTopo( topo[ln], C->bb->bbid, topo, cid, ln )) {

		for( m = 0; m < num_assign[bb->pid][topo[ln]->bbid]; m++ ) {
		  
		  if( strstr( B->deri_tree, assignlist[bb->pid][topo[ln]->bbid][m]->deri_tree ) != NULL ) {
		    // printf( "cancel: " ); printAssign( assignlist[bb->pid][topo[ln]->bbid][m], m, 0 );
		    
		    sprintf( temp, " - Y%d", topo[ln]->bbid );
		    if( !strstr( str, temp ))
		      strcat( str, temp );
		  }
		}
	      }
	    }
	    // printf( "dir:%d jc:%d nc:%d\n", dir, C->jump_cond, neg( C->jump_cond ));

	    if( dir == C->jump_cond )
	      fprintf( ilpf, "Y%d~%d + Y%d~%d%s <= 1\n",
		       bb->bbid, bb->outgoing[1], C->bb->bbid, C->bb->outgoing[1], str );

	    else if( dir == neg( C->jump_cond ))
	      fprintf( ilpf, "Y%d~%d + Y%d~%d%s <= 1\n",
		       bb->bbid, bb->outgoing[1], C->bb->bbid, C->bb->outgoing[0], str );
	  }

	  // check conflict when B falls
	  dir = B->conflictdir_fall[k];
	  if( fall != -1 && dir != -1 ) {

	    // check cancellation of effect by assignment
	    str[0]  = '\0';
	    temp[0] = '\0';

	    for( ln = i - 1; ln >= cid; ln-- ) {

	      if( isReachableTopo( topo[fall], topo[ln]->bbid, topo, ln, i ) &&
		  isReachableTopo( topo[ln], C->bb->bbid, topo, cid, ln )) {

		for( m = 0; m < num_assign[bb->pid][topo[ln]->bbid]; m++ ) {

		  if( strstr( B->deri_tree, assignlist[bb->pid][topo[ln]->bbid][m]->deri_tree ) != NULL ) {
		    // printf( "cancel: " ); printAssign( assignlist[bb->pid][topo[ln]->bbid][m], m, 0 );
		    
		    sprintf( temp, " - Y%d", topo[ln]->bbid );
		    if( !strstr( str, temp ))
		      strcat( str, temp );
		  }
		}
	      }
	    }
	    // printf( "dir:%d jc:%d nc:%d\n", dir, C->jump_cond, neg( C->jump_cond ));

	    if( dir == C->jump_cond )
	      fprintf( ilpf, "Y%d~%d + Y%d~%d%s <= 1\n",
		       bb->bbid, bb->outgoing[0], C->bb->bbid, C->bb->outgoing[1], str );

	    else if( dir == neg( C->jump_cond ))
	      fprintf( ilpf, "Y%d~%d + Y%d~%d%s <= 1\n",
		       bb->bbid, bb->outgoing[0], C->bb->bbid, C->bb->outgoing[0], str );
	  }
	} // end for BB conflicts
      }

      // BA conflict
      for( j = 0; j < num_assign[bb->pid][bb->bbid]; j++ ) {
	A = assignlist[bb->pid][bb->bbid][j];

	// printf( "check BA conflict: " ); printAssign( A, j, 0 );

	for( k = 0; k < A->num_conflicts; k++ ) {
	  C = A->conflicts[k];

	  cid = getblock( C->bb->bbid, topo, 0, i );
	  if( cid == -1 )
	    continue;

	  // printf( "against: " ); printBranch( C, 0 );

	  // check cancellation of effect by assignment
	  str[0]  = '\0';
	  temp[0] = '\0';

	  for( ln = i - 1; ln >= cid; ln-- ) {

	    if( isReachableTopo( bb, topo[ln]->bbid, topo, ln, i ) &&
		isReachableTopo( topo[ln], C->bb->bbid, topo, cid, ln )) {

	      for( m = 0; m < num_assign[bb->pid][topo[ln]->bbid]; m++ ) {

		if( strstr( A->deri_tree, assignlist[bb->pid][topo[ln]->bbid][m]->deri_tree ) != NULL ) {
		  // printf( "cancel: " ); printAssign( assignlist[bb->pid][topo[ln]->bbid][m], m, 0 );

		  sprintf( temp, " - Y%d", topo[ln]->bbid );
		  if( !strstr( str, temp ))
		    strcat( str, temp );
		}
	      }
	    }
	  }
	  if( A->conflictdir[k] == C->jump_cond )
	    fprintf( ilpf, "Y%d + Y%d~%d%s <= 1\n",
		     bb->bbid, C->bb->bbid, C->bb->outgoing[1], str );

	  else if( A->conflictdir[k] == neg( C->jump_cond ))
	    fprintf( ilpf, "Y%d + Y%d~%d%s <= 1\n",
		     bb->bbid, C->bb->bbid, C->bb->outgoing[0], str );

	} // end for BA conflicts
      } // end for assignments
    } // end if( infeas )
  } // end for i
  
  // free up memory usage
  free( num_incoming );
  if( incoming != NULL )
    for( i = 0; i < p->num_bb; i++ )
      free( incoming[i] );
  free( incoming );

  return 0;
}


int ILPconstProc( procedure *p ) {

  int  i;

  // collect constraints for each loop from the inmost (reverse order from detection)
  for( i = p->num_loops - 1; i >= 0; i-- )
    ILPconstDAG( LOOP, p->loops[i] );

  // collect constraints for procedure
  ILPconstDAG( PROC, p );

  return 0;
}


int analysis_ilp()
{
  DSTART( "analysis_ilp" );

  int i, j, k, m;
  
  char proc[100];

  procedure *p;
  block *bb;
  loop  *lp;

  uint   cost;
  double wcet;
  int    count;

  char firstterm;

  // time measurement
  double t;
  double total_t_frm;
  double total_t_sol;

  total_t_frm = 0;
  total_t_sol = 0;

  // start timing for formulation
  cycle_time(0);

  // analyse procedures separately, 
  // starting from the leaves of the procedure call graph
  for( i = 0; i < num_procs; i++ ) {
    p = procs[ proc_cg[i] ];

    sprintf( proc, "ailp%d", p->pid );
    ilpf = openfile( proc, "w" );

    fprintf( ilpf, "enter Q\n" );

    // Objective function: sum of costs of all basic blocks in topo arrays.
    // Note: cannot use directly from procedure bblist,
    // because there may be extra blocks such as unused loop exits.

    fprintf( ilpf, "maximize\n" );//, p->pid );

    firstterm = 1;

    for( j = p->num_topo-1; j >= 0; j-- ) {
      bb = p->topo[j];

      if( bb->is_loophead || bb->startaddr == -1 ) // black box or dummy
	continue;

      cost = bb->cost;
      if( bb->callpid != -1 )
        cost += *procs[ bb->callpid ]->wcet;

      if( !firstterm )
      	fprintf( ilpf, " + " );

      fprintf( ilpf, "%dY%d", cost, bb->bbid );
      firstterm = 0;
    }

    for( m = 0; m < p->num_loops; m++ ) {
      lp = p->loops[m];

      for( j = lp->num_topo-1; j >= 0; j-- ) {
	bb = lp->topo[j];
      
	if( ( bb->is_loophead && bb->loopid != lp->lpid ) || 
	    ( bb->startaddr == -1 ) ) // black box or dummy
	  continue;
      
	cost = bb->cost;
	if( bb->callpid != -1 )
	  cost += *procs[ bb->callpid ]->wcet;
	
	cost *= getBlockExecCount( bb );

	if( !firstterm )
	  fprintf( ilpf, " + " );
	fprintf( ilpf, "%dY%d", cost, bb->bbid );
	firstterm = 0;
      }
    }
    fprintf( ilpf, "\n" );

    // constraints
    fprintf( ilpf, "st\n" );
    ILPconstProc( p );

    // entry and exit
    fprintf( ilpf, "Y%d = 1\n", p->topo[0]->bbid );
    fprintf( ilpf, "Y%d = 1\n", p->topo[p->num_topo-1]->bbid );
    
    fprintf( ilpf, "end\n" );
    
    // set binary type for Y-variables
    fprintf( ilpf, "change problem milp\n" );
    
    for( j = p->num_topo - 1; j >= 0; j-- ) {
      bb = p->topo[j];

      // blocks: skip nested loophead (else duplicated)
      if( !bb->is_loophead )
	fprintf( ilpf, "change type Y%d B\n", bb->bbid );

      // edges
      for( k = 0; k < bb->num_outgoing; k++ )
	fprintf( ilpf, "change type Y%d~%d B\n", bb->bbid, bb->outgoing[k] );
    }

    for( m = 0; m < p->num_loops; m++ ) {
      lp = p->loops[m];
      
      for( j = lp->num_topo - 1; j >= 0; j-- ) {
	bb = lp->topo[j];
	
	// blocks: skip nested loophead (else duplicated)
	if( !bb->is_loophead || bb->loopid == lp->lpid )
	  fprintf( ilpf, "change type Y%d B\n", bb->bbid );

	// edges
	for( k = 0; k < bb->num_outgoing; k++ )
	  fprintf( ilpf, "change type Y%d~%d B\n", bb->bbid, bb->outgoing[k] );
      }    
    }

    // optimize
    fprintf( ilpf, "optimize\n" );
    fprintf( ilpf, "display solution variables -\n" );
    fprintf( ilpf, "quit\n" );

    fclose( ilpf );

    // stop timing
    t = cycle_time(1);
    total_t_frm += t;

    DOUT( "Time taken (analysis-formulation): %f ms (%f Mcycles)\n", t/CYCLES_PER_MSEC, t/1000000 );

    // solve ilp
    sprintf( proc, "tools/cplex < %s.ailp%d > %s.ais%d", filename, p->pid, filename, p->pid );

    // start timing for solution
    cycle_time(0);

    system( proc );

    // stop timing for solution
    t = cycle_time(1);
    total_t_sol += t;
    DOUT( "Time taken (analysis-solution): %f ms (%f Mcycles)\n", t/CYCLES_PER_MSEC, t/1000000 );

    // read solution: wcet
    sprintf( proc, "grep Objective %s.ais%d | grep solution | awk '{print $NF}' > %s.wcet", 
	     filename, p->pid, filename );
    system( proc );

    wcet = 0;
    ilpf = openfile( "wcet", "r" );
    fscanf( ilpf, "%le", &wcet );
    fclose( ilpf );

    *p->wcet = (ull)wcet;
    DOUT( "objective value: %d\n", p->wcet );

    // extract blocks with Y-value of 1
    sprintf( proc, "grep Y %s.ais%d | grep \"1\\.00\" | awk '{print $1}' | sed '/\\~/d' > %s.bp%d", 
	     filename, p->pid, filename, p->pid );
    system( proc );

    free( p->wpath );
    p->wpath = (char*) MALLOC( p->wpath, sizeof(char), "proc wpath" );
    p->wpath[0] = '\0';

    // Format of file .bp:
    // Y[pid]_[bbid]
    sprintf( proc, "bp%d", p->pid );
    ilpf = openfile( proc, "r" );

    firstterm = 1;
    while( fscanf( ilpf, "Y%d\n", &j ) != EOF ) {

      // wpath
      if( firstterm ) {
	sprintf( proc, "%d", j );
	firstterm = 0;
      }
      else
	sprintf( proc, "-%d", j );

      p->wpath = (char*) REALLOC( p->wpath, (strlen(p->wpath) + strlen(proc) + 1) * sizeof(char), "proc wpath" );
      strcat( p->wpath, proc );

      // wpvar
      bb = p->bblist[j];

      // if dummy block, ignore
      if( bb->startaddr != -1 ) {

	count = getBlockExecCount( bb );
      }
    }

    if( ilpf )
      fclose( ilpf );

  } // end for procs

  DOUT( "Time taken (analysis-formulation): %f ms\n", total_t_frm/CYCLES_PER_MSEC );
  DOUT( "Time taken (analysis-solution): %f ms\n", total_t_sol/CYCLES_PER_MSEC );

  //system( "rm -f cplex.log" );

  DRETURN( 0 );
}
