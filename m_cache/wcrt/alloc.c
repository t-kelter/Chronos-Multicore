#include "cluster.c"
#include "selectcontent.c"
#include "graphcoloring.c"
#include "slacks.c"



/*
 * Determine lower bound of a task's runtime by letting it have the whole SPM capacity.
 */
int firstAlloc() {

  int i, pidx;
  overlay_t ox;

  ox.numOwnerTasks = 1;
  ox.ownerTaskList = (int*) MALLOC( ox.ownerTaskList, sizeof(int), "ownerTaskList" );

  for( i = 0; i < numTasks; i++ ) {
    pidx = findPE( taskList[i]->peID );
    ox.ownerTaskList[0] = i;
    if( pidx != -1 && spmCapacity[pidx] > 0 ) {
      doAllocation( NULL, &ox, spmCapacity[pidx] );
      //free( ox.memBlockList );
    }
  }
  free( ox.ownerTaskList );

  return 0;
}


int resetAllocation( chart_t *msc ) {

  int i, k, pidx;
  overlay_t ox;
  time_t wcet, bcet;

  ox.numOwnerTasks = 1;
  ox.ownerTaskList = (int*) MALLOC( ox.ownerTaskList, sizeof(int), "ownerTaskList" );

  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    task_t *tx = taskList[ix];

    pidx = findPE( tx->peID );
    ox.ownerTaskList[0] = ix;
    if( pidx != -1 && spmCapacity[pidx] > 0 )
      doAllocation( msc, &ox, spmCapacity[pidx] );

    //tx->ctimeLo = analysis( tx );
   //analysis( tx, &wcet, &bcet );
    tx->ctimeLo = bcet;

    for( k = 0; k < tx->numMemBlocks; k++ )
      tx->allocated[k] = 0;

    //tx->ctimeHi = analysis( tx );
    //analysis( tx, &wcet, &bcet );
    tx->ctimeHi = wcet;
  }
  free( ox.ownerTaskList );

  return 0;
}


/*
 * Clustering + graph coloring to determine time-sharing tasks.
 * SPM space is then partitioned to the number of colors.
 */
int updateAllocationGC( sched_t *sc, chart_t *msc, int spmCapacity ) {

  int i;
  char changed;

  // find clustering based on interferences
  changed = doClustering( sc, msc );
  //if( !changed )
  //  return 0;

  // redo allocation
  for( i = 0; i < sc->spm->numOverlays; i++ ) {

    overlay_t ox;
    char *colorAssg;
    int numColors;
    //int *colorShare;

    int numMembers = sc->spm->overlayList[i]->numOwnerTasks;
    int *memberList = sc->spm->overlayList[i]->ownerTaskList;

    printf( "\nDo allocation for %d-th overlay (%d tasks)\n", i, numMembers ); fflush( stdout );

    if( numMembers == 1 ) {
      ox.numOwnerTasks = 1;
      ox.ownerTaskList = (int*) MALLOC( ox.ownerTaskList, sizeof(int), "ownerTaskList" );
      ox.ownerTaskList[0] = memberList[0];
      doAllocation( msc, &ox, spmCapacity );
      free( ox.ownerTaskList );
      continue;
    }
    
    // assign 'colors'
    colorAssg = (char*) MALLOC( colorAssg, numMembers * sizeof(char), "colorAssg" );
    numColors = taskColoring( sc->spm->overlayList[i], &colorAssg );

    // do allocation for tasks considering color assignment
    colorAllocation( msc, sc->spm->overlayList[i], colorAssg, numColors, spmCapacity );
    free( colorAssg );
  }
  return 0;
}


/*
 * Simple clustering--partitioning method.
 */
int updateAllocationIC( sched_t *sc, chart_t *msc, int spmCapacity ) {

  int i;
  char changed;

  // find clustering based on interferences
  changed = doClustering( sc, msc );
  //if( !changed )
  //  return 0;

  // redo allocation
  printf( "\nDo allocation\n" ); fflush( stdout );
  for( i = 0; i < sc->spm->numOverlays; i++ )
    doAllocation( msc, sc->spm->overlayList[i], spmCapacity );

  return 0;
}


/*
 * Simple static knapsack allocation method.
 */
int updateAllocationPK( sched_t *sc, chart_t *msc, int spmCapacity ) {

  int i;
  overlay_t *ox = (overlay_t*) MALLOC( ox, sizeof(overlay_t), "ox" );

  // everyone sharing the SPM
  ox->numOwnerTasks = sc->numAssigned;
  ox->ownerTaskList = (int*) MALLOC( ox->ownerTaskList, ox->numOwnerTasks * sizeof(int), "ownerTaskList" );
  for( i = 0; i < sc->numAssigned; i++ )
    ox->ownerTaskList[i] = sc->assignedList[i];

  doAllocation( msc, ox, spmCapacity );

  // single overlay
  sc->spm->numOverlays = 1;
  sc->spm->overlayList = (overlay_t**) MALLOC( sc->spm->overlayList, sizeof(overlay_t*), "overlayList" );
  sc->spm->overlayList[0] = ox;

  return 0;
}


char updateAllocation( int peID, chart_t *msc ) {

  sched_t *sc = msc->PEList[peID];

  int i;
  char changed;

  double t; // time measurement

  if( sc->numAssigned == 0 )
    return 0;
  if( spmCapacity[peID] <= 0 || sc->spm == NULL )
    return 0;

  printf( "\nStart allocation for PE %d\n\n", peID ); fflush( stdout );

  switch( allocmethod ) {
  case 1: STARTTIME; updateAllocationPK( sc, msc, spmCapacity[peID] ); STOPTIME; break;
  case 2: STARTTIME; updateAllocationIC( sc, msc, spmCapacity[peID] ); STOPTIME; break;
  case 3:
  case 4: STARTTIME; updateAllocationGC( sc, msc, spmCapacity[peID] ); STOPTIME; break;
  default:
    break;
  }

  printf( "\nUpdate runtimes\n" ); fflush( stdout );

  changed = 0;
  for( i = 0; i < sc->numAssigned; i++ ) {

    time_t wcet, bcet;
    task_t *tc = taskList[sc->assignedList[i]];

    //wcet = (int) analysis( tc );
    //analysis( tc, &wcet, &bcet );
    printf( "%s\t%8Lu\t%8Lu\n", tc->tname, bcet, wcet );
    if( tc->ctimeHi != wcet ) {
      if( tc->ctimeHi < wcet )
	printf( ">>>> Deterioration detected: %s ctimeHi %Lu --> %Lu\n", tc->tname, tc->ctimeHi, wcet );
      tc->ctimeHi = wcet;
      changed = 1;
    }
    if( tc->ctimeLo != bcet ) {
      if( tc->ctimeLo < bcet )
	printf( ">>>> Deterioration detected: %s ctimeLo %Lu --> %Lu\n", tc->tname, tc->ctimeLo, bcet );
      tc->ctimeLo = bcet;
      changed = 1;
    }
  }
  return changed;
}


int initAlloc( chart_t *msc ) {

  int i, j;
  for( i = 0; i < numPEs; i++ ) {
    sched_t *sc = msc->PEList[i];
    if( spmCapacity[i] > 0 ) {
      sc->spm = (alloc_t*) MALLOC( sc->spm, sizeof(alloc_t), "spm" );
      sc->spm->numOverlays = 0;
      sc->spm->overlayList = NULL;
    }
  }
  for( i = 0; i < msc->topoListLen; i++ ) {
    task_t *tc = taskList[msc->topoList[i]];
    for( j = 0; j < tc->numMemBlocks; j++ )
      tc->allocated[j] = 0;
  }
  return 0;
}


int timingAllocMSC( chart_t *msc ) {

  int i, step;
  char tmChanged, allocChanged;
  time_t wcrt, lastwcrt;

  //int downtime;
  //time_t lastbestwcrt;

  // initialize allocations
  initAlloc( msc );

  // first analysis
  do {
    tmChanged = doTiming( msc, 0 );
  } while( tmChanged );

  printf( "\nInitial analysis\n" );
  wcrt = computeWCRT( msc );
  msc->wcrt = wcrt;

  findCriticalPath( msc );

  step = 0;

  //downtime = 0;
  //lastbestwcrt = wcrt;

  // outer loop for allocation
  do {
    step++;
    lastwcrt = wcrt;

    // this is not needed as result will be the same as the analysis at the end of previous iteration
    /*
    // normal timing analysis or with slack depending whether allocation is based on interference:
    // if it is, interference must be maintained to keep the task execution time values valid
    do {
      tmChanged = doTiming( CLUSTER_BASED );
    } while( tmChanged );
    */

    /*
    if( CLUSTER_BASED ) {
      printf( "\n" );
      for( i = 0; i < msc->topoListLen; i++ ) {
	int ix = msc->topoList[i];
	int j;
	for( j = i+1; j < msc->topoListLen; j++ ) {
	  int jx = msc->topoList[j];
	  if( interfere[ix][jx] )
	    printf( "Interference: %s -- %s\n", getTaskName(ix), getTaskName(jx) );
	}
      }
      printf( "\n" );
    }
    */

    // allocation
    allocChanged = 0;
    for( i = 0; i < numPEs; i++ )
      allocChanged |= updateAllocation( i, msc );
    
    // timing analysis preserving interference pattern corresponding to allocation result
    do {
      //tmChanged = doTiming( msc, CLUSTER_BASED );
      tmChanged = doTiming( msc, 1 );
    } while( tmChanged );

    printf( "\nAfter analysis round %d\n", step );
    wcrt = computeWCRT( msc );
    
    if( wcrt > lastwcrt ) {
      printf( "Decreasing performance: %Lu -- rolling back to previous decision.\n", wcrt );
      wcrt = lastwcrt;
      break;
    }

    msc->wcrt = wcrt;
    findCriticalPath( msc );

    /*
    if( wcrt > lastbestwcrt ) {
      downtime++;
      if( downtime >= DOWNTIME_TOLERANCE ) {
	printf( "Decreasing performance for %d consecutive iterations. Rolling back to last best decision.\n", downtime );
	wcrt = lastbestwcrt;
	break;
      }
    }
    else
      lastbestwcrt = wcrt;
    */
  } while( (wcrt != lastwcrt || allocChanged) && step < MAXSTEP );

  if( step == MAXSTEP )
    printf( "\n\nIteration limit reached.\n" );
  else
    printf( "\n\nNo more improvement, stopping at step %d\n", step );
  printf( "Final application WCRT: %Lu\n\n", wcrt );

  msc->wcrt = wcrt;

  return 0;
}


int timingAllocMSC_slackCritical( chart_t *msc ) {

  int i, step;
  char tmChanged, allocChanged;
  time_t wcrt, lastwcrt;

  //int downtime;
  //time_t lastbestwcrt;

  // initialize allocations
  initAlloc( msc );

  // first analysis
  do {
    tmChanged = doTiming( msc, 0 );
  } while( tmChanged );

  printf( "\nInitial analysis\n" );
  wcrt = computeWCRT( msc );
  msc->wcrt = wcrt;

  findCriticalPath( msc );

  step = 0;

  //downtime = 0;
  //lastbestwcrt = wcrt;

  // outer loop for allocation
  do {
    step++;
    lastwcrt = wcrt;

    // try to improve interference pattern
    insertSlacks( msc );

    /*
    if( CLUSTER_BASED ) {
      printf( "\n" );
      for( i = 0; i < msc->topoListLen; i++ ) {
	int ix = msc->topoList[i];
	int j;
	for( j = i+1; j < msc->topoListLen; j++ ) {
	  int jx = msc->topoList[j];
	  if( interfere[ix][jx] )
	    printf( "Interference: %s -- %s\n", getTaskName(ix), getTaskName(jx) );
	}
      }
      printf( "\n" );
    }
    */

    // allocation
    allocChanged = 0;
    for( i = 0; i < numPEs; i++ )
      allocChanged |= updateAllocation( i, msc );
    
    // timing analysis preserving interference pattern corresponding to allocation result
    do {
      tmChanged = doTiming( msc, 1 );
    } while( tmChanged );

    printf( "\nAfter analysis round %d\n", step );
    wcrt = computeWCRT( msc );

    if( wcrt > lastwcrt ) {
      printf( "Decreasing performance: %Lu -- rolling back to previous decision.\n", wcrt );
      wcrt = lastwcrt;
      break;
    }

    msc->wcrt = wcrt;
    findCriticalPath( msc );

    /*
    if( wcrt > lastbestwcrt ) {
      downtime++;
      if( downtime >= DOWNTIME_TOLERANCE ) {
	printf( "Decreasing performance for %d consecutive iterations. Rolling back to last best decision.\n", downtime );
	wcrt = lastbestwcrt;
	break;
      }
    }
    else
      lastbestwcrt = wcrt;
    */
  } while( (wcrt != lastwcrt || allocChanged) && step < MAXSTEP );

  if( step == MAXSTEP )
    printf( "\n\nIteration limit reached.\n" );
  else
    printf( "\n\nNo more improvement, stopping at step %d\n", step );
  printf( "Final application WCRT: %Lu\n\n", wcrt );

  msc->wcrt = wcrt;

  return 0;
}
