#include <assert.h>
#include <stdlib.h>

#include "slacks.h"
#include "timing.h"
#include "topo.h"
#include "util.h"
#include "dump.h"
#include "parse.h"

#define DEBUGSLACK 0
//#define DEBUGSLACK ( strstr(getTaskName(idk),"ap_main_initmodem") != NULL )

int resetInterference( chart_t *msc ) {

  int i, j;
  for( i = 0; i < msc->topoListLen; i++ )
    for( j = 0; j < msc->topoListLen; j++ )
      interfere[msc->topoList[i]][msc->topoList[j]] = 1;

  return 0;
}

void generateWeiConflict(chart_t *msc) {

  int i, k, len = msc->topoListLen;
  for( i = 0; i < len; i++ ) 
  {
    int ix = msc->topoList[i];
    for( k = 0; k <  len; k++ ) 
    {
      int kx = msc->topoList[k];
      if( (ix == kx) || (taskList[ ix ]->peID == taskList[ kx ]->peID) ) 
      {
          interfere[ix][kx] = 0;
          interfere[kx][ix] = 0;
      }
      else
      {
          interfere[ix][kx] = 1;
          interfere[kx][ix] = 1;
      }
    }
  }

  //printf("generateWeiConflict over...\n");

}

int writeWeiConflict()
{
  printf("writing interfere of Wei now...\n");

  char name[200];
  int i, k, j, ix, kx, len, mscNum = numCharts;
  FILE *interfereFile, *interferePath;
  task_t* task;
  uint nSuccs;
  int si;

  sprintf(name, "%s.interferePath", resultFileBaseName);
  interferePath = fopen(name, "w");
  if( !interferePath ) {
    fprintf( stderr, "Failed to open file %s (slacks.c:57).\n", name );
    exit(1);
  }
  
  for( j = 1; j <= mscNum; j++) {
    sprintf(name, "%s.msc%d", resultFileBaseName, j);
    fprintf(interferePath, "%s\n", name);
    
    len = msg[j-1].topoListLen;
    
    interfereFile = fopen(name,"w");
    if( !interfereFile ) {
      fprintf( stderr, "Failed to open file %s (slacks.c:69).\n", name );
      exit(1);
    }
  
    fprintf(interfereFile, "%d\n", msg[j-1].topoListLen);
    for(i = 0; i < len; i++) {
      fprintf(interfereFile, "%s\n", taskList[msg[j-1].topoList[i]]->tname);
      /* sudiptac :: Dump the successor id in the MSC also. This is needed
       * for WCET analysis in presence of shared bus */
      /* CAUTION ::: Reading of this file must also be changed 
       * accordingly */
       task = (taskList[msg[j-1].topoList[i]]);
       nSuccs = task->numSuccs;
       fprintf(interfereFile, "%d ", nSuccs);
       for(si = 0; si < nSuccs; si++) {
        fprintf(interfereFile, "%d ", get_msc_id(&msg[j-1],
          task->succList[si]));  
       }
       fprintf(interfereFile, "\n");
    }
    //fprintf(interfereFile, "\n");
    
    for( i = 0; i < len; i++ ) 
    {
      ix = msg[j-1].topoList[i];

      for( k = 0; k <  len; k++ ) 
      {
        kx = msg[j-1].topoList[k];
        fprintf(interfereFile, "%d ", interfere[ix][kx]);
      } 
      fprintf(interfereFile, "\n");
      fflush(stdout);
    }
  
    fclose(interfereFile);
  }
  fclose(interferePath);
  return 0;
}

/* Get the MSC topological id from the global task list id */
uint get_msc_id(chart_t* msc, uint task_id)
{
  int i;    
  
  for( i = 0; i < msc->topoListLen; i++ ) 
  {
      if(msc->topoList[i] == task_id)
       return i;
  }
  
  /* Must not come here */
  return -1;
}
/* sudiptac :: for bus aware WCET analysis */
static void writeAllTime(chart_t* msc, task_t* task, int id, FILE* fp)
{
   int i;

   assert(task);    
   fprintf(fp, "TASK ID = %d\n\n", id);
   fprintf(fp, "EARLIEST START = %Lu\n", earliestReq[id]);
   fprintf(fp, "LATEST   START = %Lu\n",latestReq[id]);
   fprintf(fp, "EARLIEST FINISH = %Lu\n", earliestFin[id]);
   fprintf(fp, "LATEST FINISH = %Lu\n", latestFin[id]);
   fprintf(fp, "Successors\n");
   for(i = 0; i < task->numSuccs; i++)
      fprintf(fp, "%d\n", get_msc_id(msc, task->succList[i]));
   fprintf(fp, "\n");
}

// analyze the interference between tasks within msc
void setInterference( chart_t *msc ) {
 //printf("In interfereAnalysis\n");

 // fix update order based on topology and 
 //current schedule, so that a task is updated after all tasks preceding it
    int i, k, len = msc->topoListLen;
   extern FILE* timefp;   
//  int changed = 0;

  
 // update interference graph
 for( i = 0; i < len; i++ ) 
 {
   int ix = msc->topoList[i];

  /* sudiptac :: Write all timing informations to a file
   * i.e. earliest starting time, latest starting time, 
   * earliest finish time and latest finish time. This is 
   * required for the WCET analysis in presence of shared 
   * bus as the technique starting time of each memory 
   * request going through a shared bus */
    writeAllTime(msc, taskList[msc->topoList[i]], ix, timefp);
   
  for( k = 0; k <  len; k++ ) 
  {
    int kx = msc->topoList[k];
      if( (ix != kx) && canConflict(ix,kx)) 
    {
        interfere[ix][kx] = 1;
        interfere[kx][ix] = 1;
      /* if( CLUSTER_BASED ) 
       * printf( "Interference: %s -- %s\n",
       * getTaskName(ix), getTaskName(kx) ); */
      }
      else 
      {
      interfere[ix][kx] = 0;
        interfere[kx][ix] = 0;
      }
    }
  }

}

void dumpInterference( chart_t *msc ) {

   int i, k, len = msc->topoListLen;

//  int changed = 0;

  for( i = 0; i < len; i++ ) {
  printTimes(msc->topoList[i]);
  }

  // update interference graph
  for( i = 0; i < len; i++ ) {
    int ix = msc->topoList[i];
    for( k = 0; k <  len; k++ ) {
      int kx = msc->topoList[k];
     printf("%d ", interfere[ix][kx]);
    }
     printf("\n");
  }
}

/* Write the intereference information now */

int writeInterference()
{
    char name[256];
      int i, k, j, mscNum = numCharts, len;
    FILE *interfereFile, *interferePath;
    uint nSuccs;
    int si;
    task_t* task;

    printf("writing interference now...\n");
    fflush (stdout);

    sprintf(name, "%s.interferePath_%d", resultFileBaseName, times_iteration);
    interferePath = fopen(name, "w");
    if( !interferePath ) {
      fprintf( stderr, "Failed to open file %s (slacks.c:224).\n", name );
      exit(1);
    }
    
    for( j = 1; j <= mscNum; j++) {
      sprintf(name, "%s.%d_msc%d", resultFileBaseName, times_iteration, j);
      fprintf(interferePath, "%s\n", name);
      
      len = msg[j-1].topoListLen;
      
      interfereFile = fopen(name,"w");
      if( !interfereFile ) {
        fprintf( stderr, "Failed to open file %s (slacks.c:236).\n", name );
        exit(1);
      }
      
      fprintf(interfereFile, "%d \n", msg[j-1].topoListLen);
      for(i = 0; i < len; i++) {
        fprintf(interfereFile, "%s\n", taskList[msg[j-1].topoList[i]]->tname);
        /* sudiptac :: Dump the successor id in the MSC also. This is needed
         * for WCET analysis in presence of shared bus */
        /* CAUTION ::: Reading of this file must also be changed 
         * accordingly */
        task = (taskList[msg[j-1].topoList[i]]);
        nSuccs = task->numSuccs;
        fprintf(interfereFile, "%d ", nSuccs);
        for(si = 0; si < nSuccs; si++)
        {
          fprintf(interfereFile, "%d ", get_msc_id(&msg[j-1],
            task->succList[si]));  
        }
        fprintf(interfereFile, "\n");
      }
      
        for( i = 0; i < len; i++ ) {
          int ix = (msg[j-1].topoList)[i];
        //fprintf(tmp, "%Lu %Lu\n", taskList[ix]->ctimeHi, 
        //taskList[ix]->ctimeLo); 
          for( k = 0; k <  len; k++ ) {
             int kx = (msg[j-1].topoList)[k];
          fprintf(interfereFile, "%d ", interfere[ix][kx]);
          } 
        fprintf(interfereFile, "\n");
        }
      fflush(stdout);
      
      fclose(interfereFile);
      }
  fclose(interferePath);
  return 0;
}



/*
 * Returns 1 if pred comes before succ in timeTopoList.
 */
char comesBefore( int pred, int succ, chart_t *msc ) {

  int idp = getIndexInList( pred, msc->timeTopoList, msc->topoListLen );
  int ids = getIndexInList( succ, msc->timeTopoList, msc->topoListLen );
  return( idp > -1 && ids > -1 && idp < ids );
}


char latestTimes_slack( chart_t *msc, int *topoArr, int toposize ) {

  int i, k;
  char changed = 0;

  //FILE *fdbg;

  // reset node start time
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // prevent new interference
    time_t val = 1;
    for( k = 0; k < toposize; k++ ) {
      //int kx = msc->topoList[k];
      int kx = topoArr[k];
      if( kx == idx || taskList[kx]->peID != taskList[idx]->peID || interfere[kx][idx] )
  continue;
      if( comesBefore( kx, idx, msc ) && val < latestFin[kx] ) {
  val = latestFin[kx];
  //if( DEBUGSLACK ) printf( "task %s after %s latestFin: %Lu\n", getTaskName(idx), getTaskName(kx), val );
      }
    }

    // if the node has a predecessor not in the currently considered list,
    // retain the maximum of the value imposed by the missing predecessor(s)
    for( k = 0; k < tx->numPreds; k++ ) {
      int idm = tx->predList[k];
      if( !inList( idm, topoArr, toposize ) && val < latestFin[idm] ) {
  val = latestFin[idm];
  //if( DEBUGSLACK ) printf( "task %s pred %s latestFin: %Lu\n", getTaskName(idx), getTaskName(idm), val );
      }
    }
    latestReq[idx] = val;
  }

  // compute
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // response time
    time_t wcrt = fixedPointCalcHi( msc, idx );
    time_t value = latestReq[idx] + wcrt;
    if( latestFin[idx] != value ) {
      latestFin[idx] = value;
      changed = 1;
    }
    //fdbg = openfext( tx->tname, "prmu1", "a" );
    //fprintf( fdbg, "[%s] wcrt = %Lu\n", getTaskName(idx), wcrt );
    //fprintf( fdbg, "[%s] latestReq = %Lu\n", getTaskName(idx), latestReq[idx] );
    //fprintf( fdbg, "[%s] latestFin = %Lu\n", getTaskName(idx), latestFin[idx] );

    // update successors
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      if( latestReq[idk] < latestFin[idx] ) {
  latestReq[idk] = latestFin[idx];
  changed = 1;
      }
      //fprintf( fdbg, "--> [%s] latestReq = %Lu\n", getTaskName(idk), latestReq[idk] );
      //if( DEBUGSLACK )
      //printf( "[%s] --> [%s] latestReq = %Lu\n", getTaskName(idx), getTaskName(idk), latestReq[idk] );

    } // end for succs

    //fclose( fdbg );
  }
  return changed;
}


char earliestTimes_slack( chart_t *msc, int *topoArr, int toposize ) {

  int i, k;
  char changed = 0;

  //FILE *fdbg;

  // reset node start times
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // prevent new interference
    time_t val = 1;
    for( k = 0; k < toposize; k++ ) {
      //int kx = msc->topoList[k];
      int kx = topoArr[k];

      if( kx == idx || taskList[kx]->peID != taskList[idx]->peID || interfere[kx][idx] )
  continue;
      if( comesBefore( kx, idx, msc ) && val < earliestFin[kx] ) {
  // val = earliestFin[kx];
  val = latestFin[kx];
  //if( DEBUGSLACK ) printf( "task %s after %s earliestFin: %Lu\n", getTaskName(idx), getTaskName(kx), val );
      }
    }

    // if the node has a predecessor not in the currently considered list,
    // retain the maximum of the value imposed by the missing predecessor(s)
    for( k = 0; k < tx->numPreds; k++ ) {
      int idm = tx->predList[k];
      if( !inList( idm, topoArr, toposize ) && val < earliestFin[idm] ) {
  // val = earliestFin[idm];
  val = latestFin[idm];
  //if( DEBUGSLACK ) printf( "task %s pred %s earliestFin: %Lu\n", getTaskName(idx), getTaskName(idm), val );
      }
    }
    earliestReq[idx] = val;
  }

  // compute
  for( i = 0; i < toposize; i++ ) {
    int idx = topoArr[i];
    task_t *tx = taskList[idx];

    // response time
    time_t wcrt = fixedPointCalcLo( msc, idx );
    time_t value = earliestReq[idx] + wcrt;
    if( earliestFin[idx] != value ) {
      earliestFin[idx] = value;
      changed = 1;
    }

    //fdbg = openfext( tx->tname, "prml1", "a" );
    //fprintf( fdbg, "[%s] wcrt = %Lu\n", getTaskName(idx), wcrt );
    //fprintf( fdbg, "[%s] earliestReq = %Lu\n", getTaskName(idx), earliestReq[idx] );
    //fprintf( fdbg, "[%s] earliestFin = %Lu\n", getTaskName(idx), earliestFin[idx] );

    // update successors
    for( k = 0; k < tx->numSuccs; k++ ) {
      int idk = tx->succList[k];

      if( earliestReq[idk] < earliestFin[idx] ) {
  earliestReq[idk] = earliestFin[idx];
  changed = 1;
      }
      //fprintf( fdbg, "--> [%s] earliestReq = %Lu\n", getTaskName(idk), earliestReq[idk] );
      //if( DEBUGSLACK )
      //printf( "[%s] --> [%s] earliestReq = %Lu\n", getTaskName(idx), getTaskName(idk), earliestReq[idk] );

    } // end for succs

    //fclose( fdbg );
  }
  return changed;
}


char doTiming( chart_t *msc, char slack ) {

  int i, k;
  char changed;

  // shadow copy to detect updates
  time_t *eReq = (time_t*) MALLOC( eReq, numTasks * sizeof(time_t), "eReq" );
  time_t *lReq = (time_t*) MALLOC( lReq, numTasks * sizeof(time_t), "lReq" );
  time_t *eFin = (time_t*) MALLOC( eFin, numTasks * sizeof(time_t), "eFin" );
  time_t *lFin = (time_t*) MALLOC( lFin, numTasks * sizeof(time_t), "lFin" );

  if( slack ) {

    // fix update order based on topology and current schedule, so that a task is updated after all tasks preceding it
    int len = msc->topoListLen;

    // topological order
    msc->timeTopoList = (int*) MALLOC( msc->timeTopoList, len * sizeof(int), "timeTopoList" );
    for( k = 0; k < len; k++ )
      msc->timeTopoList[k] = msc->topoList[len-k-1];

    // modify ordering to follow scheduling
    // Note: earliest and latest times of two tasks may not follow the same ordering.
    // If the start times are tied, either ordering is okay.
    // Here we choose to place the longer task first, as a heuristic for slack insertion:
    // the shorter task which starts later will be the one that is pushed down,
    // if it has higher priority.
    do {
      changed = 0;
      for( k = 1; k < len; k++ ) {
        int idx = msc->timeTopoList[k];
        int idp = msc->timeTopoList[k-1];

        if( latestReq[idx] < latestReq[idp] ||
            ( latestReq[idx] == latestReq[idp] &&
              ( earliestReq[idx] < earliestReq[idp] ||
                ( earliestReq[idx] == earliestReq[idp] &&
                  ( latestFin[idx] > latestFin[idp] ||
                    ( latestFin[idx] == latestFin[idp] && 
                      earliestFin[idx] > earliestFin[idp] ) ) ) ) ) ) {
          // swap
          changed = 1;
          msc->timeTopoList[k]   = idp;
          msc->timeTopoList[k-1] = idx;
        }
      }
    } while( changed );
    /*
    printf( "\ntimeTopoList:\n" );
    for( k = 0; k < msc->topoListLen; k++ )
      printf( "- (%d) %s\n", msc->timeTopoList[k], getTaskName(msc->timeTopoList[k]) );
    printf( "\n" );
    */
  }

  // reset
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];

    eReq[ix] = earliestReq[ix];
    lReq[ix] = latestReq  [ix];
    eFin[ix] = earliestFin[ix];
    lFin[ix] = latestFin  [ix];

    earliestReq[ix] = 1;
    latestReq  [ix] = 1;
    earliestFin[ix] = 0;
    latestFin  [ix] = 0;
  }

  if( slack ) {
    for( i = 0; i < msc->topoListLen; i++ ) {

      // int ix = msc->timeTopoList[i];

      // earliestTimes_slack( msc, &(msc->timeTopoList[i]), msc->topoListLen - i );
      earliestTimes_slack( msc, msc->timeTopoList, msc->topoListLen );
      // printf( "\nAfter earliestTimes( %s )\n", getTaskName(ix) ); 
      // printTimes( ix );
      
      // latestTimes_slack( msc, &(msc->timeTopoList[i]), msc->topoListLen - i );
      latestTimes_slack( msc, msc->timeTopoList, msc->topoListLen );
      // printf( "\nAfter latestTimes( %s )\n", getTaskName(ix) ); 
      // printTimes( ix );
    }
  }
  else {
    for( i = 0; i < msc->topoListLen; i++ ) {
      int ix = msc->topoList[i];

      int *topoArr = NULL;
      int toposize = topoSortSubgraph( ix, &topoArr );
      /*
      printf( "\nTopological order:" );
      for( k = toposize - 1; k >= 0; k-- )
  printf( " %s", getTaskName(topoArr[k]) );
      printf( "\n" );
      */

      earliestTimes( msc, topoArr, toposize );
      // printf( "\nAfter earliestTimes( %s )\n", getTaskName(ix) ); 
      // printTimes( ix );
      
      latestTimes( msc, topoArr, toposize );
      // printf( "\nAfter latestTimes( %s )\n", getTaskName(ix) ); 
      // printTimes( ix );

      free( topoArr );
    }
  }

  changed = 0;
  for( i = 0; !changed && i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    if( eReq[ix] != earliestReq[ix] || lReq[ix] != latestReq[ix] || eFin[ix] != earliestFin[ix] || lFin[ix] != latestFin[ix] )
      changed = 1;
  }

  // update interference graph
  for( i = 0; i < msc->topoListLen; i++ ) {
    int ix = msc->topoList[i];
    for( k = 0; k < msc->topoListLen; k++ ) {
      int kx = msc->topoList[k];
      if( ix != kx && (canPreempt(ix,kx) || canPreempt(kx,ix)) ) {
  interfere[ix][kx] = 1;
  interfere[kx][ix] = 1;
  //if( CLUSTER_BASED ) printf( "Interference: %s -- %s\n", getTaskName(ix), getTaskName(kx) );
      }
      else {
  interfere[ix][kx] = 0;
  interfere[kx][ix] = 0;
      }
    }
  }

  //findCriticalPath( msc );

  free( eReq );
  free( lReq );
  free( eFin );
  free( lFin );

  return changed;
}


int getAllocatedSize( task_t *ts ) {

  int i;
  int sum = 0;

  for( i = 0; i < ts->numMemBlocks; i++ )
    if( ts->allocated[i] )
      sum += ts->memBlockList[i]->size;

  return sum;
}

time_t estGain( task_t *ts, int capacity ) {

  int i;
  char changed = 1;

  int numUnalloc = 0;
  mem_t **sortBlocks = NULL;

  int remain = capacity;
  time_t gain = 0;

  // collect unallocated blocks
  for( i = 0; i < ts->numMemBlocks; i++ ) {
    if( !ts->allocated[i] ) {
      numUnalloc++;
      sortBlocks = (mem_t**) REALLOC( sortBlocks, numUnalloc * sizeof(mem_t*), "sortBlocks" );
      sortBlocks[numUnalloc-1] = ts->memBlockList[i];
    }
  }
  // sort in decreasing order of access frequency
  while( changed ) {
    changed = 0;
    for( i = 1; i < numUnalloc; i++ ) {
      if( sortBlocks[i]->freq > sortBlocks[i-1]->freq ) {
  mem_t *tmp      = sortBlocks[i-1];
  sortBlocks[i-1] = sortBlocks[i];
  sortBlocks[i]   = tmp;
  changed = 1;
      }
    }
  }
  // gain from most-accessed blocks
  for( i = 0; i < numUnalloc && remain > 0; i++ ) {
    mem_t *mt = sortBlocks[i];
    if( mt->size <= remain ) {
      remain -= mt->size;
      gain += mt->freq * (mt->size / FETCH_SIZE) * (off_latency - spm_latency);
    }
  }
  free( sortBlocks );

  return gain;
}


int insertSlacks( chart_t *msc ) {

  int i, k, n;
  char changed;

  int round = 0;
  while( round < MAXCR ) {

    // find the maximum interference
    int maxTask = -1;
    int maxVic = -1;
    //time_t maxLen = 0;
    time_t minNewTime = 0;

    for( i = 0; i < msc->topoListLen; i++ ) {
      int ix = msc->topoList[i];
      task_t *ti = taskList[ix];

      if( isCritical[ix] != 1 )
  continue;

      for( k = 0; k < msc->topoListLen; k++ ) {
  int kx = msc->topoList[k];
  task_t *tk = taskList[kx];

  time_t maxFin, slack, allocGain;

  if( ix == kx || !isCritical[kx] || !interfere[kx][ix] || !interfere[ix][kx] || !canPreempt(kx,ix) )
    continue;
  //if( tk->ctimeHi < maxLen )
  //  continue;

  // effect of imposing slack
  maxFin = msc->wcrt;
  slack = latestFin[ix] - latestFin[kx];

  for( n = 0; n < msc->topoListLen; n++ ) {
    int nx = msc->topoList[n];

    if( nx == ix || nx == kx )
      continue;

    if( isPredecessorOf( kx, nx ))
      // pushed back
      maxFin = tmax( maxFin, latestFin[nx] + slack );

    else if( !interfere[nx][kx] || !interfere[kx][nx] ) {
      // maintain non-interference
      if( earliestReq[nx] >= latestFin[kx] && earliestReq[nx] < latestFin[kx] + slack )
        maxFin = tmax( maxFin, latestFin[nx] + (latestFin[kx] + slack - earliestReq[nx]) );
    }
  }

  // projected gain due to possible overlay
  allocGain = estGain( ti, getAllocatedSize(tk) ) + estGain( tk, getAllocatedSize(ti) );

  // length of removed preemption = k's lifetime
  maxFin -= (latestFin[kx] - earliestReq[kx]) + allocGain;

  printf( "Current WCRT: %Lu\t Projected allocGain: %Lu\t Projected WCRT: %Lu\t Gain: %lf\n",
    msc->wcrt, allocGain, maxFin, ((double) msc->wcrt - (double) maxFin) );

  //if( maxFin <= msc->wcrt ) {
  if( maxFin <= msc->wcrt && ( minNewTime == 0 || maxFin < minNewTime )) {
    maxTask = kx;
    maxVic = ix;
    //maxLen = tk->ctimeHi;
    minNewTime = maxFin;
  }
      }
    }
    if( maxTask == -1 || maxVic == -1 )
      break;

    // eliminate this interference
    printf( "Eliminate interference %s by %s\n", getTaskName(maxVic), getTaskName(maxTask) ); fflush( stdout );
    interfere[maxTask][maxVic] = 0;
    interfere[maxVic][maxTask] = 0;

    // update timing with slacks
    do {
      changed = doTiming( msc, 1 );
    } while( changed );

    printf( "%s:\t[%12Lu/%12Lu, %12Lu/%12Lu)\n", getTaskName(maxVic),
      earliestReq[maxVic], latestReq[maxVic], earliestFin[maxVic], latestFin[maxVic] );
    printf( "%s:\t[%12Lu/%12Lu, %12Lu/%12Lu)\n\n", getTaskName(maxTask),
      earliestReq[maxTask], latestReq[maxTask], earliestFin[maxTask], latestFin[maxTask] );

    //computeWCRT();

    round++;
  }

  return 0;
}
