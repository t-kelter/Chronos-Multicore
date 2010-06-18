#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "handler.c"
#include "util.c"
#include "parse.c"
#include "dump.c"
#include "topo.c"
#include "timing.c"
//#include "timing-non-preemptive.c"
#include "timingMSG.c"
#include "analysis.c"
#include "alloc.c"

FILE* timefp;
char resultFileBaseName[200];


int wcrt_analysis( int argc, char **argv ) {

  int i, j;
  //char cmd[256];

  //time_t wcet, bcet;

  if( argc < 3 ) {
    printf( "Usage: %s <config_file> <task_desc> <dpd_graph> <times_iteration>\n", 
      argv[0] );
    //printf( "<concat_method>: 0 (synchronous) | 1 (asynchronous)\n" );
    //printf( "<alloc_method>:\n" );
    //printf( "  0: NONE (analysis only)\n" );
    //printf( "  1: PROFILE_KNAPSACK   2: INTERFERENCE_CLUSTER   3: GRAPH_COLORING   4: CRITICAL_REDUCTION\n" );
    exit(1);
  }

  cfname = argv[1];
  pdname = argv[2];
  dgname = argv[3];
  times_iteration = argv[4];
  DEBUG = 0;
  //concat = atoi( argv[4] );
  //allocmethod = atoi( argv[5] );

  //if( argc > 6 )
  // DEBUG = atoi( argv[6] );

  // If the input files had the form <path>/myinput.xy then we will dump the
  // debug output to <path>/myinput.1.WCRT etc, so this string saves the common
  // part (<path>/myinput) to facilitate the generation of output file names.
  
  // Does not work?
  // sprintf( resultFileBaseName, "%s/%s", dirname( cfname ), basename( cfname ) );
  sprintf( resultFileBaseName, "%s", cfname );
  
  allocweight = DEFAULT_ALLOCWEIGHT;
  //limitsoln = DEFAULT_LIMITSOLN;

  // parse input
  readConfig();
  readTasks();
  readMSG();
  readEdgeBounds();
  topoTask();
  topoGraph();
  /* For debugging */
  dumpTaskInfo();


  /* for(i = 0; i < numCharts; i++)
	 printf("%d ", topoMSG[i]);
	 exit(1);
  */

  /* record of critical path */
  isCritical = (char*) CALLOC( isCritical, numTasks, sizeof(char), "isCritical" );

  // interference graph for non-greedy method
  // reset is not done here; should be done before first analysis of each chart
  interfere = (char**) MALLOC( interfere, numTasks * sizeof(char*), "interfere" );
  for( i = 0; i < numTasks; i++ ) {
    interfere[i] = (char*) CALLOC( interfere[i], numTasks, sizeof(char), "interfere[i]" );
  }

  for(i = 0; i < numCharts; i++) {
	  generateWeiConflict(&(msg[i]));
  }
  writeWeiConflict();
  printf("Done writing Wei conflict....\n");
  fflush(stdout);

  /*
	  FILE *taskName;
	  char taskNameFile[] = "taskNameFile";
	  taskName = fopen(taskNameFile, "w");
    if( !taskName ) {
      fprintf( stderr, "Failed to open file %s (main.c:94).\n", taskNameFile );
      exit(1);
    }
    
	  for(i = 0; i < numTasks; i ++)
		  fprintf("%s\n", taskList[i]->tname);
  */

  readCost();

  printf("Writing our conflict now....\n");
  fflush(stdout);

  // for timing analysis
  /* Allocate memory for timing analysis parameters */
  earliestReq = (time_t*) CALLOC( earliestReq, numTasks, sizeof(time_t), "earliestReq" );
  latestReq   = (time_t*) CALLOC( latestReq,   numTasks, sizeof(time_t), "latestReq"   );
  latestReq_copy   = (time_t*) CALLOC( latestReq_copy,   numTasks, sizeof(time_t), 
		  "latestReq"   );


  earliestFin = (time_t*) CALLOC( earliestFin, numTasks, sizeof(time_t), "earliestFin" );
  latestFin   = (time_t*) CALLOC( latestFin,   numTasks, sizeof(time_t), "latestFin"   );

  peers = (char **)CALLOC(peers, numTasks, sizeof(char *), "peers");

  for(i = 0; i < numTasks; i++) {
		peers[i] = (char *)CALLOC(peers[i], numTasks, sizeof(char), "peers[i]");
		
		for(j = 0; j < numTasks; j++)
		  peers[i][j] = 0;
  }
  concat = SYNCH;
  timingEstimate();

  /* dumpTaskInfo(); */

  /* Initialize all-timing info file */		  
  if(!timefp) {	  
    char filename[200];
    strcpy( filename, resultFileBaseName );
    strcat( filename, ".task_timing.db" );
	  timefp = fopen(filename, "w");
    if( !timefp ) {
      fprintf( stderr, "Failed to open file %s (main.c:138).\n", filename );
      exit(1);
    }
	}
  
  /* printf("Interfere as follow:	\n"); */
  /* Set intereference for each MSC */
  for(i = 0; i < numCharts; i++) {
    fprintf(timefp, "MSC ID %d	\n\n", i);
 	  setInterference(&(msg[i]));
 	  /* resetInterference( &(msg[i]) );
		   dumpInterference(&(msg[i]));
		*/
  }
  /* Close the timing file */
  fclose(timefp);
  writeInterference();
  
  printf("Done writing our conflict....\n");
  fflush(stdout);

  /* free memory
   freeAll(); */

  return 0;
}
