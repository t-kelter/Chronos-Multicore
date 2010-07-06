#include <stdlib.h>

#include "cluster.h"
#include "util.h"
#include "dump.h"
#include "parse.h"

/*
 * Do clustering by latestTimes.
 */
char doClustering( sched_t *sc, chart_t *msc ) {

  char changed = 0;

  int i, k, m, x;
  int idx, groupid;
  char updated;

  // group tasks with overlapping lifetime to share the SPM
  // create this grouping in a new space to compare with existing one (if any)
  int numOverlays;
  int *numOwnerTasks = NULL;
  int **ownerTaskList = NULL;
  time_t *tmStartList = NULL;
  time_t *tmEndList = NULL;


  // first task starts the first group
  idx = sc->assignedList[0];

  numOverlays = 1;
  numOwnerTasks = (int*)  REALLOC( numOwnerTasks, sizeof(int),  "numOwnerTasks" );
  ownerTaskList = (int**) REALLOC( ownerTaskList, sizeof(int*), "ownerTaskList" );
  tmStartList = (time_t*) REALLOC( tmStartList, sizeof(time_t), "tmStartList" );
  tmEndList   = (time_t*) REALLOC( tmEndList,   sizeof(time_t), "tmEndList" );

  numOwnerTasks[0] = 1;
  ownerTaskList[0] = (int*) MALLOC( ownerTaskList[0], sizeof(int), "ownerTaskList[0]" );
  ownerTaskList[0][0] = idx;
  tmStartList[0] = latestReq[idx];
  tmEndList[0]   = latestFin[idx];

  // the rest
  for( i = 1; i < sc->numAssigned; i++ ) {
    idx = sc->assignedList[i];

    // find the overlapping group
    groupid = -1;
    for( k = 0; groupid == -1 && k < numOverlays; k++ ) {
      if( numOwnerTasks[k] <= 0 );
      else if( latestReq[idx] >= tmEndList[k] );
      else if( latestFin[idx] <= tmStartList[k] );
      else
	groupid = k;
    }
    if( groupid == -1 ) {
      // start a new group

      groupid = numOverlays;
      numOverlays++;

      numOwnerTasks = (int*)  REALLOC( numOwnerTasks, numOverlays * sizeof(int), "numOwnerTasks" );
      ownerTaskList = (int**) REALLOC( ownerTaskList, numOverlays * sizeof(int*), "ownerTaskList" );
      tmStartList = (time_t*) REALLOC( tmStartList, numOverlays * sizeof(time_t), "tmStartList" );
      tmEndList   = (time_t*) REALLOC( tmEndList,   numOverlays * sizeof(time_t), "tmEndList" );

      numOwnerTasks[groupid] = 1;
      ownerTaskList[groupid] = (int*) MALLOC( ownerTaskList[groupid], sizeof(int), "ownerTaskList[.]" );
      ownerTaskList[groupid][0] = idx;
      tmStartList[groupid] = latestReq[idx];
      tmEndList  [groupid] = latestFin[idx];

      // printf( "%s [%8Lu,%8Lu) in new group %d [%8Lu,%8Lu)\n",
      //      getTaskName(idx), latestReq[idx], latestFin[idx], groupid, tmStartList[groupid], tmEndList[groupid] ); fflush( stdout );
    }
    else {
      char extended = 0;

      // printf( "%s [%8Lu,%8Lu) assigned to group %d [%8Lu,%8Lu)\n",
      //     getTaskName(idx), latestReq[idx], latestFin[idx], groupid, tmStartList[groupid], tmEndList[groupid] ); fflush( stdout );

      // insert into groupid
      enqueue( idx, &(ownerTaskList[groupid]), &(numOwnerTasks[groupid]) );

      // update interval
      if( tmStartList[groupid] > latestReq[idx] ) {
	tmStartList[groupid] = latestReq[idx];
	extended = 1;
      }
      if( tmEndList[groupid] < latestFin[idx] ) {
	tmEndList[groupid] = latestFin[idx];
	extended = 1;
      }
      if( !extended )
	continue;

      // printf( "group %d boundary extended to [%8Lu,%8Lu)\n", groupid, tmStartList[groupid], tmEndList[groupid] ); fflush( stdout );

      // check if this affects the other groups
      updated = 1;
      while( updated ) {
	updated = 0;
	for( k = 0; k < numOverlays; k++ ) {
	  if( k == groupid || numOwnerTasks[k] <= 0 )
	    continue;
	  if( tmStartList[groupid] >= tmEndList[k] )
	    continue;
	  if( tmEndList[groupid] <= tmStartList[k] )
	    continue;

	  // merge: move all tasks in group k into groupid
	  for( m = 0; m < numOwnerTasks[k]; m++ )
	    enqueue( ownerTaskList[k][m], &(ownerTaskList[groupid]), &(numOwnerTasks[groupid]) );

	  // printf( "group %d [%8Lu,%8Lu) merged into group %d [%8Lu,%8Lu)\n",
	  //  k, tmStartList[k], tmEndList[k], groupid, tmStartList[groupid], tmEndList[groupid] ); fflush( stdout );

	  // update interval
	  extended = 0;
	  if( tmStartList[groupid] > tmStartList[k] ) {
	    tmStartList[groupid] = tmStartList[k];
	    extended = 1;
	  }
	  if( tmEndList[groupid] < tmEndList[k] ) {
	    tmEndList[groupid] = tmEndList[k];
	    extended = 1;
	  }
	  // if( extended )
	  // printf( "group %d boundary extended to [%8Lu,%8Lu)\n", groupid, tmStartList[groupid], tmEndList[groupid] ); fflush( stdout );

	  free( ownerTaskList[k] );
	  ownerTaskList[k] = NULL;
	  numOwnerTasks[k] = 0;
	  tmStartList[k] = 0;
	  tmEndList  [k] = 0;

	  updated = 1;
	}
      } // end while( updated )
    } // end group assignment

    // sort taskList by index (to facilitate comparison)
    updated = 1;
    while( updated ) {
      updated = 0;
      for( k = 0; k < numOwnerTasks[groupid]-1; k++ ) {
	if( ownerTaskList[groupid][k] > ownerTaskList[groupid][k+1] ) {
	  int tmp                     = ownerTaskList[groupid][k];
	  ownerTaskList[groupid][k]   = ownerTaskList[groupid][k+1];
	  ownerTaskList[groupid][k+1] = tmp;
	  updated = 1;
	}
      }
    }
  } // end for all tasks

  // sort groups by start times (to facilitate comparison)
  updated = 1;
  while( updated ) {
    updated = 0;
    for( i = 0; i < numOverlays-1; i++ ) {
      if( tmStartList[i] < tmStartList[i+1] ) {
	int    tmpNum   = numOwnerTasks[i];
	int    *tmpPtr  = ownerTaskList[i];
	time_t tmpStart = tmStartList[i];
	time_t tmpEnd   = tmEndList  [i];

	// swap
	numOwnerTasks[i] = numOwnerTasks[i+1];
	ownerTaskList[i] = ownerTaskList[i+1];
	tmStartList[i] = tmStartList[i+1];
	tmEndList  [i] = tmEndList  [i+1];

	numOwnerTasks[i+1] = tmpNum;
	ownerTaskList[i+1] = tmpPtr;
	tmStartList[i+1] = tmpStart;
	tmEndList  [i+1] = tmpEnd;

	updated = 1;
      }
    }
  }
  // remove invalidated groups
  k = numOverlays - 1;
  while( k >= 0 && numOwnerTasks[k] <= 0 ) {
    numOverlays--;
    k--;
  }

  for( x = 0; x < numOverlays; x++ ) {
    printf( "group%d: [%8Lu, %8Lu):\n", x, tmStartList[x], tmEndList[x] );
    for( k = 0; k < numOwnerTasks[x]; k++ ) {
      // int id = ownerTaskList[x][k];
      // printf( "\n%s\t[%8Lu,%8Lu)", getTaskName(id), latestReq[id], latestFin[id] );
      printf( "------------------------------ %s\n", getTaskName( ownerTaskList[x][k] ));
    }
    printf( "\n" );
  }

  // check changes to previous grouping
  if( numOverlays != sc->spm->numOverlays )
    changed = 1;

  for( i = 0; !changed && i < numOverlays; i++ ) {
    if( numOwnerTasks[i] != sc->spm->overlayList[i]->numOwnerTasks )
      changed = 1;
    for( k = 0; !changed && k < numOwnerTasks[i]; k++ ) {
      if( ownerTaskList[i][k] != sc->spm->overlayList[i]->ownerTaskList[k] )
	changed = 1;
    }
  }
  if( !changed )
    return 0;

  // update the grouping
  freeAlloc( sc->spm );
  sc->spm->numOverlays = numOverlays;
  sc->spm->overlayList = (overlay_t**) MALLOC( sc->spm->overlayList, numOverlays * sizeof(overlay_t*), "overlayList" );
  for( i = 0; i < numOverlays; i++ ) {
    overlay_t *ox;
    sc->spm->overlayList[i] = (overlay_t*) MALLOC( sc->spm->overlayList[i], numOverlays * sizeof(overlay_t), "overlayList[i]" );
    ox = sc->spm->overlayList[i];

    ox->tmStart = tmStartList[i];
    ox->tmEnd   = tmEndList[i];
    ox->numOwnerTasks = numOwnerTasks[i];
    ox->ownerTaskList = (int*) MALLOC( ox->ownerTaskList, ox->numOwnerTasks * sizeof(int), "ox->ownerTaskList" );
    for( k = 0; k < ox->numOwnerTasks; k++ )
      ox->ownerTaskList[k] = ownerTaskList[i][k];

    /*
    if( !GREEDY && allocmethod == GRAPH_COLORING ) {
      ox->interfere = (char**) MALLOC( ox->interfere, ox->numOwnerTasks * sizeof(char*), "ox->interfere" );
      for( k = 0; k < ox->numOwnerTasks; k++ ) {
	ox->interfere[k] = (char*) MALLOC( ox->interfere[k], ox->numOwnerTasks * sizeof(char), "ox->interfere[k]" );
      }
    }
    else
      ox->interfere = NULL;
    */
    free( ownerTaskList[i] );
  }
  free( ownerTaskList );
  free( numOwnerTasks );
  free( tmStartList );
  free( tmEndList );

  return changed;
}
