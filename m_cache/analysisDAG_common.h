/*! This is a header file of the Chronos timing analyzer. */

/*
 * Common functions for DAG-based analyses.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_COMMON_H
#define __CHRONOS_ANALYSIS_DAG_COMMON_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########

uint get_hex(char* hex_string);

/* Returns the callee procedure for a procedure call instruction */
procedure* getCallee(instr* inst, procedure* proc);

/* Return the type of the instruction access MISS/L1_HIT/L2_HIT.
 * This is computed from the shared cache analysis */
acc_type check_hit_miss(block* bb, instr* inst);

/* Given a task this function returns the core number in which 
 * the task is assigned to. Assignment to cores to individual 
 * tasks are done statically before any analysis took place */
uint get_core(task_t* cur_task);

/* Check whether the block specified in the header "bb"
 * is header of some loop in the procedure "proc" */
loop* check_loop(block* bb, procedure* proc);

/* Reset start and finish time of all basic blocks in this 
 * procedure */
void reset_timestamps(procedure* proc, ull start_time);

/* Reset latest start time of all tasks in the MSC before 
 * the analysis of the MSC starts */
void reset_all_task(MSC* msc);

/* Given a starting time and a particular core, this function 
 * calculates the variable memory latency for the request */
int compute_bus_delay(ull start_time, uint ncore, acc_type type);


#endif
