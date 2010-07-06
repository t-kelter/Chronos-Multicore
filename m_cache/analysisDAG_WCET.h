/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based WCET analysis functions.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_WCET_H
#define __CHRONOS_ANALYSIS_DAG_WCET_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Goes through DAG in reverse topological order (given in topo), collecting weight.
 * Can be used for both loop (disregarding back-edge) and procedure call (disregarding loops).
 * Reverse topological order ensures that by the time we reach a block,
 * the cost of its successors are already computed.
 *
 * The cost of a basic block (final value of wcet_arr) includes:
 * - its own cost, i.e. #instruction cycles + variable access cost
 * - cost of procedure called within it
 * - cost of its immediate successor
 * (and then, minus variable access cost due to allocated variables).
 *
 * When writing ILP constraints, cost of procedure call and cost of immediate successor
 * are given as variables (with their own related constraints), e.g.
 *   W1 = W2 + cost1 - GainAlloc + WProc
 * where cost1 is the cost of the basic block (variable access cost included).
 * Thus we should first obtain this constant, then write ILP constraints,
 * and then only update the cost to include successor and procedure call.
 *
 * Note: the above is written in CPLEX as
 *   W1 - W2 + GainAlloc - WProc = cost1
 * i.e. LHS are variables only and RHS is a single constant.
 *
 */
int analyseDAGFunction_WCET(procedure *proc, int index);

int analyseDAGLoop_WCET(procedure *proc, loop *lop, int index );

/*
 * Determines WCET and WCET path of a procedure.
 * First analyse loops separately, starting from the inmost level.
 * When analysing an outer loop, the inner loop is treated as a black box.
 * Then analyse the procedure by treating the outmost loops as black boxes.
 */
void analyseProc_WCET( procedure *p );

/*
 * Determines WCET and WCET path of a program.
 * Analyse procedures separately, starting from deepest call level.
 * proc_topo stores the reverse topological order of procedure calls.
 * When analysing a procedure, a procedure call is treated as a black box.
 * The main procedure is at the top call level,
 * and WCET of program == WCET of main procedure.
 */
int analysis_dag_WCET(MSC *msc);

/* Returns the callee procedure for a procedure call instruction */
procedure* getCallee(instr* inst, procedure* proc);

/* Return the type of the instruction access MISS/L1_HIT/L2_HIT.
 * This is computed from the shared cache analysis */
acc_type check_hit_miss(block* bb, instr* inst);

/* Compute waiting time for a memory request and a given 
 * deterministic TDMA schedule */
uint compute_waiting_time(core_sched_p head_core, 
                          ull start_time, acc_type type);

/* Determine approximate memory/bus delay for a L1 cache miss */
uint determine_latency(block* bb, uint context, uint bb_cost, acc_type type);

/* Computes end alignment cost of the loop */
ull endAlign(ull fin_time);

/* computes start alignment cost */
uint startAlign();

/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeWCET(ull start_time);

/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeBCET(ull start_time);

/* Given a task this function returns the core number in which 
 * the task is assigned to. Assignment to cores to individual 
 * tasks are done statically before any analysis took place */
uint get_core(task_t* cur_task);

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC(MSC *msc, const char *tdma_bus_schedule_file);

// TODO: from here on this is all BCET stuff -> should be moved to the BCET file

/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC(MSC *msc);


#endif
