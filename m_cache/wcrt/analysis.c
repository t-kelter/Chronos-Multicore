int addFreq( task_t *tc, int fnid, int bbid, int freq ) {

  int i;
  for( i = 0; i < tc->numMemBlocks; i++ ) {
    mem_t *mt = tc->memBlockList[i];
    if( mt->fnid == fnid && mt->bbid == bbid ) {
      mt->freq += freq;
      return i;
    }
  }
  printf( "Warning: block %d-%d not found.\n", fnid, bbid );
  return -1;
}


int getNumFetch( int size ) {

  int n = size / FETCH_SIZE;
  if( size % FETCH_SIZE == 0 )
    return n;
  return n+1;
}


int readCost() {

  FILE *fptr;
  char filepath[512];
  int i, j;

/*
  fptr = openfext( tc->tname, "bct", "w" );

  for( i = 0; i < tc->numMemBlocks; i++ ) {
    mem_t *mt = tc->memBlockList[i];

    int cost = (mt->realsize / INSN_SIZE) * INSN_EXEC_TIME;
    int numfetch = getNumFetch( mt->realsize );
    //printf( "0x%x (%d): %d", mt->addr, mt->realsize, cost ); fflush( stdout );

    if( tc->allocated[i] ) {
      cost += numfetch * spm_latency;
      //printf( " + (spm)%d --> %d\n", numfetch * spm_latency, cost ); fflush( stdout );
    }
    else {
      cost += numfetch * off_latency;
      //printf( " + (off)%d --> %d\n", numfetch * off_latency, cost ); fflush( stdout );
    }
    fprintf( fptr, "%x %u\n", mt->addr, cost );
  }
  fclose( fptr );
*/
  /** WCET **/

  //printf( "Calling ianalysis...\n" ); fflush( stdout );
  //sprintf( proc, "ianalysis %s cache.config cache_L2.config 0 1", interferePath);

  //system( proc );
  //printf( "Returned from ianalysis.\n" ); fflush( stdout );

  //sprintf( proc, "grep WCET: %s.wtrace | tail -1 | awk '{print $NF}' > wt", tc->tname );
  //system( proc );
  //for each MSC, read wcet and bcet from corresponding file
for(i = 0; i < numCharts; i++)
{
	sprintf(filepath, "msc%d_wcetbcet_%s", i+1, times_iteration);
  	fptr = wcrt_openfile( filepath, "r" );

	for(j = 0; j < msg[i].topoListLen; j++)
	{
  		fscanf(fptr, "%Lu %Lu \n", &(taskList[msg[i].topoList[j]]->ctimeHi), &(taskList[msg[i].topoList[j]]->ctimeLo));

		//printf("\ntask[%d] = %Lu, %Lu\n", j, taskList[msg[i].topoList[j]]->ctimeHi, taskList[msg[i].topoList[j]]->ctimeLo);
	}
	
	fclose( fptr );
}

/*
//???
  // block frequencies
  // reset
  for( i = 0; i < tc->numMemBlocks; i++ )
    tc->memBlockList[i]->freq = 0;

  sprintf( proc, "sed '/P/d' %s.wpt > %s.bbw", tc->tname, tc->tname );
  system( proc );

  fptr = openfext( tc->tname, "bbw", "r" );
  while( fscanf( fptr, "%d %d %d", &fnid, &bbid, &freq ) != EOF )
    addFreq( tc, fnid, bbid, freq );
  fclose( fptr );

   BCET: just the value; block frequency not needed as we optimize for worst case only 

  //printf( "Calling ianalysisb...\n" ); fflush( stdout );
  //sprintf( proc, "ianalysisb %s 2 0 0 1 > %s.btrace 2> %s.bpt", tc->tname, tc->tname, tc->tname );
  //system( proc );
  //printf( "Returned from ianalysisb.\n" ); fflush( stdout );

  sprintf( proc, "grep BCET: %s.btrace | tail -1 | awk '{print $NF}' > bt", tc->tname );
  system( proc );

  fptr = wcrt_openfile( "bt", "r" );
  fscanf( fptr, "%Lu", &bcetval );
  fclose( fptr );

  //system( "rm wt bt" );

  //printf( "Analysis result: wcet: %Lu\n", wcetval );
  //printf( "Analysis result: bcet: %Lu\n", bcetval );

  *wcet = wcetval;
  *bcet = bcetval;
*/
  //return wcetval;
  return 0;
}
