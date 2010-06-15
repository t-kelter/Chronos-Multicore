#include <stdio.h>
#include <stdlib.h>

#include <math.h>


#define DEBUGTHIS 0
//#define DEBUGTHIS (strstr( getTaskName(idx), "hm-sby2" ) != NULL)


char canPreempt( int suspect, int victim ) {

  // same PE?
  if( taskList[suspect]->peID != taskList[victim]->peID )
    return 0;

  // higher priority?
  if( taskList[suspect]->priority > taskList[victim]->priority )
    return 0;

  // note: the following checks are only valid for single-periodic case

  // dependent?
  if( isPredecessorOf( suspect, victim ) || isPredecessorOf( victim, suspect ))
    return 0;

  // separated?
  if( earliestReq[suspect] > 0 && latestFin[victim] > 0 && earliestReq[suspect] >= latestFin[victim] )
    return 0;
  if( latestFin[suspect] > 0 && earliestReq[victim] > 0 && latestFin[suspect] <= earliestReq[victim] )
    return 0;

  // this is a tentative trick...
  if( earliestReq[suspect] > 0 && earliestFin[victim] > 0 && latestReq[suspect] > 0 && latestFin[victim] > 0
      && earliestReq[suspect] >= earliestFin[victim] && latestReq[suspect] >= latestFin[victim] )
    return 0;
  if( earliestFin[suspect] > 0 && earliestReq[victim] > 0 && latestFin[suspect] > 0 && latestReq[victim] > 0
      && earliestFin[suspect] <= earliestReq[victim] && latestFin[suspect] <= latestReq[victim] )
    return 0;

  return 1;
}



int canConflict( int suspect, int victim ) {

  // same PE?
 //if( taskList[suspect]->peID == taskList[victim]->peID )
 //  return 0;

  // higher priority?
  //if( taskList[suspect]->priority > taskList[victim]->priority )
  //  return 0;

  // note: the following checks are only valid for single-periodic case

  // dependent?
  if(taskList[suspect]->peID == taskList[victim]->peID) return 0;
  if( isPredecessorOf( suspect, victim )) return 0;
  if( isPredecessorOf( victim,  suspect)) return 0;

  // separated?
  if( earliestReq[suspect] > 0 && latestFin[victim] > 0 && earliestReq[suspect] >= latestFin[victim] )
    return 0;

  if( earliestReq[suspect] > 0 && latestFin[victim] > 0 && earliestReq[victim] >= latestFin[suspect] )
    return 0;

  //if( latestFin[suspect] > 0 && earliestReq[victim] > 0 
  //&& latestFin[suspect] <= earliestReq[victim] )
  // return 0;

  // this is a tentative trick...
  if( (latestFin[suspect] > 0 && latestFin[victim] > 0 && earliestReq[victim] > 0) && (latestFin[suspect] > earliestReq[victim])  && (latestFin[suspect] <= latestFin[victim]))
  return 1;

  if( (latestFin[suspect] > 0 && latestFin[victim] > 0 && earliestReq[victim] > 0) && (latestFin[victim] > earliestReq[suspect])  && (latestFin[victim] <= latestFin[suspect]))
  return 1;

  return 0;
}


int findCriticalPath( chart_t *msc ) {

  int i, k, curr;
  int numCritical;

  int maxTask = -1;
  time_t maxFin = 0;

  // reset
  for( i = 0; i < msc->topoListLen; i++ )
    isCritical[ msc->topoList[i] ] = 0;

  // last finishing task
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    if( maxFin < latestFin[ix] ) {
      maxTask = ix;
      maxFin = latestFin[ix];
    }
  }
  if( maxTask > -1 )
    isCritical[maxTask] = 1;
  
  // the latest predecessors
  curr = maxTask;
  while( curr > -1 ) {
    maxTask = -1;
    maxFin = 0;
    for( i = 0; i < taskList[curr]->numPreds; i++ ) {
      int idk = taskList[curr]->predList[i];
      if( maxFin < latestFin[idk] ) {
	maxTask = idk;
	maxFin = latestFin[idk];
      }
    }
    if( maxTask > -1 )
      isCritical[maxTask] = 1;
    curr = maxTask;
  }

  // possibly preempting tasks
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    if( !isCritical[ix] )
      continue;
    for( k = 0; k < msc->topoListLen; k++ ) {
      int kx = msc->topoList[k];
      if( !isCritical[kx] && canPreempt(kx,ix) )
	isCritical[kx] = 2;
    }
  }

  numCritical = 0;
  //printf( "Critical tasks:\n" );
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    if( isCritical[ix] > 0 ) {
      numCritical++;
      //printf( "- %s (%d)\n", getTaskName(ix), isCritical[ix] );
    }
  }

  allocweight = numCritical / (double) msc->topoListLen;
  //printf( "Critical path density: %lf\n", allocweight );

  return 0;
}


time_t gxCalcHi( chart_t *msc, int idx, time_t x ) {

  int k;
  time_t interrupt = 0;

  //FILE *fdbg = openfext( taskList[idx]->tname, "prmu1", "w" );
  //fprintf( fdbg, "%s [%8Lu, %8Lu)\n", getTaskName(idx), earliestReq[idx], latestFin[idx] );

  for( k = 0; k < msc->topoListLen; k++ ) {
    int kx = msc->topoList[k];
    task_t *tc = taskList[kx];
    if( kx != idx && canPreempt( kx, idx )) {

      // single-periodic
      interrupt += tc->ctimeHi;
      //fprintf( fdbg, "+ %s [tmHi]%6d [tot]%d [%8Lu, %8Lu)\n", getTaskName(kx), tc->ctimeHi, interrupt, earliestReq[kx], latestFin[kx] );
    }
  }
  //fclose( fdbg );
  // printf( "[%s] ctimeHi: %6Lu; interrupt: %6Lu\n", getTaskName(idx), taskList[idx]->ctimeHi, interrupt );
  return (taskList[idx]->ctimeHi + interrupt);
}


time_t gxCalcLo( chart_t *msc, int idx, time_t x ) {

  int k;
  time_t interrupt = 0;

  //FILE *fdbg = openfext( taskList[idx]->tname, "prml1", "w" );
  //fprintf( fdbg, "%s [%8Lu, %8Lu)\n", getTaskName(idx), earliestReq[idx], latestFin[idx] );

  for( k = 0; k < msc->topoListLen; k++ ) {
    int kx = msc->topoList[k];
    task_t *tc = taskList[kx];
    if( kx != idx && canPreempt( kx, idx )) {

      // single-periodic
      interrupt += tc->ctimeLo;
      //fprintf( fdbg, "+ %s [tmLo]%6Lu [tot]%Lu [%8Lu, %8Lu)\n", getTaskName(kx), tc->ctimeLo, interrupt, earliestReq[kx], latestFin[kx] );
    }
  }
  //fclose( fdbg );
  // printf( "[%s] ctimeLo: %6Lu; interrupt: %6Lu\n", getTaskName(idx), taskList[idx]->ctimeLo, interrupt );
  return (taskList[idx]->ctimeLo + interrupt);
}


time_t fixedPointCalcHi( chart_t *msc, int idx ) {

  int k;
  time_t x, gx;
  double util = 0;

  for( k = 0; k < msc->topoListLen; k++ ) {
    int kx = msc->topoList[k];
    task_t *tc = taskList[kx];
    if( kx != idx && canPreempt( kx, idx )) {
      util += (tc->ctimeHi / (double) tc->period);
      //if( DEBUGTHIS ) printf( "~ %s [ctimeHi]%6Lu [period]%6Lu [util]%6f\n", getTaskName(kx), tc->ctimeHi, tc->period, util );
    }
  }
  if( util >= 1 )
    printf( "\nSchedule is infeasible.\n\n" ), exit(1);

  x = (time_t) ceil( taskList[idx]->ctimeHi / (1 - util));

  gx = gxCalcHi( msc, idx, x );

  while( x < gx ) {
    x = gx;
    gx = gxCalcHi( msc, idx, x );
  }
  return x;
}


time_t fixedPointCalcLo( chart_t *msc, int idx ) {

  int k;
  time_t x, gx;
  double util = 0;

  for( k = 0; k < msc->topoListLen; k++ ) {
    int kx = msc->topoList[k];
    task_t *tc = taskList[kx];
    if( kx != idx && canPreempt( kx, idx )) {
      util += (tc->ctimeLo / (double) tc->period);
      //if( DEBUGTHIS ) printf( "~ %s [ctimeLo]%6Lu [period]%6Lu [util]%6f\n", getTaskName(kx), tc->ctimeLo, tc->period, util );
    }
  }
  if( util >= 1 )
    printf( "\nSchedule is infeasible.\n\n" ), exit(1);

  x = (time_t) floor( taskList[idx]->ctimeLo / (1 - util));

  gx = gxCalcLo( msc, idx, x );

  while( x < gx ) {
    x = gx;
    gx = gxCalcLo( msc, idx, x );
  }
  return x;
}


char latestTimes( chart_t *msc, int *topoArr, int toposize ) {

  int i, j, k, idi, idj, flag;
  char changed = 0;

  //FILE *fdbg;

  // reset node start time
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // if the node has a predecessor not in the currently considered list,
    // retain the maximum of the value imposed by the missing predecessor(s)
    time_t val = 1;
    for( k = 0; k < tx->numPreds; k++ ) {
      int idm = tx->predList[k];

     //if( !inList( idm, topoArr, toposize )) 
   {
	if( val < latestFin[idm] ) {
	  val = latestFin[idm];
	  //if( DEBUGTHIS ) printf( "task %s pred %s latestFin: %Lu\n", getTaskName(idx), getTaskName(idm), val );
	}
      }
    }

    // FOR NON-PREEMPTIVE: a task starts after other tasks in the same core which
    // (1) is ready earlier, or (2) is ready at the same time and has higher priority.

/*
     for( k = 0; k < msc->topoListLen; k++ ) {
       int idm = msc->topoList[k];
	   
      if( k == i || taskList[idx]->peID != taskList[idm]->peID )
	continue;
     	  
      if( latestReq[idm] < latestReq[idx] ||
	  latestReq[idm] == latestReq[idx] && taskList[idm]->priority < taskList[idx]->priority ) {
	if( val < latestFin[idm] ) {
	  val = latestFin[idm];
	  //if( DEBUGTHIS ) printf( "task %s pred %s latestFin: %Lu\n", getTaskName(idx), getTaskName(idm), val );
	}
      }
    }
*/	  

    latestReq[idx] = val;
 
 // }


//for( i = 0; i < toposize; i++ ) 
//{

	for(j = 0; j < toposize; j++)
	{
		idj = topoArr[j];
		if(i == j) continue;
		if(peers[idx][idj] == 1)
		{
		    latestReq[idx] = latestReq[idx] + taskList[idj]->ctimeHi;
		}
	}
}


  // compute latest finish time
  for( i = toposize - 1; i >= 0; i-- ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // response time
    time_t wcrt = taskList[idx]->ctimeHi; //fixedPointCalcHi( msc, idx );
    time_t value = latestReq[idx] + wcrt;
    if( latestFin[idx] != value ) {
      latestFin[idx] = value;
      changed = 1;
      //if( DEBUGTHIS ) printf( "task %s wcrt: %Lu latestFin: %Lu\n", getTaskName(idx), wcrt, latestFin[idx] );
    }
    //fdbg = openfext( tx->tname, "prmu1", "a" );
    //fprintf( fdbg, "[%s] wcrt = %Lu\n", getTaskName(idx), wcrt );
    //fprintf( fdbg, "[%s] latestReq = %Lu\n", getTaskName(idx), latestReq[idx] );
    //fprintf( fdbg, "[%s] latestFin = %Lu\n", getTaskName(idx), latestFin[idx] );

    // update successors
    /*
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      if( latestReq[idk] < latestFin[idx] ) {
	latestReq[idk] = latestFin[idx];
	changed = 1;
      }
      //fprintf( fdbg, "--> [%s] latestReq = %Lu\n", getTaskName(idk), latestReq[idk] );
      //if( DEBUGTHIS ) printf( "[%s] --> [%s] latestReq = %Lu\n", getTaskName(idx), getTaskName(idk), latestReq[idk] );

    } // end for succs */

    //fclose( fdbg );
  }
  return changed;
}


char earliestTimes( chart_t *msc, int *topoArr, int toposize ) {

  int i, k;
  char changed = 0;

  //FILE *fdbg;

  // reset node start times
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // if the node has a predecessor not in the currently considered list,
    // retain the maximum of the value imposed by the missing predecessor(s)
    time_t val = 1;
    for( k = 0; k < tx->numPreds; k++ ) {
      int idm = tx->predList[k];

      //if( !inList( idm, topoArr, toposize )) 
      {
	if( val < earliestFin[idm] ) {
	  val = earliestFin[idm];
	  //if( DEBUGTHIS ) printf( "task %s pred %s earliestFin: %Lu\n", getTaskName(idx), getTaskName(idm), val );
	}
      }
    }

    earliestReq[idx] = val;

  // compute earliestFin

    // response time
    time_t wcrt = taskList[idx]->ctimeLo; //fixedPointCalcLo( msc, idx );
    time_t value = earliestReq[idx] + wcrt;
    if( earliestFin[idx] < value ) 
   {
      earliestFin[idx] = value;
      changed = 1;
      //if( DEBUGTHIS ) printf( "task %s wcrt: %Lu earliestFin: %Lu\n", getTaskName(idx), wcrt, earliestFin[idx] );
    }

    //fdbg = openfext( tx->tname, "prml1", "a" );
    //fprintf( fdbg, "[%s] wcrt = %Lu\n", getTaskName(idx), wcrt );
    //fprintf( fdbg, "[%s] earliestReq = %Lu\n", getTaskName(idx), earliestReq[idx] );
    //fprintf( fdbg, "[%s] earliestFin = %Lu\n", getTaskName(idx), earliestFin[idx] );

/*
    // update successors
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      if( earliestReq[idk] < earliestFin[idx] ) {
	earliestReq[idk] = earliestFin[idx];
	changed = 1;
      }
    } // end for succs
  	}
*/
  }
  return changed;
}


/*
 * WCRT of the whole application is defined as:
 *   max{t}(latestFin(t)) - min{t}(earliestReq(t))
 */
time_t computeWCRT( chart_t *msc ) {

  int i;

  char isSet = 0;
  time_t minStart = 1;
  time_t maxFin = 0;

  printf( "\n" );
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];

    //if( allocmethod == NONE )
    printf( "%s:\t[%8Lu/%8Lu, %8Lu/%8Lu)\n", getTaskName(ix), earliestReq[ix], latestReq[ix], earliestFin[ix], latestFin[ix] );
    //else
    //printf( "%s:\t[%8Lu, %8Lu)\n", getTaskName(ix), latestReq[ix], latestFin[ix] );
    if( !isSet || minStart > earliestReq[ix] ) {
      minStart = earliestReq[ix];
      isSet = 1;
    }
    if( maxFin < latestFin[ix] )
      maxFin = latestFin[ix];
  }
  printf( "Execution runtime: [%Lu,%Lu): %Lu\n\n", minStart, maxFin, (maxFin - minStart) );

  return (maxFin - minStart);
}

static char overlap(int i, int j)
{
	if(earliestReq[i] >= earliestReq[j] && earliestReq[i] < latestReq[j])
		return 1;
	else if(latestReq[i] > earliestReq[j] && latestReq[i] <= latestReq[j])
		return 1;
	else return 0;
}

static char
isPred(chart_t *msc, int id, int pred)
{
	int i = 0;
	if(taskList[id]->numPreds == 0)
		return 0;
	for(; i < taskList[id]->numPreds; i++)
	{
		if(taskList[id]->predList[i] == pred)
			return 1;
                  if(isPred(msc, taskList[id]->predList[i], pred))
		    return 1;
	}
	return 0;
}

int timingEstimateMSC( chart_t *msc )
{
	  int i, j, len, step, idi, idj;

	  time_t oldWCRT, newWCRT;

	  step = 0;
	  oldWCRT = 0;
	  newWCRT = 0;
	  len = msc->topoListLen;


	  // reset
	  for( i = 0; i < msc->topoListLen; i++ ) {
	    int id = msc->topoList[i];
	    earliestReq[id] = 1;
	    latestReq  [id] = 1;
	    latestReq_copy[id] = 1;
	    earliestFin[id] = 0;
	    latestFin  [id] = 0;
	  }

	  earliestTimes( msc, msc->topoList, len );
	  latestTimes( msc, msc->topoList, len);

	  do 
	  {
	    /* for( i = 0; i < msc->topoListLen; i++ ) {
	      int id = msc->topoList[i];

	      // determine topological order of dependency graph rooted at id
	      // result is reverse-topological
	      // int *topoArr = NULL; */
	      /*
	      int k;
	      printf( "\nTopological order:" );
	      for( k = toposize - 1; k >= 0; k-- )
				printf( " %s", getTaskName(topoArr[k]) );
	      printf( "\n" );
	      */

	      //printf( "\nAnalyse for %d %s\n", id, getTaskName(id) ); 
			//fflush( stdout );
	      //earliestTimes( msc, topoArr, toposize );
	      //printf( "\nAfter earliestTimes( %s )\n", getTaskName(id) ); 
	      //printTimes( id );
	      //printf( "\nAfter latestTimes( %s )\n", getTaskName(id) ); 
	      //printTimes( id );

	      latestTimes( msc, msc->topoList, len);
		   
			for(i = 0; i < len; i++)
		   {
				idi =  msc->topoList[i];
			   
				for(j = 0; j < len; j++)
				{
					idj =  msc->topoList[j];
					
					if(i == j ||taskList[idi]->peID != taskList[idj]->peID)
					  continue;
					
					else if(!isPred(msc, idj, idi ) && 
					  !isPred(msc, idi, idj ) &&overlap(idi, idj))
					{
					 	peers[idi][idj] = 1;
					   peers[idj][idi] = 1;
				   }
			   }
		  }

		 /* free( topoArr ); */

	    printf( "\nAfter analysis round %d\n", step );
	    newWCRT = computeWCRT( msc );
	    
		 if( newWCRT == oldWCRT )
	      break;

	    /* findCriticalPath( msc ); */

	    oldWCRT = newWCRT;
	    step++;

	   } while( step < MAXSTEP );

	   msc->wcrt = newWCRT;

	   return 0;
}
