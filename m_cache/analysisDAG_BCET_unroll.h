/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based BCET analysis functions.
 *
 * These functions fully unroll all loops during the analysis.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_BCET_UNROLL_H
#define __CHRONOS_ANALYSIS_DAG_BCET_UNROLL_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC_unroll(MSC *msc, const char *tdma_bus_schedule_file);


#endif
