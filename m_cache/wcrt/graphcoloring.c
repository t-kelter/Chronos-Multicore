#include <stdlib.h>
#include <string.h>

#include "graphcoloring.h"
#include "cycle_time.h"
#include "timing.h"
#include "handler.h"
#include "dump.h"

/*
 * Performs greedy heuristic for the graph coloring problem.
 * Returns the number of colors used, and updates colorAssg with the assigned colors.
 */
int graphColoring( int numNodes, int *outdegree, int **outedges, char **colorAssg ) {

  int numColors;
  int *sortedIndex;

  int i, j;
  char changed;

  int color;
  int numColored;

  // sort in decreasing order of degree
  sortedIndex = (int*) MALLOC( sortedIndex, numNodes * sizeof(int), "sortedIndex" );
  for( i = 0; i < numNodes; i++ )
    sortedIndex[i] = i;

  changed = 1;
  while( changed ) {
    changed = 0;

    for( i = 1; i < numNodes; i++ ) {
      if( outdegree[sortedIndex[i]] > outdegree[sortedIndex[i-1]] ) {
	int temp;
	changed = 1;
	temp = sortedIndex[i-1];
	sortedIndex[i-1] = sortedIndex[i];
	sortedIndex[i] = temp;
      }
    }
  }

  for( i = 0; i < numNodes; i++ )
    (*colorAssg)[i] = -1;

  // enumerate the colors
  numColored = 0;
  for( color = 0; numColored < numNodes; color++ ) {

    // traverse nodes in decreasing order of degree
    for( i = 0; i < numNodes; i++ ) {
      int id = sortedIndex[i];
      char found = 0;

      // skip already colored nodes
      if( (*colorAssg)[id] != -1 )
	continue;

      // check for neighbors having the same color
      for( j = 0; !found && j < outdegree[id]; j++ )
	if( (*colorAssg)[outedges[id][j]] == color )
	  found = 1;

      if( !found ) {
	(*colorAssg)[id] = color;
	numColored++;
	//printf( "Node %d color %d\n", id, (*colorAssg)[id] );
      }
    }
  }
  numColors = color;
  //printf( "%d colors used.\n", numColors );

  /*
  // for each node, assign the smallest color index that is not used by any neighbor
  numColors = 0;
  for( i = 0; i < numNodes; i++ ) {
    int id = sortedIndex[i];

    char color = 0;
    changed = 1;

    while( changed ) {
      changed = 0;
      for( j = 0; !changed && j < outdegree[id]; j++ ) {
	if( (*colorAssg)[outedges[id][j]] == color ) {
	  color++;
	  changed = 1;
	}
      }
    }
    (*colorAssg)[id] = color;
    // printf( "Node %d color %d\n", id, (*colorAssg)[id] );

    if( numColors < color + 1 ) {
      numColors = color + 1;
      // printf( "%d colors used.\n", numColors );
    }
  }
  */

  free( sortedIndex );

  /*
  // tweak: if a node has a 'choice', choose a grouping with higher similarity in memory requirement
  char *freeC = (char*) MALLOC( freeC, numColors * sizeof(char), "color choices" );

  for( i = 0; i < numNodes; i++ ) {
    int id = sortedIndex[i];
    int numFree = 0;

    // check available color choices
    for( color = 0; color < numColors; color++ )
      freeC[color] = 1;
    for( j = 0; j < outdegree[id]; j++ )
      freeC[(*colorAssg)[outedges[id][j]]] = 0;

    for( color = 0; color < numColors; color++ )
      if( freeC[color] )
	numFree++;
    if( numFree <= 1 )
      continue;

  }
  free( freeC );
  */

  return numColors;
}


/*
 * Wrapper for graphColoring.
 * Transforms task interactions into interference graph and calls graphColoring.
 */
//int taskColoring( int *memberList, int numMembers, char **colorAssg ) {
int taskColoring( overlay_t *ox, char **colorAssg ) {

  int i, j;
  int numColors;

  int *memberList = ox->ownerTaskList;
  int numMembers = ox->numOwnerTasks;

  int *outdegree = (int*)  MALLOC( outdegree, numMembers * sizeof(int),  "outdegree" );
  int **outedges = (int**) MALLOC( outedges,  numMembers * sizeof(int*), "outedges" );
  // note: outedges contain the task index in the array, not task id

  // printf( "memberList(%d):%x  outdegree:%x  outedges:%x  colorAssg:%x\n", numMembers, memberList, outdegree, outedges, colorAssg ); fflush( stdout );

  for( i = 0; i < numMembers; i++ ) {
    int idx = memberList[i];

    outdegree[i] = 0;
    outedges[i] = NULL;

    for( j = 0; j < numMembers; j++ ) {
      int idj = memberList[j];
      //printf( "idx:%d  idj:%d\n", idx, idj ); fflush( stdout );
      //printf( "canPreempt(idx,idj):%d  canPreempt(idj,idx):%d\n", canPreempt(idx,idj), canPreempt(idj,idx) ); fflush( stdout );

      if( idx != idj && (canPreempt(idx,idj) || canPreempt(idj,idx)) ) {
	outdegree[i]++;
	outedges[i] = (int*) REALLOC( outedges[i], outdegree[i] * sizeof(int), "outedges[i]" );
	outedges[i][outdegree[i]-1] = j;

	/*
	printf( "GC edge: %s -- %s\n", getTaskName(idx), getTaskName(idj) );
	// update interference graph
	interfere[idx][idj] = 1;
	interfere[idj][idx] = 1;
	*/
      }
    }
  }
  numColors = graphColoring( numMembers, outdegree, outedges, colorAssg );

  printf( "%d colors used:", numColors ); fflush( stdout );
  for( i = 0; i < numMembers; i++ )
    printf( " %d", (*colorAssg)[i] );
  printf( "\n" ); fflush( stdout );

  free( outdegree );
  for( i = 0; i < numMembers; i++ )
    free( outedges[i] );
  free( outedges );

  return numColors;
}


/*
 * Partitions SPM to 'colors', based on total utilization of tasks assigned to each color.
 * Calls external SPM partitioning routine and stores the result in colorShare.
 */
int colorPartition( int *memberList, int numMembers, char *colorAssg, int **colorShare, int capacity ) {

  FILE *fptr;
  int i, k;
  int pid, size;

  // prepare parameter file
  fptr = wcrt_openfile( "dopartn.par", "w" );
  fprintf( fptr, "insnsize %d\n", INSN_SIZE );
  fprintf( fptr, "fetchsize %d\n", FETCH_SIZE );
  fprintf( fptr, "latency_spm %d\n", spm_latency );
  fprintf( fptr, "latency_off %d\n", off_latency );
  fprintf( fptr, "capacity %d\n", capacity );
  fclose( fptr );

  // print task information
  fptr = wcrt_openfile( "dopartn.tsk", "w" );
  fprintf( fptr, "%d\n", numMembers );
  for( i = 0; i < numMembers; i++ ) {
    task_t *tx = taskList[memberList[i]];

    // find total codesize and minimum bb size
    int totsize = 0;
    int minsize = -1; 
    for( k = 0; k < tx->numMemBlocks; k++ ) {
      mem_t *mt = tx->memBlockList[k];
      totsize += mt->size;
      if( minsize == -1 || minsize > mt->size )
	minsize = mt->size;
    }
    fprintf( fptr, "%s %Lu %Lu %d %d\n", tx->tname, tx->ctimeHi, tx->period, minsize, totsize );
  }
  fclose( fptr );

  // this is to induce determinism in allocation result across different schemes
  system( "sort dopartn.tsk > tmp; mv tmp dopartn.tsk" );

  // print task sets based on colors
  fptr = wcrt_openfile( "dopartn.split", "w" );
  for( i = 0; i < numMembers; i++ ) {
    int idx = memberList[i];
    fprintf( fptr, "%d %s\n", colorAssg[i], taskList[idx]->tname );
  }
  fclose( fptr );

  // call partitioning routine
  STARTTIME;
  system( "partnspm dopartn dopartn > partnspm.trace\n" );
  STOPTIME;

  // read results
  fptr = wcrt_openfile( "dopartn.partnspm", "r" );
  while( fscanf( fptr, "%d %d", &pid, &size ) != EOF ) {
    (*colorShare)[pid] = size;
    // printf( "Color %d share %d bytes\n", pid, (*colorShare)[pid] );
  }
  fclose( fptr );

  //system( "rm dopartn.* partnspm.*" );

  return 0;
}


/*
 * Calls spm allocation that considers colors.
 */
int colorAllocation( chart_t *msc, overlay_t *ox, char *colorAssg, int numColors, int capacity ) {

  int i;
  FILE *fptr;

  int k, idx;
  int fnid, bbid, addr, flag;
  char tname[MAXLEN];

  task_t *tx;
  double res;

  // for tracking purpose
  int *taskshare = (int*) CALLOC( taskshare, ox->numOwnerTasks, sizeof(int), "taskshare" );
  int *colorshare = (int*) CALLOC( colorshare, numColors, sizeof(int), "colorshare" );

  // prepare parameter file
  fptr = wcrt_openfile( "doalloc.par", "w" );
  fprintf( fptr, "insnsize %d\n", INSN_SIZE );
  fprintf( fptr, "fetchsize %d\n", FETCH_SIZE );
  fprintf( fptr, "latency_spm %d\n", spm_latency );
  fprintf( fptr, "latency_off %d\n", off_latency );
  fprintf( fptr, "capacity %d\n", capacity );
  fclose( fptr );

  // prepare list of tasks
  fptr = wcrt_openfile( "doalloc.tsc", "w" );
  fprintf( fptr, "%d\n", ox->numOwnerTasks );

  if( ox->numOwnerTasks == 1 ) {
    task_t *ti = taskList[ox->ownerTaskList[0]];
    fprintf( fptr, "%s %Lu %Lu 1.0 0\n", ti->tname, ti->ctimeHi, ti->period );
    fclose( fptr );
  }
  else {
    for( i = 0; i < ox->numOwnerTasks; i++ ) {
      int ix = ox->ownerTaskList[i];
      task_t *ti = taskList[ix];

      /*
      // as a heuristic, consider possible preemption from other tasks into the runtime
      time_t rtime = ti->ctimeHi;
      for( k = 0; k < ox->numOwnerTasks; k++ ) {
        int kx = ox->ownerTaskList[k];
        if( i != k && canPreempt( ix, kx ))
          rtime += taskList[kx]->ctimeHi;
      }
      fprintf( fptr, "%s %Lu %Lu %d\n", ti->tname, rtime, ti->period, isCritical[ix] );
      */
      double adjust = allocweight;
      if( msc != NULL ) {
	if( isCritical[ix] == 1 )
	  adjust += (latestFin[ix] - earliestReq[ix]) / (double) msc->wcrt;
	else if( isCritical[ix] == 2 )
	  adjust += ti->ctimeHi / (double) msc->wcrt;
      }
      //printf( "ctimeHi: %Lu   taskwcrt: %Lu   totwcrt: %Lu   adjust: %lf\n", ti->ctimeHi, (latestFin[ix] - earliestReq[ix]), msc->wcrt, adjust );
      fprintf( fptr, "%s %Lu %Lu %lf %d\n", ti->tname, ti->ctimeHi, ti->period, adjust, colorAssg[i] );
    }
    fclose( fptr );

    // this is to induce determinism in allocation result across different schemes
    system( "sort doalloc.tsc > tmp; mv tmp doalloc.tsc" );
  }

  // call allocation routine
  system( "spmcolors doalloc doalloc 1 > ilptrace\n" );

  // sanity check
  fptr = wcrt_openfile( "doalloc.alloc", "r" );
  if( fscanf( fptr, "%s %d %d %x", tname, &fnid, &bbid, &addr ) == EOF ) {
    // rerun with induced solution
    fclose( fptr );
    printf( "Optimal solution failed -- using induced solution.\n" );
    system( "spmcolorsInduce doalloc doalloc 1 > ilptrace\n" );
  }
  else
    fclose( fptr );


  system( "grep Area ilptrace | sort" );

  system( "grep Area ilptrace | sed '/A/s/Area//' > colorshare" );
  fptr = wcrt_openfile( "colorshare", "r" );
  while( fscanf( fptr, "%d %lf", &idx, &res ) != EOF )
    colorshare[idx] = (int) res;
  fclose( fptr );

  //ox->numMemBlocks = 0;
  //ox->memBlockList = NULL;

  // read solution
  fptr = wcrt_openfile( "doalloc.alloc", "r" );
  while( fscanf( fptr, "%s %d %d %x", tname, &fnid, &bbid, &addr ) != EOF ) {

    //printf( "%s %d %d %x\n", tname, fnid, bbid, addr ); fflush( stdout );

    // find the owner of this block
    k = ox->numOwnerTasks - 1;
    while( k > 0 && strcmp( taskList[ox->ownerTaskList[k]]->tname, tname ) != 0 )
      k--;
    tx = taskList[ox->ownerTaskList[k]];

    // find the block position
    idx = tx->numMemBlocks - 1;
    while( idx > 0 && tx->memBlockList[idx]->addr != addr )
      idx--;
    tx->allocated[idx] = 1;

    taskshare[k] += tx->memBlockList[idx]->size;

    //ox->numMemBlocks++;
    //ox->memBlockList = (mem_t**) REALLOC( ox->memBlockList, ox->numMemBlocks * sizeof(mem_t*), "ox->memBlockList" );
    //ox->memBlockList[ox->numMemBlocks-1] = &(tx->memBlockList[idx]);
  }
  fclose( fptr );

  // reset block realsize
  for( i = 0; i < ox->numOwnerTasks; i++ ) {
    task_t *tc = taskList[ox->ownerTaskList[i]];
    for( k = 0; k < tc->numMemBlocks; k++ )
      tc->memBlockList[k]->realsize = tc->memBlockList[k]->size;
  }

  // read inserted jumps
  fptr = wcrt_openfile( "doalloc.jump", "r" );
  while( fscanf( fptr, "%s %d %d %x %d", tname, &fnid, &bbid, &addr, &flag ) != EOF ) {

    // find the owner of this insn
    k = ox->numOwnerTasks - 1;
    while( k > 0 && strcmp( taskList[ox->ownerTaskList[k]]->tname, tname ) != 0 )
      k--;

    tx = taskList[ox->ownerTaskList[k]];
    for( idx = 0; idx < tx->numMemBlocks; idx++ ) {
      mem_t *mt = tx->memBlockList[idx];
      if( mt->fnid == fnid && mt->bbid == bbid ) {
	mt->realsize += INSN_SIZE;
	break;
      }
    }
    if( flag )
      taskshare[k] += INSN_SIZE;
  }
  fclose( fptr );

  for( i = 0; i < ox->numOwnerTasks; i++ )
    printf( "%s:\t%4d/%4d bytes (%d)\n", getTaskName(ox->ownerTaskList[i]),
	    taskshare[i], colorshare[(int)colorAssg[i]], colorAssg[i] );

  free( taskshare );
  free( colorshare );

  //system( "rm doalloc.* colorshare ilptrace" );

  return 0;
}
