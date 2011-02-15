/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based BCET analysis functions.
 *
 * The functions in this file use the approach published in the SCOPES paper,
 * which means that they analyse the program from the innermost to the outermost
 * loops, assuming a fixed bus alignment of the loop header.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_BCET_STRUCTURAL_H
#define __CHRONOS_ANALYSIS_DAG_BCET_STRUCTURAL_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC_structural(MSC *msc, const char *tdma_bus_schedule_file);


#endif
