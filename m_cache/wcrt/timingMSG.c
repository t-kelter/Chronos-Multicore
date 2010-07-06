#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "timingMSG.h"
#include "alloc.h"
#include "slacks.h"
#include "util.h"
#include "parse.h"
#include "handler.h"
#include "timing.h"
#include "dump.h"

#include "config.h"

extern char resultFileBaseName[];

/*
 * Makes a clone of src.
 */
int copyTask( task_t *dst, task_t *src ) {

  int i;

  strcpy( dst->tname, src->tname );
  dst->actorID  = src->actorID;
  dst->peID     = src->peID;
  dst->priority = src->priority;
  dst->period   = src->period;
  dst->ctimeLo  = src->ctimeLo;
  dst->ctimeHi  = src->ctimeHi;

  dst->numPreds = src->numPreds;
  dst->predList = (int*) MALLOC( dst->predList, dst->numPreds * sizeof(int), "dst->predList" );
  for( i = 0; i < dst->numPreds; i++ )
    dst->predList[i] = src->predList[i];

  dst->numSuccs = src->numSuccs;
  dst->succList = (int*) MALLOC( dst->succList, dst->numSuccs * sizeof(int), "dst->succList" );
  for( i = 0; i < dst->numSuccs; i++ )
    dst->succList[i] = src->succList[i];

  dst->numMemBlocks = src->numMemBlocks;
  dst->memBlockList = (mem_t**) MALLOC( dst->memBlockList, dst->numMemBlocks * sizeof(mem_t*), "dst->memBlockList" );
  for( i = 0; i < dst->numMemBlocks; i++ )
    dst->memBlockList[i] = src->memBlockList[i];

  dst->allocated = (char*) CALLOC( dst->allocated, dst->numMemBlocks, sizeof(char), "dst->allocated" );

  return 0;
}


int mapDependency( task_t *tc, int *map ) {

  int i;

  for( i = 0; i < tc->numPreds; i++ )
    tc->predList[i] = map[tc->predList[i]];

  for( i = 0; i < tc->numSuccs; i++ )
    tc->succList[i] = map[tc->succList[i]];

  return 0;
}


/*
 * Adds tasks from msg[chartid] to chart ctx, maintaining the topological order.
 * For cyclic MSG: if a task from msg[chartid] is already in ctx, duplicate that task instance.
 * Assumption: each chart in the original MSG has unique tasks,
 * so just check if first task in msg[chartid] is already in ctx;
 * if so, it means the whole chart msg[chartid] is already in ctx.
 */
int addChartTasks( int chartid, chart_t *ctx ) {

  int i, k;

  int *map = NULL;  // if duplicated, map[i] = id of the task duplicated from i

  chart_t *cid = &(msg[chartid]);
  char toDup = inList( cid->topoList[0], ctx->topoList, ctx->topoListLen );

  // assume no empty chart!
  int newlen = ctx->topoListLen + cid->topoListLen;
  ctx->topoList = (int*) REALLOC( ctx->topoList, newlen * sizeof(int), "ctx->topoList" );

  if( toDup ) {

    // need to duplicate
    int newStart = numTasks;

    map = (int*) MALLOC( map, numTasks * sizeof(int), "map" );
    for( i = 0; i < numTasks; i++ )
      map[i] = i;

    for( i = 0; i < cid->topoListLen; i++ ) {
      int idx = cid->topoList[i];

      // duplicate task
      task_t *tc = (task_t*) MALLOC( tc, sizeof(task_t), "tc" );
      copyTask( tc, taskList[idx] );

      // keep the mapping
      map[idx] = numTasks;

      //printf( "Added copy of task %s (%d) with id %d\n", getTaskName(idx), idx, map[idx] );

      // add to taskList
      numTasks++;
      taskList = (task_t**) REALLOC( taskList, numTasks * sizeof(task_t*), "taskList" );
      taskList[numTasks-1] = tc;
    }

    // adjust predecessors and successors according to map
    // Note that map is constructed from old numTasks;
    // there shouldn't be a duplicate of the tasks currently being added.
    for( i = newStart; i < numTasks; i++ )
      mapDependency( taskList[i], map );

    //dumpTaskInfo_dep();

    // add new tasks to chart
    for( i = 0; i < cid->topoListLen; i++ )
      ctx->topoList[ctx->topoListLen + i] = map[cid->topoList[i]];
    ctx->topoListLen = newlen;

    // add new tasks to ctx, by PE
    for( k = 0; k < numPEs; k++ ) {
      sched_t *sc = ctx->PEList[k];
      sched_t *sr = cid->PEList[k];

      int newassg = sc->numAssigned + sr->numAssigned;
      if( newassg <= 0 )
	continue;

      sc->assignedList = (int*) REALLOC( sc->assignedList, newassg * sizeof(int), "sc->assignedList" );
      for( i = 0; i < sr->numAssigned; i++ ) {
	sc->assignedList[sc->numAssigned + i] = map[sr->assignedList[i]];
      }
      sc->numAssigned = newassg;
    }
    free( map );

    return newlen;
  }

  // normal
  for( i = 0; i < cid->topoListLen; i++ )
    ctx->topoList[ctx->topoListLen + i] = cid->topoList[i];
  ctx->topoListLen = newlen;

  for( k = 0; k < numPEs; k++ ) {
    sched_t *sc = ctx->PEList[k];
    sched_t *sr = cid->PEList[k];

    int newassg = sc->numAssigned + sr->numAssigned;
    if( newassg <= 0 )
      continue;

    sc->assignedList = (int*) REALLOC( sc->assignedList, newassg * sizeof(int), "sc->assignedList" );
    for( i = 0; i < sr->numAssigned; i++ ) {
      sc->assignedList[sc->numAssigned + i] = sr->assignedList[i];
    }
    sc->numAssigned = newassg;
  }
  return newlen;
}


int copyChart( chart_t *new, chart_t *src, int lastnode, char nextsucc ) {

  int i;
  int stopnode = msg[lastnode].succList[nextsucc-1];
  int stoptask = msg[stopnode].topoList[0];

  for( i = 0; i < src->topoListLen; i++ ) {
    if( src->topoList[i] == stoptask )
      break;
    enqueue( src->topoList[i], &(new->topoList), &(new->topoListLen) );
  }
  return new->topoListLen;
}


/*
 * Returns the last node in bpStack and the corresponding nextSucc as (node, next).
 * The nextSucc value is incremented. If it reaches the end, the node is deleted and len is updated.
 */
int popNode( int *node, int *next, int **bpStack, int **nextSucc, int *len ) {

  *node = (*bpStack)[(*len)-1];
  *next = (*nextSucc)[(*len)-1];

  (*nextSucc)[(*len)-1]++;
  if( (*nextSucc)[(*len)-1] >= msg[(*len)-1].numSuccs )
    (*len)--;

  return 0;
}


int countEdge( path_t *px, int src, int dst ) {

  px->edgeBounds[src][dst]--;
  return px->edgeBounds[src][dst];
}


/*
 * Adds dependency between tasks of the same process occuring consecutively.
 * Assumes tasks are topologically sorted.
 * Records added dependencies in (preds, succs) pair.
 */
int addConcatDependency( chart_t *cx, int **predRecord, int **succRecord, int *recordLen ) {

  int i, k;

  for( i = 0; i < cx->topoListLen - 1; i++ ) {
    int ix = cx->topoList[i];
    task_t *ti = taskList[ix];
    char found = 0;

    for( k = i+1; !found && k < cx->topoListLen; k++ ) {
      int kx = cx->topoList[k];
      task_t *tk = taskList[kx];

      if( ti->actorID == tk->actorID ) {
	found = 1;
	enqueueUnique( ix, &(tk->predList), &(tk->numPreds) );
	enqueueUnique( kx, &(ti->succList), &(ti->numSuccs) );
	enqueuePair( ix, predRecord, kx, succRecord, recordLen );
	//printf( "Added concat dependency %d %s to %d %s\n", ix, getTaskName(ix), kx, getTaskName(kx) );
	//printTask_dep(ix); printTask_dep(kx);
      }
    }
  }
  return 0;
}


/*
 * Removes dependency between tasks stated in records.
 * Note that removal may fail for added dependencies that also exist in the original chart.
 */
int recoverConcatDependency( int *predRecord, int *succRecord, int recordLen ) {

  int i;
  task_t *tx;

  for( i = 0; i < recordLen; i++ ) {
    int pred = predRecord[i];
    int succ = succRecord[i];

    //printf( "Removing concat dependency %d %s to %d %s\n", pred, getTaskName(pred), succ, getTaskName(succ) );
    tx = taskList[pred];
    if( tx->succList[tx->numSuccs-1] == succ ) {
      tx->succList[tx->numSuccs-1] = -1;
      tx->numSuccs--;
    }
    //else
    //  printf( "Warning: succ %d not found at expected position in task %d\n", succ, pred );

    tx = taskList[succ];
    if( tx->predList[tx->numPreds-1] == pred ) {
      tx->predList[tx->numPreds-1] = -1;
      tx->numPreds--;
    }
    //else
    //  printf( "Warning: pred %d not found at expected position in task %d\n", pred, succ );
  }
  return 0;
}


int initPath( path_t *px ) {

  int i, k;

  px->msc = (chart_t*) MALLOC( px->msc, sizeof(chart_t), "chart_t" );
  initChart( px->msc );

  px->edgeBounds = (char**) MALLOC( px->edgeBounds, numCharts * sizeof(char*), "px->edgeBounds" );
  for( i = 0; i < numCharts; i++ ) {
    px->edgeBounds[i] = (char*) MALLOC( px->edgeBounds[i], numCharts * sizeof(char), "px->edgeBounds[i]" );
    for( k = 0; k < numCharts; k++ )
      px->edgeBounds[i][k] = MAX_UNFOLD;
  }
  for( i = 0; i < numEdgeBounds; i++ ) {
    edge_t *eb = &(edgeBounds[i]);
    px->edgeBounds[eb->src][eb->dst] = eb->bound;
  }
  return 0;
}


int copyPath( path_t *new, path_t *src, int lastnode, char nextsucc ) {

  int i;
  int curr, prev;

  int stopnode = msg[lastnode].succList[nextsucc-1];
  int stoptask = msg[stopnode].topoList[0];

  prev = -1;
  for( i = 0; i < src->msc->topoListLen; i++ ) {
    curr = src->msc->topoList[i];
    if( curr == stoptask )
      break;
    enqueue( curr, &(new->msc->topoList), &(new->msc->topoListLen) );
    if( prev > -1 )
      countEdge( new, prev, curr );
    prev = curr;
  }
  return new->msc->topoListLen;
}


int getNextValidSuccId( path_t *px, int chartid, int from ) {

  int i;

  for( i = from; i < msg[chartid].numSuccs; i++ ) {
    int succ = msg[chartid].succList[i];
    if( px->edgeBounds[chartid][succ] > 0 )
      return i;
  }
  return -1;
}


int timingEstimate_synch_acyclic() {

  int i, k;

  time_t *cummu = (time_t*) CALLOC( cummu, numCharts, sizeof(time_t), "cummulated WCRT" );
  int *wpath = (int*) CALLOC( wpath, numCharts, sizeof(int), "WCRT path" );

  if( allocmethod == NONE ) {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      //resetAllocation( &(msg[i]) );
      timingEstimateMSC( &(msg[i]) );
    }
  }
  else if( allocmethod == SLACK_CRITICAL ) {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      resetAllocation( &(msg[i]) );
      timingAllocMSC_slackCritical( &(msg[i]) );
    }
  }
  else {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      resetAllocation( &(msg[i]) );
      timingAllocMSC( &(msg[i]) );
    }
  }
 
  for( i = 0; i < numCharts; i++ )
    printf( "WCRT[%d]: %Lu\n", i, msg[i].wcrt );
  printf( "\n" );

  // traverse MSG to find heaviest path
  for( i = numCharts-1; i >= 0; i-- ) {
    int id = topoMSG[i];

    time_t max = 0;
    int maxID = -1;

    // maximum among successors
    for( k = 0; k < msg[id].numSuccs; k++ ) {
      int sid = msg[id].succList[k];
      if( cummu[sid] > max ) {
	max = cummu[sid];
	maxID = sid;
      }
    }
    cummu[id] = max + msg[id].wcrt;
    wpath[id] = maxID;
  }
  printf( "Total WCRT: %Lu\n", cummu[topoMSG[0]] );
    
  // path
  i = topoMSG[0];
  printf( "Longest path:" );
  while( i != -1 ) {
    printf( " %d", i );
    i = wpath[i];
  }
  printf( "\n" );
  
  free( cummu );
  free( wpath );

  return 0;
}


/*
 * Handle cyclic MSG with bounds on back edges. 
 * Use implicit path enumeration with ILP.
 */
int timingEstimate_synch() {

  int i, k;
  FILE *ilpf;

  double soln;

  int **incoming;
  int *incomingLen;

  if( allocmethod == NONE ) {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      //resetAllocation( &(msg[i]) );
      timingEstimateMSC( &(msg[i]) );
    }
  }
  else if( allocmethod == SLACK_CRITICAL ) {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      resetAllocation( &(msg[i]) );
      timingAllocMSC_slackCritical( &(msg[i]) );
    }
  }
  else {
    for( i = 0; i < numCharts; i++ ) {
      resetInterference( &(msg[i]) );
      resetAllocation( &(msg[i]) );
      timingAllocMSC( &(msg[i]) );
    }
  }
 
  for( i = 0; i < numCharts; i++ )
    printf( "WCRT[%d]: %Lu\n", i, msg[i].wcrt );
  printf( "\n" );

  ilpf = wcrt_openfile( "wcrtsyn.ilp", "w" );

  /* sudiptac ::: changing the syntax for using lp_solve */		  
  /* fprintf( ilpf, "enter Q\n" ); */

  // cost function
  fprintf( ilpf, "maximize\n" );
  for( i = 0; i < numCharts; i++ ) {
    fprintf( ilpf, "%Lu Node%d", msg[i].wcrt, i );
    if( i < numCharts - 1 )
      fprintf( ilpf, " + " );
  }

  /* fprintf( ilpf, "\nst\n" ); */
  fprintf( ilpf, "\nsubject to\n" ); 

  // start and end nodes
  fprintf( ilpf, "Ya_%d = 1\n", startNode );
  fprintf( ilpf, "Y%d_z = 1\n", endNode );

  // bound on back edges
  for( i = 0; i < numEdgeBounds; i++ )
    fprintf( ilpf, "Y%d_%d <= %d\n", edgeBounds[i].src, edgeBounds[i].dst, edgeBounds[i].bound );

  // flow constraints: node count == outgoing edges
  for( i = 0; i < numCharts; i++ ) {
    chart_t *cx = &(msg[i]);
    fprintf( ilpf, "Node%d", i );
    for( k = 0; k < cx->numSuccs; k++ )
      fprintf( ilpf, " - Y%d_%d", i, cx->succList[k] );
	 /* sudiptac ::: CHANGED */	
    if( i == endNode )
      fprintf( ilpf, " - Y%d_z", i );
    fprintf( ilpf, " = 0\n" );
  }

  // collect incoming edges first, since we do not store this
  incoming = (int**) MALLOC( incoming, numCharts * sizeof(int*), "incoming" );
  incomingLen = (int*) CALLOC( incomingLen, numCharts, sizeof(int), "incomingLen" );
  for( i = 0; i < numCharts; i++ )
    incoming[i] = NULL;

  for( i = 0; i < numCharts; i++ ) {
    chart_t *cx = &(msg[i]);
    for( k = 0; k < cx->numSuccs; k++ ) {
      int sx = cx->succList[k];
      enqueue( i, &(incoming[sx]), &(incomingLen[sx]) );
    }
  }

  // flow constraints: node count == incoming edges
  for( i = 0; i < numCharts; i++ ) {
    fprintf( ilpf, "Node%d", i );
    for( k = 0; k < incomingLen[i]; k++ )
      fprintf( ilpf, " - Y%d_%d", incoming[i][k], i );
    if( i == startNode )
      fprintf( ilpf, " - Ya_%d", i );
    fprintf( ilpf, " = 0\n" );
  }

  fprintf( ilpf, "end\n" );

  /*
  fprintf( ilpf, "change problem milp\n" );
  for( i = 0; i < numCharts; i++ ) {
    chart_t *cx = &(msg[i]);
    fprintf( ilpf, "change type Node%d I\n", i );
    for( k = 0; k < cx->numSuccs; k++ )
      fprintf( ilpf, "change type Y%d_%d I\n", i, cx->succList[k] );
    for( k = 0; k < incomingLen[i]; k++ )
      fprintf( ilpf, "change type Y%d_%d I\n", incoming[i][k], i );
  }
  */

  for( i = 0; i < numCharts; i++ )
    free( incoming[i] );
  free( incoming );
  free( incomingLen );

  // optimize
  //fprintf( ilpf, "set mip tolerances integrality 0\n" );
  /* sudiptac ::: CHANGED */ 
  /* fprintf( ilpf, "optimize\n" );
  fprintf( ilpf, "display solution variables -\n" );
  fprintf( ilpf, "quit\n" ); */

  fclose( ilpf );

  // solve ilp
  /* sudiptac ::: Changed */
  const char *lp_solve_dir = LP_SOLVE_PATH;
  char lp_solve_call[200];
  strcpy( lp_solve_call, lp_solve_dir );
  strcat( lp_solve_call, "/lp_solve -rxli " );
  strcat( lp_solve_call, lp_solve_dir );
  strcat( lp_solve_call, "/xli_CPLEX wcrtsyn.ilp > wcrtsyn.sol" );
  system( lp_solve_call );

  // read solution ::: sudiptac :: CHANGED
  system( "grep objective wcrtsyn.sol | awk '{print $NF}' > wcrtsyn.val" );

  ilpf = wcrt_openfile( "wcrtsyn.val", "r" );
  fscanf( ilpf, "%le", &soln );
  fclose( ilpf );

  // node counts :: CHANGED :: sudiptac
  /* system( "grep Node wcrtsyn.sol" ); */

  printf( "\nTotal WCRT: %Lu\n\n", (time_t) soln );


  FILE *f;
  char path[MAXLEN];
  sprintf(path, "%s.%d.WCRT", resultFileBaseName, times_iteration);
  f = wcrt_openfile(path, "w");
  if( !f ) {
    fprintf( stderr, "Failed to open file %s (timingMSG.c:548).\n", path );
    exit(1);
  }
  
  fprintf(f, "%Lu", (time_t) soln);

  return 0;
}


int timingEstimate_asynch_acyclic() {

  //int i;
  int idx;
  time_t max;
  chart_t *cx, *maxcx, *prevcx;

  int backpt;
  int nextsc;

  // backtrack stack and successor marker for depth-first traversal
  int backtracklen = 0;
  int *backtrack = NULL;
  int *nextsucc = NULL;

  // record of added dependency
  int recordLen;
  int *predRecord;
  int *succRecord;

  // concatenation result
  int numPaths = 0;
  //chart_t **pathList = NULL;

  cx = (chart_t*) MALLOC( cx, sizeof(chart_t), "chart_t" );
  initChart( cx );

  // start node
  //idx = topoMSG[0];
  idx = startNode;

  max = 0;
  maxcx = NULL;
  prevcx = NULL;

  while( 1 ) {

    addChartTasks( idx, cx );

    if( msg[idx].numSuccs <= 0 ) {

      // complete path
      numPaths++;
      //pathList = (chart_t**) REALLOC( pathList, numPaths * sizeof(chart_t*), "pathList" );
      //pathList[numPaths-1] = cx;

      // add dependency among tasks of the same process
      recordLen = 0;
      predRecord = NULL;
      succRecord = NULL;
      addConcatDependency( cx, &predRecord, &succRecord, &recordLen );

      // analyse the WCRT of the resulting MSC
      // no constraint on first analysis: assume any pair may interfere
      resetInterference( cx );

      if( allocmethod == NONE )
        timingEstimateMSC( cx );
      else {
        resetAllocation( cx );
        
        if( allocmethod == SLACK_CRITICAL )
          timingAllocMSC_slackCritical( cx );
        else
          timingAllocMSC( cx );
      }
 
      // recover dependencies
      recoverConcatDependency( predRecord, succRecord, recordLen );
      free( predRecord );
      free( succRecord );

      if( backtracklen <= 0 )  // done
        break;

      prevcx = cx;

      // next path
      cx = (chart_t*) MALLOC( cx, sizeof(chart_t), "chart_t" );
      initChart( cx );

      // pop backtrack point
      popNode( &backpt, &nextsc, &backtrack, &nextsucc, &backtracklen );

      // copy task list from previous path up to this point
      //copyChart( cx, pathList[numPaths-1], backpt, nextsc );
      copyChart( cx, prevcx, backpt, nextsc );

      // next node to explore
      idx = msg[backpt].succList[nextsc];

      printf( "WCRT[%d]: %Lu\n", numPaths-1, prevcx->wcrt );
      if( prevcx->wcrt > max ) {
        max = prevcx->wcrt;
	      maxcx = prevcx;
      } else
        freeChart( prevcx );

      continue;
    }

    // if node has more than one successor, set as backtrack point
    if( msg[idx].numSuccs > 1 )
      enqueuePair( idx, &backtrack, 1, &nextsucc, &backtracklen );

    // next node: unless from backtrack, it's always the first successor
    idx = msg[idx].succList[0];
  }

  /*
  max = 0;
  idx = -1;
  for( i = 0; i < numPaths; i++ ) {
    printf( "WCRT[%d]: %Lu\n", i, pathList[i]->wcrt );
    if( pathList[i]->wcrt > max ) {
      max = pathList[i]->wcrt;
      idx = i;
    }
  }
  */
  printf( "\n\nGlobal WCRT: %Lu\n\n", max );
  //printTaskList( pathList[idx] );
  printTaskList( maxcx );

  free( backtrack );
  free( nextsucc );

  freeChart( maxcx );
  free( maxcx );

  /*
  for( i = 0; i < numPaths; i++ )
    freeChart( pathList[i] );
  free( pathList );
  */

  return 0;
}


/*
 * This version has problem with backtracking, because nodes may repeat along a path.
 */
int timingEstimate_asynch_() {

  int idx, prev;
  time_t max;

  path_t *px, *maxpx, *prevpx;
  chart_t *cx;

  int backpt;
  int nextsc;
  int nextnextsc;

  // backtrack stack and successor marker for depth-first traversal
  int backtracklen = 0;
  int *backtrack = NULL;
  int *nextsucc = NULL;

  // record of added dependency
  int recordLen;
  int *predRecord;
  int *succRecord;

  // concatenation result
  int numPaths = 0;

  FILE *pathf = wcrt_openfile( "wcrtasyn.paths", "w" );
  fprintf( pathf, "Path %d\n", numPaths );
  printf( "Path %d\n", numPaths ); fflush( stdout );

  px = (path_t*) MALLOC( px, sizeof(path_t), "path_t" );
  initPath( px );

  // start node
  idx = startNode;

  // add dummy end node
  cx = &(msg[endNode]);
  enqueue( numCharts, &(cx->succList), &(cx->numSuccs) );

  max = 0;
  maxpx = NULL;
  prevpx = NULL;

  while( 1 ) {

    chart_t *cx = px->msc;

    fprintf( pathf, " %d", idx );
    printf( " %d", idx ); fflush( stdout );

    if( idx < numCharts )
      addChartTasks( idx, cx );

    else {

      // complete path
      numPaths++;
      fprintf( pathf, "\n\n" );
      printf( "\n\n" ); fflush( stdout );

      // add dependency among tasks of the same process
      recordLen = 0;
      predRecord = NULL;
      succRecord = NULL;
      addConcatDependency( cx, &predRecord, &succRecord, &recordLen );
 
      // analyse the WCRT of the resulting MSC
      // no constraint on first analysis: assume any pair may interfere
      resetInterference( cx );

      if( allocmethod == NONE )
        timingEstimateMSC( cx );
      else {
        resetAllocation( cx );
        if( allocmethod == SLACK_CRITICAL )
          timingAllocMSC_slackCritical( cx );
        else
          timingAllocMSC( cx );
      }
 
      // recover dependencies
      recoverConcatDependency( predRecord, succRecord, recordLen );
      free( predRecord );
      free( succRecord );

      if( backtracklen <= 0 )  // done
        break;

      // pop backtrack point
      do {
        popNode( &backpt, &nextsc, &backtrack, &nextsucc, &backtracklen );
        nextsc = getNextValidSuccId( px, backpt, nextsc );
      } while( nextsc == -1 );

      // next node to explore
      idx = msg[backpt].succList[nextsc];
      if( idx < numCharts )
        countEdge( px, backpt, idx );

      // next path
      prevpx = px;
      px = (path_t*) MALLOC( px, sizeof(path_t), "path_t" );
      initPath( px );

      // copy task list from previous path up to this point
      copyPath( px, prevpx, backpt, nextsc );

      printf( "WCRT[%d]: %Lu\n", numPaths-1, prevpx->msc->wcrt );
      if( prevpx->msc->wcrt > max ) {
        max = prevpx->msc->wcrt;
        maxpx = prevpx;
      } else
        wcrt_freePath( prevpx );

      fprintf( pathf, "Path %d\n", numPaths );
      printf( "Path %d\n", numPaths ); fflush( stdout );

      continue;
    }

    // continue to next node
    nextsc = getNextValidSuccId( px, idx, 0 );
    if( nextsc == -1 ) {

      // discard this path
      fprintf( pathf, " X\n\n" );
      printf( " X\n\n" ); fflush( stdout );

      if( backtracklen <= 0 )  // done
        break;

      // pop backtrack point
      do {
        popNode( &backpt, &nextsc, &backtrack, &nextsucc, &backtracklen );
        nextsc = getNextValidSuccId( px, backpt, nextsc );
      } while( nextsc == -1 );

      // next node to explore
      idx = msg[backpt].succList[nextsc];
      if( idx < numCharts )
        countEdge( px, backpt, idx );

      // next path
      prevpx = px;
      px = (path_t*) MALLOC( px, sizeof(path_t), "path_t" );
      initPath( px );

      // copy task list from previous path up to this point
      copyPath( px, prevpx, backpt, nextsc );

      fprintf( pathf, "Path %d\n", numPaths );
      printf( "Path %d\n", numPaths ); fflush( stdout );

      continue;
    }

    // if node has more successors not yet exhausted, set as backtrack point
    nextnextsc = getNextValidSuccId( px, idx, nextsc + 1 );
    if( nextsc > -1 )
      enqueuePair( idx, &backtrack, nextnextsc, &nextsucc, &backtracklen );

    // next node
    prev = idx;
    idx = msg[prev].succList[nextsc];
    if( idx < numCharts )
      countEdge( px, prev, idx );
  }
  fclose( pathf );

  printf( "\n\nGlobal WCRT: %Lu\n\n", max );
  printTaskList( maxpx->msc );

  free( backtrack );
  free( nextsucc );

  wcrt_freePath( maxpx );
  free( maxpx );

  return 0;
}


/*
 * Handle cyclic MSG with bounds on back edges.
 * REQUIRES THAT PATHS HAVE BEEN ENUMERATED in a file <dgname>.paths
 * Then for each path, construct the concatenated MSC and analyse.
 */
int timingEstimate_asynch() {

  FILE *pathf;
  char token[24];

  int i, idx;
  int id, len;
  int orgNumTasks;

  int maxid;
  time_t max;
  chart_t *cx, *maxcx;

  // record of added dependency
  int recordLen;
  int *predRecord;
  int *succRecord;

  // read enumerated paths
  pathf = openfext( dgname, "paths", "r" );
  while( fscanf( pathf, "%s %d %d", token, &id, &len ) != EOF ) {

    // construct concatenated MSC
    cx = (chart_t*) MALLOC( cx, sizeof(chart_t), "chart_t" );
    initChart( cx );

    // keep track of task duplication
    orgNumTasks = numTasks;

    //printf( "\n========================================" );
    printf( "\nPath %d:", id );
    for( i = 0; i < len; i++ ) {
      fscanf( pathf, "%d", &idx );
      printf( " %d", idx );
      addChartTasks( idx, cx );
    }
    //printf( "\n========================================" );
    //for( i = 0; i < cx->topoListLen; i++ )
    //  printTask_dep(cx->topoList[i]);
    printf( "\n\n" );

    // expand data structures
    if( numTasks > orgNumTasks ) {

      earliestReq = (time_t*) REALLOC( earliestReq, numTasks * sizeof(time_t), "earliestReq" );
      latestReq   = (time_t*) REALLOC( latestReq,   numTasks * sizeof(time_t), "latestReq"   );
      earliestFin = (time_t*) REALLOC( earliestFin, numTasks * sizeof(time_t), "earliestFin" );
      latestFin   = (time_t*) REALLOC( latestFin,   numTasks * sizeof(time_t), "latestFin"   );

      interfere = (char**) REALLOC( interfere, numTasks * sizeof(char*), "interfere" );
      for( i = 0; i < numTasks; i++ ) {
        interfere[i] = (char*) CALLOC( interfere[i], numTasks, sizeof(char), "interfere[i]" );
      }
    }

    // add dependency among tasks of the same process
    recordLen = 0;
    predRecord = NULL;
    succRecord = NULL;
    addConcatDependency( cx, &predRecord, &succRecord, &recordLen );

    // analyse the WCRT of the resulting MSC
    // no constraint on first analysis: assume any pair may interfere
    resetInterference( cx );

    if( allocmethod == NONE )
      timingEstimateMSC( cx );
    else {
      resetAllocation( cx );
      if( allocmethod == SLACK_CRITICAL )
        timingAllocMSC_slackCritical( cx );
      else
        timingAllocMSC( cx );
    }

    // recover dependencies
    recoverConcatDependency( predRecord, succRecord, recordLen );
    free( predRecord );
    free( succRecord );

    // recover original taskList and data structures
    if( numTasks > orgNumTasks ) {
      for( i = orgNumTasks; i < numTasks; i++ ) {
        freeTask( taskList[i] );
        free( taskList[i] );
        free( interfere[i] );
      }
      numTasks = orgNumTasks;
    }

    printf( "WCRT[%d]: %Lu\n", id, cx->wcrt );
    if( cx->wcrt > max ) {
      max = cx->wcrt;
      maxid = id;
      maxcx = cx;
    }
    else {
      freeChart( cx );
      free( cx );
    }
  }
  fclose( pathf );

  printf( "\n\nGlobal WCRT: %Lu\n\n", max );
  printf( "WCRT Path %d\n", maxid );
  //printTaskList( maxcx );

  freeChart( maxcx );
  free( maxcx );

  return 0;
}


int timingEstimate() {

  if( concat == ASYNCH )
    return timingEstimate_asynch();

  return timingEstimate_synch();
}
