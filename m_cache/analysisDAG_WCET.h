/*! This is a header file of the Chronos timing analyzer. */

/*
 * DAG-based WCET analysis functions.
 */

#ifndef __CHRONOS_ANALYSIS_DAG_WCET_H
#define __CHRONOS_ANALYSIS_DAG_WCET_H

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

static uint get_hex(char* hex_string);

#ifdef _PRINT
static void dump_prog_info(procedure* proc);
#endif

/* Return the procedure pointer in the task data structure */
static procedure* get_task_callee(uint startaddr);

/* Returns the callee procedure for a procedure call instruction */
procedure* getCallee(instr* inst, procedure* proc);

#ifdef _DEBUG
/* This is for debugging. Dumped chmc info after preprocessing */
static void dump_pre_proc_chmc(procedure* proc);
#endif

/* Attach hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2(instr* inst, CHMC** chmc_l2, 
                             int n_chmc_l2, int inst_id);

/* Attach hit/miss classification to the instruction */
static void classify_inst(instr* inst, CHMC** chmc, int n_chmc, int inst_id);

/* Reset start and finish time of all basic blocks in this 
 * procedure */
static void reset_timestamps(procedure* proc, ull start_time);

/* Attach chmc classification for L2 cache to the instruction 
 * data structure */
static void preprocess_chmc_L2(procedure* proc);

/* Attach chmc classification to the instruction data structure */
static void preprocess_chmc(procedure* proc);

/* Return the type of the instruction access MISS/L1_HIT/L2_HIT.
 * This is computed from the shared cache analysis */
acc_type check_hit_miss(block* bb, instr* inst);

/* This sets the latest starting time of a block
 * during WCET calculation */
static void set_start_time(block* bb, procedure* proc);

static void set_start_time_opt(block* bb, procedure* proc, uint context);

/* Compute waiting time for a memory request and a given 
 * deterministic TDMA schedule */
uint compute_waiting_time(core_sched_p head_core, 
                          ull start_time, acc_type type);

/* Given a starting time and a particular core, this function 
 * calculates the variable memory latency for the request */
static int compute_bus_delay(ull start_time, uint ncore, acc_type type);

/* Check whether the block specified in the header "bb"
 * is header of some loop in the procedure "proc" */
static loop* check_loop(block* bb, procedure* proc);

/* Determine approximate memory/bus delay for a L1 cache miss */
uint determine_latency(block* bb, uint context, uint bb_cost, acc_type type);

/* Computes end alignment cost of the loop */
ull endAlign(ull fin_time);

/* computes start alignment cost */
uint startAlign();

/* Preprocess one loop for optimized bus aware WCET calculation */
/* This takes care of the alignments of loop at te beginning and at the 
 * end */
static void preprocess_one_loop(loop* lp, procedure* proc, ull start_time);

/* Preprocess each loop for optimized bus aware WCET calculation */
static void preprocess_all_loops(procedure* proc);

/* Computes the latest finish time and worst case cost of a loop.
 * This procedure fully unrolls the loop virtually during computation.
 * Can be optimized for specific bus schedule ? */
static void computeWCET_loop(loop* lp, procedure* proc);

/* Compute worst case finish time and cost of a block */
static void computeWCET_block(block* bb, procedure* proc, loop* cur_lp);

static void computeWCET_proc(procedure* proc, ull start_time);

/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeWCET(ull start_time);

/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeBCET(ull start_time);

/* Given a MSC and a task inside it, this function computes 
 * the latest start time of the argument task. Finding out 
 * the latest start time is important as the bus aware WCET
 * analysis depends on the same */
static void update_succ_latest_time(MSC* msc, task_t* task);

/* Given a task this function returns the core number in which 
 * the task is assigned to. Assignment to cores to individual 
 * tasks are done statically before any analysis took place */
uint get_core(task_t* cur_task);

/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks 
 * imposed by the partial order of the MSC */
static ull get_latest_start_time(task_t* cur_task, uint core);

/* Reset latest start time of all tasks in the MSC before 
 * the analysis of the MSC starts */
static void reset_all_task(MSC* msc);

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC(MSC *msc, const char *tdma_bus_schedule_file);

// TODO: from here on this is all BCET stuff -> should be moved to the BCET file

/* Attach hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2_BCET(instr* inst, CHMC** chmc_l2, int n_chmc_l2,
      int inst_id);

/* Attach hit/miss classification to the instruction */
static void classify_inst_BCET(instr* inst, CHMC** chmc, 
                               int n_chmc, int inst_id);

/* Attach chmc classification for L2 cache to the instruction 
 * data structure */
static void preprocess_chmc_L2_BCET(procedure* proc);

/* Attach chmc classification to the instruction data structure */
static void preprocess_chmc_BCET(procedure* proc);

/* This sets the latest starting time of a block
 * during BCET calculation */
static void set_start_time_BCET(block* bb, procedure* proc);

/* Computes the latest finish time and worst case cost
 * of a loop */
static void computeBCET_loop(loop* lp, procedure* proc);

/* Compute worst case finish time and cost of a block */
static void computeBCET_block(block* bb, procedure* proc, loop* cur_lp);

static void computeBCET_proc(procedure* proc, ull start_time);

/* Given a MSC and a task inside it, this function computes 
 * the earliest start time of the argument task. Finding out 
 * the earliest start time is important as the bus aware BCET
 * analysis depends on the same */
static void update_succ_earliest_time(MSC* msc, task_t* task);

/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks 
 * imposed by the partial order of the MSC */
static ull get_earliest_start_time(task_t* cur_task, uint core);

/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC(MSC *msc);


#endif
