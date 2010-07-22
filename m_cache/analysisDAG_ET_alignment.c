#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_ET_alignment.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"

/* This is the entry point for the non-MSC-aware version of the DAG-based analysis. The function
 * does not consider the mscs, it just searches the list of known functions for the 'main' function
 * and starts the analysis there.
 */
void computeET_alignment( ull start_time )
{

}

/* Analyze worst case execution time of all the tasks inside
 * a MSC. The MSC is given by the argument */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file )
{
}
