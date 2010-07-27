// Include standard library headers
#include <stdlib.h>
#include <string.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "DAG_WCET.h"
#include "block.h"
#include "dump.h"
#include "handler.h"
#include "path.h"



int detectDirection( branch *bru, block *bv ) {

  if( bru->bb->outgoing[1] == bv->bbid )
    return bru->jump_cond;
  else
    return neg( bru->jump_cond );

  /*
  char  *jumpdest;
  instr *insn;

  // If bru's jump dest. == start address of bv, 
  // then direction( bru, bv ) == bru's jump condition, otherwise the negation.

  insn = bru->bb->instrlist[ bru->bb->num_instr - 1 ];
  jumpdest = getJumpDest( insn );

  // printf( "dest: %s jumpdest: %s jumpdir: %d\n", bv->instrlist[0]->addr, jumpdest, jumpdir );

  if( jumpdest == NULL )
     return NA;

  if( strcmp( bv->instrlist[0]->addr, jumpdest ) == 0 )
    return bru->jump_cond;
  else
    return neg( bru->jump_cond );
  */
}


/*
 * Restores the count of incoming conflicts for each branch.
 */
int restoreIncomingConflicts() {

  int i, j;
  procedure *p;
  branch *br;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];

    for( j = 0; j < p->num_bb; j++ ) {
      br = branchlist[i][j];
      if( br != NULL )
	br->num_active_incfs = br->num_incfs;
    }
  }
  return 0;
}


/*
 * Returns 1 if bbid appears in bb_seq before target, 0 otherwise.
 */
char in_seq_bef( int bbid, int target, int *bb_seq, int bb_len ) {

  int i;
  // bb_seq has reverse order
  for( i = bb_len - 1; i >= 0 && bb_seq[i] != target; i-- )
    if( bb_seq[i] == bbid )
      return 1;
  return 0;
}


int effectCancelled( branch *br, assign *assg, path *pv, block **bblist, int num_bb ) {

  int i, ln, id;

  for( i = 0; i < num_bb; i++ ) {
    id = bblist[i]->bbid;

    if( id == br->bb->bbid || in_seq_bef( id, br->bb->bbid, pv->bb_seq, pv->bb_len )) {
	    
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


char BBconflictInPath( branch *bru, char direction, block *bv, path *pv, block **bblist, int num_bb )
{
  int  cf, id;
  char res;
  branch *br;

  // check for each branch conflicting with bru, whether it occurs in this path
  for( cf = 0; cf < bru->num_conflicts; cf++ ) {
    br = bru->conflicts[cf];

    id = findBranch( br, pv->branch_eff, pv->branch_len );
    if( id == -1 )
      continue;

    res = pv->branch_dir[id];
    if( ( direction == bru->jump_cond && bru->conflictdir_jump[cf] == res ) ||
        ( direction == neg( bru->jump_cond ) && bru->conflictdir_fall[cf] == res ) ) {

      // check cancellation of effect by assignment
      id = effectCancelled( br, NULL, pv, bblist, num_bb );

      if( id == -1 ) {
        return 1;
      }
    }
  }
  return 0;
}


char BAconflictInPath( block *bu, block *bv, path *pv, block **bblist, int num_bb ) {

  int cf, ln, id;
  char res;
  assign *assg;
  branch *br;
	    
  for( ln = 0; ln < num_assign[bu->pid][bu->bbid]; ln++ ) {
    assg = assignlist[bu->pid][bu->bbid][ln];
    
    for( cf = 0; cf < assg->num_conflicts; cf++ ) {
      br = assg->conflicts[cf];
      
      id = findBranch( br, pv->branch_eff, pv->branch_len );
      if( id == -1 )
	continue;
      
      res = pv->branch_dir[id];
      if( assg->conflictdir[cf] == res ) {

	// check cancellation of effect by assignment
	id = effectCancelled( br, assg, pv, bblist, num_bb );

	if( id == -1 ) {
	  return 1;
	}
      }
    }
  }
  return 0;
}


/*
 * Returns 1 if bb contains an assignment to a component of addr
 * (thus cancelling any effect before it), returns 0 otherwise.
 */
char assignsTo( block *bb, char *addr ) {

  int i;
  for( i = 0; i < num_assign[bb->pid][bb->bbid]; i++ )
    if( strstr( addr, assignlist[bb->pid][bb->bbid][i]->deri_tree ) != NULL )
      return 1;
  return 0;
}


/*
 * Checks incoming conflicts of br, returns 1 if there exists 
 * incoming conflict that clashes with direction dir.
 */
char hasIncomingConflict( branch *br, char dir, block **bblist, int start, int num_bb ) {

  int i, j, id, brid, pid;
  branch *bc;
  assign *assg;

  if( !br->num_active_incfs )
    return 0;

  pid = br->bb->pid;

  for( i = start; i < num_bb; i++ ) {
    id = bblist[i]->bbid;

    if( br->in_conflict[id] ) {

      // if conflict is from branch
      bc = branchlist[pid][id];
      if( bc != NULL ) {
	brid = findBranch( br, bc->conflicts, bc->num_conflicts );
	if( brid != -1 && ( bc->conflictdir_jump[brid] == dir || bc->conflictdir_fall[brid] == dir ))
	  return 1;
      }

      // if conflict is from assign
      for( j = 0; j < num_assign[pid][id]; j++ ) {
	assg = assignlist[pid][id][j];
	brid = findBranch( br, assg->conflicts, assg->num_conflicts );
	if( brid != -1 && assg->conflictdir[brid] == dir )
	  return 1;
      }
    }
  }
  return 0;
}


/*
 * Traverse CFG in reverse topological order (already given in bblist)
 * to collect cost, eliminating infeasible paths.
 */
int traverse( int pid, block **bblist, int num_bb, int *in_degree, uint *cost )
{
  DSTART( "traverse" );

  int  i, j, k, id, pt;
  char direction, extend;

  path   *pu, *pv;
  block  *bu, *bv;
  branch *bru;

  for( i = 0; i < num_bb; i++ ) {
    bu  = bblist[i];
    bru = branchlist[pid][bu->bbid];

    // printf( "Node %d cost %d: \n", bu->bbid, cost[bu->bbid] ); fflush( stdout );
    // printBlock( bu );

    if( !bu->num_outgoing ) {  // bu is a sink
      // cost(bu) = { sum(bu) | sum(bu) is the sum of costs of each instruction in bu };

      pu = (path*) MALLOC( pu, sizeof(path), "path" );
      pu->cost       = cost[bu->bbid];
      pu->bb_len     = 1;
      pu->bb_seq     = (int*) MALLOC( pu->bb_seq, sizeof(int), "path bb_seq" );
      pu->bb_seq[0]  = bu->bbid;
      pu->branch_len = 0;
      pu->branch_eff = NULL;
      pu->branch_dir = NULL;

      num_paths[bu->bbid]++;
      pathlist[bu->bbid] = (path**)
	REALLOC( pathlist[bu->bbid], num_paths[bu->bbid] * sizeof(path*), "pathlist elm" );
      pathlist[bu->bbid][ num_paths[bu->bbid]-1 ] = pu;
      continue;
    }

    // Step 1: Compute the WCET paths of each branch
    for( j = 0; j < bu->num_outgoing; j++ ) {

      id = getblock( bu->outgoing[j], bblist, 0, i-1 );
      if( id == -1 )
        prerr( "Block %d-%d not found.\n", pid, bu->outgoing[j] );
    
      bv = bblist[id];
      // printf( "out: " ); printBlock( bv );

      for( pt = 0; pt < num_paths[bv->bbid]; pt++ ) {
	pv = pathlist[bv->bbid][pt];

	// branches with potential conflict
	if( bru != NULL ) {

	  direction = detectDirection( bru, bv );
	  // printf( "%d:%d->%d: dirn = %d\n", pid, bu->bbid, bv->bbid, direction );

	  /*
	   * temporary disable conflict detection 
	   *
	  // test BB conflicts
	  if( BBconflictInPath( bru, direction, bv, pv, bblist, num_bb ))
	    continue;
	  */
	}

	/*
	 * temporary disable conflict detection 
	 *
	// test BA conflicts
	if( BAconflictInPath( bu, bv, pv, bblist, num_bb ))
	  continue;
	*/

	// else, include this path for bu

	pu = (path*) MALLOC( pu, sizeof(path), "path" );
	pu->bb_len = pv->bb_len + 1;

	pu->cost = pv->cost + cost[bu->bbid];

	// extra cost if bu-->bv is a region transition
  //int rid;
	//if( regionmode ) {

	//  if( bu->callpid != -1 )
	//    rid = procs[bu->callpid]->bblist[ procs[bu->callpid]->num_bb - 1 ]->regid;

	//  if( bu->callpid == -1 || rid == -1 ) {
	//    if( bu->regid != -1 && bv->regid != -1 && bu->regid != bv->regid ) {
	//      printf( "region transition %d-%d(%d) --> %d-%d(%d) cost: %u\n",
	//	      bu->pid, bu->bbid, bu->regid, bv->pid, bv->bbid, bv->regid, regioncost[bv->regid] );
	//      fflush( stdout );
	//      pu->cost += regioncost[bv->regid];
	//    }
	//  }
	//  // region transition due to procedure call at end of bu
	//  else {
	//    if( rid != -1 && bv->regid != -1 && rid != bv->regid ) {
	//      printf( "region transition %d-%d(%d) procedure return %d(%d) --> %d-%d(%d) cost: %u\n",
	//	      bu->pid, bu->bbid, bu->regid, bu->callpid, rid,
	//	      bv->pid, bv->bbid, bv->regid, regioncost[bv->regid] ); fflush( stdout );
	//      pu->cost += regioncost[bv->regid];
	//    }
	//  }

	//} // end if( regionmode )

	extend = 0;
	if( bru != NULL && hasIncomingConflict( bru, direction, bblist, i+1, num_bb ))
	  extend = 1;

	pu->branch_len = pv->branch_len;
	if( extend )
	  pu->branch_len++;
	
	pu->bb_seq = (int*) MALLOC( pu->bb_seq, pu->bb_len * sizeof(int), "path bb_seq" );
	pu->branch_eff = (branch**)
	  MALLOC( pu->branch_eff, pu->branch_len * sizeof(branch*), "path branch_eff" );
	pu->branch_dir = (char*)
	  MALLOC( pu->branch_dir, pu->branch_len * sizeof(char), "path branch_dir" );

	copySeq( pu, pv );
	pu->bb_seq[ pu->bb_len - 1 ] = bu->bbid;

	if( extend )
	  sortedInsertBranch( pu, bru, direction );

	num_paths[bu->bbid]++;
	pathlist[bu->bbid] = (path**)
	  REALLOC( pathlist[bu->bbid], num_paths[bu->bbid] * sizeof(path*), "pathlist elm" );
	pathlist[bu->bbid][ num_paths[bu->bbid]-1 ] = pu;

      } // end for paths of bv

    } // end for bu's children

    if( num_paths[bu->bbid] <= 0 )
      prerr( "\nNo feasible path at %d-%d!\n\n", pid, bu->bbid );


    // Step 2: Consolidate

    // if( edges e1, ..., en in subgraph(bu) are not conflicting with some predecessors )
    //   combine the two paths in cost(bu) if they only differ in term ei
    // Note that for each node bu, we keep a list of nodes conflicting with bu and can reach bu.
  
    // Step 2.1 Update incoming conflicts list: clear bu since it is already visited
    for( j = 0; j < procs[pid]->num_bb; j++ ) {
      bru = branchlist[pid][j];
      if( bru != NULL && bru->in_conflict[bu->bbid] )
	bru->num_active_incfs--;
    }
  
    // Step 2.2 Merge paths
    for( pt = 0; pt < num_paths[bu->bbid]; pt++ ) {
      pu = pathlist[bu->bbid][pt];

      // check each branch in this path for expired conflicts
      k = 0;
      while( k < pu->branch_len ) {

	// remove if no more incoming conflict, or cancelled by assignment in bu
	if( !pu->branch_eff[k]->num_active_incfs
	    || assignsTo( bu, pu->branch_eff[k]->deri_tree ))
	  removeBranch( pu, k );
	else
	  k++;
      }
    } // end for paths

    // printf( "Consolidation: Decision cancelled over\n" ); fflush( stdout );	
 
    if( num_paths[bu->bbid] > 1 ) {

      // sort by increasing cost, then decreasing number of branches
      sortPath( pathlist[bu->bbid], num_paths[bu->bbid] );

      for( pt = 0; pt < num_paths[bu->bbid] - 1; pt++ ) {
	pu = pathlist[bu->bbid][pt];

	for( k = pt + 1; k < num_paths[bu->bbid]; k++ ) {
	  pv = pathlist[bu->bbid][k];

	  // remove pu if its conflict list is a superset of pv's
	  // (i.e. pu has less cost yet more conflicts than pv, thus cannot be wcet path)
	  if( subsetConflict( pv, pu )) {
	    pu->bb_len = -1;
	    break;
	  }
	}
      }
      // remove the marked paths (id: the index to overwrite)
      id = -1;
      for( pt = 0; pt < num_paths[bu->bbid]; pt++ ) {
	pu = pathlist[bu->bbid][pt];

	if( pu->bb_len == -1 ) {
	  freePath( bu->bbid, pt );
	  if( id == -1 )
	    id = pt;
	}
	else {
	  if( id > -1 )
	    pathlist[bu->bbid][id++] = pu;
	}
      }
      if( id > -1 )
	num_paths[bu->bbid] = id;
    }

    // stats
    if( num_paths[bu->bbid] > max_paths )
      max_paths = num_paths[bu->bbid];
 
    // free paths in nodes which are dead (i.e. already processed)
    for( j = 0; j < bu->num_outgoing; j++ )
      in_degree[ bu->outgoing[j] ]--;

    // note that the node in top topo order is excluded
    for( j = 0; j < num_bb - 1; j++ ) {
      id = bblist[j]->bbid;
      if( in_degree[id] == 0 && pathFreed[id] == 0 ) {
	freePathsInNode( id );
	pathFreed[id] = 1;
      }
    }

    DOUT( "Paths at %d-%d: %d\n", pid, bu->bbid, num_paths[bu->bbid] );
    DACTION(
        for( pt = 0; pt < num_paths[bu->bbid]; pt++ )
          printPath( pathlist[bu->bbid][pt] );
    );
    DOUT( "\n" );

  } // end for bb

  DRETURN( 0 );
}  


path* find_WCETPath( int pid, block **bblist, int num_bb, int *in_degree, uint *cost ) {

  int  i, id, start;
  int  num;
  uint WCET;
  path *p;

  num = procs[pid]->num_bb;

  pathFreed = (char*)   CALLOC( pathFreed, num, sizeof(char), "pathFreed" );
  num_paths = (int*)    CALLOC( num_paths, num, sizeof(int),  "num_paths" );
  pathlist  = (path***) MALLOC( pathlist, num * sizeof(path**), "pathlist" );
  for( i = 0; i < num; i++ )
    pathlist[i] = NULL;

  restoreIncomingConflicts();

  max_paths = 0;

  // traverse CFG
  traverse( pid, bblist, num_bb, in_degree, cost );

  // find path with maximum cost at top topo level
  start = bblist[num_bb - 1]->bbid;
  id    = -1;
  WCET  = 0;

  for( i = 0; i < num_paths[start]; i++ ) {
    if( pathlist[start][i]->cost > WCET ) {
      id = i;
      WCET = pathlist[start][i]->cost;	
    }
  }
  if( id == -1 )
    prerr( "Error: no wcet path selected.\n" );

  // copy the longest path, to be returned
  p = (path*) MALLOC( p, sizeof(path), "path" );
  p->cost   = pathlist[start][id]->cost;
  p->bb_len = pathlist[start][id]->bb_len;

  p->bb_seq = (int*) MALLOC( p->bb_seq, p->bb_len * sizeof(int), "path bb_seq" );
  for( i = 0; i < pathlist[start][id]->bb_len; i++ )
    p->bb_seq[i] = pathlist[start][id]->bb_seq[i];

  // free up memory usage
  for( i = 0; i < num_bb; i++ ) {
    id = bblist[i]->bbid;
    if( pathFreed[id] == 0 )
      freePathsInNode( id );
  }

  free( pathlist );
  free( num_paths );
  free( pathFreed );

  return p;
}
