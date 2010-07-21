/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based WCET analysis functions.
 *
 * The functions in this file use the new approach which bounds the alignment
 * of each loop invocation and each procedure call, so that the WCET can be
 * computed for an alignment inside the given bounds. This hopefully leads to
 * a decreased WCET.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_WCET_ALIGNMENT_H
#define __CHRONOS_ANALYSIS_DAG_WCET_ALIGNMENT_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeWCET_alignment(ull start_time);

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC_alignment(MSC *msc, const char *tdma_bus_schedule_file);


#endif
