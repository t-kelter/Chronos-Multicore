// Include standard library headers
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#define DEF_GLOBALS
#include "header.h"
#undef DEF_GLOBALS

#define DEF_INFEASIBILITY_GLOBALS
#include "infeasible.h"
#undef DEF_INFEASIBILITY_GLOBALS

#include "handler.h"
#include "dump.h"
#include "block.h"
#include "parseCFG.h"
#include "loopdetect.h"
//#include "infeasible.h"
//#include "findConflicts.h"
#include "path.h"
//#include "DAG_WCET.h"
#include "topo.h"
//#include "analysisILP.h"
#include "analysisDAG_BCET_structural.h"
#include "analysisDAG_BCET_unroll.h"
#include "analysisDAG_ET_alignment.h"
#include "analysisDAG_WCET_structural.h"
#include "analysisDAG_WCET_unroll.h"
//#include "analysisEnum.h"
#include "analysisCache.h"
#include "analysisCacheL2.h"
#include "updateCacheL2.h"
#include "pathDAG.h"
#include "busSchedule.h"
#include "wcrt/wcrt.h"
#include "wcrt/cycle_time.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


// List of analysis methods
enum AnalysisMethod {
    ANALYSIS_UNROLL,
    ANALYSIS_STRUCTURAL,
    ANALYSIS_ALIGNMENT
};
typedef unsigned long long ticks;
#define CPU_MHZ 3000000000


// #########################################
// #### Declaration of static variables ####
// #########################################


/* Stores whether the debugmacros have already been initialized for this file. */
static _Bool firstDebugmacroInit = 1;


// #########################################
// #### Declaration of static functions ####
// #########################################


static void analysis( MSC *msc, const char *tdma_bus_schedule_file,
                      enum AnalysisMethod method,
                      enum LoopAnalysisType alignmentAnalysisLAType,
                      _Bool alignmentAnalysisTryStructural );
static void readMSCfromFile( const char *interferFileName, int msc_index,
                             _Bool *interference_changed );
static void writeWCETandCacheInfoFiles( int num_msc );
static __inline__ ticks getticks(void);;

#ifdef WITH_WEI_COMPARISON
static void writeWeiComparison( int num_msc, const char *finalStatsBasename );
static void writeWeiStatistic( int num_msc );
#endif


// #########################################
// #### Definitions of public functions ####
// #########################################


int main(int argc, char **argv )
{
  DINITDEBUGMACROS( firstDebugmacroInit, "debugmacros.conf" );
  DSTART( "main" );

  char hitmiss[MAX_LEN];
  int n, i;
  _Bool flag;
  int num_msc = 0;
  float sum;
  char interferFileName[MAX_LEN];
  ticks start,end;

  times_iteration = 0;

  /* sudiptac :: Reset debugging mode */
  g_testing_mode = 0;
  /* Compute with shared bus */
  g_shared_bus = 1;
  /* For independent tasks running on multiple cores */
  g_independent_task = 1;
  /* For no bus modelling */
  g_no_bus_modeling = 0;

  /* also read conflict info and tasks info */
  interferePathName = argv[1];
  num_core = atoi( argv[4] );
  const char * const tdma_bus_schedule_file = argv[5];
  infeas = 0;

  /* Set the analysis method to use. */
  enum AnalysisMethod current_analysis_method = ANALYSIS_ALIGNMENT;
  enum LoopAnalysisType alignmentLAType = LOOP_ANALYSIS_GLOBAL_CONVERGENCE;
  _Bool alignmentTryStructural = 0;
  switch( argv[6][0] ) {
    case 'a':
      current_analysis_method = ANALYSIS_ALIGNMENT;
      for ( i = 1; argv[6][i] != '\0'; i++ ) {
        char option_char = argv[6][i];
        switch( option_char ) {
          case 'c': alignmentLAType = LOOP_ANALYSIS_GLOBAL_CONVERGENCE; break;
          case 'g': alignmentLAType = LOOP_ANALYSIS_GRAPH_TRACKING; break;
          case '+': alignmentTryStructural = 1; break;
          default: assert( 0 && "Unknown option!" );
        }
      }
      break;
    case 's':
      current_analysis_method = ANALYSIS_STRUCTURAL;
      break;
    case 'u':
      current_analysis_method = ANALYSIS_UNROLL;
      break;
    default:
      assert( 0 && "Unknown analysis method!" );
  }

  /* For private L2 cache analysis */		  
  // TODO: This should be covered by a new input format for the input files
  if(argc > 7) {
    g_private = atoi(argv[7]);  
  } else {
    g_private = 0;
  }

  /* sudiptac :: Allocate the earliest/latest start time structure for
   * all the cores */
  CALLOC( earliest_core_time, ull *, num_core, sizeof(ull), "earliest_core_time" );
  CALLOC( latest_core_time, ull *, num_core, sizeof(ull), "latest_core_time" );

  /* Set the basic parameters of L1 and L2 instruction caches */		  
  set_cache_basic( argv[2] );
  set_cache_basic_L2( argv[3] );

  /* Allocate memory for capturing conflicting task information */		  
  CALLOC(numConflictTask, char *, cache_L2.ns, sizeof(char), "numConflictTask");
  CALLOC(numConflictMSC, char *, cache_L2.ns, sizeof(char), "numConflictMSC");

  /* Reset/initialize allocated memory */ 		  
  for(n = 0; n < cache_L2.ns; n++) {
    numConflictTask[n] = 0;
    numConflictMSC[n] = 0;
  }

  /* Monitor time from this point */
  STARTTIME;

  /* Start reading the interference file and build the intereference 
   * information */
  /* TODO: The input files for Chronos are relatively complicated to read and at
   *       the moment the bus analysis and the wcrt computation use different input
   *       files which makes the whole input file handling very erroneous. A new input
   *       file format should be developed which can be used by both the bus analysis
   *       and the wcrt analysis. This is currently undertaken in git branch
   *       "new_input_format".
   */
  FILE *interferPath = fopen(interferePathName, "r");
  if( !interferPath ) {
    fprintf(stderr, "Failed to open file: %s (main.c:213)\n", interferePathName);
    exit(1);
  }

  /* Read the entire file containing the interference information */		  
  while(fscanf(interferPath, "%s\n", interferFileName)!= EOF) {
    if(num_msc == 0) {
      CALLOC(msc, MSC**, 1, sizeof(MSC*), "MSC*");
    } else {
      REALLOC(msc, MSC**, (num_msc + 1) * sizeof(MSC*), "MSC*");
    }
    num_msc++;

    readMSCfromFile( interferFileName, num_msc - 1, NULL );
    MSC * const currentMSC = msc[num_msc - 1];

    DOUT( "Reading MSC from '%s'", interferFileName );

    /* Now go through all the tasks to read their CFG and build 
     * relevant data structures like loops, basic blocks and so 
     * on */	  
    for(i = 0; i < currentMSC->num_task; i ++) {

      task_t * const currentTask = &currentMSC->taskList[i];
      currentTask->task_id = i;

      DOUT( "Reading task %s\n", currentTask->task_name );

      filename = currentTask->task_name;
      procs     = NULL;
      num_procs = 0;
      proc_cg   = NULL;
      infeas = 0;

      /* Read the cfg of the task into the global array 'procs' */
      read_cfg();

      /* Allocate memory for the procedure copies. */
      CALLOC(currentTask->proc_cg_ptr, proc_copy *, num_procs,
          sizeof(proc_copy), "currentTask->proc_cg_ptr");

      readInstr();

      /* Detect loops in all procedures of the task */ 
      detect_loops();

      topo_sort();

      /* compute incoming info for each basic block */
      calculate_incoming();

      /* This function allocates memory for all analysis and subsequent WCET
       * computation of the task */
      constructAll( currentTask );

      /* Set the main procedure (entry procedure) in this task */
      currentTask->main_copy = main_copy;
      currentTask->procs = procs;
      currentTask->num_proc= num_procs;

      /* Now do L1 cache analysis of the current task and compute 
       * hit-miss-unknown classification of every instruction....
       * Data cache is assumed to be perfect in this case */
      cacheAnalysis();
      /* Free all L1 cache state memories of the task as they are 
       * no longer needed */ 
      freeAllCacheState();

      printf("L1 cache analysis finished\n");

      /* Now do private L2 cache analysis of this task */
      cacheAnalysis_L2();
      /* Free L2 cache states */
      freeAll_L2();

      printf("L2 cache analysis finished\n\n");
    }

    /* Private cache analysis for all tasks are done here. But due 
     * to the intereference some of the classification in L2 cache 
     * need to be updated */
    /* If private L2 cache analysis .... no update of interference */
    if( !g_private ) {
      printf("Update cache state in msc '%s'\n\n", currentMSC->msc_name);
      updateCacheState(currentMSC);
    }

    /* This function allocates all memory required for computing and 
     * storing hit-miss classification */
    pathDAG(currentMSC);

    /* Compute WCET and BCET of this MSC. remember MSC... not task
     * so we need to compute WCET/BCET of each task in the MSC */
    /* CAUTION: In presence of shared bus these two function changes
     * to account for the bus delay */
    start = getticks();
    analysis( currentMSC, tdma_bus_schedule_file, current_analysis_method,
        alignmentLAType, alignmentTryStructural );
    end = getticks();

    /* FIXME: What's this function doing here ? */ 	  
    /* If private L2 cache analysis .... no update of interference */	  
    if(!g_private)
      resetHitMiss_L2(currentMSC);

    /* Initializing conflicting information */
    for(n = 0; n < cache_L2.ns; n++) {
      numConflictMSC[n] = 0;
    }
  }
  /* Done with timing analysis of all the MSC-s */
  fclose(interferPath);

  /* Assert correct function of debug macros. */
  DASSERT( DSIZE() == 1 );

  /* sudiptac :::: Return from here in case of independent task running on 
   * multiple cores. Because in that case no timing interval computation is 
   * needed and the WCRT module is not called */
  if(g_independent_task) {
    printf("===================================================\n");
    printf("WCET computation time = %lf secs\n", (end - start)/((1.0) * CPU_MHZ));
    printf("===================================================\n");
    DRETURN( 0 );
  }	 

  /* Start of the iterative algorithm */		  
  times_iteration ++;

  // Print the input for the WCRT submodule
  writeWCETandCacheInfoFiles( num_msc );

  /* To get Wei's result */
#ifdef WITH_WEI_COMPARISON
  writeWeiStatistic( num_msc );
#endif

  flag = 1;

  /* Yes... Now we have to call the WCRT module to compute WCRT */
  /* This is an iterative computation */
  while(flag) {

    flag = 0;
    printf("\nCall wcrt analysis the %d time\n\n", times_iteration);

    if(num_core == 1 || 
       num_core == 2 ||
       num_core == 4) {
      wcrt_analysis( "simple_test.cf", "simple_test.pd", "simple_test.msg" );
    } else {
      fprintf(stderr, "Invalid number of cores!\n");
      exit(1);
    }

    /* Open and get the new interference file */  
    sprintf(interferFileName, "%s.interferePath_%d", "simple_test.cf", times_iteration);
    interferPath = fopen(interferFileName, "r");
    if( !interferPath ) {
     fprintf(stderr, "Failed to open file: %s (main.c:624)\n", interferFileName);
     exit(1);
    }

    for(i = 0; i < num_msc; i ++) {
      fscanf(interferPath, "%s\n", (char*)&interferFileName);

      readMSCfromFile( interferFileName, i + 1, &flag );
    }
    fclose(interferPath);

    /* No change in interference ---- break the loop */
    if( flag == 0 ) {
      break;
    } else {

      for(i = 0; i < num_msc; i ++) {
        printf("Update CS for %s\n", msc[i]->msc_name);

        /* Update L2 cache state */
        updateCacheState(msc[i]);

        pathDAG(msc[i]);

        /* Compute WCET and BCET of each task. */
        analysis( msc[i], tdma_bus_schedule_file, current_analysis_method,
            alignmentLAType, alignmentTryStructural  );

        /* FIXME: What's this function doing here ? */
        resetHitMiss_L2(msc[i]);
      }

      /* Iteration increased */
      times_iteration ++;

      // Print the input for the WCRT submodule
      writeWCETandCacheInfoFiles( num_msc );
    }
  }

  STOPTIME;

  /* DONE: All Analysis */

  // The base path of all final output files. Individual files only add a suffix
  char *finalStatsBasename = "simple_test";
  
  /* To compare against our results */
#ifdef WITH_WEI_COMPARISON
  writeWeiComparison( num_msc, finalStatsBasename );
#endif

  sum = 0;
  for(i = 0; i < cache_L2.ns; i ++)
    sum = sum + numConflictTask[i];

  printf("Average conflict tasks:    	%f\n", sum/cache_L2.ns);


  /* Get the final WCRT value */
  sprintf(hitmiss, "%s-wcrt.res", finalStatsBasename);
  FILE *wcrt = fopen(hitmiss, "w");
  if( !wcrt ) {
   fprintf(stderr, "Failed to open file: %s (main.c:815)\n", hitmiss);
   exit(1);
  }

  char summary1[200];
  sprintf( summary1, "%s.cf.1.WCRT", finalStatsBasename );
  char summary2[200];
  sprintf( summary2, "%s.cf.2.WCRT", finalStatsBasename );
  ull wcet_our, wcet_wei;

  if(times_iteration > 1) {
    FILE *file = fopen(summary2, "r");
    if( !file ) {
     fprintf(stderr, "Failed to open file: %s (main.c:929)\n", summary2);
     exit(1);
    }

    fscanf(file, "%Lu", &wcet_our);
    fprintf(wcrt,"our %Lu\n", wcet_our);
    fclose(file);
  } else {
    FILE *file = fopen(summary1, "r");
    if( !file ) {
     fprintf(stderr, "Failed to open file: %s (main.c:939)\n", summary1);
     exit(1);
    }

    fscanf(file, "%Lu", &wcet_our);
    fprintf(wcrt,"our %Lu\n", wcet_our);
    fclose(file);
  }
  FILE *file = fopen(summary1, "r");
  if( !file ) {
   fprintf(stderr, "Failed to open file: %s (main.c:949)\n", summary1);
   exit(1);
  }

  fscanf(file, "%Lu", &wcet_wei);
  fprintf(wcrt,"wei %Lu\n", wcet_wei);
  fprintf(wcrt,"differ %Lu\n", wcet_wei - wcet_our);
  fprintf(wcrt,"runtime %f s\n", t/(CYCLES_PER_MSEC * 1000.0));

  fprintf(wcrt,"average conflict tasks/set  %f\n", sum/cache_L2.ns);

  fclose(file);
  fclose(wcrt);

  printf("%d core, No change in interfere now, exit\n", num_core);

  /* Assert correct function of debug macros. */
  DASSERT( DSIZE() == 1 );

  DRETURN( 0 );
}


// #########################################
// #### Definitions of static functions ####
// #########################################


/*
 * Switches between the alternatives of analysis methods.
 */
static void analysis( MSC *msc, const char *tdma_bus_schedule_file,
                      enum AnalysisMethod method,
                      enum LoopAnalysisType alignmentAnalysisLAType,
                      _Bool alignmentAnalysisTryStructural )
{
  DSTART( "analysis" );

  switch ( method ) {

    case ANALYSIS_UNROLL:
      compute_bus_WCET_MSC_unroll(msc, tdma_bus_schedule_file);
      compute_bus_BCET_MSC_unroll(msc, tdma_bus_schedule_file);
      break;

    case ANALYSIS_STRUCTURAL:
      compute_bus_WCET_MSC_structural(msc, tdma_bus_schedule_file);
      compute_bus_BCET_MSC_structural(msc, tdma_bus_schedule_file);
      break;

    case ANALYSIS_ALIGNMENT:
      // Computes BCET and WCET together
      compute_bus_ET_MSC_alignment(msc, tdma_bus_schedule_file,
          alignmentAnalysisLAType, alignmentAnalysisTryStructural);
      break;

    default:
      fprintf( stderr, "Invalid choice of analysis method.\n" );
      exit(1);
  }

  // Assert that all results are sound
  int i;
  for ( i = 0; i < msc->num_task; i++ ) {
    assert( msc->taskList[i].bcet <= msc->taskList[i].wcet &&
        "Invalid BCET/WCET results for task" );
  }

  // Output the results if desired
  DACTION(
      for ( i = 0; i < msc->num_task; i++ ) {
        task_t * const t = &( msc->taskList[i] );
        DOUT( "Results for task %s : BCET %llu \tWCET %llu (jitter %u%%)\n",
            t->task_name, t->bcet, t->wcet,
            ( ( t->wcet - t->bcet ) * 100 ) / t->wcet );
      }
  );

  DEND();
}


/* Reads an MSC from the given file and allocates the memory for
 * storing the msc.
 *
 * 'msc_index' The new msc will be stored at this index in the
 *             global array 'msc'.
 * 'interference_changed' If the task interference changed, compared to
 *                        values already stored in msc[num_msc - 1],
 *                        then this location is set to '1' if the
 *                        pointer is not NULL.
 */
static void readMSCfromFile( const char *interferFileName, int msc_index, _Bool *interference_changed )
{
  FILE *interferFile = fopen(interferFileName,"r");
  if( !interferFile ) {
    fprintf(stderr, "Failed to open file: %s (main.c:231)\n", interferFileName);
    exit(1);
  }

  /* Read number of tasks */
  int num_task = 0;
  fscanf(interferFile, "%d\n", &num_task);

  if ( interference_changed != NULL ) {
    *interference_changed = 0;
  }

  /* Allocate memory for this MSC */
  CALLOC_IF_NULL( msc[msc_index], MSC*, sizeof(MSC), "MSC");

  strcpy(msc[msc_index]->msc_name, interferFileName);
  msc[msc_index]->num_task = num_task;

  /* Allocate memory for all tasks in the MSC and intereference data
   * structure */
  CALLOC_IF_NULL(msc[msc_index]->taskList, task_t*,
      num_task * sizeof(task_t), "taskList");
  CALLOC_IF_NULL(msc[msc_index]->interferInfo, int**,
      num_task * sizeof(int*), "interferInfo*");

  /* Get/set names of all tasks in the MSC */
  int i;
  for(i = 0; i < num_task; i ++) {

    fscanf(interferFile, "%s\n", (char*)&(msc[msc_index]->taskList[i].task_name));
    /* sudiptac ::: Read also the successor info. Needed for WCET analysis
     * in presence of shared bus */
    fscanf(interferFile, "%d", &(msc[msc_index]->taskList[i].numSuccs));
    uint nSuccs = msc[msc_index]->taskList[i].numSuccs;

    /* Allocate memory for successor List */
    CALLOC_IF_NULL( msc[msc_index]->taskList[i].succList, int*,
        nSuccs * sizeof(int), "succList" );

    /* Now read all successor id-s of this task in the same MSC. Task id-s
     * are ordered in topological order */
    int si;
    for(si = 0; si < nSuccs; si++) {
      fscanf(interferFile, "%d", &(msc[msc_index]->taskList[i].succList[si]));
    }

    fscanf(interferFile, "\n");
  }

  fscanf(interferFile, "\n");

  /* Set other parameters of the tasks */
  for(i = 0; i < num_task; i ++) {
    msc[msc_index]->taskList[i].task_id = i;
    CALLOC_IF_NULL(msc[msc_index]->interferInfo[i], int*,
        num_task * sizeof(int), "interferInfo");

    /* Set the intereference info of task "i" i.e. all ("j" < num_task)
     * are set to "1" if task "i" interefere with "j" in this msc in the
     * timeline */
    int j;
    for(j = 0; j < num_task; j++) {
      const int old_value = msc[msc_index]->interferInfo[i][j];
      fscanf(interferFile, "%d ", &(msc[msc_index]->interferInfo[i][j]));

      // Set flag if given and interference changed
      if ( msc[msc_index]->interferInfo[i][j] != old_value ) {
        if ( interference_changed != NULL ) {
          *interference_changed = 1;
        }
      }
    }

    fscanf(interferFile, "\n");
  }

  fclose(interferFile);
}

/* This is a helper function that prints the results of the WCET and BCET
 * results to files, so that the WCRT analysis submodule can read them.
 *
 * 'num_msc' should be the current total number of mscs
 */
static void writeWCETandCacheInfoFiles( int num_msc )
{
  /* Go through all the MSC-s */
  uint i;
  for(i = 1; i <= num_msc; i ++) {
    const MSC * const current_msc = msc[i-1];

    /* Create the WCET and BCET filename */
    char wbcostPath[MAX_LEN];
    sprintf(wbcostPath, "msc%d_wcetbcet_%d", i, times_iteration);
    FILE *file = fopen(wbcostPath, "w" );
    if( !file ) {
      fprintf(stderr, "Failed to open file: %s (main.c:479)\n", wbcostPath);
      exit(1);
    }

    /* Write WCET of each task in the file */
    /* This is read in again, by the WCRT module. */
    uint j;
    for(j = 0; j < current_msc->num_task; j++) {
      const task_t * const task = &current_msc->taskList[j];
      fprintf(file, "%Lu %Lu \n", task->wcet, task->bcet);
    }
    fclose(file);
  }
}


/* This is just some statistics used for comparison against an old approach.
 * This methods dumps the results of that approach.
 *
 * 'num_msc' should be the current total number of mscs
 */
#ifdef WITH_WEI_COMPARISON
static void writeWeiStatistic( int num_msc )
{
  char filename[MAX_LEN];

  int i;
  for(i = 1; i <= num_msc; i ++) {
    sprintf(filename, "msc%d_hitmiss_statistic_wei", i);
    FILE *hitmiss_statistic = fopen(filename, "w");
    if( !hitmiss_statistic ) {
      fprintf(stderr, "Failed to open file: %s (main.c:538)\n", filename);
      exit(1);
    }

    int j;
    for(j = 0; j < msc[i-1]->num_task; j++) {
      fprintf(hitmiss_statistic,"%Lu  %Lu %Lu \n",
          msc[i-1]->taskList[j].hit_wcet_L2,
          msc[i-1]->taskList[j].unknow_wcet_L2, msc[i-1]->taskList[j].wcet);
      fprintf(hitmiss_statistic,"%Lu  %Lu %Lu \n",
          msc[i-1]->taskList[j].hit_bcet_L2,
          msc[i-1]->taskList[j].unknow_bcet_L2, msc[i-1]->taskList[j].bcet);
      fflush(stdout);
    }
    fclose(hitmiss_statistic);
  }
}
#endif

/* This is just some statistics used for comparison against an old approach.
 * This methods outputs the comparison between that approach and ours.
 *
 * 'num_msc' should be the current total number of mscs
 */
#ifdef WITH_WEI_COMPARISON
static void writeWeiComparison( int num_msc, const char *finalStatsBasename )
{
   char hitmiss[MAX_LEN];
   ull hit_statistics, unknow_statistics, differ;
   ull hit_statistics_wei, unknow_statistics_wei;
  
   sprintf(hitmiss, "%s-hitmiss.res", finalStatsBasename);
   FILE *hitmiss_statistic = fopen(hitmiss, "w");
   if( !hitmiss_statistic ) {
    fprintf(stderr, "Failed to open file: %s (main.c:808)\n", hitmiss);
    exit(1);
   }

   int i;
   for(i = 1; i <= num_msc; i ++) {

     sprintf(hitmiss, "msc%d_hitmiss_statistic_wei", i);
     FILE *file_wei = fopen(hitmiss, "r");
     if( !file_wei ) {
      fprintf(stderr, "Failed to open file: %s (main.c:824)\n", hitmiss);
      exit(1);
     }

     sprintf(hitmiss, "msc%d_hitmiss_statistic_private", i);
     FILE *file_private = fopen(hitmiss, "r");
     if( !file_private ) {
      fprintf(stderr, "Failed to open file: %s (main.c:831)\n", hitmiss);
      exit(1);
     }

     int j;
     for(j = 0; j < msc[i-1]->num_task; j++) {
       // task name
       fprintf(hitmiss_statistic, "%s ", msc[i-1]->taskList[j].task_name);

       // WCET L1
       fprintf(hitmiss_statistic,"%Lu %Lu %Lu ",
           msc[i-1]->taskList[j].hit_wcet,
           msc[i-1]->taskList[j].unknow_wcet,
           msc[i-1]->taskList[j].miss_wcet);
       // hit in L2
       fscanf(file_wei ,"%Lu  ", &hit_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", hit_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].hit_wcet_L2);
       // printf("hit_statistics_wei = %Lu\n", hit_statistics_wei);
       fscanf(file_private ,"%Lu  ", &hit_statistics);
       fprintf(hitmiss_statistic,"%Lu ", hit_statistics);
       // printf("hit_statistics = %Lu\n", hit_statistics);

       // unknow in L2
       fscanf(file_wei ,"%Lu  ", &unknow_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", unknow_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].unknow_wcet_L2);

       // printf("unknow_statistics_wei = %Lu\n", unknow_statistics_wei);
       fscanf(file_private ,"%Lu  \n", &unknow_statistics);
       fprintf(hitmiss_statistic,"%Lu ", unknow_statistics);
       // printf("unknow_statistics = %Lu\n", unknow_statistics);
       // miss in L2
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].miss_wcet_L2);
       // difference
       // printf("msc[i-1]->taskList[j].hit_wcet_L2 = %Lu\n",
       // msc[i-1]->taskList[j].hit_wcet_L2);
       // printf("hit_statistics_wei = %Lu\n", hit_statistics_wei);

       differ = msc[i-1]->taskList[j].hit_wcet_L2 - hit_statistics_wei;
       fprintf(hitmiss_statistic,"%Lu ", differ);

       // printf("differ = %Lu\n", differ);
       // Our WCET
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].wcet);
       //wei's WCET
       ull wcet_wei;
       fscanf(file_wei ,"%Lu  ", &wcet_wei);
       fscanf(file_wei, "\n");
       fprintf(hitmiss_statistic,"%Lu ", wcet_wei);

       // BCET L1
       fprintf(hitmiss_statistic,"%Lu %Lu %Lu ",
           msc[i-1]->taskList[j].hit_bcet,
           msc[i-1]->taskList[j].unknow_bcet,
           msc[i-1]->taskList[j].miss_bcet);
       // hit in L2
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].hit_bcet_L2);
       fscanf(file_wei ,"%Lu  ", &hit_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", hit_statistics_wei);
       fscanf(file_private ,"%Lu  ", &hit_statistics);
       fprintf(hitmiss_statistic,"%Lu ", hit_statistics);
       // unknow in L2
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].unknow_bcet_L2);
       fscanf(file_wei ,"%Lu  ", &unknow_statistics_wei);
       fprintf(hitmiss_statistic,"%Lu ", unknow_statistics_wei);
       fscanf(file_private ,"%Lu  \n", &unknow_statistics);
       fprintf(hitmiss_statistic,"%Lu ", unknow_statistics);
       // miss in L2
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].miss_bcet_L2);
       // our bcet
       fprintf(hitmiss_statistic,"%Lu ", msc[i-1]->taskList[j].bcet);
       // wei's bcet
       fscanf(file_wei ,"%Lu  \n", &wcet_wei);
       fprintf(hitmiss_statistic,"%Lu \n", wcet_wei);
       fflush(stdout);
     }

     fclose(file_private);
     fclose(file_wei);
   }
   fclose(hitmiss_statistic);
}
#endif


/* sudiptac:: For performance measurement */
static __inline__ ticks getticks(void)
{
  unsigned a, d;
  __asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d));
  return ((ticks)a) | (((ticks)d) << 32);
}
