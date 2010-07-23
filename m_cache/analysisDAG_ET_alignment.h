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


/* A data type to represent an offset range. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint lower_bound;
  uint upper_bound;
} tdma_offset_range;


// ######### Function declarations  ###########


/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_ET_MSC_alignment(MSC *msc, const char *tdma_bus_schedule_file);


#endif
