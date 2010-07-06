/*
 * Declarations for general framework.
 */


#include <time.h>

/*
 * Time-keeping:
 *
 * Change CYCLES_PER_MSEC according to CPU speed where this is run.
 * e.g. 3 GHz = 3E+9 cyc/sec = 3E+6 cyc/millisec
 *
 * CLOCKS_PER_SEC is defined in <time.h>
 */

//#define CYCLES_PER_MSEC 3000000
// #define CLOCKS_PER_MSEC (CLOCKS_PER_SEC / 1000)


typedef unsigned int uint;
typedef unsigned long long ull;

// #define BYTE_PER_INSTR 8
// #define MISS_PENALTY 10

#define OP_LEN 9  // length of assembly token

#define ILP  1    // ILP formulation
#define DAG  2    // DAG based analysis
#define ENUM 3    // path enumeration

#define INSN_SIZE 8
#define UNROLL 2


/*for cache analysis
  *
*/
#define FIRST_ITERATION 1
#define NEXT_ITERATION 2
#define INVALID                  0

#define MAX_CACHE_SETS      1024
#define MAX_NEST_LOOP   32
#define MAX_LEN 512

// 14-bit tag is selected for the following reason:
// - it is enough: can analyze program up to 16MB for a 1-KB cache
// - in cache.c, a valid bit is needed with tag in some cases, thus the valid
//   bit and the tag can be accomodated in a short int
#define MAX_TAG_BITS        14

#define CACHE_LINE(addr)    ((addr) & cache.l_msk)
#define SET(addr)       (((addr) & cache.s_msk) >> cache.lsb)
#define TAG(addr)       (((addr) & cache.t_msk) >> cache.s_lb)
#define TAGSET(addr)        (((addr) & cache.t_s_msk) >> cache.lsb)
#define TAGSET2(tag, set)   (((tag) << cache.nsb) | (set))
#define LSB_OFF(addr)       ((addr) >> cache.lsb)
#define LSB_ON(addr)        ((addr) << cache.lsb)



#define CACHE_LINE_L2(addr)    ((addr) & cache_L2.l_msk)
#define SET_L2(addr)       (((addr) & cache_L2.s_msk) >> cache_L2.lsb)
#define TAG_L2(addr)       (((addr) & cache_L2.t_msk) >> cache_L2.s_lb)
#define TAGSET_L2(addr)        (((addr) & cache_L2.t_s_msk) >> cache_L2.lsb)
#define TAGSET2_L2(tag, set)   (((tag) << cache_L2.nsb) | (set))
#define LSB_OFF_L2(addr)       ((addr) >> cache_L2.lsb)
#define LSB_ON_L2(addr)        ((addr) << cache_L2.lsb)


// clear the LSB bits (all LSB bits are set to 0 whatever they are)

#define CLEAR_LSB(addr)     (((addr) >> cache.lsb) << cache.lsb)

#define CLEAR_LSB_L2(addr)     (((addr) >> cache_L2.lsb) << cache_L2.lsb)

#define MBLK_ID(sa, addr)   (TAGSET(addr) - TAGSET(sa))

#define IC_HIT          1
#define IC_MISS         10
//#define IC_UNCLEAR      2

#define IC_HIT_L2          10
#define IC_MISS_L2        100

#define MC_INC_SIZE     8


#define ALWAYS_HIT 1
#define ALWAYS_MISS 2
#define FIRST_MISS 3
#define UNKNOW 4
#define HIT_UPPER 5

#define NOT_USED 0
#define USED   1

#define MISS 0
#define HIT 1

//typedef struct block block;
//typedef struct loop loop;
//typedef struct procedure procedure;

#define print 0

struct procs;
enum acc_tag;

typedef struct { 
  char addr[OP_LEN];
  char op[OP_LEN];
  char r1[OP_LEN];
  char r2[OP_LEN];
  char r3[OP_LEN];
  enum acc_tag* acc_t;
  enum acc_tag* acc_t_l2;
} instr;


// cache configuration
typedef struct {
    // decided by cache config parameters
    int     ns;     // #sets
    int     ls;     // cache line size
    int     nsb;        // nsb = log2(ns)
    int     lsb;        // lsb = log2(ls)
    int     cmp;    // Level one cache miss penalty
    int     na;     // #associativity

    // decided by cache config + program size
    int     nt; // #tags
    int     ntb;    // tag bits = log2(nt)

    // combined bits
    int     s_lb;   // (set + line) bits
    int     t_sb;   // (tag + set) bits
    int     nt_s;   // # of (tag + set)

    // masks for set, tag, etc
    uint    l_msk;  // block mask
    uint    s_msk;  // set mask
    uint    t_msk;  // tag mask
    uint    t_s_msk;// set+tag mask
} cache_t;

/*
// memory block data structure (complete memory block info)
typedef struct {
  int addr;
  int cacheset;
  int category; // always-hit, always-miss, persistent, unknown
} mblk;


// memory block data structure (complete memory block info)
// in Chronos
typedef struct {
    unsigned short  set;    // cache line
    unsigned short  tag;    // valid tag
} mem_blk_t;
*/


//one way of a cache line  data structure

typedef struct{
    int hitmiss;
    char *hitmiss_addr;

    int hit, miss, unknow;
    int *hit_addr, *miss_addr, *unknow_addr;
    char *hit_change_miss, *age;
    int hit_copy;
    int unknow_copy;

    uint  bcost;           // block execution time: best case for cache analysis
    uint  bcost_copy;
    
   uint  wcost;           // block execution time: worst case for cache analysis
   uint  wcost_copy;
}CHMC;

typedef struct {
    unsigned short  num_entry;    // number of entries
    int *entry;    // entry address
} cache_line_way_t;

//one of the cache states  for a bb
typedef struct {
    cache_line_way_t *** must;        //must cache state
    cache_line_way_t *** may;         //may cache state
    cache_line_way_t *** persist;         //persist cache state
    
    //block *source_bb;
} cache_state;


/*
//data structure of all cache states for one bb 
typedef struct {  
    int loop_level;                   //loop level of the bb
    cache_state_t **cs;
}cache_state;
*/

  
//basic block data structure
typedef struct {
  int   bbid;
  int   pid;            // id of the procedure it belongs to
  int   startaddr;      // address of its first instruction

  
  int   *outgoing;      // destination id-s of outgoing edges (at most 2)
  int  num_outgoing;   // 0, 1, or 2
  int  num_outgoing_copy;
  
  int   *incoming;      // destination id-s of outgoing edges (at most 2)
  int  num_incoming;   // 
  
  int   callpid;        // id of called procedure (by compiler convention, at most one per block)
  uint  size;           // size of block in bytes

  //uint  cost;           // block execution time
 // int reset;
  
  //ull   hit;
  //ull   miss;

//  uint  bcost;           // block execution time: best case for cache analysis
//  uint  wcost;           // block execution time: worst case for cache analysis

//  int numMblks;         // memory block of size == cache line size
//  mblk *mBlks;
//int *hitmisscategory; // hitmisscategory[i] = category of mBlk[i]

  //int   regid;          // for dynamic optimizations: the id of the region it belongs to
  int   loopid;         // id of the (innermost) loop it participates in; -1 if not in loop
  char  is_loophead;
  instr **instrlist;    // list of assembly instructions in the block
  int   num_instr;

  CHMC **chmc, **chmc_L2;
  int num_chmc, num_chmc_L2;

  cache_state *bb_cache_state, *bb_cache_state_L2;
  int num_cache_state, num_cache_state_L2;
  struct procs* proc_ptr;
  //procedure *proc_self;
  //loop * lp_ptr;

  int num_access; 
  
  int num_cache_fetch;    
  int num_cache_fetch_L2;    
  
//  char *hitmiss;
//  int hit, miss, unknow;
//  int *hit_addr, *miss_addr, *unknow_addr;

/* sudiptac :: needed for WCET aanalysis with shared data 
 * bus */
 ull start_time;
 ull finish_time;
 ull start_opt[64];
 ull fin_opt[64];
 ull latest_bus[64];
 ull latest_latency[64];
} block;

// loop data structure
typedef struct {
  int   lpid;
  int   pid;            // id of the procedure it belongs to
  int   loopbound;      // max. number of loop iterations
  block *loophead;      // pointer to the block that heads the loop
  block *loopsink;      // pointer to the block that is the sink of the loop
  block *loopexit;      // pointer to the (normal) exit block of the loop
  int   level;          // nesting level of the loop, outmost being 0
  int   nest;           // id of the loop that this loop is immediately nested in (-1 if not nested)
  char  is_dowhile;     // indicates if the loop is a do-while loop (0/1)
  block **topo;         // topologically sorted pointers to the blocks it contains, excluding nested loops
  int   num_topo;
  ull   *wcet;           // result of wcet analysis for this procedure
  ull    *bcet;
  
  ull   *hit_wcet;
  ull   *miss_wcet;
  ull   *unknow_wcet;
  
  ull   *hit_wcet_L2;
  ull   *miss_wcet_L2;
  ull   *unknow_wcet_L2;


  ull   *hit_bcet;
  ull   *miss_bcet;
  ull   *unknow_bcet;
  
  ull   *hit_bcet_L2;
  ull   *miss_bcet_L2;
  ull   *unknow_bcet_L2;

  int   num_cost;
  
  char num_fm;
  char num_fm_L2;
  
  char  *wpath;         // wcet path, a binary sequence (as string) reflecting branch choices
  ull start_opt[64];
  ull fin_opt[64];
  ull wcet_opt[64];
}loop;


// procedure data structure
typedef struct procs {
  int   pid;
  block **bblist;       // list of basic blocks in the procedure
  int   num_bb;
  loop  **loops;        // list of loops in the procedure
  int   num_loops;
  int   *calls;         // id-s of called procedure
  int   num_calls;
  block **topo;         // topologically sorted pointers to blocks in bblist, excluding loops
  int   num_topo;       // <= num_bb, as loops are excluded

  ull   *wcet;           // result of wcet analysis for this procedure
  ull    *bcet;
  
  ull   *hit_wcet;
  ull   *miss_wcet;
  ull   *unknow_wcet;
  
  ull   *hit_wcet_L2;
  ull   *miss_wcet_L2;
  ull   *unknow_wcet_L2;


  ull   *hit_bcet;
  ull   *miss_bcet;
  ull   *unknow_bcet;
  
  ull   *hit_bcet_L2;
  ull   *miss_bcet_L2;
  ull   *unknow_bcet_L2;

  int   num_cost;
  
  char  *wpath;         // wcet path, a binary sequence (as string) reflecting branch choices
  char *hit_cache_set_L2;
  cache_line_way_t *hit_addr;
  //char *hit_cache_set_L2_copy;

  /* Required for WCET calculation in presence of shared bus */
  ull running_cost;
  ull running_finish_time;
}procedure;

typedef struct
{
    procedure **proc;
    int num_proc;
}proc_copy;


typedef struct
{
    int task_id;
    char task_name[MAX_LEN];
    procedure *main_copy;
    int num_proc;
    proc_copy *proc_cg_ptr;
    procedure* entry_proc;
	 /* sudiptac :: For bus-aware WCET analysis */
	 int numSuccs;
	 int* succList;

    ull wcet;
    ull bcet;
    
    ull hit_wcet;
    ull miss_wcet;
    ull unknow_wcet;    

    ull hit_wcet_L2;
    ull miss_wcet_L2;
    ull unknow_wcet_L2; 


    ull hit_bcet;
    ull miss_bcet;
    ull unknow_bcet;    

    ull hit_bcet_L2;
    ull miss_bcet_L2;
    ull unknow_bcet_L2; 

	 /* sudiptac :: For bus aware WCET analysis */
	 ull l_start;	  
}task_t;

typedef struct
{
    task_t *taskList;
    int num_task;
    char msc_name[MAX_LEN];
    int **interferInfo;
}MSC;




char *interferePathName;
char *filename;
char *cache_config;
char *cache_config_L2;

char *numConflictTask; //to sum up number of tasks that map to the same cache set
char *numConflictMSC; //to sum up number of tasks that map to the same cache set within one MSC

int  method;            // analysis methods: ILP/DAG/ENUM
char infeas;            // infeasibility checking on/off 
char debug;             // text dump on/off

 double t;          //record execution time


procedure **procs;      // list of procedures in the program
int  num_procs;
int  main_id;           // id of main procedure
int  *proc_cg;          // reverse topological order of procedure call graph

MSC **msc;


//proc_copy *proc_cg_ptr;//pointers to all copies of the procedure

int  total_bb;          // total number of basic blocks in the whole program

//int reset;
int times_iteration;
int num_core;
int instr_per_block;
int instr_per_block_L2;
//int numblks;
//int blksize;
//int blkstaddr;

//int total_loop_level;

cache_t cache, cache_L2;
//cache_state *copy;

//int cache_line_bits;
//int cache_line_len;

//int num_instr_line; //number of instructions for one cache line

int *loop_level_arr;

procedure *main_copy;

// dynamic optimizations
int numregs;
uint *regioncost;

int regionmode; 

/* sudiptac :: Data structures and definitions used for WCET
 * analysis with shared data bus */
#define L1_HIT_LATENCY 1
#define L2_HIT_LATENCY 6
#define MISS_PENALTY 36
#define MAX_BUS_DELAY 120
#define IS_CALL(x) ((strcmp((x), "jal") == 0) ? 1 : 0)
#define GET_CALLEE(x) (x->r1)
typedef enum acc_tag{
	L2_MISS = 0,
	L1_HIT,
	L2_HIT
} acc_type;	

/* Global taking the value of current core number
 * in which the program is being executed */
uint ncore;

/* Types of TDMA bus schedule */
#define PARTITION_CORE -1
typedef enum {
	SCHED_TYPE_1 = 0,
	SCHED_TYPE_2,
	SCHED_TYPE_3
} sched_type;

struct core_sched {
	ull start_time; /* starting time of the first slot for the core */	
	uint interval;	 /* time interval between two consecutive slots of the same core */ 
	uint slot_len;	 /* slot length of the core in this segment */	
};
typedef struct core_sched core_sched_s;
typedef struct core_sched* core_sched_p;

struct segment {
	ull seg_start;	 /* starting time of the segment */	
	ull seg_end;	 /* ending time of the segment */	
	core_sched_p* per_core_sched;	/* Core wise schedule information */
};
typedef struct segment segment_s;
typedef struct segment* segment_p;

struct schedule {
	sched_type type;	   /* type of the TDMA bus schedule */
	segment_p* seg_list; /* list of segments in the whole schedule length */	
	uint n_segments;	   /* number of segments in the full schedule */ 	
	uint n_cores;	      /* number of cores active */ 	
};
typedef struct schedule sched_s;
typedef struct schedule* sched_p;

/* Global representing currently analyzed task */
task_t* cur_task;

/* Stores current latest time of all the cores */
ull* latest;

/* Global TDMA bus schedule */
sched_p global_sched_data;
uint cur_context;
uint acc_bus_delay;

/* Set if testing mode is ON */
uint g_testing_mode;
/* Set if shared bus analysis is on */
uint g_shared_bus;
/* Set if running independent tasks on multiple cores */
uint g_independent_task;
/* Set if private L2 cache aanalysis mode is on */
uint g_private;
/* Set if no bus modeing is turned on */
uint g_no_bus_modeling;

/* Total number of instructions analyzed */
ull all_inst;
/* Set if optimized mode is on */
int g_optimized;

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

