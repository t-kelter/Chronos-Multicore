#include <stdio.h>
#include <stdlib.h>

#include "topo.h"
#include "header.h"
#include "util.h"

/*
 * Finds tasks without a predecessor and puts them into startTaskList.
 * Returns the number of start tasks.
 */
int collectStartTasks( int *taskIDList, int numChartTasks, int **startTaskList ) {

  int k, count;

  count = 0;
  for( k = 0; k < numChartTasks; k++ ) {
    int idx = taskIDList[k];
    if( taskList[idx]->numPreds == 0 )
      enqueue( idx, startTaskList, &count );
  }
  return count;
}



/*
 * Determine topological order of the taskList.
 * Updates topoArr with the ordered sequence, and returns the length of topoArr.
int topoSortMSC(chart_t *msc) 
{

  int k, id;
  int toposize;

  int *worklist;
  int worklistlen;

  int *indegree = (int*) MALLOC( indegree, (msc->topoListLen)* sizeof(int), "indegree" );
  for( k = 0; k < msc->topoListLen; k++ )
  {
  	id = msc->topoList[k];
  	 indegree[k] = taskList[id]->numPreds;
  }
  // put start tasks into working list
  worklist = NULL;
  worklistlen = collectStartTasks( taskIDList, numChartTasks, &worklist );
  
 // if( DEBUG ) 
  {
    printf( "%d start tasks:", worklistlen );
    for( k = 0; k < worklistlen; k++ )
      printf( " %d", worklist[k] );
    printf( "\n" ); fflush( stdout );
  }
  

  // start sorting
  toposize = 0;

  //while Q is non-empty do
  while( worklistlen > 0 ) {

    int idx = dequeue( &worklist, &worklistlen );
    task_t *tx = taskList[idx];

    // insert curr into topoArr
    enqueue( idx, topoArr, &toposize );

    // for each successor of curr
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      // account for this edge
      indegree[idk]--;

      // if idk has no other incoming edges, insert it into worklist
      if( indegree[idk] == 0 )
	enqueue( idk, &worklist, &worklistlen );
    }
  }
  free( indegree );
  free( worklist ); 

  return toposize;
}
*/



/*
 * Determine topological order of the taskList.
 * Updates topoArr with the ordered sequence, and returns the length of topoArr.
 */
int topoSort( int *taskIDList, int numChartTasks, int **topoArr ) {

  int k;
  int toposize;

  int *worklist;
  int worklistlen;

  int *indegree = (int*) MALLOC( indegree, numTasks * sizeof(int), "indegree" );
  for( k = 0; k < numTasks; k++ )
    indegree[k] = taskList[k]->numPreds;

  // put start tasks into working list
  worklist = NULL;
  worklistlen = collectStartTasks( taskIDList, numChartTasks, &worklist );
  /*
  if( DEBUG ) {
    printf( "%d start tasks:", worklistlen );
    for( k = 0; k < worklistlen; k++ )
      printf( " %d", worklist[k] );
    printf( "\n" ); fflush( stdout );
  }
  */

  // start sorting
  toposize = 0;

  //while Q is non-empty do
  while( worklistlen > 0 ) {

    int idx = dequeue( &worklist, &worklistlen );
    task_t *tx = taskList[idx];

    // insert curr into topoArr
    enqueue( idx, topoArr, &toposize );

    // for each successor of curr
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      // account for this edge
      indegree[idk]--;

      // if idk has no other incoming edges, insert it into worklist
      if( indegree[idk] == 0 )
	enqueue( idk, &worklist, &worklistlen );
    }
  }
  free( indegree );
  free( worklist ); 

  return toposize;
}


/*
 * Sort topoLists by topological order.
 */
int topoTask() {

  int i, k;

  for( i = 0; i < numCharts; i++ ) {
    int *topoArr = NULL;
    int toposize = topoSort( msg[i].topoList, msg[i].topoListLen, &topoArr );
    if( toposize != msg[i].topoListLen )
      printf( "WARNING: Detected %d isolated tasks in chart %d\n", (toposize - msg[i].topoListLen), i );

    // update with the sorted list
    for( k = 0; k < toposize; k++ )
      msg[i].topoList[k] = topoArr[k];
    free( topoArr );
  }
  return 0;
}


/*
 * Sort MSG nodes by topological order.
 */
int topoGraph() {

  int i, k;
  int toposize;

  int *worklist;
  int worklistlen;

  int *indegree = (int*) CALLOC( indegree, numCharts, sizeof(int), "indegree" );
  for( k = 0; k < numCharts; k++ ) {
    for( i = 0; i < msg[k].numSuccs; i++ ) {
      // don't count the back-edges
      if( msg[k].succList[i] > k )
        indegree[msg[k].succList[i]]++;
    }
  }

  worklist = NULL;
  worklistlen = 0;
  //enqueue( 0, &worklist, &worklistlen );
  enqueue( startNode, &worklist, &worklistlen );

  // start sorting
  toposize = 0;
  topoMSG = NULL;

  //while Q is non-empty do
  while( worklistlen > 0 ) {

    int idx = dequeue( &worklist, &worklistlen );
    chart_t *tx = &(msg[idx]);

    // insert curr into topoMSG
    enqueue( idx, &topoMSG, &toposize );

    // for each successor of curr
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      // skip the back-edges
      if( idk < idx )
	continue;

      // account for this edge
      indegree[idk]--;

      // if idk has no other incoming edges, insert it into worklist
      if( indegree[idk] == 0 )
	enqueue( idk, &worklist, &worklistlen );
    }
  }
  free( indegree );
  free( worklist ); 

  return toposize;
}


/*
 * Determine reverse topological order of the task dependency graph rooted at start.
 * Updates topoArr with the ordered sequence, and returns the length of topoArr.
 */
int topoSortSubgraph( int startid, int **topoArr ) {

  int i, idx;
  char fin;

  char *markfin = (char*) CALLOC( markfin, numTasks, sizeof(char), "markfin" );
  // markfin: 0: unvisited, 1: visited but unfinished, 2: finished

  int *comefrom = NULL;
  // keep the task from which we came, to return to

  int comefromsize = 0;
  int toposize = 0;

  idx = startid;
  while( 1 ) {

    task_t *tc = taskList[idx];

    //printTask_dep( idx );

    fin = 1;
    for( i = 0; fin && i < tc->numSuccs; i++ )
      if( markfin[tc->succList[i]] != 2 )
	fin = 0;

    if( !tc->numSuccs || fin ) {  // finished
      markfin[idx] = 2;

      enqueue( idx, topoArr, &toposize );
      //printf( "--- done, added to topo\n" ); fflush(stdout);

      // returning
      if( !comefromsize )
	break;

      idx = comefrom[ --comefromsize ];
      continue;
    }

    markfin[idx] = 1;
    enqueue( idx, &comefrom, &comefromsize );

    // go to next unvisited task
    fin = 1;
    for( i = 0; fin && i < tc->numSuccs; i++ ) {
      int sx = tc->succList[i];
      if( markfin[sx] != 2 ) {
	idx = sx;
	fin = 0;
	//printf( "- succ %d %s queued\n", sx, getTaskName(sx) ); fflush(stdout);
      }
    }
    if( fin )
      printf( "Unvisited task not detected.\n" ), exit(1);

  } // end while

  free( comefrom );
  free( markfin );

  return toposize;
}
