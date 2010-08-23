/*! This is a header file of the Chronos timing analyzer. */

/*
 * Declarations for general framework.
 *
 * When DEF_GLOBALS is defined, this file defines all global variables, if that
 * is not the case, then it just declares them as external.
 */

#ifndef __CHRONOS_HEADER_H
#define __CHRONOS_HEADER_H

#include <time.h>
#include <stdio.h>

#include "wcrt/cycle_time.h"


// ######### Macros #########

#define TRUE 1
#define FALSE 0

#define OP_LEN 9  // length of assembly token
#define INSN_SIZE 8
#define UNROLL 2

/* Maximum string length. */
#define MAX_LEN 512
/* Maximal supported loop nesting level. */
#define MAX_NEST_LOOP   32

/* Memory allocation with error check. */

#define MALLOC( ptr, type, size, msg ) \
  { ptr = ( type ) malloc( (size) ); \
  if( !(ptr) ) { printf( "\nError: malloc failed for %s.\n\n", (msg) ); exit(1); } }

#define CALLOC( ptr, type, len, size, msg ) \
  { ptr = ( type ) calloc( (len), (size) ); \
  if( !(ptr) ) { printf( "\nError: calloc failed for %s.\n\n", (msg) ); exit(1); } }

#define REALLOC( ptr, type, size, msg ) \
  { ptr = ( type ) realloc( (ptr), (size) ); \
  if( !(ptr) ) { printf( "\nError: realloc failed for %s.\n\n", (msg) ); exit(1); } }

// A macro for allocating and assigning memory of the given size to 'ptr',
// depending on whether a previous allocation exists
#define CALLOC_IF_NULL( ptr, type, size, msg ) \
  { if ( ptr == NULL ) { \
      ptr = ( type ) calloc( 1, size ); \
      if( !(ptr) ) { printf( "\nError: CALLOC_IF_NULL failed for %s.\n\n", (msg) ); exit(1); } \
    } \
  }

// A macro for allocating /reallocating and assigning memory of the given
// size to 'ptr', depending on whether a previous allocation exists
#define CALLOC_OR_REALLOC( ptr, type, size, msg ) \
  { if ( ptr == NULL ) { \
      ptr = ( type ) calloc( 1, size ); \
    } else { \
      ptr = ( type ) realloc( ptr, size ); \
    } \
    if( !(ptr) ) { printf( "\nError: CALLOC_OR_REALLOC failed for %s.\n\n", (msg) ); exit(1); } \
  }

// MIN / MAX macros

#define MIN(a,b) ( (a < b) ? a : b )
#define MAX(a,b) ( (a < b) ? b : a )

/*
 * Declarations for WCET analysis.
 */
#define PROC 1
#define LOOP 2

// Helper declarations
#define IS_CALL(x) ((strcmp((x), "jal") == 0) ? 1 : 0)
#define GET_CALLEE(x) (x->r1)


// ######### Datatype declarations  ###########


typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ull;

/* Helper enum to form loop contexts with the 'loop_level_arr' */
enum LoopLevelStates
{
  INVALID = 0, FIRST_ITERATION, NEXT_ITERATION
};

enum AnalysisType
{
  ANALYSIS_ILP = 1, // ILP formulation
  ANALYSIS_DAG, // DAG based analysis
  ANALYSIS_ENUM
// path enumeration
};

struct procs;

typedef struct
{
  char addr[OP_LEN];
  char op[OP_LEN];
  char r1[OP_LEN];
  char r2[OP_LEN];
  char r3[OP_LEN];
} instr;

/* Cache configuration for a specific cache level. */
typedef struct
{
  // decided by cache config parameters
  int ns; // #sets
  int ls; // cache line size
  int nsb; // nsb = log2(ns)
  int lsb; // lsb = log2(ls)
  int hit_latency; // duration of a cache hit
  int miss_latency; // cache miss penalty
  int na; // #associativity

  // decided by cache config + program size
  int nt; // #tags
  int ntb; // tag bits = log2(nt)

  // combined bits
  int s_lb; // (set + line) bits
  int t_sb; // (tag + set) bits
  int nt_s; // # of (tag + set)

  // masks for set, tag, etc
  uint l_msk; // block mask
  uint s_msk; // set mask
  uint t_msk; // tag mask
  uint t_s_msk;// set+tag mask
} cache_t;

typedef struct
{
  int hitmiss;
  char *hitmiss_addr;

  int hit, miss, unknow;
  int *hit_addr, *miss_addr, *unknow_addr;
  char *hit_change_miss, *age;
  int hit_copy;
  int unknow_copy;

  uint bcost; // block execution time: best case for cache analysis
  uint bcost_copy;

  uint wcost; // block execution time: worst case for cache analysis
  uint wcost_copy;
} CHMC;

//one way of a cache line  data structure
typedef struct
{
  unsigned short num_entry; // number of entries
  int *entry; // entry address
} cache_line_way_t;

//one of the cache states  for a bb
typedef struct
{
  cache_line_way_t *** must; //must cache state
  cache_line_way_t *** may; //may cache state
  cache_line_way_t *** persist; //persist cache state

//block *source_bb;
} cache_state;

//basic block data structure
typedef struct
{
  int bbid;
  int pid; // id of the procedure it belongs to
  int startaddr; // address of its first instruction

  int *outgoing; // destination id-s of outgoing edges (at most 2)
  int num_outgoing; // 0, 1, or 2
  int num_outgoing_copy;

  int *incoming; // destination id-s of outgoing edges (at most 2)
  int num_incoming; //

  int callpid; // id of called procedure (by compiler convention, at most one per block)
  uint size; // size of block in bytes

  uint cost; // block execution time

  int loopid; // id of the (innermost) loop it participates in; -1 if not in loop
  char is_loophead;
  instr **instrlist; // list of assembly instructions in the block
  int num_instr;

  /* These are the cache hit/miss classifications. The cache analysis does a persistence
   * analysis, therefore, we distinguish for each loop, between the first and the rest of
   * the iterations (as they may have a different cache state). If a block is nested in
   * multiple loops, then it has different cache states depending on whether the loops are
   * in the first or another iteration, leading to 2^(no. of surrounding loops) possibly
   * different CHMCs. The number of CHMCs is stored in the 'num_chmc' / 'num_chmc_L2' fields,
   * and the CHMCs themselves are stored in the 'chmc' / 'chmc_L2' arrays.
   *
   * The index of a certain CHMC for basic block 'b' in loop context 'c' is then computed as:
   *
   * index_{b,c} = \sum_{i \in [0,l]} ( 1 << i ) * c_i
   *
   * When 'l' is the loop nesting level of the basic block. 'c_i' is '1' when loop 'i' is
   * not in the first iteration, and '0' if it is. Thus the index for the case when all loops
   * are in their first iteration, is 0, the index for the case when the outmost and the two
   * innermost loop of 4 loops are in their second iteration is 1101 (binary) or 13 (decimal)
   * and so on. */
  CHMC **chmc, **chmc_L2;
  int num_chmc, num_chmc_L2;

  cache_state *bb_cache_state, *bb_cache_state_L2;
  int num_cache_state, num_cache_state_L2;
  struct procs* proc_ptr;

  int num_access;

  int num_cache_fetch;
  int num_cache_fetch_L2;

  /* sudiptac :: needed for WCET aanalysis with shared data bus */
  /* The loop contexts in the following are the indexes into the 'chmc' field. */
  ull start_time; // The latest starting time of the block (only valid, if it is in the function's body and not in a loop)
  ull finish_time; // The latest finishing time of the block (only valid, if it is in the function's body and not in a loop)
  ull start_opt[64]; // The latest starting time of the block depending on its loop context (only valid, if it is in a loop)
  ull fin_opt[64]; // The latest finishing time of the block depending on its loop context (only valid, if it is in a loop)

  /* The total BCET that this block causes in the course of a full BCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_bcet;
  /* The total WCET that this block causes in the course of a full WCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_wcet;

} block;

/*
 * Regarding loops:
 *
 * loopexit:
 *   There may be multiple exits from multiple points inside the loop
 *   due to breaks or premature returns. For WCET estimation, should choose
 *   the "normal" loop exit (i.e. exit due to violation of loop condition).
 *   Loop exits are detected automatically; if multiple exits are detected,
 *   user will be prompted to choose the normal exit.
 *
 * is_dowhile:
 *   If the loop is a do-while loop, each block in the loop including loophead
 *   is assumed to be executed [loopbound] times.
 *   If the loop is not a do-while loop, the execution count of the loophead
 *   is taken to be [loopbound] + 1, to include the last loop-condition testing
 *   (that leads to exit) that happens in the loophead.
 *
 * loopbound and is_dowhile for all loops are to be provided by user
 * in a file [filename].lb, using the following format:
 *   <proc_id> <loop_id> <loopbound> <is_dowhile>
 * User will be prompted to do so after all loops are detected and displayed.
 *
 * Note:
 * If the file [filename].lb already exists (e.g. from previous run),
 * it will directly be read in and user will not be prompted.
 * So make sure that the file (if exists) is updated with correct information.
 *
 */

// loop data structure
typedef struct
{
  int lpid;
  int pid; // id of the procedure it belongs to

  block *loophead; // pointer to the block that heads the loop
  block *loopsink; // pointer to the block that is the sink of the loop
  block *loopexit; // pointer to the (normal) exit block of the loop

  int loopbound; // max. number of loop iterations
  int level; // nesting level of the loop, outmost being 0
  int nest; // id of the loop that this loop is immediately nested in (-1 if not nested)
  char is_dowhile; // indicates if the loop is a do-while loop (0/1)
  block **topo; // topologically sorted pointers to the blocks it contains, excluding nested loops
  int num_topo;
  ull *wcet; // result of wcet analysis for this procedure
  ull *bcet;

  int num_cost;

  char num_fm;
  char num_fm_L2;

  char *wpath; // wcet path, a binary sequence (as string) reflecting branch choices
  ull start_opt[64]; // The starting time of the loop depending on its loop context (see block:chmc above)
  ull fin_opt[64]; // The finishing time of the loop depending on its loop context (see block:chmc above)
  ull bcet_opt[64]; // The BCET of the loop depending on its loop context (see block:chmc above)
  ull wcet_opt[64]; // The WCET of the loop depending on its loop context (see block:chmc above)

  /* The total BCET that this loop causes in the course of a full BCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_bcet;
  /* The total WCET that this loop causes in the course of a full WCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_wcet;

} loop;

// procedure data structure
typedef struct procs
{
  int pid;
  block **bblist; // list of basic blocks in the procedure
  int num_bb;
  loop **loops; // list of loops in the procedure
  int num_loops;
  int *calls; // id-s of called procedure
  int num_calls;
  block **topo; // topologically sorted pointers to blocks in bblist, excluding loops
  int num_topo; // <= num_bb, as loops are excluded

  ull *wcet; // result of wcet analysis for this procedure
  ull *bcet;

  int num_cost;

  char *wpath; // wcet path, a binary sequence (as string) reflecting branch choices
  char *hit_cache_set_L2;
  cache_line_way_t *hit_addr;

  /* Required for WCET calculation in presence of shared bus */
  ull running_cost;
  ull running_finish_time;

  /* The total BCET that this procedure causes in the course of a full BCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_bcet;
  /* The total WCET that this procedure causes in the course of a full WCET analysis.
   * (Mainly used for statistical purposes.) */
  ull total_wcet;

} procedure;

typedef struct
{
  procedure **proc;
  int num_proc;
} proc_copy;

typedef struct
{
  int task_id;
  char task_name[MAX_LEN];

  procedure *main_copy;
  procedure **procs; // The list of procedures in the task
  int num_proc;      // The number of procedures in the task
  proc_copy *proc_cg_ptr;
  procedure* entry_proc;

  /* sudiptac :: For bus-aware WCET analysis */
  int numSuccs;
  int* succList;

  ull wcet;
  ull bcet;

  /* sudiptac :: For bus aware WCET analysis */
  ull earliest_start_time;
  ull latest_start_time;

  milliseconds bcet_analysis_time;
  milliseconds wcet_analysis_time;

} task_t;

typedef struct
{
  task_t *taskList;
  int num_task;
  char msc_name[MAX_LEN];
  int **interferInfo;
} MSC;

/* sudiptac :: Data structures and definitions used for WCET 
 analysis with shared data bus */

typedef enum acc_tag
{
  L2_MISS = 0, L1_HIT, L2_HIT
} acc_type;

/* Types of TDMA bus schedule */
typedef enum
{
  SCHED_TYPE_1 = 0, SCHED_TYPE_2, SCHED_TYPE_3
} sched_type;

struct core_sched
{
  ull start_time; /* starting time of the first slot for the core */
  uint interval; /* time interval between two consecutive slots of the same core */
  uint slot_len; /* slot length of the core in this segment */
};
typedef struct core_sched core_sched_s;
typedef struct core_sched* core_sched_p;

struct segment
{
  ull seg_start; /* starting time of the segment */
  ull seg_end; /* ending time of the segment */
  core_sched_p* per_core_sched; /* Core wise schedule information */
};
typedef struct segment segment_s;
typedef struct segment* segment_p;

struct schedule
{
  sched_type type; /* type of the TDMA bus schedule */
  segment_p* seg_list; /* list of segments in the whole schedule length */
  uint n_segments; /* number of segments in the full schedule */
  uint n_cores; /* number of cores active */
};
typedef struct schedule sched_s;
typedef struct schedule* sched_p;

// ######### Global variables declarations / definitions ###########


#ifdef DEF_GLOBALS
#define EXTERN 
#else
#define EXTERN extern
#endif

EXTERN char *interferePathName;
EXTERN char *filename;

EXTERN char *numConflictTask; //to sum up number of tasks that map to the same cache set
EXTERN char *numConflictMSC; //to sum up number of tasks that map to the same cache set within one MSC

EXTERN char infeas; // infeasibility checking on/off

EXTERN procedure **procs; // list of procedures in the program
EXTERN int num_procs;
EXTERN int main_id; // id of main procedure
EXTERN int *proc_cg; // reverse topological order of procedure call graph

EXTERN MSC **msc;

EXTERN int total_bb; // total number of basic blocks in the whole program

EXTERN int times_iteration;
EXTERN int num_core;
EXTERN cache_t cache, cache_L2;

EXTERN int *loop_level_arr;
EXTERN procedure *main_copy;
/*
 * Declarations for WCET analysis.
 */
EXTERN ull *enum_paths_proc; // number of paths in each procedure
EXTERN ull *enum_paths_loop; // number of paths in each loop (in currently analysed procedure)

EXTERN unsigned short ****enum_pathlist; // enum_pathlist[p][b]: list of enumerated paths kept at proc. p block b
EXTERN unsigned short ***enum_pathlen; // enum_pathlen[p][b][n]: length of the n-th path in enum_pathlist[p][b]

#ifdef EXTERN
EXTERN char do_inline;
#else
EXTERN char do_inline = 0;
#endif

/* sudiptac :: Data structures and definitions used for 
 WCET analysis with shared data bus */

/* Global taking the value of current core number
 * in which the program is being executed */
EXTERN uint ncore;
/* Global representing currently analysed task */
EXTERN task_t* cur_task;
/* Stores current earliest time at which a certain core (index) is available. */
EXTERN ull* earliest_core_time;
/* Stores current latest time at which a certain core (index) is available. */
EXTERN ull* latest_core_time;
/* Global TDMA bus schedule */
EXTERN sched_p global_sched_data;
/* Set if testing mode is ON */
EXTERN uint g_testing_mode;
/* Set if shared bus analysis is on */
EXTERN uint g_shared_bus;
/* Set if running independent tasks on multiple cores */
EXTERN uint g_independent_task;
/* Set if private L2 cache analysis mode is on */
EXTERN uint g_private;
/* Set if no bus modelling is turned on */
EXTERN uint g_no_bus_modeling;

#endif
