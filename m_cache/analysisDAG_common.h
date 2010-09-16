/*! This is a header file of the Chronos timing analyzer. */

/*
 * Common functions for DAG-based analyses.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_COMMON_H
#define __CHRONOS_ANALYSIS_DAG_COMMON_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########


/* The different scenarios for which a memory access classification 
 * can be made. */
enum AccessScenario {
  ACCESS_SCENARIO_BCET,
  ACCESS_SCENARIO_WCET
};


// ######### Function declarations  ###########

/* #### Cache analysis helper functions #### */

/* This function returns a context id for the iterations an inner loop inside a
 * surrounding loop.
 *
 * 'lp' the loop for which a context is requested.
 * 'surroundingLoopContext' may be the context of the surrounding loop, or '0',
 *                          to denote that there is no surrounding loop.
 * 'firstInnerIteration' if this is true, then a context for the first iteration
 *                       of the inner loop is returned, else a context for the
 *                       other iterations of the inner loop is returned.
 */
uint getInnerLoopContext( const loop *lp, uint surroundingLoopContext,
    _Bool firstInnerIteration );

/* Returns the maximum loop context that can be used in this procedure. */
uint getMaximumLoopContext( const procedure * const proc );

/* Attach best-case chmc classification to the instruction data structure */
void preprocess_chmc_BCET( procedure* proc );
/* Attach best-case chmc classification for L2 cache to the instruction
 * data structure */
void preprocess_chmc_L2_BCET( procedure* proc );

/* Attach worst-case chmc classification to the instruction data structure */
void preprocess_chmc_WCET( procedure* proc );
/* Attach worst-case chmc classification for L2 cache to the instruction
 * data structure */
void preprocess_chmc_L2_WCET( procedure* proc );

/* Return the type of the instruction access MISS/L1_HIT/L2_HIT.
 * This is computed from the shared cache analysis. The context
 * is given as a context index, see header.h:num_chmc for further
 * details.
 *
 * 'scenario' determines for which scenario (BCET/WCET) the access
 * type shall be determined. */
acc_type check_hit_miss(const block *bb, const instr *inst, 
                        uint context, enum AccessScenario scenario);

/* #### Structural analysis helper functions #### */

/* Returns the callee procedure for a procedure call instruction */
procedure *getCallee( const instr * const inst, const procedure * const proc);
/* Check whether the block specified in the header "bb"
 * is header of some loop in the procedure "proc" */
loop* check_loop( const block * const bb, const procedure * const proc);

/* #### WCET/BCET analysis helper functions #### */

/* This sets the earliest starting time of a block during BCET calculation
 * (Not context-aware) */
void set_start_time_BCET( block* bb, procedure* proc );
/* This sets the latest starting time of a block during WCET calculation.
 * (Not context-aware) */
void set_start_time_WCET( block* bb, procedure* proc );

/* Returns the BCET of a single instruction. */
ull getInstructionBCET( const instr *instruction );
/* Returns the WCET of a single instruction. */
ull getInstructionWCET( const instr *instruction );

/* Reset start and finish time of all basic blocks in this 
 * procedure */
void reset_timestamps(procedure* proc, ull start_time);
/* Reset latest start time of all tasks in the MSC before 
 * the analysis of the MSC starts */
void reset_all_task(MSC* msc);

/* Determine latency of a memory access in the presence of a shared bus
 *
 * 'bb' is the block after which the access takes place
 * 'access_time' is the precise time when the access takes place
 * 'type' specifies whether the memory access is a L1/L2 cache hit or miss
 * 'has_waited_for_next_tdma_slot' is an output variable which (if not NULL)
 *                                 will contain the information whether the
 *                                 current access had to wait for the next
 *                                 TDMA slot
 * 'accessScenario' should indicate whether we are analyzing WCET or BCET
 */
uint determine_latency( const block * const bb, const ull access_time,
    const acc_type type, _Bool * const has_waited_for_next_tdma_slot,
    enum AccessScenario accessScenario );


/* Computes the end alignment cost of a loop iteration which ends at 'fin_time'.
 * This is only needed for analyses which use the concept of aligning loops
 * to the TDMA slots to increase the analysis precision. */
ull endAlign( ull fin_time );
/* Computes the start alignment cost of a loop which starts at 'start_time'.
 * This is only needed for analyses which use the concept of aligning loops
 * to the TDMA slots to increase the analysis precision. */
ull startAlign( ull start_time );


/* #### WCRT analysis helper functions #### */

/* Given a MSC and a task inside it, this function computes
 * the earliest start time of the argument task. Finding out
 * the earliest start time is important as the bus aware BCET
 * analysis depends on the same */
void update_succ_task_earliest_start_time( MSC* msc, task_t* task );
/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks
 * imposed by the partial order of the MSC */
ull get_earliest_task_start_time( task_t* cur_task, uint core );

/* Given a MSC and a task inside it, this function computes
 * the latest start time of the argument task. Finding out
 * the latest start time is important as the bus aware WCET
 * analysis depends on the same */
void update_succ_task_latest_start_time( MSC* msc, task_t* task );
/* Returns the latest starting of a task in the MSC */
ull get_latest_task_start_time( task_t* cur_task, uint core );

/* #### Other helper functions #### */

/* Given a task this function returns the core number in which
 * the task is assigned to. Assignment to cores to individual
 * tasks are done statically before any analysis took place */
uint get_core(task_t* cur_task);


#endif
