#include "knapsack.c"


/*
 * Use for code SPM.
 */
int doAllocationILP( chart_t *msc, overlay_t *ox, int capacity ) {

  int i;
  FILE *fptr;

  int k, idx;
  int fnid, bbid, addr, flag;
  char tname[MAXLEN];

  task_t *tx;

  // for tracking purpose
  int *taskshare = (int*) CALLOC( taskshare, ox->numOwnerTasks, sizeof(int), "taskshare" );

  // reset block realsize
  for( i = 0; i < ox->numOwnerTasks; i++ ) {
    task_t *tc = taskList[ox->ownerTaskList[i]];
    for( k = 0; k < tc->numMemBlocks; k++ )
      tc->memBlockList[k]->realsize = tc->memBlockList[k]->size;
  }

  if( ox->numOwnerTasks <= 0 ) {
    printf( "No task assigned.\n" );
    return -1;
  }

  // prepare parameter file
  fptr = wcrt_openfile( "doalloc.par", "w" );
  fprintf( fptr, "insnsize %d\n", INSN_SIZE );
  fprintf( fptr, "fetchsize %d\n", FETCH_SIZE );
  fprintf( fptr, "latency_spm %d\n", spm_latency );
  fprintf( fptr, "latency_off %d\n", off_latency );
  fprintf( fptr, "capacity %d\n", capacity );
  fclose( fptr );

  // prepare list of tasks
  fptr = wcrt_openfile( "doalloc.tsk", "w" );
  fprintf( fptr, "%d\n", ox->numOwnerTasks );

  if( ox->numOwnerTasks == 1 ) {
    task_t *ti = taskList[ox->ownerTaskList[0]];
    fprintf( fptr, "%s %Lu %Lu 1.0\n", ti->tname, ti->ctimeHi, ti->period );
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
      fprintf( fptr, "%s %Lu %Lu %lf\n", ti->tname, ti->ctimeHi, ti->period, adjust );
    }
    fclose( fptr );

    // this is to induce determinism in allocation result across different schemes
    system( "sort doalloc.tsk > tmp; mv tmp doalloc.tsk" );
  }

  // call allocation routine
  system( "spm doalloc doalloc 1 > ilptrace" );

  // sanity check
  fptr = wcrt_openfile( "doalloc.alloc", "r" );
  if( fscanf( fptr, "%s %d %d %x", tname, &fnid, &bbid, &addr ) == EOF ) {
    // rerun with induced solution
    fclose( fptr );
    printf( "Optimal solution failed -- using induced solution.\n" );
    system( "spmInduce doalloc doalloc 1 > ilptrace\n" );
  }
  else
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
    printf( "%s:\t%4d/%4d bytes\n", getTaskName(ox->ownerTaskList[i]), taskshare[i], capacity );

  free( taskshare );

  //system( "rm doalloc.*" );

  return 0;
}

/*
 * Use for data SPM.
 */
int doAllocationKnapsack( overlay_t *ox, int capacity ) {

  int num;
  int *gain;
  int *weight;
  char *alloc;

  int i, k, last;

  int *startIndex = (int*) MALLOC( startIndex, ox->numOwnerTasks * sizeof(int), "startIndex" );

  num = 0;
  for( i = 0; i < ox->numOwnerTasks; i++ ) {
    startIndex[i] = num;
    num += taskList[ox->ownerTaskList[i]]->numMemBlocks;
  }

  gain   = (int*) MALLOC( gain, num * sizeof(int), "gain" );
  weight = (int*) MALLOC( weight, num * sizeof(int), "weight" );
  alloc  = (char*) CALLOC( alloc, num, sizeof(char), "alloc" );

  last = 0;
  for( i = 0; i < ox->numOwnerTasks; i++ ) {
    task_t *tc = taskList[ox->ownerTaskList[i]];
    for( k = 0; k < tc->numMemBlocks; k++ ) {
      mem_t *mt = tc->memBlockList[k];
      weight[last+k] = mt->size;
      gain[last+k] = mt->freq;
    }
    last += tc->numMemBlocks;
  }

  DP_knapsack( capacity, num, gain, weight, alloc );

  //ox->numMemBlocks = 0;
  //ox->memBlockList = NULL;

  for( i = 0; i < num; i++ ) {
    if( alloc[i] ) {
      task_t *tx;
      int idx;

      // find the owner of this block
      k = ox->numOwnerTasks - 1;
      while( k > 0 && startIndex[k] > i )
	k--;

      tx = taskList[ox->ownerTaskList[k]];
      idx = i - startIndex[k];
      tx->allocated[idx] = 1;

      //ox->numMemBlocks++;
      //ox->memBlockList = (mem_t**) REALLOC( ox->memBlockList, ox->numMemBlocks * sizeof(mem_t*), "ox->memBlockList" );
      //ox->memBlockList[ox->numMemBlocks-1] = &(tx->memBlockList[idx]);
    }
  }
  free( gain );
  free( weight );
  free( alloc );

  return 0;
}


int doAllocation( chart_t *msc, overlay_t *ox, int capacity ) {

  int i, k;

  // reset allocation
  for( i = 0; i < ox->numOwnerTasks; i++ ) {
    int idx = ox->ownerTaskList[i];
    for( k = 0; k < taskList[idx]->numMemBlocks; k++ )
      taskList[idx]->allocated[k] = 0;
  }
  if( capacity <= 0 )
    return -1;

  return doAllocationILP( msc, ox, capacity );
}
