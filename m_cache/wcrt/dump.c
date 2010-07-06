#include <stdio.h>
#include <string.h>

#include "dump.h"

char* extractTaskName( char *fullname ) {

  char *c = strrchr( fullname, '/' );
  if( c != NULL )
    return c + 1;
  return fullname;
}


/*
 * Returns the name of the task, trimmed if it contains full path.
 */
char* getTaskName( int pidx ) {

  return extractTaskName( taskList[pidx]->tname );
}


/*
char* getTaskPath( int pidx ) {

  char *c = strrchr( taskList[pidx]->tname, '/' ) + 1;
  char *path;
  int len = 0;

  if( c == NULL ) {
    char *ptr = &(taskList[pidx]->tname[0]);
    while( ptr != c ) {
      ptr++;
      len++;
    }
  }
  path = (char*) MALLOC( path, (len+1) * sizeof(char), "path" );
  if( len > 0 )
    strncpy( path, taskList[pidx]->tname, len );
  path[len] = '\0';
  return path;
}
*/


int printTask( int pidx ) {

  int k;
  task_t *tc = taskList[pidx];

  printf( "%s\t Actor:%d  PE:%d  Priority:%d  Period:%Lu",
	  getTaskName(pidx), tc->actorID, tc->peID, tc->priority, tc->period );

  if( tc->ctimeLo > 0 || tc->ctimeHi > 0 )
    printf( "  Time:[%6Lu,%6Lu)", tc->ctimeLo, tc->ctimeHi );

  printf( "  <--(" );  fflush( stdout );
  for( k = 0; k < tc->numPreds; k++ )
    printf( " %s", getTaskName( tc->predList[k] ));

  printf( " ) -->(" ); fflush( stdout );
  for( k = 0; k < tc->numSuccs; k++ )
    printf( " %s", getTaskName( tc->succList[k] ));

  printf( " )\n" );

  return 0;
}


int printPE( int peID, int spmCapacity, sched_t *sc ) {
  
  int k;

  printf( "PE %d (%d bytes SPM):\n", peID, spmCapacity );
  for( k = 0; k < sc->numAssigned; k++ )
    printf( "- %s\n", getTaskName( sc->assignedList[k] ));
  printf( "\n" );

  return 0;
}


int printTaskList( chart_t *msc ) {

  int i;
  for( i = 0; i < msc->topoListLen; i++ )
    printf( "- %s\n", getTaskName( msc->topoList[i] ));
  printf( "\n" );

  return 0;
}


int dumpTaskInfo() {

  int i, k;

  printf( "=======================\n" );
  printf( "%d tasks in list\n", numTasks );
  printf( "=======================\n" );

  for( i = 0; i < numTasks; i++ )
    printTask( i );

  printf( "\nMSG in topological order:\n" );
  for( k = 0; k < numCharts; k++ ) {
    int id = topoMSG[k];

    printf( "Chart %d -->(", id );
    for( i = 0; i < msg[id].numSuccs; i++ )
      printf( " %d", msg[id].succList[i] );
    printf( " ):\n" );

    printTaskList( &(msg[id]) );

    for( i = 0; i < numPEs; i++ )
      printPE( peID[i], spmCapacity[i], msg[id].PEList[i] );
    printf( "\n" );
  }
  printf( "\n" );

  return 0;
}


int printTask_dep( int pidx ) {

  int k;
  task_t *tc = taskList[pidx];

  printf( "[%d]%s\t ", pidx, getTaskName(pidx) );

  printf( "  <--(" );  fflush( stdout );
  for( k = 0; k < tc->numPreds; k++ )
    printf( " [%d]%s", tc->predList[k], getTaskName(tc->predList[k]) );

  printf( " ) -->(" ); fflush( stdout );
  for( k = 0; k < tc->numSuccs; k++ )
    printf( " [%d]%s", tc->succList[k], getTaskName(tc->succList[k]) );

  printf( " )\n" );
  return 0;
}

int dumpTaskInfo_dep() {

  int i;

  printf( "\n=======================\n" );
  printf( "%d tasks in list\n", numTasks );
  printf( "=======================\n" );

  for( i = 0; i < numTasks; i++ )
    printTask_dep( i );
  printf( "\n" );

  return 0;
}


int printTimes( int idx ) {

  //int i;
  printf( "%s\t Req[%Lu,%Lu)  Fin[%Lu,%Lu)\n", getTaskName(idx),
	  earliestReq[idx], latestReq[idx], earliestFin[idx], latestFin[idx] );

  /*
  for( i = 0; i < msg[pChart[idx]].topoListLen; i++ ) {
    int idk = msg[pChart[idx]].topoList[i];
    if( idk != idx )
      printf( "(%s\t, %s)\t phaseReq:%Lu  phaseFin:%Lu  maxSepReqFin:%Lu\n",
	      getTaskName(idx), getTaskName(idk),
	      phaseReq[idx][idk], phaseFin[idx][idk], maxSepReqFin[idx][idk] );
  }
  */
  return 0;
}


int printMem( int pidx, char verbose ) {

  int i, sum = 0;
  task_t *tc = taskList[pidx];

  printf( "%s", getTaskName(pidx) );
  if( verbose )
    printf( "\n" );
  for( i = 0; i < tc->numMemBlocks; i++ ) {
    mem_t *mt = tc->memBlockList[i];
    sum += mt->size;
    if( verbose )
      printf( "[%d][%d] %x %d %d (%d)\n", mt->fnid, mt->bbid, mt->addr, mt->size, mt->freq, tc->allocated[i] );
  }
  printf( ":\t%5d bytes\n", sum );

  return 0;
}
