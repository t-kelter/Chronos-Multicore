#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int initSched( sched_t *sc ) {

  sc->numAssigned = 0;
  sc->assignedList = NULL;
  sc->spm = NULL;
  return 0;
}


int initChart( chart_t *cx ) {

  int i;
  cx->numSuccs = 0;
  cx->succList = NULL;
  cx->topoListLen = 0;
  cx->topoList = NULL;
  cx->timeTopoList = NULL;
  cx->wcrt = 0;

  cx->PEList = (sched_t**) MALLOC( cx->PEList, numPEs * sizeof(sched_t*), "PEList" );
  for( i = 0; i < numPEs; i++ ) {
    cx->PEList[i] = (sched_t*) MALLOC( cx->PEList[i], sizeof(sched_t), "sched_t" );
    initSched( cx->PEList[i] );
  }
  return 0;
}


/*
 * Returns the index in taskList which corresponds to the task with specified tname;
 * returns -1 if it is not found.
 */
int findTask( char tname[] ) {

  int i;
  for( i = 0; i < numTasks; i++ ) {
    if( strcmp( taskList[i]->tname, tname ) == 0 )
      return i;
  }
  printf( "Warning: task %s not found.\n", tname );
  return -1;
}


/*
 * Returns the index in PEList which corresponds to the PE with specified id;
 * returns -1 if it is not found.
 */
int findPE( int id ) {

  int i;
  for( i = 0; i < numPEs; i++ ) {
    if( peID[i] == id )
      return i;
  }
  return -1;
}


/*
 * Returns the index in topoList of the chart that contains taskList[idx].
 * i.e. finds the task chart that the task belongs to.
 */
int findChart( int idx ) {

  int i, j;
  for( i = 0; i < numCharts; i++ )
    for( j = 0; j < msg[i].topoListLen; j++ ) 
      if( msg[i].topoList[j] == idx )
	return i;
  return -1;
}


/*
 * Recursively determines if task pred is a predecessor of succ.
 */
char isPredecessorOf( int pred, int succ ) {

  int i;
  task_t *ts = taskList[succ];

  if( pred == succ )
    return 0;  // by definition; shouldn't reach this by recursion

  for( i = 0; i < ts->numPreds; i++ ) {
    int px = ts->predList[i];
    if( px == pred )
      return 1;
    if( isPredecessorOf( pred, px ))
      return 1;
  }
  return 0;
}


/*
 * Reads task description file.
 * Format: one task per line
 * <taskID:int> <actorID:int> <peID:int> <priority:int> <period:time_t> <exectime:time_t>
 */
int readTasks() {

  char tname[MAXLEN];
  int actor, pe, priority;
  time_t period;

  FILE *fptr = wcrt_openfile( pdname, "r" );

  numTasks = 0;
  taskList = NULL;

  while( fscanf( fptr, "%s %d %d %d %Lu", tname, &actor, &pe, &priority, &period ) != EOF ) {

    task_t *tc = (task_t*) MALLOC( tc, sizeof(task_t), "tc" );
    strcpy( tc->tname, tname );
    tc->actorID  = actor;
    tc->peID     = pe;
    tc->priority = priority;
    tc->period   = period;
    tc->ctimeLo  = 0;
    tc->ctimeHi  = 0;
    tc->numPreds = 0;
    tc->predList = NULL;
    tc->numSuccs = 0;
    tc->succList = NULL;
    tc->numMemBlocks = 0;
    tc->memBlockList = NULL;
    tc->allocated    = NULL;

    numTasks++;
    taskList = (task_t**) REALLOC( taskList, numTasks * sizeof(task_t*), "taskList" );
    taskList[numTasks-1] = tc;
  }
  fclose( fptr );

  return 0;
}


/*
 * Reads the Message Sequence Graph.
 * Format:
 *
 * <nodeID_1> <numNodeSuccessors_1> <succNodeID_1.1> <succNodeID_1.1.2> ... <succNodeID_1.1.N>
 * <numTasksInChart_1>
 * <taskName_1.1> <numTaskSuccessors_1.1> <succTaskName_1.1.1> <succTaskName_1.1.2> ... <succTaskName_1.1.N>
 * <taskName_1.2> <numTaskSuccessors_1.2> <succTaskName_1.2.1> <succTaskName_1.2.2> ... <succTaskName_1.2.N>
 * ...
 * <nodeID_2> <numNodeSuccessors_2> <succNodeID_2.1> <succNodeID_2.1.2> ... <succNodeID_2.1.N>
 * <numTasksInChart_2>
 * <taskName_2.1> <numTaskSuccessors_2.1> <succTaskName_2.1.1> <succTaskName_2.1.2> ... <succTaskName_2.1.N>
 * <taskName_2.2> <numTaskSuccessors_2.2> <succTaskName_2.2.1> <succTaskName_2.2.2> ... <succTaskName_2.2.N>
 * ...
 */
int readMSG() {

  char tname[MAXLEN];
  int gid, numtasks, numsuccs;
  int pidx, sidx, pex;
  int i, k;
  task_t *tc;
  sched_t *sc;

  FILE *fptr = wcrt_openfile( dgname, "r" );

  numCharts = 0;
  msg = NULL;

  pChart = (int*) MALLOC( pChart, numTasks * sizeof(int), "pChart" );
  for( k = 0; k < numTasks; k++ )
    pChart[k] = -1;

  while( fscanf( fptr, "%d %d", &gid, &numsuccs ) != EOF ) {

    if( numCharts < gid + 1 )
      numCharts = gid + 1;
    msg = (chart_t*) REALLOC( msg, numCharts * sizeof(chart_t), "msg" );

    // successor nodes
    msg[gid].numSuccs = numsuccs;
    msg[gid].succList = (int*) MALLOC( msg[gid].succList, numsuccs * sizeof(int), "msg succList" );
    for( i = 0; i < numsuccs; i++ ) {
      fscanf( fptr, "%d", &sidx );
      msg[gid].succList[i] = sidx;
    }

    // PEs
    msg[gid].PEList = (sched_t**) MALLOC( msg[gid].PEList, numPEs * sizeof(sched_t*), "msg PEList" );
    for( i = 0; i < numPEs; i++ ) {
      msg[gid].PEList[i] = (sched_t*) MALLOC( msg[gid].PEList[i], sizeof(sched_t), "sched" );
      initSched( msg[gid].PEList[i] );
    }

    // tasks
    fscanf( fptr, "%d", &numtasks );
    msg[gid].topoListLen = numtasks;
    msg[gid].topoList = (int*) MALLOC( msg[gid].topoList, numtasks * sizeof(int), "msg topoList" );

    for( i = 0; i < numtasks; i++ ) {
      fscanf( fptr, "%s %d", tname, &numsuccs );

      pidx = findTask( tname );
      if( pidx == -1 )
	printf( "Invalid task %s in chart %d\n", tname, gid ), exit(1);

      msg[gid].topoList[i] = pidx;
      pChart[pidx] = gid;

      tc = taskList[pidx];

      // validate each successor
      for( k = 0; k < numsuccs; k++ ) {
	fscanf( fptr, "%s", (char*)&tname );

	sidx = findTask( tname );
	if( sidx == -1 )
	  printf( "Invalid successor %s\n", tname ), exit(1);

	tc->numSuccs++;
	tc->succList = (int*) REALLOC( tc->succList, tc->numSuccs * sizeof(int), "succList" );
	tc->succList[tc->numSuccs-1] = sidx;
	
	// reverse
	taskList[sidx]->numPreds++;
	taskList[sidx]->predList = (int*)
	  REALLOC( taskList[sidx]->predList, taskList[sidx]->numPreds * sizeof(int), "predList" );
	taskList[sidx]->predList[taskList[sidx]->numPreds-1] = pidx;
      }

      // PE allocation
      pex = findPE( tc->peID );
      if( pex == -1 )
	printf( "Task %s assigned to invalid PE %d.\n", tname, tc->peID ), exit(1);

      sc = msg[gid].PEList[pex];
      sc->numAssigned++;
      sc->assignedList = (int*) REALLOC( sc->assignedList, sc->numAssigned * sizeof(int), "assignedList" );
      sc->assignedList[sc->numAssigned-1] = pidx;

    } // end for tasks
  }
  fclose( fptr );
  return 0;
}


int readEdgeBounds() {

  FILE *fptr = openfext( dgname, "eb", "r" );

  int src, dst, bound;
  char token[24];

  fscanf( fptr, "%s %d", token, &startNode );
  fscanf( fptr, "%s %d", token, &endNode );

  numEdgeBounds = 0;
  edgeBounds = NULL;

  while( fscanf( fptr, "%d %d %d", &src, &dst, &bound ) != EOF ) {
    numEdgeBounds++;
    edgeBounds = (edge_t*) REALLOC( edgeBounds, numEdgeBounds * sizeof(edge_t), "edgeBounds" );
    edgeBounds[numEdgeBounds-1].src = src;
    edgeBounds[numEdgeBounds-1].dst = dst;
    edgeBounds[numEdgeBounds-1].bound = bound;
  }
  fclose( fptr );

  return 0;
}


int readTaskMemoryReq( task_t *tc ) {

  int k;
  FILE *fptr;

  int fn, bb, addr, tb, nb, cf;
  int sa, ea, sm, x;

  fptr = openfext( tc->tname, "cfg", "r" );

  while( fscanf( fptr, "%d %d %x %d %d %d", &fn, &bb, &addr, &tb, &nb, &cf ) != EOF ) {
    mem_t *mt = (mem_t*) MALLOC( mt, sizeof(mem_t), "mt" );
    mt->fnid = fn;
    mt->bbid = bb;
    mt->addr = addr;
    mt->freq = 0;

    tc->numMemBlocks++;
    tc->memBlockList = (mem_t**) REALLOC( tc->memBlockList, tc->numMemBlocks * sizeof(mem_t*), "memAddrList" );
    tc->memBlockList[tc->numMemBlocks-1] = mt;
  }
  fclose( fptr );

  // size inference
  for( k = 1; k < tc->numMemBlocks; k++ )
    tc->memBlockList[k-1]->size = tc->memBlockList[k]->addr - tc->memBlockList[k-1]->addr;

  fptr = openfext( tc->tname, "arg", "r" );
  fscanf( fptr, "%x %x %x %d %d %d %d", &sa, &ea, &sm, &x, &x, &x, &x );
  fclose( fptr );

  tc->memBlockList[tc->numMemBlocks-1]->size = ea - tc->memBlockList[tc->numMemBlocks-1]->addr + INSN_SIZE;

  tc->allocated = (char*) CALLOC( tc->allocated, tc->numMemBlocks, sizeof(char), "allocated" );
  for( k = 0; k < tc->numMemBlocks; k++ )
    tc->memBlockList[k]->realsize = tc->memBlockList[k]->size;

  return 0;
}


/*
 * We require that for each task, there exist a file <tname>.cfg
 * containing the list of basic blocks to allocate into SPM.
 */
int readMemoryReq() {

  int i;
  for( i = 0; i < numTasks; i++ )
    readTaskMemoryReq( taskList[i] );

  return 0;
}


int readConfig() {

  FILE *fptr;
  char tmp[20];
  int pe, capacity = 0;

  numPEs = 0;
  peID = NULL;
  spmCapacity = NULL;
  totalcapacity = 0;

  fptr = wcrt_openfile( cfname, "r" );
  fscanf( fptr, "%s %d %d", tmp, &spm_latency, &off_latency );

  while( fscanf( fptr, "%d %d", &pe, &capacity ) != EOF ) {
    numPEs++;
    peID = (int*) REALLOC( peID, numPEs * sizeof(int), "peID" );
    peID[numPEs-1] = pe;
    spmCapacity = (int*) REALLOC( spmCapacity, numPEs * sizeof(int), "spmCapacity" );
    spmCapacity[numPEs-1] = capacity;

    totalcapacity += capacity;
  }
  fclose( fptr );
  return 0;
}

/*
   Reads the WCET/BECT values per Task from a fixed input file.
 */
void readCost()
{
  FILE *fptr;
  char filepath[512];
  int i, j;

  for(i = 0; i < numCharts; i++) {
    sprintf(filepath, "msc%d_wcetbcet_%d", i+1, times_iteration);
      fptr = wcrt_openfile( filepath, "r" );

    for(j = 0; j < msg[i].topoListLen; j++) {
      fscanf(fptr, "%Lu %Lu \n", &(taskList[msg[i].topoList[j]]->ctimeHi), 
        &(taskList[msg[i].topoList[j]]->ctimeLo));
      //printf("\ntask[%d] = %Lu, %Lu\n", j,
      //  taskList[msg[i].topoList[j]]->ctimeHi,
      //  taskList[msg[i].topoList[j]]->ctimeLo);
    }
    
    fclose( fptr );
  }
}

int freeAlloc( alloc_t *ac ) {

  int i;
  for( i = 0; i < ac->numOverlays; i++ ) {
    free( ac->overlayList[i]->ownerTaskList );
    free( ac->overlayList[i] );
  }
  free( ac->overlayList );

  return 0;
}

int freeSched( sched_t *sc ) {

  if( sc->spm != NULL ) {
    freeAlloc( sc->spm );
    free( sc->spm );
  }
  free( sc->assignedList );

  return 0;
}


int freeTask( task_t *tx ) {

  free( tx->predList );
  free( tx->succList );
  free( tx->memBlockList );
  free( tx->allocated );

  return 0;
}


int freeChart( chart_t *cx ) {

  int i;
  for( i = 0; i < numPEs; i++ ) {
    freeSched( cx->PEList[i] );
    free( cx->PEList[i] );
  }
  free( cx->PEList );
  free( cx->succList );
  free( cx->topoList );
  free( cx->timeTopoList );

  return 0;
}


int wcrt_freePath( path_t *px ) {

  int i;

  freeChart( px->msc );
  free( px->msc );

  for( i = 0; i < numCharts; i++ )
    free( px->edgeBounds[i] );
  free( px->edgeBounds );

  return 0;
}


int freeAll() {

  int i;

  for( i = 0; i < numTasks; i++ ) {
    freeTask( taskList[i] );
    free( taskList[i] );
  }
  free( taskList );
  free( peID );
  free( spmCapacity );

  for( i = 0; i < numCharts; i++ ) {
    freeChart( &(msg[i]) );
    free( &(msg[i]) );
  }
  free( msg );
  free( pChart );

  free( earliestReq );
  free( latestReq );
  free( earliestFin );
  free( latestFin );

  return 0;
}
