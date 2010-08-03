/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based BCET and WCET analysis functions.
 *
 * The functions in this file use the new approach which bounds the alignment
 * of each loop invocation and each procedure call, so that the loop BCET and
 * WCET can be computed for an alignment inside the given bounds. This hopefully
 * leads to more precise results.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_ET_ALIGNMENT_H
#define __CHRONOS_ANALYSIS_DAG_ET_ALIGNMENT_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########


/* A list of the supported loop analysis methods. */
enum LoopAnalysisType {
  /* Computes globally valid alignment bounds for the loop header. */
  LOOP_ANALYSIS_GLOBAL_CONVERGENCE,
  /* Tracks the development of the alignment bounds in a graph. */
  LOOP_ANALYSIS_GRAPH_TRACKING
};


/* A data type to represent an offset range. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint lower_bound;
  uint upper_bound;
} tdma_offset_bounds;


// ######### Function declarations  ###########


/* Analyze worst case execution time of all the tasks inside a MSC.
 *
 * 'msc' is the MSC to analyze.
 * 'tdma_bus_schedule_file' holds the filename of the TDMA bus specification to use
 * 'analysis_type_to_use' should be the type of loop analysis that shall be used
 * 'try_penalized_alignment' if this is true, then the alignment analysis will try to
 *                           work like the purely structural analysis by assuming an
 *                           offset range of [0,0] and adding the appropriate
 *                           alignment penalties. */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file,
   enum LoopAnalysisType analysis_type_to_use, _Bool try_penalized_alignment );


#endif
