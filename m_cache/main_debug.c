#include <string.h>
#include <stdio.h>
#include <time.h>

#include "header.h"
#include "cycle_time.c"
#include "cycle_time.h"
#include "analysis.h"
#include "infeasible.h"
#include "handler.c"
#include "dump.c"
#include "block.c"
#include "parseCFG.c"
#include "loopdetect.c"
//#include "infeasible.c"
//#include "findConflicts.c"
#include "path.c"
//#include "DAG_WCET.c"
#include "topo.c"
//#include "analysisILP.c"
#include "analysisDAG_WCET.c"
#include "analysisDAG_BCET.c"
//#include "analysisEnum.c"
#include "analysisCache.c"
#include "analysisCacheL2.c"
#include "updateCacheL2.c"
#include "pathDAG.c"
/* For bus-aware WCET calculation */
#include "busSchedule.c"

/*!
 * This is the main function for a debuggin version of the analyzer.
 */
int main( int argc, char **argv ) {

  int i, j;
  procedure *p;

  if( argc < 2 ) {
    printf( "\nUsage: opt <filename> <TDMA schedule file> [debug_on]\n" ), exit(1);}

  filename = argv[1];
  if( argc > 3 )
    debug  = atoi( argv[3] );

  if(debug) {		  
  printf( "\nReading CFG...\n" ); fflush( stdout );
  }
  procs     = NULL;
  num_procs = 0;
  proc_cg   = NULL;
  /* Reading the control flow graph */
  read_cfg();
  if( debug ) 
	 print_cfg();

  if(debug)		  
  {
		printf( "\nReading assembly instructions...\n" ); 
		fflush(stdout); 
  }		
  readInstr();
  if( debug ) print_instrlist();

  if(debug)	{	  
  printf( "\nDetecting loops...\n" ); fflush( stdout );
  }
  detect_loops();
  if( debug ) print_loops();

  if(debug) {		  
  printf( "\nConstructing topological order of procedures and loops...\n" ); fflush( stdout );
  }
  topo_sort();
  if( debug ) print_topo();
  
  /* Compute incoming edge info for each basic block */
  calculate_incoming();

  /* sudiptac: Use bus aware path-based WCET computation */
  printf( "\nInitial WCET estimation with shared bus...\n" );
  
  /* Set the TDMA bus schedule */
  setSchedule(argv[2]);

  /* Do L1 cache analysis */
  /* {
	  int top_func;	  
	  
	  assert(proc_cg);
	  top_func = proc_cg[num_procs - 1];
	  main_copy = procs[top_func];
	  set_cache_basic("l1");
	  cacheAnalysis();
  }*/ 
  /* FIXME: start time set to zero, core set to zero */
  ncore = 0;
  g_testing_mode = 1;
  /* Compute with shared bus */
  g_shared_bus = 1;
  computeWCET(0);
  /* Compute without shared bus */
  g_shared_bus = 0;
  computeWCET(0);
  		  
  return 0;
}
