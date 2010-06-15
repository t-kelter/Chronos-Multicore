/* 
 * SS_SMT.C - simulador for a SMT architecture
 *
 * This simulator was developed by Ronaldo A. L. Goncalves using 
 * the original sim-outorder simulator from SimpleScalar 3.0 tool.
 *
 * Ronaldo is professor at the Informatics Department in the State 
 * University of Maringa - Brazil. 
 *
 * Contacts with him can be done by e_mail: ronaldo@din.uem.br
 * More details about him can be found in http://www.din.uem.br/~ronaldo
 *
 * This work was done as part of his PhD thesis in the SEMPRE project.
 * His advisor was Dr. Philippe Navaux from Federal University of Rio 
 * Grande do Sul - Brazil and contacts with him can be done by e-mail:
 * navaux@inf.ufrgs.br
 * 
 * Also, the professors Mateo Valero and Eduard Ayguade from UPC/Barcelona
 * help Ronaldo to do this simulator, during a technological interchange 
 * accomplished at the labs of the DAC/UPC. Contacts with them can be done
 * by e_mail: mateo@ac.upc.es and eduard@ac.upc.es, respectively.
 *
 * The original "sim_outorder.c" file is a part of the SimpleScalar tool 
 * suite written by Todd M. Austin as a part of the Multiscalar Research 
 * Project.
 *  
 * The SimpleScalar tool suite is currently maintained by Doug Burger and 
 * Todd M. Austin.
 * 
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * SimpleScalar Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * Adaptation 1.0  2000/02/26  Ronaldo A. L. Goncalves
 * The original sim-outorder.c was remodeled to do the ss_smt.c version 1.0.
 * The main modifications/adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots). However, the funtional units are
 *    shared among these slots. The cache system can be configured as shared
 *    or distributed or as some other models. The pipeline stages (fetch,
 *    dispatch, issue, writeback and commit) are shared among these slots.
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. Many function calls were modified to pass the slot's number
 *    argument.
 * 3) The options and stats handle was modified to support many options to
 *    configure the SMT pipeline, like these: -imodules:num, -fu:type, 
 *    option "forced" for -bpred:type, -max:cycles, -l1_bus_opt and so on. The 
 *    parameter <nbanks> was included in the cache configuration.
 * 4) The handle of the command line was modified in order to support the
 *    input of many applications.
 * 5) Sharing the bus between dl1 and il1 caches.
 * 6) A lot of files from Simplescalar Toll 3.0 was modified to agree
 *    with this simulator. So, IS NOT POSSIBLE to execute this simulator
 *    using the original SS Tool 3.0.
 * 
 * Revision 1.5  1998/08/27 16:27:48  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 * instruction predecoding moved to loader bank
 * Alpha target support added
 * added support for quadword's
 * added fault support
 * added option ("-max:inst") to limit number of instructions analyzed
 * explicit BTB sizing option added to branch predictors, use
 *       "-btb" option to configure BTB
 * added queue statistics for IFQ, RUU, and LSQ; all terms of Little's
 *       law are measured and reports; also, measures fraction of cycles
 *       in which queue is full
 * added fast forward option ("-fastfwd") that skips a specified number
 *       of instructions (using functional simulation) before starting timing
 *       simulation
 * sim-outorder speculative loads no longer allocate memory pages,
 *       this significantly reduces memory requirements for programs with
 *       lots of mispeculation (e.g., cc1)
 * branch predictor updates can now optionally occur in ID, WB,
 *       or CT
 * added target-dependent myprintf() support
 * fixed speculative quadword store bug (missing most significant word)
 * sim-outorder now computes correct result when non-speculative register
 *       operand is first defined speculative within the same inst
 * speculative fault handling simplified
 * dead variable "no_ea_dep" removed
 *
 * Revision 1.4  1997/04/16  22:10:23  taustin
 * added -commit:width support (from kskadron)
 * fixed "bad l2 D-cache parms" fatal string
 *
 * Revision 1.3  1997/03/11  17:17:06  taustin
 * updated copyright
 * `-pcstat' option support added
 * long/int tweaks made for ALPHA target support
 * better defaults defined for caches/TLBs
 * "mstate" command supported added for DLite!
 * supported added for non-GNU C compilers
 * buglet fixed in speculative trace generation
 * multi-level cache hierarchy now supported
 * two-level predictor supported added
 * I/D-TLB supported added
 * many comments added
 * options package supported added
 * stats package support added
 * resource configuration options extended
 * pipetrace support added
 * DLite! support added
 * writeback throttling now supported
 * decode and issue B/W now decoupled
 * new and improved (and more precise) memory scheduler added
 * cruft for TLB paper removed
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "cache.h"
#include "loader.h"
#include "syscall.h"
#include "bpred.h"
#include "resource.h"
#include "bitmap.h"
#include "options.h"
#include "eval.h"
#include "stats.h"
#include "ptrace.h"
#include "dlite.h"
#include "sim.h"
#include "eio.h"
#include "symbol.h"
#include "libexo.h"

/*
 * This file implements a very detailed out-of-order issue superscalar
 * processor with a two-level memory system and speculative execution support.
 * This simulator is a performance simulator, tracking the latency of all
 * pipeline operations.
 */

/* The variable "errno", from "include <errno.h>", is just only and because there are many slots and each one can cause an error, this structure was created in order to keep the integrity of each error from different slot. See sim_init for initialization */

int aux_errno[MAX_SLOTS];

extern int errno;

/* Because that all processes have the same addressing space provided by   */
/* compiler, the cache miss in a unified bank can be so much poor, we try  */
/* provide a different inicial PC for each one. So, we forces the use of   */
/* "fast_forward instructions" in order to try provide this. The use of    */
/* "-page:step" option also help. But, it is not sufficient and the use of */
/* associativy cache is necessary when the number os process is big. How big */
/* is the associativity so big is the performance, in this case. */

#define MIN_FORWARD 0 
static int forward_step; /* more a different increment for each process in the icache */

md_addr_t data_offset;  /* set a different increment for each process in the dcache*/


/* number of slots (replicated structures). This variable is exported in host.h */
int process_num = 1;

/* number of independet i-cache modules with independent bus and that can be fetched simultaneously. The "process_num" must be a multiple of "il1_modules_num". Each i-cache module serves a different pool of n SLOTS such that n = IL1_MODULE_WIDTH. */

int il1_modules_num = 1;
#define IL1_MODULE_WIDTH    		(process_num / il1_modules_num)
#define FIRST_SLOT_OF_MOD(module)   	(module*IL1_MODULE_WIDTH)

/* number of banks inside of each icache module, but just one bank can be fetched per time. The banks are multiplexed inside each icache module. The "IL1_MODULE_WIDTH" must be a multiple of "il1_banks_num". The bank number "b" inside of a module "m" serves the slot "sn" such that:
         -> m = (sn / IL1_MODULE_WIDTH)
   	 -> b = WHICHIS_IL1BANK_OF_SLOT(sn)

Remenber: sn goes from 0 to process_num 
          b goes from 0 to il1_banks_num 
          m goes from 0 to il1_modules_num 
*/

int il1_banks_num = 1;

#define IL1_BANKS_TOTAL  (il1_modules_num*il1_banks_num)

/* number of d-cache banks that can be used. The process_num must be a multiple of dl1_banks_num. Each d-cache bank serves n SLOTS such as n = DL1_BANK_WIDTH. */
int dl1_banks_num = 1;

#define DL1_BANK_WIDTH     (process_num / dl1_banks_num)
#define IL1_BANK_WIDTH     (process_num / IL1_BANKS_TOTAL)

#define WHICHIS_DL1BANK_OF_SLOT(slot)	(slot / DL1_BANK_WIDTH)
#define WHICHIS_IL1BANK_OF_SLOT(slot)	(slot / IL1_BANK_WIDTH)

/* number of unified L2-cache banks. Both il1_banks_num and dl1_banks_num must be multiple of l2_banks_num */

int l2_banks_num = 1;

#define DL2_BANK_WIDTH	   		(dl1_banks_num / l2_banks_num)
#define IL2_BANK_WIDTH	   		(IL1_BANKS_TOTAL / l2_banks_num)
#define FIRST_DL1BANK_OF_DL2(dl2bank)   (dl2bank * DL2_BANK_WIDTH)	 
#define FIRST_IL1BANK_OF_IL2(il2bank)   (il2bank * IL2_BANK_WIDTH)  
#define WHICHIS_DL2BANK_OF_DL1(dl1bank) (dl1bank / DL2_BANK_WIDTH)
#define WHICHIS_IL2BANK_OF_IL1(il1bank) (il1bank / IL2_BANK_WIDTH)

int decode_depth = 0; /* see sim_check_opt for initialization */

/* This variable is set or when one program try to execute a "exit" instruction either when one program reaches its "max:inst option". In these cases, the architectiure continues to processing the other applications on the other slot for the current cycle in order to all applications to be executed the same quantity of cycle */  
int finished = FALSE;

/* when a instruction exit ocurres the exit_slot receives the current slot's number */
int exit_slot = -1;

/* to register the number of both slot and i-cache bank which have higher miss */
int max_slot_delay = 0;
int max_delayed_slot = -1;
int max_bank_delay = 0;
int max_delayed_bank = -1;
static unsigned int max_delayed_bank_cycle = 0;

/* simulated registers: one independent bank for each pipeline of the SMT processor. See sim_init for initialization */

static struct regs_t *regs[MAX_SLOTS] = {NULL};

/* simulated memory, one independet memory structure for each pipeline of the SMT processor - distributed memory. Normalmente, a inicializacao abaixo eh dependente de compilador. Em muitos deles eh necessario inicializar todos os campos ponteiros tal como {NULL, NULL, NULL ...NULL}. See sim_init for initialization*/

static struct mem_t *mem[MAX_SLOTS] = {NULL};

/*
 * simulator options. */

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* maximum number of cycle's to exexecute */
static unsigned int max_cycles;

/* number of insts skipped before timing starts */
static int fastfwd_count;

/* pipeline trace range and output filename */
static int ptrace_nelt;
static char *ptrace_opts[2];

/* instruction fetch queue size (in insts) */
static int ruu_ifq_size;

/* extra branch mis-prediction latency */
static int ruu_branch_penalty;

/* speed of front-end of machine relative to execution core */
static int fetch_speed;

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type;

/* bimodal predictor config (<table_size>). See sim_init for initialization */
static int bimod_nelt;
static int bimod_config[1];

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
static int twolev_nelt;
static int twolev_config[4];

/* combining predictor config (<meta_table_size> */
static int comb_nelt;
static int comb_config[1];

/* return address stack (RAS) size. See sim_init for initialization */
static int ras_size;

/* BTB predictor config (<num_sets> <associativity>) */
static int btb_nelt;
static int btb_config[2];

/* instruction fetch width */
static int ruu_fetch_width;

/* instruction decode B/W (insts/cycle) is the same for each slot */
static int ruu_decode_width;

/* instruction issue B/W (insts/cycle) is the same for each slot */
static int ruu_issue_width;

/* run pipeline with in-order issue */
static int ruu_inorder_issue;

/* issue instructions down wrong execution paths */
static int ruu_include_spec;

/* instruction commit B/W (insts/cycle) is the same for each slot */
static int ruu_commit_width;

/* register update unit (RUU) size is the same for each slot */
static int RUU_size;

/* load/store queue (LSQ) size is the same for each slot*/
static int LSQ_size;

/* register update unit (RUU) total size to simulate "shared" RUU */
static int ruu_totalsize;

/* load/store queue (LSQ) total size to simulate "shared" LSQ*/
static int lsq_totalsize;

/* to define "distributed" or "shared" ruu/lsq for the slots */
static char *ruulsq_opt = "shared";

/* l1 data cache config, i.e., {<config>|none} */
static char *cache_dl1_opt;

/* l1 data cache hit latency (in cycles) */
static int cache_dl1_lat;

/* l2 data cache config, i.e., {<config>|none} */
static char *cache_dl2_opt;

/* l2 data cache hit latency (in cycles) */
static int cache_dl2_lat;

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il1_opt;

/* l1 instruction cache hit latency (in cycles) */
static int cache_il1_lat;

/* l2 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il2_opt;

/* l2 instruction cache hit latency (in cycles) */
static int cache_il2_lat;

/* l1-l2 bus option: <0>-individual <1>-shared between il1&dl1*/
static int l1_bus_opt = 1;

/* flush caches on system calls */
static int flush_on_syscalls;

/* convert 64-bit inst addresses to 32-bit inst equivalents */
static int compress_icache_addrs;

/* memory access latency (<first_chunk> <inter_chunk>) */
static int mem_nelt;
static int mem_lat[2];

/* memory access bus width (in bytes) */
static int mem_bus_width;

/* instruction TLB config, i.e., {<config>|none} */
static char *itlb_opt;

/* data TLB config, i.e., {<config>|none} */
static char *dtlb_opt;

/* inst/data TLB miss latency (in cycles) */
static int tlb_miss_lat;


/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
static int pcstat_nelt;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#ifdef TARGET_PISA
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - ld_text_base[sn]) >> 1) + ld_text_base[sn]) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))
#else /* !TARGET_PISA */
#define IACOMPRESS(A)	(A)
#define ISCOMPRESS(SZ)		(SZ)
#endif /* TARGET_PISA */

/* operate in backward-compatible bugs mode (for testing only) */
static int bugcompat_mode;

/*
 * functional unit resource configuration
 */

char *fu_pool_type = "hetero";

/* resource pool indices, NOTE: update these if you change FU_CONFIG */
#define FU_IALU_INDEX			0
#define FU_IMULT_INDEX			1
#define FU_MEMPORT_INDEX		2
#define FU_FPALU_INDEX			3
#define FU_FPMULT_INDEX			4

/* homo resource pool indices, NOTE: update these if you change FU_CONFIG */
#define FU_HOMO_INDEX			0

/* compact resource pool indices, NOTE: update these if you change FU_CONFIG */
#define FU_DIVMULT_INDEX		1

/* hetero resource pool definition, NOTE: update FU_*_INDEX defs if you change this.See sim_init for initialization */

struct res_desc fu_hetero_config[5] = {
  {
    "integer-ALU",
    4,
    0,
    {
      { IntALU, 1, 1 }
    }
  },
  {
    "integer-MULT/DIV",
    1,
    0,
    {
      { IntMULT, 3, 1 },
      { IntDIV, 20, 19 }
    }
  },
  {
    "memory-port",
    2,
    0,
    {
      { RdPort, 1, 1 },
      { WrPort, 1, 1 }
    }
  },
  {
    "FP-ALU",
    4,
    0,
    {
      { FloatADD, 2, 1 },
      { FloatCMP, 2, 1 },
      { FloatCVT, 2, 1 }
    }
  },
  {
    "FP-MULT/DIV",
    1,
    0,
    {
      { FloatMULT, 4, 1 },
      { FloatDIV, 12, 12 },
      { FloatSQRT, 24, 24 }
    }
  },
};

/* homo resource pool definition, NOTE: update FU_*_INDEX defs if you change this.See sim_init for initialization */
struct res_desc fu_homo_config[1]  = {
  {
    "homogeneous",
    4,
    0,
    {
      { IntALU, 1, 1 },
      { IntMULT, 3, 1 },
      { IntDIV, 20, 19 },
      { FloatADD, 2, 1 },
      { FloatCMP, 2, 1 },
      { FloatCVT, 2, 1 },
      { FloatMULT, 4, 1 },
      { FloatDIV, 12, 12 },
      { FloatSQRT, 24, 24 },
      { RdPort, 1, 1 },
      { WrPort, 1, 1 }
    },
  }
};

/* compact resource pool definition, NOTE: update FU_*_INDEX defs if you change this.See sim_init for initialization */

struct res_desc fu_compact_config[4] = {
  {
    "Integer-ALU",
    1,
    0,
    {
      { IntALU, 1, 1 }
    }
  },
  {
    "Div/Mult",
    1,
    0,
    { { IntALU, 1, 1 },

      { FloatADD, 2, 1 },
      { FloatCMP, 2, 1 },
      { FloatCVT, 2, 1 },

      { IntMULT, 3, 1 },
      { IntDIV, 20, 19 },
      { FloatMULT, 4, 1 },
      { FloatDIV, 12, 12 },
      { FloatSQRT, 24, 24 }
    }
  },
  {
    "Memory-Port",
    1,
    0,
    {
      { RdPort, 1, 1 },
      { WrPort, 1, 1 }
    }
  },
  {
    "FP-ALU",
    1,
    0,
    {
      { FloatADD, 2, 1 },
      { FloatCMP, 2, 1 },
      { FloatCVT, 2, 1 }
    }
  },
};


/* total number of integer ALU's available */
static int res_ialu;

/* total number of integer multiplier/dividers available */
static int res_imult;

/* total number of memory system ports available (to CPU) */
static int res_memport;

/* total number of floating point ALU's available */
static int res_fpalu;

/* total number of floating point multiplier/dividers available */
static int res_fpmult;

/* total number of homogenoeus functional units available */
static int res_homo;

/* total number of div/mult funcitional units available */
static int res_divmult;

/*
 * simulator stats. These variables must be replicated for all SMT's pipeline. So, their initializations must be done in the "sim_init" function because process_num is variable
 */

/* total number of instructions executed */
static counter_t sim_total_insn[MAX_SLOTS];

/* total number of previous instructions executed */
static counter_t prev_sim_total_insn[MAX_SLOTS];

/* total number of instructions NOPs in the IFQ */
static counter_t sim_total_nops_insn[MAX_SLOTS];

/* total number of memory references committed */
static counter_t sim_num_refs[MAX_SLOTS];

/* total number of memory references executed */
static counter_t sim_total_refs[MAX_SLOTS];

/* total number of loads committed */
static counter_t sim_num_loads[MAX_SLOTS];

/* total number of loads executed */
static counter_t sim_total_loads[MAX_SLOTS];

/* total number of branches committed */
static counter_t sim_num_branches[MAX_SLOTS];

/* total number of branches executed */
static counter_t sim_total_branches[MAX_SLOTS];

/* total number of branches committed and predicted correctly */
static counter_t sim_num_good_pred[MAX_SLOTS];

/* cycle counter */
static tick_t sim_cycle=0;

/* occupancy counters */
static counter_t IFQ_count[MAX_SLOTS];	/* cumulative IFQ occupancy */
static counter_t IFQ_fcount[MAX_SLOTS];	/* cumulative IFQ full count */
static counter_t RUU_count[MAX_SLOTS];	/* cumulative RUU occupancy */
static counter_t RUU_fcount[MAX_SLOTS];	/* cumulative RUU full count */
static counter_t LSQ_count[MAX_SLOTS];	/* cumulative LSQ occupancy */
static counter_t LSQ_fcount[MAX_SLOTS];	/* cumulative LSQ full count */

/* total non-speculative bogus addresses seen (debug var) */
static counter_t sim_invalid_addrs[MAX_SLOTS];

/*
 * simulator state variables
 */

/* instruction sequence counter, used to assign unique id's to insts */
static unsigned int inst_seq[MAX_SLOTS];

/* pipetrace instruction sequence counter */
static unsigned int ptrace_seq[MAX_SLOTS];

/* speculation mode, non-zero when mis-speculating, i.e., executing
   instructions down the wrong path, thus state recovery will eventually have
   to occur that resets processor register and memory state back to the last
   precise state. See sim_init for the initializations */
static int spec_mode[MAX_SLOTS];

/* cycles until slot is ready to fetches instructions again. See sim_init for the initializations */
int ruu_slot_delay[MAX_SLOTS];

/* perfect prediction enabled */
static int pred_perfect = FALSE;

/* forced prediction enabled.*/
static int pred_forced = FALSE;

/* value for forced prediction. Default is 95% */
static int bpred_rate = 95;

/* speculative bpred-update enabled */
static char *bpred_spec_opt;
static enum { spec_ID, spec_WB, spec_CT } bpred_spec_update;

/* level 1 instruction cache, entry level instruction cache */
static struct cache_t *cache_il1[MAX_SLOTS];

/* level 2 instruction cache */
static struct cache_t *cache_il2[MAX_SLOTS];

/* level 1 data cache, entry level data cache */
static struct cache_t *cache_dl1[MAX_SLOTS];

/* level 2 data cache */
static struct cache_t *cache_dl2[MAX_SLOTS];

/* instruction TLB */
static struct cache_t *itlb[MAX_SLOTS];

/* data TLB */
static struct cache_t *dtlb[MAX_SLOTS];

/* branch predictor */
static struct bpred_t *pred[MAX_SLOTS];

/* functional unit resource pool. See sim_init for the initializations */
static struct res_pool *fu_pool = NULL;

/* text-based stat profiles */
static struct stat_stat_t *pcstat_stats[MAX_SLOTS][MAX_PCSTAT_VARS];
static counter_t pcstat_lastvals[MAX_SLOTS][MAX_PCSTAT_VARS];
static struct stat_stat_t *pcstat_sdists[MAX_SLOTS][MAX_PCSTAT_VARS];

/* wedge all stat values into a counter_t */
#define STATVAL(STAT)							\
  ((STAT)->sc == sc_int							\
   ? (counter_t)*((STAT)->variant.for_int.var)			\
   : ((STAT)->sc == sc_uint						\
      ? (counter_t)*((STAT)->variant.for_uint.var)		\
      : ((STAT)->sc == sc_counter					\
	 ? *((STAT)->variant.for_counter.var)				\
	 : (panic(sn,"bad stat class"), 0))))


/* memory access latency, assumed to not cross a page boundary */
static unsigned int			/* total latency of access */
mem_access_latency(int blk_sz, int sn)	/* block size accessed, slot's number */
{
  int chunks = (blk_sz + (mem_bus_width - 1)) / mem_bus_width;

  assert(chunks > 0);

  return (/* first chunk latency */mem_lat[0] +
	  (/* remainder chunk latency */mem_lat[1] * (chunks - 1)));
}


/*
 * cache miss handlers
 */

/* l1 data cachE block miss handler function */
static unsigned int			/* latency of block access */
dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now,		/* time of access */
              int sn)                   /* slot's number to be accessed */
{
  unsigned int lat;
 

  if (cache_dl2[0])
    { int dl2bank, dl1bank;

      /* access next level of data cache hierarchy */

      dl1bank = WHICHIS_DL1BANK_OF_SLOT(sn);
      dl2bank = WHICHIS_DL2BANK_OF_DL1(dl1bank);

if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
   fprintf(stderr,"\ndl2 bank >> %d \n",dl2bank);


      /* CAUTION: Note that bsize from l1 must be <= bsize l2 */
      lat = cache_access(cache_dl2[dl2bank], cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, /* repl addr */NULL,
                         /* slot's number */ sn);
      if (cmd == Read)
	return lat;
      else
	{
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
    }
  else
    {
      /* access main memory */
      if (cmd == Read)
	return mem_access_latency(bsize,sn);
      else
	{
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
    }
}

/* l2 data cache block miss handler function */
static unsigned int			/* latency of block access */
dl2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now,		/* time of access */
              int sn)                   /* slot's number */ 
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize,sn);
  else
    {
      /* FIXME: unlimited write buffers */
      return 0;
    }
}

/* l1 inst cache l1 block miss handler function */
static unsigned int			/* latency of block access */
il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now,		/* time of access */
              int sn)                   /* slot's number */
{
  unsigned int lat;


if (cache_il2[0])
    {  int il2bank, il1bank;

      il1bank = WHICHIS_IL1BANK_OF_SLOT(sn);
      il2bank = WHICHIS_IL2BANK_OF_IL1(il1bank);

if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
   fprintf(stderr,"\nil2 bank >> %d \n",il2bank);

      /* access next level of inst cache hierarchy */
      lat = cache_access(cache_il2[il2bank], cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, /* repl addr */NULL, 
                         /* slot's number*/ sn);
      if (cmd == Read)
	return lat;
      else
	panic(sn,"writes to instruction memory not supported");
    }
  else
    {

      /* access main memory */
      if (cmd == Read)
	return mem_access_latency(bsize,sn);
      else
	panic(sn,"writes to instruction memory not supported");
    }
}

/* l2 inst cache block miss handler function */
static unsigned int			/* latency of block access */
il2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now,		/* time of access */
              int sn) 			/* slot's number */
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize,sn);
  else
    panic(sn,"writes to instruction memory not supported");
}


/*
 * TLB miss handlers
 */

/* inst cache block miss handler function */
static unsigned int			/* latency of block access */
itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,		/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now,		/* time of access */
               int sn) 			/* slot's number */
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

/* data cache block miss handler function */
static unsigned int			/* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,	/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now,		/* time of access */
               int sn) 			/* slot's number */
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}


/* register simulator-specific options. The database odb is lust one for all
   pipelines on the SMT processor to make more easy the global configuration 
   than on many diferent databases */
void
sim_reg_options(struct opt_odb_t *odb)
{  
  char opt_name[64]; /* to contain the option name */
   
  opt_reg_header(odb, 
"SS_SMT: This simulator implements a SMT processor architecture that\n"
"has many pipelines totally independents. Each pipeline is a very detailed\n"
"out-of-order issue superscalar processor with a two-level memory system\n"
"and speculative execution support. This simulator is a performance simulator\n" 
"and was developed from SimpleScalar 3.0 Tool.\n");

  opt_reg_string(odb, "-fu:type",
		 "type of functional units: <hetero|homo:n|compact>",
                 &fu_pool_type, /* default */"hetero",
                 /* print */TRUE, /* format */NULL,0);

  /* instruction limit. It is the same for all slots */

  opt_reg_uint(odb,"-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL,0);

 /* cycle limit. It is the same for all slots */

  opt_reg_uint(odb,"-max:cycles", "maximum number of cycles to execute",
	       &max_cycles, /* default */0,
	       /* print */TRUE, /* format */NULL,0);

  opt_reg_uint(odb,"-imodules:num", "number of i-cache modules",
	       &il1_modules_num, /* default 1*/il1_modules_num,
	       /* print */TRUE, /* format */NULL,0);


  opt_reg_uint(odb,"-il1banks:num", "number of i-cache internal banks",
	       &il1_banks_num, /* default 1*/il1_banks_num,
	       /* print */TRUE, /* format */NULL,0);

  opt_reg_uint(odb,"-dl1banks:num", "number of d-cache banks",
	       &dl1_banks_num, /* default 1*/dl1_banks_num,
	       /* print */TRUE, /* format */NULL,0);

  opt_reg_uint(odb,"-ul2banks:num", "number of unified l2-cache banks",
	       &l2_banks_num, /* default 1*/l2_banks_num,
	       /* print */TRUE, /* format */NULL,0);

  opt_reg_uint(odb,"-decode:depth", "depth for decode",
	       &decode_depth, /* default = ifqsize */decode_depth,
	       /* print */TRUE, /* format */NULL,0);

  /* trace options */

  opt_reg_int(odb,"-fastfwd", "number of insts skipped before timing starts",
        &fastfwd_count, /* default */MIN_FORWARD,/* print */TRUE, /* format */NULL,0);

  opt_reg_string_list(odb, "-ptrace",
	 "generate pipetrace, i.e., <fname|stdout|stderr> <range>",
	      ptrace_opts, /* arr_sz */2, &ptrace_nelt, /* default */NULL,
	      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE,0);

  opt_reg_note(odb,
"  Pipetrace range arguments are formatted as follows:\n"
"\n"
"    {{@|#}<start>}:{{@|#|+}<end>}\n"
"\n"
"  Both ends of the range are optional, if neither are specified, the entire\n"
"  execution is traced.  Ranges that start with a `@' designate an address\n"
"  range to be traced, those that start with an `#' designate a cycle count\n"
"  range.  All other range values represent an instruction count range.  The\n"
"  second argument, if specified with a `+', indicates a value relative\n"
"  to the first argument, e.g., 1000:+100 == 1000:1100.  Program symbols may\n"
"  be used in all contexts.\n"
"\n"
"    Examples:   -ptrace FOO.trc #0:#1000\n"
"                -ptrace BAR.trc @2000:\n"
"                -ptrace BLAH.trc :1500\n"
"                -ptrace UXXE.trc :\n"
"                -ptrace FOOBAR.trc @main:+278\n",0
	       );

  /* ifetch options */

  opt_reg_int(odb, "-fetch:ifqsize", "instruction fetch queue size (in insts)", &ruu_ifq_size, /* default */4,/* print */TRUE, /* format */NULL,0);
  

  opt_reg_int(odb, "-fetch:mplat", "extra branch mis-prediction latency",
	      &ruu_branch_penalty, /* default */3,
	      /* print */TRUE, /* format */NULL,0);
 

  opt_reg_int(odb,"-fetch:speed",
	      "speed of front-end of machine relative to execution core",
	      &fetch_speed, /* default */1,
	      /* print */TRUE, /* format */NULL,0);

  
  /* branch predictor options */

  opt_reg_note(odb,
"  Branch predictor configuration examples for 2-level predictor:\n"
"    Configurations:   N, M, W, X\n"
"      N   # entries in first level (# of shift register(s))\n"
"      W   width of shift register(s)\n"
"      M   # entries in 2nd level (# of counters, or other FSM)\n"
"      X   (yes-1/no-0) xor history and address for 2nd level index\n"
"    Sample predictors:\n"
"      GAg     : 1, W, 2^W, 0\n"
"      GAp     : 1, W, M (M > 2^W), 0\n"
"      PAg     : N, W, 2^W, 0\n"
"      PAp     : N, W, M (M == 2^(N+W)), 0\n"
"      gshare  : 1, W, 2^W, 1\n"
"  Predictor `comb' combines a bimodal and a 2-level predictor.\n",0
               );

  opt_reg_string(odb, "-bpred:type",
		 "branch predictor type <nottaken|taken|perfect|bimod|2lev|comb|forced:value>}",
                 &pred_type, /* default */"bimod",
                 /* print */TRUE, /* format */NULL,0);

  opt_reg_int_list(odb, "-bpred:bimod",
		   "bimodal predictor config (<table size>)",
		   bimod_config, bimod_nelt, &bimod_nelt,
		   /* default */bimod_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE,0);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2-level predictor config "
		   "(<l1size> <l2size> <hist_size> <xor>)",
                   twolev_config, twolev_nelt, &twolev_nelt,
		   /* default */twolev_config,
                   /* print */TRUE, /* format */NULL, /* !accrue */FALSE,0);

  opt_reg_int_list(odb, "-bpred:comb",
		   "combining predictor config (<meta_table_size>)",
		   comb_config, comb_nelt, &comb_nelt,
		   /* default */comb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE,0);

  opt_reg_int(odb, "-bpred:ras",
              "return address stack size (0 for no return stack)",
              &ras_size, /* default */ras_size,
              /* print */TRUE, /* format */NULL,0);

  opt_reg_int_list(odb, "-bpred:btb",
		   "BTB config (<num_sets> <associativity>)",
		   btb_config, btb_nelt, &btb_nelt,
		   /* default */btb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE,0);

  opt_reg_string(odb, "-bpred:spec_update",
		 "speculative predictors update in {ID|WB} (default non-spec)",
		 &bpred_spec_opt, /* default */NULL,
		 /* print */TRUE, /* format */NULL,0);

  /* fetch options */

  opt_reg_int(odb, "-fetch:width",
	      "instruction fetch Width (insts/cycle)",
	      &ruu_fetch_width, /* default = decode_width*speed_fetch */0,
	      /* print */TRUE, /* format */NULL,0);

  /* decode options */

  opt_reg_int(odb, "-decode:width",
	      "instruction decode B/W (insts/cycle)",
	      &ruu_decode_width, /* default */4,
	      /* print */TRUE, /* format */NULL,0);

  /* issue options */

  opt_reg_int(odb, "-issue:width",
	      "instruction issue B/W (insts/cycle)",
	      &ruu_issue_width, /* default */4,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_flag(odb, "-issue:inorder", "run pipeline with in-order issue",
	       &ruu_inorder_issue, /* default */FALSE,
	       /* print */TRUE, /* format */NULL,0);

  opt_reg_flag(odb, "-issue:wrongpath",
	       "issue instructions down wrong execution paths",
	       &ruu_include_spec, /* default */TRUE,
	       /* print */TRUE, /* format */NULL,0);

  /* commit options */

  opt_reg_int(odb, "-commit:width",
	      "instruction commit B/W (insts/cycle)",
	      &ruu_commit_width, /* default */4,
	      /* print */TRUE, /* format */NULL,0);

  /* register scheduler options */

  opt_reg_int(odb, "-ruu:size",
	      "register update unit (RUU) size",
	      &RUU_size, /* default */16,
	      /* print */TRUE, /* format */NULL,0);

  /* memory scheduler options  */

  opt_reg_int(odb, "-lsq:size",
	      "load/store queue (LSQ) size",
	      &LSQ_size, /* default */8,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_string(odb, "-ruulsq:type",
		 "it defines <distributed> or <shared> ruu/lsq for the slots",
		 &ruulsq_opt, /* default is <shared> */ruulsq_opt, 
		 /* print */TRUE, NULL,0);
  

  /* cache options */

  opt_reg_note(odb,
"  The cache config parameter <config> has the following format:\n"
"\n"
"    <name>:<nbanks>:<csize>:<bsize>:<assoc>:<repl>\n"
"\n"
"    <name>   - name of the cache being defined\n"
"    <nbanks> - number of banks of this cache\n"
"    <csize>  - size of each bank in K bytes\n"
"    <bsize>  - block size of each bank\n"
"    <assoc>  - associativity of each bank\n"
"    <repl>   - block replacement strategy, 'l'-LRU, 'f'-FIFO, 'r'-random\n"
"\n"
"    Example:   -cache:dl1 dl1:1:64:32:1:l\n"
"\n"
"    The number of sets will be like:"
"\n"
"             (<csize>*1024)/(<bsize>*<assoc>)"
"\n"
"    The total size will be: <nbanks>*<csize>\n"
"\n"
"    CAUTION! for tlb caches don't use <nbanks>!!!\n"
"\n"
"    Example:   -dtlb dtlb:16:2048:2:r\n"
"\n",0
);

  opt_reg_string(odb, "-cache:dl1",
		 "l1 data cache config, i.e., {<config>|none}",
		 &cache_dl1_opt, "dl1:1:16:32:4:l", /* 16K -> nsets = 128 */
		 /* print */TRUE, NULL,0);

  opt_reg_int(odb, "-cache:dl1lat",
	      "l1 data cache hit latency (in cycles)",
	      &cache_dl1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_string(odb, "-cache:dl2",
		 "l2 data cache config, i.e., {<config>|none}",
		 &cache_dl2_opt, "ul2:1:256:64:4:l", /* 256K -> nsets = 1024 */
		 /* print */TRUE, NULL,0);

  opt_reg_int(odb, "-cache:dl2lat",
	      "l2 data cache hit latency (in cycles)",
	      &cache_dl2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_note(odb,
"  Second level cache can be unified by pointing a second level of the\n"
"  instruction cache to second level data cache using the \"dl2\" cache\n"
"  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
"\n"
"    A unified l2 cache (il2 is pointed at dl2):\n"
"      -cache:il1 il1:1:16:64:1:l -cache:il2 dl2\n"
"      -cache:dl1 dl1:1:16:32:1:l -cache:dl2 ul2:1:256:64:2:l\n"
"\n",0 );

 
  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|none}",
		 &cache_il1_opt, "il1:1:16:32:1:l", /* 16K -> nsets = 512 */
		 /* print */TRUE, NULL,0);      

  opt_reg_int(odb, "-cache:il1lat",
	      "l1 instruction cache hit latency (in cycles)",
	      &cache_il1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_string(odb, "-cache:il2",
		 "l2 instruction cache config, i.e., {<config>|dl2|none}",
		 &cache_il2_opt, "dl2",  /* unified with dl2 = 256K */
		 /* print */TRUE, NULL,0);

  opt_reg_int(odb, "-cache:il2lat",
	      "l2 instruction cache hit latency (in cycles)",
	      &cache_il2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_int(odb, "-l1bus:opt",
	      "l1 bus: <0>-individual for dl1 & il1 <1>-shared for them",
	      &l1_bus_opt, /* default 0*/l1_bus_opt,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_flag(odb, "-cache:flush", "flush caches on system calls",
	  &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL,0);

  opt_reg_flag(odb, "-cache:icompress",
	       "convert 64-bit inst addresses to 32-bit inst equivalents",
	       &compress_icache_addrs, /* default */FALSE,
	       /* print */TRUE, NULL,0);

  /* mem options */

  opt_reg_int_list(odb, "-mem:lat",
		   "memory access latency (<first_chunk> <inter_chunk>)",
		   mem_lat, mem_nelt, &mem_nelt, mem_lat,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE,0);

  strcpy(opt_name, "-mem:width");
  opt_reg_int(odb, opt_name, "memory access bus width (in bytes)",
	      &mem_bus_width, /* default */8,
	      /* print */TRUE, /* format */NULL,0);

  /* TLB options */

  opt_reg_string(odb, "-tlb:itlb",
		 "instruction TLB config, i.e., {<config>|none}",
		 &itlb_opt, "itlb:256:4096:4:l", /* print */TRUE, NULL,0);

  opt_reg_string(odb, "-tlb:dtlb",
		 "data TLB config, i.e., {<config>|none}",
		 &dtlb_opt, "dtlb:512:4096:4:l", /* print */TRUE, NULL,0);

  opt_reg_int(odb, "-tlb:lat",
	      "inst/data TLB miss latency (in cycles)",
	      &tlb_miss_lat, /* default */30,
	      /* print */TRUE, /* format */NULL,0);

  /* hetero resource configuration. Remember those functional units are shared */

  opt_reg_int(odb, "-res:ialu",
	      "total number of integer ALU's available",
	      &res_ialu, /* default */fu_hetero_config[FU_IALU_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_int(odb, "-res:imult",
	      "total number of integer multiplier/dividers available",
	      &res_imult, /* default */fu_hetero_config[FU_IMULT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);
 

  opt_reg_int(odb, "-res:memport",
	      "total number of memory system ports available (to CPU)",
	      &res_memport, /* default */ fu_hetero_config[FU_MEMPORT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);
  

  opt_reg_int(odb, "-res:fpalu",
	      "total number of floating point ALU's available",
	      &res_fpalu, /* default */fu_hetero_config[FU_FPALU_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_int(odb, "-res:fpmult",
	      "total number of floating point multiplier/dividers available",
	      &res_fpmult, /* default */ fu_hetero_config[FU_FPMULT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);

/* homo resource configuration. Remember those functional units are shared */

 /* compact resource configuration. Remember those functional units are shared and the variables res_ialu, res_fpalu, res_memport are used in this configuration */

 opt_reg_int(odb, "-res:divmult",
	      "total number of div/mult functional unit available",
	      &res_divmult, /* default */ fu_compact_config[FU_DIVMULT_INDEX].quantity,
	      /* print */TRUE, /* format */NULL,0);

  opt_reg_string_list(odb, "-pcstat",
       "profile stat(s) against text addr's (mult uses ok)",
        pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
	 /* !print */FALSE, /* format */NULL, /* accrue */TRUE,0);

  opt_reg_flag(odb, "-bugcompat",
	       "operate in backward-compatible bugs mode (for testing only)",
	       &bugcompat_mode, /* default */FALSE, /* print */TRUE, NULL,0);
}

void
check_cache_sizes(int csize, int bsize, int assoc)
{
  if (csize < 1 || (csize & (csize-1)) != 0)
    fatal(0,"cache size must be positive non-zero and a power of two");

  if (bsize < 1 || (bsize & (bsize-1)) != 0)
    fatal(0,"cache bsize must be positive non-zero and a power of two");

  if (assoc < 1 || (assoc & (assoc-1)) != 0)
    fatal(0,"cache assoc must be positive non-zero and a power of two");

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb,        /* options database */
		  int argc, char **argv)        /* command line arguments */
{
  char name[64], aux_name[64], c;
  int nsets, l1bsize, l2bsize, assoc, csize, sn, bank;

  if ((max_insts < 0) || (max_insts >= 2147483647))
     fatal(0,"bad max of instructions: %d", max_insts);

  if ( (il1_modules_num <= 0) || ((process_num % il1_modules_num) !=0) ||
       (process_num < il1_modules_num) )
     fatal(0,"process_num must be multiple of il1_modules_num");

  if (fastfwd_count < 0 || fastfwd_count >= 2147483647)
       fatal(0,"bad fast forward count: %d", fastfwd_count);

  if (ruu_ifq_size < 1 || (ruu_ifq_size & (ruu_ifq_size - 1)) != 0)
       fatal(0,"inst fetch queue size must be positive > 0 and a power of two");
  
  if (ruu_branch_penalty < 1)
       fatal(0,"mis-prediction penalty must be at least 1 cycle");

  if (fetch_speed < 1)
       fatal(0,"front-end speed must be positive and non-zero");

  if ( (bpred_rate < 1) || (bpred_rate > 100) )
       fatal(0,"bpred_rate must be an integer between 1 and 100");


  if (sscanf(pred_type, "%[^:]:%d",
	name, &bpred_rate) == 2)
  {  if (!mystricmp(name, "forced"))
     {
       /* perfect predictor */
       for (sn=0;sn<process_num;sn++)
           pred[sn] = NULL;
       pred_forced = TRUE;
     }
     else fatal(0,"bad predictor type `%s'", pred_type);
  }
  else
  for (sn=0;sn<process_num;sn++)
  if (!mystricmp(pred_type, "perfect"))
    {
      /* perfect predictor */
      pred[sn] = NULL;
      pred_perfect = TRUE;
    }
  else if (!mystricmp(pred_type, "taken"))
    {
      /* static predictor, not taken */
      pred[sn] = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0,sn);
    }
  else if (!mystricmp(pred_type, "nottaken"))
    {
      /* static predictor, taken */
      pred[sn] = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0,sn);
    }
  else if (!mystricmp(pred_type, "bimod"))
       {
           /* bimodal predictor, bpred_create() checks BTB_SIZE */
           if (bimod_nelt != 1)
	      fatal(sn,"slot %d : bad bimod predictor config (<table_size>)",sn);
           if (btb_nelt != 2)
	      fatal(sn,"slot %d : bad btb config (<num_sets> <associativity>)",sn);

           /* bimodal predictor, bpred_create() checks BTB_SIZE */
           pred[sn] = bpred_create(BPred2bit,
			  /* bimod table size */bimod_config[0],
			  /* 2lev l1 size */0,
			  /* 2lev l2 size */0,
			  /* meta table size */0,
			  /* history reg size */0,
			  /* history xor address */0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size,
                          /* slot's number */ sn);
       }
       else if (!mystricmp(pred_type, "2lev"))
            {
                /* 2-level adaptive predictor, bpred_create() checks args */
               if (twolev_nelt != 4)
	          fatal(sn,"slot %d : bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)",sn);
               if (btb_nelt != 2)
	          fatal(sn,"slot %d : bad btb config (<num_sets> <associativity>)",sn);

               pred[sn] = bpred_create(BPred2Level,
			  /* bimod table size */0,
			  /* 2lev l1 size */twolev_config[0],
			  /* 2lev l2 size */twolev_config[1],
			  /* meta table size */0,
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size,
                          /* slot's number */ sn);
           }
           else if (!mystricmp(pred_type, "comb"))
                {
                   /* combining predictor, bpred_create() checks args */
                   if (twolev_nelt != 4)
	              fatal(sn,"slot %d : bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)",sn);
                   if (bimod_nelt != 1)
	              fatal(sn,"slot %d : bad bimod predictor config (<table_size>)",sn);
                   if (comb_nelt != 1)
	              fatal(sn,"slot %d : bad combining predictor config (<meta_table_size>)",sn);
                   if (btb_nelt != 2)
	              fatal(sn,"slot %d : bad btb config (<num_sets> <associativity>)",sn);

                   pred[sn] = bpred_create(BPredComb,
			  /* bimod table size */bimod_config[0],
			  /* l1 size */twolev_config[0],
			  /* l2 size */twolev_config[1],
			  /* meta table size */comb_config[0],
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size,
                          /* slot's number */ sn);
                }
                else fatal(sn,"slot %d : cannot parse predictor type `%s'", sn, pred_type);

  if (!bpred_spec_opt)
     bpred_spec_update = spec_CT;
  else if (!mystricmp(bpred_spec_opt, "ID"))
          bpred_spec_update = spec_ID;
       else if (!mystricmp(bpred_spec_opt, "WB"))
               bpred_spec_update = spec_WB;
            else fatal(0,"bad speculative update stage specifier, use {ID|WB}");
  
  if (ruu_fetch_width) /* is it was defined */
     if (ruu_fetch_width < 0 || (ruu_fetch_width & (ruu_fetch_width-1)) != 0)
        fatal(0,"fetch width must be positive non-zero and a power of two");
  
  if (ruu_decode_width < 1 || (ruu_decode_width & (ruu_decode_width-1)) != 0)
      fatal(0,"decode width must be positive non-zero and a power of two");

  if (ruu_issue_width < 1 || (ruu_issue_width & (ruu_issue_width-1)) != 0)
      fatal(0,"issue width must be positive non-zero and a power of two");

  if (ruu_commit_width < 1)
      fatal(0,"commit width must be positive non-zero");

  if (RUU_size < 2 || (RUU_size & (RUU_size-1)) != 0)
      fatal(0,"RUU size must be a positive number > 1 and a power of two");

  if (LSQ_size < 2 || (LSQ_size & (LSQ_size-1)) != 0)
      fatal(0,"LSQ size must be a positive number > 1 and a power of two");

  if (!mystricmp(ruulsq_opt, "shared")) /* The total must be limited */
  {   ruu_totalsize = RUU_size;
      lsq_totalsize = LSQ_size;
  }
  else /* the total must be maximun in order to does not be a limit */
  {   ruu_totalsize = process_num*RUU_size + 1;
      lsq_totalsize = process_num*LSQ_size + 1;
  }

  if ( (!decode_depth) ||
       (decode_depth > ruu_ifq_size) )
      decode_depth = ruu_ifq_size;
  else 
     if (decode_depth < 1)
      fatal(0,"decode_depth must be positive non-zero");

   

  /* use a level 1 D-cache? */
  
  if (!mystricmp(cache_dl1_opt, "none"))
  {
        for (sn=0;sn<dl1_banks_num;sn++)
            cache_dl1[sn] = NULL;

      /* the level 2 D-cache cannot be defined */
      if (strcmp(cache_dl2_opt, "none"))
	fatal(0,"the l1 data cache must defined if the l2 cache is defined");
      for (bank=0;bank<process_num;bank++) 
           cache_dl2[bank] = NULL;
  }
  else /* dl1 is defined */
  {
      if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%d:%c",
		 name, &dl1_banks_num, &csize, &l1bsize, &assoc, &c) != 6) 
	fatal(0,"bad l1 D-cache parms: <name>:<nbanks><csize>:<bsize>:<assoc>:<repl>");

      if ( (dl1_banks_num <= 0) || ((process_num % dl1_banks_num) !=0) ||
           (process_num < dl1_banks_num) )
         fatal(0,"process_num must be multiple of dl1_banks_num");

      check_cache_sizes(csize, l1bsize, assoc);

      data_offset = (csize*1024) / (process_num/dl1_banks_num);

      nsets = (csize*1024)/(l1bsize*assoc);

      fprintf(stderr, "\ndl1 -> nsets = %d\n",nsets);
       
      for (bank=0;bank<dl1_banks_num;bank++)
      {  sprintf(aux_name, "%s_%s",name, itoa[bank]); 
         cache_dl1[bank] = cache_create(aux_name, nsets, l1bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c,bank),
			       dl1_access_fn, /* hit lat */cache_dl1_lat,bank);
      }

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_dl2_opt, "none"))
	 for (bank=0;bank<process_num;bank++) 
             cache_dl2[bank] = NULL;
      else
      {
	 if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%d:%c",
		     name, &l2_banks_num, &csize, &l2bsize, &assoc, &c) != 6)
	    fatal(0,"bad l2 D-cache parms: "
		  "<name>:<nbanks>:<csize>:<bsize>:<assoc>:<repl>");

          if ( (l2_banks_num <= 0) || ((dl1_banks_num % l2_banks_num) !=0) 
               || (dl1_banks_num < l2_banks_num) )
             fatal(0,"l2_banks_num must be multiple of dl1_banks_num");

          check_cache_sizes(csize, l2bsize, assoc);

          if (l2bsize < l1bsize)
            fatal(0,"l1 bsize must fit inside l2 bsize");

          nsets = (csize*1024)/(l2bsize*assoc);

          fprintf(stderr, "\ndl2 -> nsets = %d\n",nsets);

	  for (bank=0;bank<l2_banks_num;bank++)
          {  sprintf(aux_name, "%s_%s",name, itoa[bank]); 
             cache_dl2[bank] = cache_create(aux_name, nsets, l2bsize, /* balloc 
                        */FALSE, /* usize */0, assoc, cache_char2policy(c,0),
			dl2_access_fn, /* hit lat */cache_dl2_lat,bank);
          }
       }
  }

  /* use a level 1 I-cache? */

  if (!mystricmp(cache_il1_opt, "none"))
  {   for (bank=0;bank<IL1_BANKS_TOTAL;bank++)
           cache_il1[bank] = NULL;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
         fatal(0,"the l1 inst cache must defined if the l2 cache is defined");
      
      for (bank=0;bank<l2_banks_num;bank++)
          cache_il2[bank] = NULL;
  } 
  else if (!mystricmp(cache_il1_opt, "dl1"))
       {
           fatal(0,"I-cache l1 cannot access D-cache l1");
       }
  else if (!mystricmp(cache_il1_opt, "dl2"))
       {
           fatal(0,"I-cache l1 cannot access D-cache l2");
       }
  else /* il1 is defined */
  {
       if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%d:%c",
		   name, &il1_banks_num, &csize, &l1bsize, &assoc, &c) != 6)
	   fatal(0,"bad l1 I-cache parms: "
                 "<name>:<nbanks>:<csize>:<bsize>:<assoc>:<repl>");

       if ( (il1_banks_num <= 0) || ((IL1_MODULE_WIDTH % il1_banks_num) !=0) ||
            (IL1_MODULE_WIDTH < il1_banks_num) )
       fatal(0,"IL1_MODULE_WIDTH must be multiple of il1_banks_num");

       check_cache_sizes(csize, l1bsize, assoc);

       forward_step = (csize*1024) / (IL1_MODULE_WIDTH/il1_banks_num); 
  
       nsets = (csize*1024) / (l1bsize*assoc);

       fprintf(stderr, "\nil1 -> nsets = %d\n",nsets);

       for (bank=0;bank<IL1_BANKS_TOTAL;bank++)
       {  int module, aux_bank;

          module = bank / il1_banks_num;

          aux_bank = bank % il1_banks_num; 

          sprintf(aux_name, "%s_M%s_B%s", name,itoa[module],itoa[aux_bank]);
                  
          cache_il1[bank] = cache_create(aux_name, nsets, l1bsize, 
                                     /* balloc */FALSE, /* usize */0, assoc, 
                                     cache_char2policy(c,bank),il1_access_fn, 
                                     /* hit lat */cache_il1_lat, bank);
       }

       /* is the level 2 D-cache defined? */
       if (!mystricmp(cache_il2_opt, "none"))
	    for (bank=0;bank<l2_banks_num;bank++)
                cache_il2[bank] = NULL;
       else if (!mystricmp(cache_il2_opt, "dl2"))
	    {  if (!cache_dl2[0])
	          fatal(0,"I-cache l2 cannot access D-cache l2");

               if ( ((il1_banks_num % l2_banks_num) !=0) 
                    || (il1_banks_num < l2_banks_num) )
                  fatal(0,"l2_banks_num must be multiple of il1_banks_num");

               if (l2bsize < l1bsize)
                  fatal(0,"l1 bsize must fit inside l2 bsize");

               if (l2bsize < l1bsize)
                  fatal(0,"l1 bsize must fit inside l2 bsize");

               fprintf(stderr, "\nil2 nsets = dl2 nsets \n");

	       for (bank=0;bank<l2_banks_num;bank++)
                   cache_il2[bank] = cache_dl2[bank];

	     }
             else
	     {   
                 if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%d:%c",
		           name, &l2_banks_num, &csize, &l2bsize, &assoc, &c) != 6)
	           fatal(0,"bad l2 I-cache parms: "
		           "<name>:<nbanks><csize>:<bsize>:<assoc>:<repl>");

                 if (cache_dl2[0])
                    fatal(0,"if there is a dl2, il2 must be shared");

                 if ( (l2_banks_num <= 0) || ((il1_banks_num % l2_banks_num) !=0) 
                      || (il1_banks_num < l2_banks_num) )
                 fatal(0,"l2_banks_num must be multiple of il1_banks_num");
             
                 check_cache_sizes(csize, l2bsize, assoc);

                 if (l2bsize < l1bsize)
                    fatal(0,"l1 bsize must fit inside l2 bsize");
    
                 nsets = (csize*1024) / (l2bsize*assoc); 
                
                 fprintf(stderr, "\nil2 -> nsets = %d\n",nsets);

                 for (bank=0;bank<l2_banks_num;bank++)
                 {   sprintf(aux_name, "%s_%s",name, itoa[bank]);
                     cache_il2[bank] = cache_create(aux_name, nsets, l2bsize, 
                                         /* balloc */FALSE, /* usize */0, 
                                         assoc, cache_char2policy(c,0),
				         il2_access_fn, /* hit lat */ 
                                         cache_il2_lat, bank);
                 }
	     }
  }

  /* use an I-TLB? */

  if (!mystricmp(itlb_opt, "none"))
     for (sn=0;sn<process_num;sn++)
         itlb[sn] = NULL;
  else
  {  if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &csize, &l1bsize, &assoc, &c) != 5)
	fatal(0,"bad TLB parms: <name>:<csize>:<page_size>:<assoc>:<repl>",0);

      check_cache_sizes(csize, l1bsize, assoc);

      nsets = (csize*1024) / (l1bsize*assoc);

      fprintf(stderr, "\nitlb -> nsets = %d\n",nsets);
      
      for (sn=0;sn<process_num;sn++)
      {  sprintf(aux_name, "%s_%s",name, itoa[sn]);
         itlb[sn] = cache_create(aux_name, nsets, l1bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c,sn), itlb_access_fn,
			  /* hit latency */1,sn);
      }
  }

  /* use a D-TLB? */

  if (!mystricmp(dtlb_opt, "none"))
     for (sn=0;sn<process_num;sn++)
         dtlb[sn] = NULL;
  else
  {  if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &csize, &l1bsize, &assoc, &c) != 5)
        fatal(0,"bad TLB parms: <name>:<csize>:<page_size>:<assoc>:<repl>",0);

     check_cache_sizes(csize, l1bsize, assoc);
      
     nsets = (csize*1024) / (l1bsize*assoc);

     fprintf(stderr, "\ndtlb -> nsets = %d\n",nsets);

     for (sn=0;sn<process_num;sn++)
     {  sprintf(aux_name, "%s_%s",name, itoa[sn]);
        dtlb[sn] = cache_create(aux_name, nsets, l1bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c,sn), dtlb_access_fn,
			  /* hit latency */1,sn);
     }
  }

  if (cache_dl1_lat < 1)
      fatal(sn,"l1 data cache latency must be greater than zero");

  if (cache_dl2_lat< 1)
      fatal(sn,"l2 data cache latency must be greater than zero");

  if (cache_il1_lat< 1)
      fatal(sn,"l1 instruction cache latency must be greater than zero");

  if (cache_il2_lat < 1)
      fatal(sn,"l2 instruction cache latency must be greater than zero");

  if (mem_nelt != 2)
      fatal(sn,"bad memory access latency (<first_chunk> <inter_chunk>)");

  if (mem_lat[0] < 1 || mem_lat[1] < 1)
      fatal(sn,"all memory access latencies must be greater than zero");

  if (mem_bus_width < 1 || (mem_bus_width & (mem_bus_width-1)) != 0)
      fatal(sn,"memory bus width must be positive non-zero and a power of two");

  if (tlb_miss_lat < 1)
      fatal(sn,"TLB miss latency must be greater than zero");


  /* configuration of the functional units. These resources are shared */

  if (res_ialu < 1)
          fatal(sn,"number of integer ALU's must be greater than zero");

  if (res_ialu > MAX_INSTS_PER_CLASS)
          fatal(sn,"number of integer ALU's must be <= MAX_INSTS_PER_CLASS");

     
  if (res_imult < 1)
          fatal(sn,"number of integer multiplier/dividers must be greater than zero");

  if (res_imult > MAX_INSTS_PER_CLASS)
          fatal(sn,"number of integer mult/div's must be <= MAX_INSTS_PER_CLASS");
  
  if (res_memport < 1)
          fatal(sn,"number of memory system ports must be greater than zero");

  if (res_memport > MAX_INSTS_PER_CLASS)
          fatal(sn,"number of memory system ports must be <= "
          "MAX_INSTS_PER_CLASS");
  
  if (res_fpalu < 1)
          fatal(sn,"number of floating point ALU's must be greater than zero");

  if (res_fpalu > MAX_INSTS_PER_CLASS)
          fatal(sn,"number of floating point ALU's must be <= "
          "MAX_INSTS_PER_CLASS");

  if (res_fpmult < 1)
          fatal(sn,"number of floating point multiplier/dividers must be > zero");

  if (res_fpmult > MAX_INSTS_PER_CLASS)
           fatal(sn,"number of FP mult/div's must be <= MAX_INSTS_PER_CLASS"); 
  

  if (res_divmult < 1)
            fatal(sn,"number of divmult functional units must be greater than zero");
  
  if (res_divmult > MAX_INSTS_PER_CLASS)
            fatal(sn,"number of divmult functional units must be <= "
                  "MAX_INSTS_PER_CLASS");


  if (!mystricmp(fu_pool_type, "hetero"))
    {   

       fu_hetero_config[FU_IALU_INDEX].quantity = res_ialu;
  
       fu_hetero_config[FU_IMULT_INDEX].quantity = res_imult;
        
       fu_hetero_config[FU_MEMPORT_INDEX].quantity = res_memport;
  
       fu_hetero_config[FU_FPALU_INDEX].quantity = res_fpalu;

       fu_hetero_config[FU_FPMULT_INDEX].quantity = res_fpmult;

   }
   else  
   {  if (!mystricmp(fu_pool_type, "compact"))    
      {
       
         fu_compact_config[FU_IALU_INDEX].quantity = res_ialu;
  
         fu_compact_config[FU_DIVMULT_INDEX].quantity = res_divmult;
        
         fu_compact_config[FU_MEMPORT_INDEX].quantity = res_memport;
  
         fu_compact_config[FU_FPALU_INDEX].quantity = res_fpalu;
  
      }
      else /* homogeneous fus */   
      {   char name[64];

           if (sscanf(fu_pool_type, "%[^:]:%d", name, &res_homo) != 2)
              fatal(0,"invalid type of functional units (-fu:type)"); 

           if (!mystricmp(name, "homo"))
             fu_homo_config[FU_HOMO_INDEX].quantity = res_homo; 
           else fatal(0,"invalid type of functional units (-fu:type)");

           if (res_homo < 1)
              fatal(sn,"number of homogeneous functional units must be greater than zero");
  
           if (res_homo > MAX_INSTS_PER_CLASS)
               fatal(sn,"number of homogeneous functional units must be "
                    "<= MAX_INSTS_PER_CLASS");
      }

  }


  /* initialize dlite_active */

  for (sn=0;sn<process_num;sn++)
       dlite_active[sn] = dlite_initial;

}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)            /* output stream */
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)   /* stats database */
{
  int i,sn, bank; 
  char stat_name[64];
  char formula_name[256];

  /* register baseline stats */

fprintf(stderr,"\nStarting sim_reg_stats...\n");
  
  sn = 0;

  strcpy(stat_name, "sim_elapsed_time");
  stat_reg_int(sdb, stat_name,
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL,sn); 

  strcpy(stat_name, "exited_slot");
  stat_reg_int(sdb, stat_name,
	       "Slot that has exited firstly",
	       &exit_slot, exit_slot, NULL,sn);
 
  strcpy(stat_name, "max_delayed_slot");
  stat_reg_int(sdb, stat_name,
	       "slot that had the maximum delay",
	       &max_delayed_slot, max_delayed_slot, NULL,sn);

  strcpy(stat_name, "max_slot_delay");
  stat_reg_int(sdb, stat_name,
	       "maximal delay for a slot",
	       &max_slot_delay, max_slot_delay, NULL,sn);

  strcpy(stat_name, "max_delayed_bank");
  stat_reg_int(sdb, stat_name,
	       "bank that had the maximum delay",
	       &max_delayed_bank, max_delayed_bank, NULL,sn);

  strcpy(stat_name, "max_bank_delay");
  stat_reg_int(sdb, stat_name,
	       "maximal delay for a i cache bank",
	       &max_bank_delay, max_bank_delay, NULL,sn);

  strcpy(stat_name, "max_delayed_bank_cycle");
  stat_reg_int(sdb, stat_name,
	       "cycle which has the maximal delay for a i cache bank",
	       &max_delayed_bank_cycle, max_delayed_bank_cycle, NULL,sn);


  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_insn_%s", itoa[sn]);    /* sim_num_insn is defined in sim.h */
     stat_reg_counter(sdb, stat_name,                    /* and it is declared in main.c */
		      "total number of instructions committed",
		      &sim_num_insn[sn], sim_num_insn[sn], NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_refs_%s", itoa[sn]);     
     stat_reg_counter(sdb,stat_name,
		      "total number of loads and stores committed",
		      &sim_num_refs[sn], 0, NULL,sn);
  }		

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_loads_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name,
		      "total number of loads committed",
		      &sim_num_loads[sn], 0, NULL,sn);
  }		   

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_stores_%s", itoa[sn]);
     sprintf(formula_name, "sim_num_refs_%s - sim_num_loads_%s", itoa[sn], itoa[sn]);
     stat_reg_formula(sdb, stat_name,
		      "total number of stores committed",
     		      formula_name, NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_branches_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name,
		      "total number of branches committed",
		      &sim_num_branches[sn], /* initial value */0, /* format */NULL,sn);
  }
    
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_num_good_pred_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name,
		      "total number of branches committed and predicted correctly",
		      &sim_num_good_pred[sn], /* initial value */0, /* format */NULL,sn);
  }


  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_inst_rate_%s", itoa[sn]);
     sprintf(formula_name, "sim_num_insn_%s / sim_elapsed_time", itoa[sn]);     
     stat_reg_formula(sdb, stat_name,
		      "simulation speed (in insts/sec)",
		      formula_name, NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_insn_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name,
		      "total number of instructions executed",
		      &sim_total_insn[sn], 0, NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "prev_sim_total_insn_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name,
		      "total prev number of instructions executed",
		      &prev_sim_total_insn[sn], 0, NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_nops_insn_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name,
		      "total number of NOPs instructions in the IFQ",
		      &sim_total_nops_insn[sn], 0, NULL,sn);
  }

  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_refs_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name,
		      "total number of loads and stores executed",
		      &sim_total_refs[sn], 0, NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_loads_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name,
		      "total number of loads executed",
		      &sim_total_loads[sn], 0, NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_stores_%s", itoa[sn]);
     sprintf(formula_name, "sim_total_refs_%s - sim_total_loads_%s", itoa[sn], itoa[sn]);        
     stat_reg_formula(sdb, stat_name,
		      "total number of stores executed",
		      formula_name, NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_total_branches_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name,
		      "total number of branches executed",
		      &sim_total_branches[sn], /* initial value */0, /* format */NULL,sn);
  }

  
  /* register performance stats */
  
  stat_reg_counter(sdb,"sim_cycle",
		   "total simulation time in cycles",
		   &sim_cycle, /* initial value */0, /* format */NULL,0);

  		     
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_IPC_%s", itoa[sn]);
     sprintf(formula_name, "sim_num_insn_%s / sim_cycle", itoa[sn]);  
     stat_reg_formula(sdb, stat_name,
		      "instructions per cycle",
		      formula_name, /* format */NULL,sn);
  }

  sn = 0;
  sprintf(formula_name, "sim_IPC_%s", itoa[sn]); 
  for (sn=1;sn<process_num;sn++)
     sprintf(formula_name, "%s + sim_IPC_%s",formula_name, itoa[sn]);  
  stat_reg_formula(sdb, "total_IPC", "Total IPC",
		   formula_name, NULL,0);

  		   
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_CPI_%s", itoa[sn]);
     sprintf(formula_name, "sim_cycle / sim_num_insn_%s", itoa[sn]);	   	   	   
     stat_reg_formula(sdb, stat_name,
		   "cycles per instruction",
		   formula_name, /* format */NULL,sn);
  }
  		   
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_exec_BW_%s", itoa[sn]);
     sprintf(formula_name, "sim_total_insn_%s / sim_cycle", itoa[sn]);	   	 
     stat_reg_formula(sdb, stat_name,
		   "total instructions (mis-spec + committed) per cycle",
		   formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_IPB_%s", itoa[sn]);
     sprintf(formula_name, "sim_num_insn_%s / sim_num_branches_%s", itoa[sn], itoa[sn]);
     stat_reg_formula(sdb, stat_name,
		   "instruction per branch",
		   formula_name, /* format */NULL,sn);
  }

  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_goodpred_%s", itoa[sn]);
     sprintf(formula_name, "sim_num_good_pred_%s / sim_num_branches_%s", itoa[sn], itoa[sn]);
     stat_reg_formula(sdb, stat_name,
		   "good prediction rate",
		   formula_name, /* format */NULL,sn);
  }

  /* occupancy stats */
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "IFQ_count_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name, "cumulative IFQ occupancy",
                   &IFQ_count[sn], /* initial value */0, /* format */NULL,sn);
  }
		
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "IFQ_fcount_%s", itoa[sn]);    
     stat_reg_counter(sdb, stat_name, "cumulative IFQ full count",
                   &IFQ_fcount[sn], /* initial value */0, /* format */NULL,sn);
  }
  		
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ifq_occupancy_%s", itoa[sn]);
     sprintf(formula_name, "IFQ_count_%s / sim_cycle", itoa[sn]);	    
     stat_reg_formula(sdb, stat_name, "avg IFQ occupancy (insn's)",
                   formula_name, /* format */NULL,sn);
  }
   
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ifq_rate_%s", itoa[sn]);
     sprintf(formula_name, "sim_total_insn_%s / sim_cycle", itoa[sn]);      
     stat_reg_formula(sdb, stat_name, "avg IFQ dispatch rate (insn/cycle)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ifq_latency_%s", itoa[sn]);
     sprintf(formula_name, "ifq_occupancy_%s / ifq_rate_%s", itoa[sn], itoa[sn]);  
     stat_reg_formula(sdb, stat_name, "avg IFQ occupant latency (cycle's)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ifq_full_%s", itoa[sn]);
     sprintf(formula_name, "IFQ_fcount_%s / sim_cycle", itoa[sn]);    
     stat_reg_formula(sdb, stat_name, "fraction of time (cycle's) IFQ was full",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "RUU_count_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name, "cumulative RUU occupancy",
                      &RUU_count[sn], /* initial value */0, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "RUU_fcount_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name, "cumulative RUU full count",
                      &RUU_fcount[sn], /* initial value */0, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ruu_occupancy_%s", itoa[sn]);
     sprintf(formula_name, "RUU_count_%s / sim_cycle", itoa[sn]);
     stat_reg_formula(sdb, stat_name, "avg RUU occupancy (insn's)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ruu_rate_%s", itoa[sn]);
     sprintf(formula_name, "sim_total_insn_%s / sim_cycle", itoa[sn]);	   
     stat_reg_formula(sdb, stat_name, "avg RUU dispatch rate (insn/cycle)",
                      formula_name, /* format */NULL,sn);
  }
  		   
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ruu_latency_%s", itoa[sn]);
     sprintf(formula_name, "ruu_occupancy_%s / ruu_rate_%s", itoa[sn], itoa[sn]);
     stat_reg_formula(sdb, stat_name, "avg RUU occupant latency (cycle's)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "ruu_full_%s", itoa[sn]);
     sprintf(formula_name, "RUU_fcount_%s / sim_cycle", itoa[sn]);
     stat_reg_formula(sdb, stat_name, "fraction of time (cycle's) RUU was full",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "LSQ_count_%s", itoa[sn]); 
     stat_reg_counter(sdb, stat_name, "cumulative LSQ occupancy",
                      &LSQ_count[sn], /* initial value */0, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "LSQ_fcount_%s", itoa[sn]); 		   
     stat_reg_counter(sdb, stat_name, "cumulative LSQ full count",
                      &LSQ_fcount[sn], /* initial value */0, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "lsq_occupancy_%s", itoa[sn]);
     sprintf(formula_name, "LSQ_count_%s / sim_cycle", itoa[sn]);
     stat_reg_formula(sdb, stat_name, "avg LSQ occupancy (insn's)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "lsq_rate_%s", itoa[sn]);
     sprintf(formula_name, "sim_total_insn_%s / sim_cycle", itoa[sn]);
     stat_reg_formula(sdb, stat_name, "avg LSQ dispatch rate (insn/cycle)",
                      formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "lsq_latency_%s", itoa[sn]);
     sprintf(formula_name, "lsq_occupancy_%s / lsq_rate_%s", itoa[sn], itoa[sn]);
     stat_reg_formula(sdb, stat_name, "avg LSQ occupant latency (cycle's)",
                   formula_name, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "lsq_full_%s", itoa[sn]);
     sprintf(formula_name, "LSQ_fcount_%s / sim_cycle", itoa[sn]);
     stat_reg_formula(sdb, stat_name, "fraction of time (cycle's) LSQ was full",
                   formula_name, /* format */NULL,sn);
  }
  
  /* register predictor stats */
  if (pred[0])
     bpred_reg_stats(pred, sdb);

  /* register cache stats */
  
  if (cache_il1[0])
  for (bank=0;bank<IL1_BANKS_TOTAL;bank++)
    cache_single_reg_stats(cache_il1[bank], sdb);

  if (cache_il2[0])
     for (bank=0;bank<l2_banks_num;bank++)
     cache_single_reg_stats(cache_il2[bank], sdb);

  if (cache_dl1[0])
  for (bank=0;bank<dl1_banks_num;bank++)
     cache_single_reg_stats(cache_dl1[bank], sdb);

  if ( (cache_dl2[0]) && (!cache_dl1[0]) )
     for (bank=0;bank<l2_banks_num;bank++)
     cache_single_reg_stats(cache_dl2[bank], sdb);

  if (itlb[0])
     cache_reg_stats(itlb, sdb);

  if (dtlb[0])
     cache_reg_stats(dtlb, sdb);

  /* debug variable(s) */
  
  for (sn=0;sn<process_num;sn++)
  {  sprintf(stat_name, "sim_invalid_addrs_%s", itoa[sn]);   
     stat_reg_counter(sdb, stat_name,
		   "total non-speculative bogus addresses seen (debug var)",
                   &sim_invalid_addrs[sn], /* initial value */0, /* format */NULL,sn);
  }
  
  for (sn=0;sn<process_num;sn++)
    for (i=0; i<pcstat_nelt; i++)
      {
        char buf[512], buf1[512];
        struct stat_stat_t *stat;

        /* track the named statistical variable by text address */

        /* find it... */
        stat = stat_find_stat(sdb, pcstat_vars[i]);
        if (!stat)
	  fatal(sn,"slot %d : cannot locate any statistic named `%s'", sn, pcstat_vars[i]);

        /* stat must be an integral type */
        if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
	  fatal(sn,"slot %d : `-pcstat' statistical variable `%s' is not an integral type", sn, stat->name);

        /* register this stat */
        pcstat_stats[sn][i] = stat;
        pcstat_lastvals[sn][i] = STATVAL(stat);

        /* declare the sparce text distribution */
        sprintf(buf, "%s_by_pc", stat->name);
        sprintf(buf1, "%s (by text address)", stat->desc);
        pcstat_sdists[sn][i] = stat_reg_sdist(sdb, buf, buf1,
					/* initial value */0,
					/* print format */(PF_COUNT|PF_PDF),
					/* format */"0x%lx %lu %.2f",
					/* print fn */NULL,sn);
      }
      
  /* register the load statistics */  
     ld_reg_stats(sdb);
     
  /* register the memory statistics */
     mem_reg_stats(mem, sdb);

fprintf(stderr,"\nFinished sim_reg_stats...\n");


}

/* forward declarations */
static void ruu_init(int sn);
static void lsq_init(int sn);
static void rslink_init(int nlinks, int sn);
static void eventq_init(int sn);
static void readyq_init(int sn);
static void cv_init(int sn);
static void tracer_init(int sn);
static void fetch_init(int sn);


/* default register state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_reg_obj(struct regs_t *regs,		/* registers to access */
	      int is_write,			/* access type */
	      enum md_reg_type rt,		/* reg bank to probe */
	      int reg,				/* register number */
	      struct eval_value_t *val, 	/* input, output */
              int sn);

/* default memory state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mem_obj(struct mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input/output buffer */
	      int nbytes,			/* size of access */
              int sn);                          /* slot's number */

/* default machine state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* registers to access */
		 struct mem_t *mem,		/* memory space to access */
                 int sn);                       /* slot's number */

/* total RS links allocated at program start */
#define MAX_RS_LINKS                    /* default */ 4096

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{   int i,j,
        line_number,
        sn; /* slot's number */
    int file_analysed; /* logical variable to control the programs load */
    int new_argc; /* argument number inside a command line */
    char *new_argv[256], aux_fname[1024], argument[1024];
    char line[1024];
    char fu_pool_name[64];
    char *FNAME;
    FILE *fd;

fprintf(stderr,"\nStarting sim_load_prog...\n");


  /* this code must identify inside from text file pointed by "fname" which are the programs to be executed. Each line from that file must contain one comand line for one program, including program name and argumentfnames. The file must has "process_num" lines at least. The applications which will be executed start in the "start_command_line" line and finish in the "last_command_line" linen. A part of this code was adapted from "process_file" function of the original SS 3.0 "options.c" file */

  if (argc!=1)
     fatal(sn,"there are so much fnames in the end of command line");
   
  FNAME=fname;
  fd = fopen(fname, "r");
  if (!fd)
    fatal(sn,"could not open programs file `%s'", fname);

   sn=0;
   line_number = 1;
   while ( (!feof(fd))&&(sn<process_num) ) /* for each slot do */
   {
      line[0]='\n';
      fgets(line, 1024, fd);

      /* skip blank lines and comments */
      if ((line[0] == '\0') || (line[0] == '\n') || (line[0] == '#'))
           continue; 	

      /* identifies program name */
      
      j = 0; i=0;
      while ((line[i]!='\0')&&(line[i]!='\n')&&(line[i]!=' ')) {
        aux_fname[j]=line[i];
        i++; j++;
      }
      aux_fname[j]='\0';
 
      /* update fname to point to program name */
      fname = aux_fname;

      /* alocate memory and copy string on argv */
      if ((new_argv[0] = mystrdup(fname))==NULL)
         fatal(sn,"out of virtual memory");


      /* In this point, fname points to program name and argv has one 
         argument. Now, it identifies the arguments for the program */
    
      file_analysed = 0;
      new_argc = 1;

      while (!file_analysed) {
      
         while ((line[i]==' ')||(line[i]=='\t')) i++; /* skip blank chars */
      
         j=0;  
         while ((line[i]!='\0')&&(line[i]!='\n')&&(line[i]!=' ')) {
           argument[j]=line[i];
           i++; j++;
         }
         argument[j]='\0';  /* it can has more one arg if j!=0 */

         file_analysed = (j==0); /* if true, there is not more arguments */

         if (!file_analysed) {   

           if (new_argc>=255)
             fatal(sn,"there are so much arguments for %s file",fname);
      
           /* alocate memory and copy string on argv */

           if ((new_argv[new_argc] = mystrdup(argument))==NULL)
             fatal(sn,"out of virtual memory");

           new_argc++; /* go to next argument */
         }
      } /* while (!file_analysed) */

      /* in this point, new_argv contain de new argv and new_argc contain
         the new argc, in order to continue the original processing */
     
      
      /* verifies if the standard input must be redirected to specific file. */
      /* To redirect the input, the two last arguments must be a "<" followed */
      /* the name of file. So, if new_argv[new_argc-2] == "<" then  */
      /* new_argv[new_argc-1] must contain the file name */
     
     if ( (new_argc >= 3) &&
          (!strcmp(new_argv[new_argc-2],"<")) )/* the input must be redirected*/
     { sim_inputname[sn] = mystrdup(new_argv[new_argc-1]);
       sim_inputfd[sn] = fopen(sim_inputname[sn], "r");

fprintf(stderr,"\nfile = %s\n",sim_inputname[sn]);

       if (!sim_inputfd[sn])
          fatal(sn,"it is not posible open the file = %s",sim_inputname[sn]);
       new_argc = new_argc - 2;
     }
      
     

      if ( (line_number >= start_command_line) &&
           (line_number <= last_command_line) )
      {
         /* emit the command line for each program reuse */
         fprintf(stderr, "sim: program %d: ",sn);
         for (i=0; i < new_argc; i++)
             fprintf(stderr, "%s ", new_argv[i]);
         fprintf(stderr, "\n"); 
         /* load program text and data, set up environment, memory, and regs */

         envp[0] = NULL;
         ld_load_prog(fname, new_argc, new_argv, envp, regs[sn], mem[sn], TRUE,sn);


         /* initialize here, so symbols can be loaded */
         if (ptrace_nelt == 2) {
           /* generate a pipeline trace */
            ptrace_open(/* fname */ptrace_opts[0], /* range */ptrace_opts[1],sn);
         }
         else if (ptrace_nelt == 0) {
                  /* no pipetracing */;}
              else
               fatal(sn,"bad pipetrace args, use: <fname|stdout|stderr> <range>");
         sn++;
      }
      line_number++; 
   } /* while */

   if (sn!=process_num)
     fatal(sn,"error: it is necessary %d applications at least inside of %s file\n",process_num, FNAME);

   /* finish initialization of the simulation engine */

   sn = 0;
   if (!mystricmp(fu_pool_type, "hetero"))
   {   
       strcpy(fu_pool_name,"fu_hetero_pool");
       fu_pool = res_create_pool(fu_pool_name, fu_hetero_config, N_ELT(fu_hetero_config),sn);
   }
   else
   {   if (!mystricmp(fu_pool_type, "compact"))
       {
          strcpy(fu_pool_name,"fu_compact_pool");
          fu_pool = res_create_pool(fu_pool_name, fu_compact_config, 
                    N_ELT(fu_compact_config),sn);

       }
       else /* "homo" option */
       {
          strcpy(fu_pool_name,"fu_homo_pool");
          fu_pool = res_create_pool(fu_pool_name, fu_homo_config, N_ELT(fu_homo_config),sn);
       }
      
   }

   for (sn=0;sn<process_num;sn++) {
      rslink_init(MAX_RS_LINKS,sn);
      tracer_init(sn);
      fetch_init(sn);
      cv_init(sn);
      eventq_init(sn);
      readyq_init(sn);
      ruu_init(sn);
      lsq_init(sn);
   }

   /* initialize the DLite debugger */
   dlite_init(simoo_reg_obj, simoo_mem_obj, simoo_mstate_obj);

fprintf(stderr,"\nFinished sim_load_prog...\n");

}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)             /* output stream */
{
  /* nada */
}

/* un-initialize the simulator */
void
sim_uninit(void)
{ int sn;

  for (sn=0;sn<process_num;sn++)
    if (ptrace_nelt > 0)
       ptrace_close(sn);
} 


/*
 * processor core definitions and declarations
 */

/* inst tag type, used to tag an operation instance in the RUU */
typedef unsigned int INST_TAG_TYPE;

/* inst sequence type, used to order instructions in the ready list, if
   this rolls over the ready list order temporarily will get messed up,
   but execution will continue and complete correctly */
typedef unsigned int INST_SEQ_TYPE;


/* total input dependencies possible */
#define MAX_IDEPS               3

/* total output dependencies possible */
#define MAX_ODEPS               2

/* a register update unit (RUU) station, this record is contained in the
   processors RUU, which serves as a collection of ordered reservations
   stations.  The reservation stations capture register results and await
   the time when all operands are ready, at which time the instruction is
   issued to the functional units; the RUU is an order circular queue, in which
   instructions are inserted in fetch (program) order, results are stored in
   the RUU buffers, and later when an RUU entry is the oldest entry in the
   machines, it and its instruction's value is retired to the architectural
   register file in program order, NOTE: the RUU and LSQ share the same
   structure, this is useful because loads and stores are split into two
   operations: an effective address add and a load/store, the add is inserted
   into the RUU and the load/store inserted into the LSQ, allowing the add
   to wake up the load/store when effective address computation has file_analysed */
struct RUU_station {
  /* inst info */
  md_inst_t IR;			/* instruction bits */
  enum md_opcode op;			/* decoded instruction opcode */
  md_addr_t PC, next_PC, pred_PC;	/* inst PC, next PC, predicted PC */
  int in_LSQ;				/* non-zero if op is in LSQ */
  int ea_comp;				/* non-zero if op is an addr comp */
  int recover_inst;			/* start of mis-speculation? */
  int stack_recover_idx;		/* non-speculative TOS for RSB pred */
  struct bpred_update_t dir_update;	/* bpred direction update info */
  int spec_mode;			/* non-zero if issued in spec_mode */
  md_addr_t addr;			/* effective address for ld/st's */
  INST_TAG_TYPE tag;			/* RUU slot tag, increment to
					   squash operation */
  INST_SEQ_TYPE seq;			/* instruction sequence, used to
					   sort the ready list and tag inst */
  unsigned int ptrace_seq;		/* pipetrace sequence number */

  /* instruction status */
  int queued;				/* operands ready and queued */
  int issued;				/* operation is/was executing */
  int completed;			/* operation has completed execution */

  /* output operand dependency list, these lists are used to
     limit the number of associative searches into the RUU when
     instructions complete and need to wake up dependent insts */
  int onames[MAX_ODEPS];		/* output logical names (NA=unused) */
  struct RS_link *odep_list[MAX_ODEPS];	/* chains to consuming operations */

  /* input dependent links, the output chains rooted above use these
     fields to mark input operands as ready, when all these fields have
     been set non-zero, the RUU operation has all of its register
     operands, it may commence execution as soon as all of its memory
     operands are known to be read (see lsq_refresh() for details on
     enforcing memory dependencies) */
  int idep_ready[MAX_IDEPS];		/* input operand ready? */
};

/* non-zero if all register operands are ready, update with MAX_IDEPS */
#define OPERANDS_READY(RS)                                              \
  ((RS)->idep_ready[0] && (RS)->idep_ready[1] && (RS)->idep_ready[2])

/* register update unit, combination of reservation stations and reorder
   buffer device, organized as a circular queue */
static struct RUU_station *RUU[MAX_SLOTS];  /* register update unit */
static int RUU_head[MAX_SLOTS], RUU_tail[MAX_SLOTS];		/* RUU head and tail pointers */
static int RUU_num[MAX_SLOTS];		/* num entries currently in RUU */

/* allocate and initialize register update unit (RUU) */
static void
ruu_init(int sn)
{
  RUU[sn] = calloc(RUU_size, sizeof(struct RUU_station));
  if (!RUU[sn])
    fatal(sn,"out of virtual memory");

  RUU_num[sn] = 0;
  RUU_head[sn] = RUU_tail[sn] = 0;
  RUU_count[sn] = 0;
  RUU_fcount[sn] = 0;
}

/* dump the contents of the RUU */
static void
ruu_dumpent(struct RUU_station *rs,		/* ptr to RUU station */
	    int index,				/* entry index */
	    FILE *stream,			/* output stream */
	    int header,				/* print header? */
            int sn)
{
  if (!stream)
    stream = stderr;

  if (header)
    fprintf(stream, "idx: %2d: opcode: %s, inst: `",
	    index, MD_OP_NAME(rs->op));
  else
    fprintf(stream, "       opcode: %s, inst: `",
	    MD_OP_NAME(rs->op));
  md_print_insn(rs->IR, rs->PC, stream);
  fprintf(stream, "'\n");
  myfprintf(sn,stream, "         PC: 0x%08p, NPC: 0x%08p (pred_PC: 0x%08p)\n",
	    rs->PC, rs->next_PC, rs->pred_PC);
  fprintf(stream, "         in_LSQ: %s, ea_comp: %s, recover_inst: %s\n",
	  rs->in_LSQ ? "t" : "f",
	  rs->ea_comp ? "t" : "f",
	  rs->recover_inst ? "t" : "f");
  myfprintf(sn,stream, "         spec_mode: %s, addr: 0x%08p, tag: 0x%08x\n",
	    rs->spec_mode ? "t" : "f", rs->addr, rs->tag);
  fprintf(stream, "         seq: 0x%08x, ptrace_seq: 0x%08x\n",
	  rs->seq, rs->ptrace_seq);
  fprintf(stream, "         queued: %s, issued: %s, completed: %s\n",
	  rs->queued ? "t" : "f",
	  rs->issued ? "t" : "f",
	  rs->completed ? "t" : "f");
  fprintf(stream, "         operands ready: %s\n",
	  OPERANDS_READY(rs) ? "t" : "f");
}

/* dump the contents of the RUU */
static void
ruu_dump(FILE *stream, int sn)				/* output stream */
{
  int num, head;
  struct RUU_station *rs;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** RUU state **\n");
  fprintf(stream, "RUU_head: %d, RUU_tail: %d\n", RUU_head[sn], RUU_tail[sn]);
  fprintf(stream, "RUU_num: %d\n", RUU_num[sn]);

  num = RUU_num[sn];
  head = RUU_head[sn];
  while (num)
    {
      rs = &RUU[sn][head];
      ruu_dumpent(rs, rs - RUU[sn], stream, /* header */TRUE,sn);
      head = (head + 1) % RUU_size;
      num--;
    }
}

/*
 * load/store queue (LSQ): holds loads and stores in program order, indicating
 * status of load/store access:
 *
 *   - issued: address computation complete, memory access in progress
 *   - completed: memory access has completed, stored value available
 *   - squashed: memory access was squashed, ignore this entry
 *
 * loads may execute when:
 *   1) register operands are ready, and
 *   2) memory operands are ready (no earlier unresolved store)
 *
 * loads are serviced by:
 *   1) previous store at same address in LSQ (hit latency), or
 *   2) data cache (hit latency + miss latency)
 *
 * stores may execute when:
 *   1) register operands are ready
 *
 * stores are serviced by:
 *   1) depositing store value into the load/store queue
 *   2) writing store value to the store buffer (plus tag check) at commit
 *   3) writing store buffer entry to data cache when cache is free
 *
 * NOTE: the load/store queue can bypass a store value to a load in the same
 *   cycle the store executes (using a bypass network), thus stores complete
 *   in effective zero time after their effective address is known
 */
static struct RUU_station *LSQ[MAX_SLOTS];   /* load/store queue */
static int LSQ_head[MAX_SLOTS],              /* LSQ head and tail pointers */
           LSQ_tail[MAX_SLOTS];     
static int LSQ_num[MAX_SLOTS];               /* num entries currently in LSQ */

/*
 * input dependencies for stores in the LSQ:
 *   idep #0 - operand input (value that is store'd)
 *   idep #1 - effective address input (address of store operation)
 */
#define STORE_OP_INDEX                  0
#define STORE_ADDR_INDEX                1

#define STORE_OP_READY(RS)              ((RS)->idep_ready[STORE_OP_INDEX])
#define STORE_ADDR_READY(RS)            ((RS)->idep_ready[STORE_ADDR_INDEX])

/* allocate and initialize the load/store queue (LSQ) */
static void
lsq_init(int sn)
{
  LSQ[sn] = calloc(LSQ_size, sizeof(struct RUU_station));
  if (!LSQ[sn])
    fatal(sn,"out of virtual memory");

  LSQ_num[sn] = 0;
  LSQ_head[sn] = LSQ_tail[sn] = 0;
  LSQ_count[sn] = 0;
  LSQ_fcount[sn] = 0;
}

/* dump the contents of the RUU */
static void
lsq_dump(FILE *stream, int sn)				/* output stream */
{
  int num, head;
  struct RUU_station *rs;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** LSQ state **\n");
  fprintf(stream, "LSQ_head: %d, LSQ_tail: %d\n", LSQ_head[sn], LSQ_tail[sn]);
  fprintf(stream, "LSQ_num: %d\n", LSQ_num[sn]);

  num = LSQ_num[sn];
  head = LSQ_head[sn];
  while (num)
    {
      rs = &LSQ[sn][head];
      ruu_dumpent(rs, rs - LSQ[sn], stream, /* header */TRUE,sn);
      head = (head + 1) % LSQ_size;
      num--;
    }
}


/*
 * RS_LINK defs and decls
 */

/* a reservation station link: this structure links elements of a RUU
   reservation station list; used for ready instruction queue, event queue, and
   output dependency lists; each RS_LINK node contains a pointer to the RUU
   entry it references along with an instance tag, the RS_LINK is only valid if
   the instruction instance tag matches the instruction RUU entry instance tag;
   this strategy allows entries in the RUU can be squashed and reused without
   updating the lists that point to it, which significantly improves the
   performance of (all to frequent) squash events */
struct RS_link {
  struct RS_link *next;			/* next entry in list */
  struct RUU_station *rs;		/* referenced RUU resv station */
  INST_TAG_TYPE tag;			/* inst instance sequence number */
  union {
    tick_t when;			/* time stamp of entry (for eventq) */
    INST_SEQ_TYPE seq;			/* inst sequence */
    int opnum;				/* input/output operand number */
  } x;
};


/* RS link free list, grab RS_LINKs from here, when needed */
static struct RS_link *rslink_free_list[MAX_SLOTS];

/* NULL value for an RS link */
#define RSLINK_NULL_DATA	{ NULL, NULL, 0 }
static struct RS_link RSLINK_NULL = RSLINK_NULL_DATA;


/* create and initialize an RS link */
#define RSLINK_INIT(RSL, RS)						\
  ((RSL).next = NULL, (RSL).rs = (RS), (RSL).tag = (RS)->tag)

/* non-zero if RS link is NULL */
#define RSLINK_IS_NULL(LINK)            ((LINK)->rs == NULL)

/* non-zero if RS link is to a valid (non-squashed) entry */
#define RSLINK_VALID(LINK)              ((LINK)->tag == (LINK)->rs->tag)

/* extra RUU reservation station pointer */
#define RSLINK_RS(LINK)                 ((LINK)->rs)

/* get a new RS link record */
#define RSLINK_NEW(DST, RS)						\
  { struct RS_link *n_link;						\
    if (!rslink_free_list[sn])						\
      panic(sn,"out of rs links");						\
    n_link = rslink_free_list[sn];					\
    rslink_free_list[sn] = rslink_free_list[sn]->next;			\
    n_link->next = NULL;						\
    n_link->rs = (RS); n_link->tag = n_link->rs->tag;			\
    (DST) = n_link;							\
  }

/* free an RS link record */
#define RSLINK_FREE(LINK)						\
  {  struct RS_link *f_link = (LINK);					\
     f_link->rs = NULL; f_link->tag = 0;				\
     f_link->next = rslink_free_list[sn];					\
     rslink_free_list[sn] = f_link;						\
  }

/* FIXME: could this be faster!!! */
/* free an RS link list */
#define RSLINK_FREE_LIST(LINK)						\
  {  struct RS_link *fl_link, *fl_link_next;				\
     for (fl_link=(LINK); fl_link; fl_link=fl_link_next)		\
       {								\
	 fl_link_next = fl_link->next;					\
	 RSLINK_FREE(fl_link);						\
       }								\
  }

/* initialize the free RS_LINK pool */
static void
rslink_init(int nlinks,		/* total number of RS_LINK available */
            int sn)                 /* slot's number */
{
  int i;
  struct RS_link *link;

  rslink_free_list[sn] = NULL;
  for (i=0; i<nlinks; i++)
    {
      link = calloc(1, sizeof(struct RS_link));
      if (!link)
	fatal(sn,"out of virtual memory");
      link->next = rslink_free_list[sn];
      rslink_free_list[sn] = link;
    }
}

/* service all functional unit release events, this function is called
   once per cycle, and it used to step the BUSY timers attached to each
   functional unit in the function unit resource pool, as long as a functional
   unit's BUSY count is > 0, it cannot be issued an operation */
static void
ruu_release_fu(void)
{
  int i;

  /* walk all resource units, decrement busy counts by one */
  for (i=0; i<fu_pool->num_resources; i++)
    {
      /* resource is released when BUSY hits zero */
      if (fu_pool->resources[i].busy > 0)
	fu_pool->resources[i].busy--;
    }
}


/*
 * the execution unit event queue implementation follows, the event queue
 * indicates which instruction will complete next, the writeback handler
 * drains this queue
 */

/* pending event queue, sorted from soonest to latest event (in time), NOTE:
   RS_LINK nodes are used for the event queue list so that it need not be
   updated during squash events */
static struct RS_link *event_queue[MAX_SLOTS];

/* initialize the event queue structures */
static void
eventq_init(int sn)
{
  event_queue[sn] = NULL;
}

/* dump the contents of the event queue */
static void
eventq_dump(FILE *stream, int sn)		/* output stream */
{
  struct RS_link *ev;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** event queue state **\n");

  for (ev = event_queue[sn]; ev != NULL; ev = ev->next)
    {
      /* is event still valid? */
      if (RSLINK_VALID(ev))
	{
	  struct RUU_station *rs = RSLINK_RS(ev);

	  fprintf(stream, "idx: %2d: @ %.0f\n",
		  (int)(rs - (rs->in_LSQ ? LSQ[sn] : RUU[sn])), (double)ev->x.when);
	  ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ[sn] : RUU[sn]),
		      stream, /* !header */FALSE,sn);
	}
    }
}

/* insert an event for RS into the event queue, event queue is sorted from
   earliest to latest event, event and associated side-effects will be
   apparent at the start of cycle WHEN */
static void
eventq_queue_event(struct RUU_station *rs, tick_t when, int sn)
{
  struct RS_link *prev, *ev, *new_ev;

  if (rs->completed)
    panic(sn,"event completed");

  if (when <= sim_cycle)
    panic(sn,"event occurred in the past");

  /* get a free event record */
  RSLINK_NEW(new_ev, rs);
  new_ev->x.when = when;

  /* locate insertion point */
  for (prev=NULL, ev=event_queue[sn];
       ev && ev->x.when < when;
       prev=ev, ev=ev->next);

  if (prev)
    {
      /* insert middle or end */
      new_ev->next = prev->next;
      prev->next = new_ev;
    }
  else
    {
      /* insert at beginning */
      new_ev->next = event_queue[sn];
      event_queue[sn] = new_ev;
    }
}

/* return the next event that has already occurred, returns NULL when no
   remaining events or all remaining events are in the future */
static struct RUU_station *
eventq_next_event(int sn)
{
  struct RS_link *ev;

  if (event_queue[sn] && event_queue[sn]->x.when <= sim_cycle)
    {
      /* unlink and return first event on priority list */
      ev = event_queue[sn];
      event_queue[sn] = event_queue[sn]->next;

      /* event still valid? */
      if (RSLINK_VALID(ev))
	{
	  struct RUU_station *rs = RSLINK_RS(ev);

	  /* reclaim event record */
	  RSLINK_FREE(ev);

	  /* event is valid, return resv station */
	  return rs;
	}
      else
	{
	  /* reclaim event record */
	  RSLINK_FREE(ev);

	  /* receiving inst was squashed, return next event */
	  return eventq_next_event(sn);
	}
    }
  else
    {
      /* no event or no event is ready */
      return NULL;
    }
}


/*
 * the ready instruction queue implementation follows, the ready instruction
 * queue indicates which instruction have all of there *register* dependencies
 * satisfied, instruction will issue when 1) all memory dependencies for
 * the instruction have been satisfied (see lsq_refresh() for details on how
 * this is accomplished) and 2) resources are available; ready queue is fully
 * constructed each cycle before any operation is issued from it -- this
 * ensures that instruction issue priorities are properly observed; NOTE:
 * RS_LINK nodes are used for the event queue list so that it need not be
 * updated during squash events
 */

/* the ready instruction queue */
static struct RS_link *ready_queue[MAX_SLOTS];

/* initialize the event queue structures */
static void
readyq_init(int sn)
{
  ready_queue[sn] = NULL;
}

/* dump the contents of the ready queue */
static void
readyq_dump(FILE *stream, int sn)			/* output stream */
{
  struct RS_link *link;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** ready queue state **\n");

  for (link = ready_queue[sn]; link != NULL; link = link->next)
    {
      /* is entry still valid? */
      if (RSLINK_VALID(link))
	{
	  struct RUU_station *rs = RSLINK_RS(link);

	  ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ[sn] : RUU[sn]),
		      stream, /* header */TRUE,sn);
	}
    }
}

/* insert ready node into the ready list using ready instruction scheduling
   policy; currently the following scheduling policy is enforced:

     memory and long latency operands, and branch instructions first

   then

     all other instructions, oldest instructions first

  this policy works well because branches pass through the machine quicker
  which works to reduce branch misprediction latencies, and very long latency
  instructions (such loads and multiplies) get priority since they are very
  likely on the program's critical path */
static void
readyq_enqueue(struct RUU_station *rs, int sn)		/* RS to enqueue */
{
  struct RS_link *prev, *node, *new_node;

  /* node is now queued */
  if (rs->queued)
    panic(sn,"node is already queued");
  rs->queued = TRUE;

  /* get a free ready list node */
  RSLINK_NEW(new_node, rs);
  new_node->x.seq = rs->seq;

  /* locate insertion point */
  if (rs->in_LSQ || MD_OP_FLAGS(rs->op) & (F_LONGLAT|F_CTRL))
    {
      /* insert loads/stores and long latency ops at the head of the queue */
      prev = NULL;
      node = ready_queue[sn];
    }
  else
    {
      /* otherwise insert in program order (earliest seq first) */
      for (prev=NULL, node=ready_queue[sn];
	   node && node->x.seq < rs->seq;
	   prev=node, node=node->next);
    }

  if (prev)
    {
      /* insert middle or end */
      new_node->next = prev->next;
      prev->next = new_node;
    }
  else
    {
      /* insert at beginning */
      new_node->next = ready_queue[sn];
      ready_queue[sn] = new_node;
    }
}


/*
 * the create vector maps a logical register to a creator in the RUU (and
 * specific output operand) or the architected register file (if RS_link
 * is NULL)
 */

/* an entry in the create vector */
struct CV_link {
  struct RUU_station *rs;               /* creator's reservation station */
  int odep_num;                         /* specific output operand */
};

/* a NULL create vector entry */
static struct CV_link CVLINK_NULL = { NULL, 0 };

/* get a new create vector link */
#define CVLINK_INIT(CV, RS,ONUM)	((CV).rs = (RS), (CV).odep_num = (ONUM))

/* size of the create vector (one entry per architected register) */
#define CV_BMAP_SZ              (BITMAP_SIZE(MD_TOTAL_REGS))

/* the create vector, NOTE: speculative copy on write storage provided
   for fast recovery during wrong path execute (see tracer_recover() for
   details on this process */
static BITMAP_TYPE(MD_TOTAL_REGS, use_spec_cv[MAX_SLOTS]);
static struct CV_link create_vector[MAX_SLOTS][MD_TOTAL_REGS];
static struct CV_link spec_create_vector[MAX_SLOTS][MD_TOTAL_REGS];

/* these arrays shadow the create vector an indicate when a register was
   last created */
static tick_t create_vector_rt[MAX_SLOTS][MD_TOTAL_REGS];
static tick_t spec_create_vector_rt[MAX_SLOTS][MD_TOTAL_REGS];

/* read a create vector entry */
#define CREATE_VECTOR(N)        (BITMAP_SET_P(use_spec_cv[sn], CV_BMAP_SZ, (N))\
				 ? spec_create_vector[sn][N]                \
				 : create_vector[sn][N])

/* read a create vector timestamp entry */
#define CREATE_VECTOR_RT(N)     (BITMAP_SET_P(use_spec_cv[sn], CV_BMAP_SZ, (N))\
				 ? spec_create_vector_rt[sn][N]             \
				 : create_vector_rt[sn][N])

/* set a create vector entry */
#define SET_CREATE_VECTOR(N,L)  (spec_mode[sn]                             \
				 ? (BITMAP_SET(use_spec_cv[sn], CV_BMAP_SZ, (N)),\
				    spec_create_vector[sn][N] = (L))        \
				 : (create_vector[sn][N] = (L)))

/* initialize the create vector */
static void
cv_init(int sn)
{
  int i;

  /* initially all registers are valid in the architected register file,
     i.e., the create vector entry is CVLINK_NULL */
  for (i=0; i < MD_TOTAL_REGS; i++)
    {
      create_vector[sn][i] = CVLINK_NULL;
      create_vector_rt[sn][i] = 0;
      spec_create_vector[sn][i] = CVLINK_NULL;
      spec_create_vector_rt[sn][i] = 0;
    }

  /* all create vector entries are non-speculative */
  BITMAP_CLEAR_MAP(use_spec_cv[sn], CV_BMAP_SZ);
}

/* dump the contents of the create vector */
static void
cv_dump(FILE *stream, int sn)				/* output stream */
{
  int i;
  struct CV_link ent;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** create vector state **\n");

  for (i=0; i < MD_TOTAL_REGS; i++)
    {
      ent = CREATE_VECTOR(i);
      if (!ent.rs)
	fprintf(stream, "[cv%02d]: from architected reg file\n", i);
      else
	fprintf(stream, "[cv%02d]: from %s, idx: %d\n",
		i, (ent.rs->in_LSQ ? "LSQ" : "RUU"),
		(int)(ent.rs - (ent.rs->in_LSQ ? LSQ[sn] : RUU[sn])));
    }
}


/*
 *  RUU_COMMIT() - instruction retirement pipeline stage
 */

/* this function commits the results of the oldest completed entries from the
   RUU and LSQ to the architected reg file, stores in the LSQ will commit
   their store data to the data cache at this point as well */
static void
ruu_commit(void)
{
  int i, sn, lat, events, n_committed;
  static int next_slot = -1;
  int not_done;
  int committed[MAX_SLOTS];

  for (sn=0;sn<process_num;sn++)
      committed[sn] = FALSE;
  
  next_slot++;
  next_slot = next_slot % process_num;

  not_done = process_num;
  
  /* tries commit instructions for each slot */

  n_committed = 0;

  sn = next_slot;
  while ( (not_done) &&
          (n_committed < ruu_commit_width) )
  {
     /* to restore the error integrity */
     errno = aux_errno[sn]; 

     
     /* all values must be retired to the architected reg file in program order */

     if ( (RUU_num[sn] > 0) &&
             (!committed[sn]) )
     {
        struct RUU_station *rs = &(RUU[sn][RUU_head[sn]]);

        if (!rs->completed)
	{
	     /* at least RUU entry must be complete */
             not_done--;
             committed[sn] = TRUE;    
	}
        else
        {  int broke = FALSE;

           /* default commit events */
           events = 0;

           /* load/stores must retire load/store queue entry as well */
           if (RUU[sn][RUU_head[sn]].ea_comp)
	   {
	     /* load/store, retire head of LSQ as well */
	     if (LSQ_num[sn] <= 0 || !LSQ[sn][LSQ_head[sn]].in_LSQ)
	        panic(sn,"RUU out of sync with LSQ");

	     /* load/store operation must be complete */
	     if (!LSQ[sn][LSQ_head[sn]].completed)
	     {
	        /* load/store operation is not yet complete */
                not_done--;
                committed[sn] = TRUE;
                broke = TRUE;
	     }
             else
             {

	        if ((MD_OP_FLAGS(LSQ[sn][LSQ_head[sn]].op) & (F_MEM|F_STORE))
	            == (F_MEM|F_STORE))
	        {
	           struct res_template *fu;

	           /* stores must retire their store value to the cache at commit,
		     try to get a store port (functional unit allocation) */

	           fu = res_get(fu_pool, MD_OP_FUCLASS(LSQ[sn][LSQ_head[sn]].op));
	           if (fu)
		   {  md_addr_t addr, offset;

		      /* reserve the functional unit */
		      if (fu->master->busy)
		         panic(sn,"functional unit already in use");

		      /* schedule functional unit release event */
		      fu->master->busy = fu->issuelat;

                      /* "data_offset" puts an different offset in each */
                      /* address in order to simulate different initial */
                      /* memory pages allocated for different code. It must */
                      /* provides less misses in the cache during the start */

                      offset = (sn % (process_num / dl1_banks_num))*data_offset;
                      addr = (LSQ[sn][LSQ_head[sn]].addr + offset)&~3;

		      /* go to the data cache */
		      if (cache_dl1[0]) /* there is dl1 cache */
		      {  int dl1bank, dl2bank, start, finish, i;

                         dl1bank = WHICHIS_DL1BANK_OF_SLOT(sn);

if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
        fprintf(stderr,"\n dl1 bank %d slot %d.......\n",dl1bank, sn);


		         /* commit store value to D-cache */
		         lat =
			   cache_access(cache_dl1[dl1bank], Write, addr, NULL, 4, 
                                     sim_cycle, NULL, NULL,sn);
		         if (lat > cache_dl1_lat)
			   events |= PEV_CACHEMISS;

                         /* Broadcast bus_free for all dl1_banks those are connected in the same */
                         /* l2 bank in order control the utilization of "dl1-dl2" bus by all dl1 */
                         /* banks. It is considered that only one "dl1-dl2" bus is shared among */
                         /* different dl1-banks. */
                         /* Besides, there is another "il1-il2" bus for instructions, and, if */
                         /* "il2=dl2", it is supposed there are 2 ports (2 bus) in this */
                         /* unified level 2 cache. Also, if "il2!=dl2", it is considered 2 */
                         /* ports (2 bus) in the memory. Otherwise, to simulates just 1 port in the */
                         /* l2 cache or in the memory, it is necessary to set l1_bus_opt = 1 */

                         dl2bank = WHICHIS_DL2BANK_OF_DL1(dl1bank);
                         start=FIRST_DL1BANK_OF_DL2(dl2bank);
                         finish=start + DL2_BANK_WIDTH;
                         for (i=start;i<finish;i++)
                          cache_dl1[i]->bus_free = cache_dl1[dl1bank]->bus_free;

                         /* handle l1_bus_free */

                         if (l1_bus_opt)
                         {  start = FIRST_IL1BANK_OF_IL2(dl2bank); /* remember: il2 & dl2 are unified */
                            finish = start + IL2_BANK_WIDTH;
                            for (i=start; i<finish; i++)
                                cache_il1[i]->bus_free = cache_dl1[dl1bank]->bus_free;
                         } 
		       }

		       /* all loads and stores must to access D-TLB */
		       if (dtlb[0]) /* there is dtlb */
		       {
		          /* access the D-TLB */
		          lat =
			   cache_access(dtlb[sn], Read, addr, NULL, 4, 
                                     sim_cycle, NULL, NULL,sn);
		          if (lat > 1)
			    events |= PEV_TLBMISS;
		       }
		   }
	           else
		   {
		      /* no store ports left, cannot continue to commit insts */
                      committed[sn] = TRUE;
                      not_done--;
                      broke = TRUE;              
		   }
	        } /* if ((MD_OP_FLAGS(LSQ[sn][LSQ_head[sn]].op) & (F_MEM|F_STORE)) */

                if (!broke)
                {
	           /* invalidate load/store operation instance */
	           LSQ[sn][LSQ_head[sn]].tag++;

	           /* indicate to pipeline trace that this instruction retired */
	           ptrace_newstage(LSQ[sn][LSQ_head[sn]].ptrace_seq, PST_COMMIT, events);
	           ptrace_endinst(LSQ[sn][LSQ_head[sn]].ptrace_seq);

	           /* commit head of LSQ as well */
	           LSQ_head[sn] = (LSQ_head[sn] + 1) % LSQ_size;
	           LSQ_num[sn]--;
                }
             } /* else from if (!LSQ[sn][LSQ_head[sn]].completed) */
	   }   /* if (RUU[sn][RUU_head[sn]].ea_comp) */

           if (!broke)
           {
              if (pred[sn]
	         && bpred_spec_update == spec_CT
	         && (MD_OP_FLAGS(rs->op) & F_CTRL))
	      {
	         bpred_update(pred[sn],
		 /* branch address */rs->PC,
                 /* actual target address */rs->next_PC,
                 /* taken? */rs->next_PC != (rs->PC + sizeof(md_inst_t)),
                 /* pred taken? */rs->pred_PC != (rs->PC + sizeof(md_inst_t)),
                 /* correct pred? */rs->pred_PC == rs->next_PC,
                 /* opcode */rs->op,
                 /* dir predictor update pointer */&rs->dir_update);
	      }
         
              if ( ((pred_forced)||(pred[sn])) &&
                 (MD_OP_FLAGS(rs->op) & F_CTRL) && 
                 (rs->pred_PC == rs->next_PC) )
                    sim_num_good_pred[sn]++;

              /* invalidate RUU operation instance */
              RUU[sn][RUU_head[sn]].tag++;

              /* indicate to pipeline trace that this instruction retired */
              ptrace_newstage(RUU[sn][RUU_head[sn]].ptrace_seq, PST_COMMIT, events);
              ptrace_endinst(RUU[sn][RUU_head[sn]].ptrace_seq);

              /* commit head entry of RUU */
              RUU_head[sn] = (RUU_head[sn] + 1) % RUU_size;
              RUU_num[sn]--;

              /* one more instruction committed to architected state */
              n_committed++;

              for (i=0; i<MAX_ODEPS; i++)
	      {
	         if (rs->odep_list[i])
	         panic(sn,"retired instruction has odeps\n");
              }
           } /* if !broke */

        } /* if rs->committed .. */
     }   /* if (RUU_num[sn] > 0 ... */
     else
     {   if (!committed[sn])
         {  not_done--;
            committed[sn] = TRUE;
         }
     }

     /* to store the error integrity */
     aux_errno[sn] = errno;
     sn++;
     sn = sn % process_num;
  } /*  while (not_done) */
}


/*
 *  RUU_RECOVER() - squash mispredicted microarchitecture state
 */

/* recover processor microarchitecture state back to point of the
   mis-predicted branch at RUU[sn][BRANCH_INDEX] */
static void
ruu_recover(int branch_index, int sn)	/* index of mis-pred branch */
{
  int i, RUU_index = RUU_tail[sn], LSQ_index = LSQ_tail[sn];
  int RUU_prev_tail = RUU_tail[sn], LSQ_prev_tail = LSQ_tail[sn];

  /* recover from the tail of the RUU towards the head until the branch index
     is reached, this direction ensures that the LSQ can be synchronized with
     the RUU */

  /* go to first element to squash */
  RUU_index = (RUU_index + (RUU_size-1)) % RUU_size;
  LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;

  /* traverse to older insts until the mispredicted branch is encountered */
  while (RUU_index != branch_index)
    {
      /* the RUU should not drain since the mispredicted branch will remain */
      if (!RUU_num[sn])
	panic(sn,"empty RUU");

      /* should meet up with the tail first */
      if (RUU_index == RUU_head[sn])
	panic(sn,"RUU head and tail broken");

      /* is this operation an effective addr calc for a load or store? */
      if (RUU[sn][RUU_index].ea_comp)
	{
	  /* should be at least one load or store in the LSQ */
	  if (!LSQ_num[sn])
	    panic(sn,"RUU and LSQ out of sync");

	  /* recover any resources consumed by the load or store operation */
	  for (i=0; i<MAX_ODEPS; i++)
	    {
	      RSLINK_FREE_LIST(LSQ[sn][LSQ_index].odep_list[i]);
	      /* blow away the consuming op list */
	      LSQ[sn][LSQ_index].odep_list[i] = NULL;
	    }
      
	  /* squash this LSQ entry */
	  LSQ[sn][LSQ_index].tag++;

	  /* indicate in pipetrace that this instruction was squashed */
	  ptrace_endinst(LSQ[sn][LSQ_index].ptrace_seq);

	  /* go to next earlier LSQ slot */
	  LSQ_prev_tail = LSQ_index;
	  LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;
	  LSQ_num[sn]--;
	}

      /* recover any resources used by this RUU operation */
      for (i=0; i<MAX_ODEPS; i++)
	{
	  RSLINK_FREE_LIST(RUU[sn][RUU_index].odep_list[i]);
	  /* blow away the consuming op list */
	  RUU[sn][RUU_index].odep_list[i] = NULL;
	}
      
      /* squash this RUU entry */
      RUU[sn][RUU_index].tag++;

      /* indicate in pipetrace that this instruction was squashed */
      ptrace_endinst(RUU[sn][RUU_index].ptrace_seq);

      /* go to next earlier slot in the RUU */
      RUU_prev_tail = RUU_index;
      RUU_index = (RUU_index + (RUU_size-1)) % RUU_size;
      RUU_num[sn]--;
    } /* while (RUU_index != branch_index) */

  /* reset head/tail pointers to point to the mis-predicted branch */
  RUU_tail[sn] = RUU_prev_tail;
  LSQ_tail[sn] = LSQ_prev_tail;

  /* revert create vector back to last precise create vector state, NOTE:
     this is accomplished by resetting all the copied-on-write bits in the
     USE_SPEC_CV bit vector */
  BITMAP_CLEAR_MAP(use_spec_cv[sn], CV_BMAP_SZ);

  /* FIXME: could reset functional units at squash time */
}


/*
 *  RUU_WRITEBACK() - instruction result writeback pipeline stage
 */

/* forward declarations */
static void tracer_recover(int sn);

/* writeback completed operation results from the functional units to RUU,
   at this point, the output dependency chains of completing instructions
   are also walked to determine if any dependent instruction now has all
   of its register operands, if so the (nearly) ready instruction is inserted
   into the ready instruction queue */
static void
ruu_writeback(void)
{
  int i,sn;
  struct RUU_station *rs;

  /* tries execute writeback for each slot */
  for (sn=0;sn<process_num;sn++)
  {
     /* to restore the error integrity */
     errno = aux_errno[sn];

     /* service all completed events */
     while ((rs = eventq_next_event(sn)))
     {
         /* RS has completed execution and (possibly) produced a result */
         if (!OPERANDS_READY(rs) || rs->queued || !rs->issued || rs->completed)
	    panic(sn,"inst completed and !ready, !issued, or completed");

         /* operation has completed */
         rs->completed = TRUE;

         /* does this operation reveal a mis-predicted branch? */
         if (rs->recover_inst)
	 { 
	    if (rs->in_LSQ)
	      panic(sn,"mis-predicted load or store?!?!?");

	    /* recover processor state and reinit fetch to correct path */
	    ruu_recover((rs - RUU[sn]),sn);
	    tracer_recover(sn);
	    bpred_recover(pred[sn], rs->PC, rs->stack_recover_idx);

            /* stall fetch until I-fetch and I-decode recover */
	    ruu_slot_delay[sn] = ruu_branch_penalty;
    
	    /* continue writeback of the branch/control instruction */
	 }

         /* if we speculatively update branch-predictor, do it here */
         if (pred[sn]
	    && bpred_spec_update == spec_WB
	    && !rs->in_LSQ
	    && (MD_OP_FLAGS(rs->op) & F_CTRL))
	 {
	    bpred_update(pred[sn],
               /* branch address */rs->PC,
	       /* actual target address */rs->next_PC,
	       /* taken? */rs->next_PC != (rs->PC + sizeof(md_inst_t)),
	       /* pred taken? */rs->pred_PC != (rs->PC + sizeof(md_inst_t)),
	       /* correct pred? */rs->pred_PC == rs->next_PC,
	       /* opcode */rs->op,
	       /* dir predictor update pointer */&rs->dir_update);
	 }

         /* entered writeback stage, indicate in pipe trace */
         ptrace_newstage(rs->ptrace_seq, PST_WRITEBACK, rs->recover_inst ? PEV_MPDETECT : 0);

         /* broadcast results to consuming operations, this is more efficiently
         accomplished by walking the output dependency chains of the
	 completed instruction */
         for (i=0; i<MAX_ODEPS; i++)
	 {
	    if (rs->onames[i] != NA)
	    {
	       struct CV_link link;
	       struct RS_link *olink, *olink_next;

	       if (rs->spec_mode)
	       {
		  /* update the speculative create vector, future operations
		     get value from later creator or architected reg file */
		  link = spec_create_vector[sn][rs->onames[i]];
		  if (/* !NULL */link.rs
		      && /* refs RS */(link.rs == rs && link.odep_num == i))
		    {
		      /* the result can now be read from a physical register,
			 indicate this as so */
		      spec_create_vector[sn][rs->onames[i]] = CVLINK_NULL;
		      spec_create_vector_rt[sn][rs->onames[i]] = sim_cycle;
		    }
		  /* else, creator invalidated or there is another creator */
		}
	        else
		{
		   /* update the non-speculative create vector, future
		   operations get value from later creator or architected
	           reg file */
		   link = create_vector[sn][rs->onames[i]];
		   if (/* !NULL */link.rs
		       && /* refs RS */(link.rs == rs && link.odep_num == i))
		   {
		      /* the result can now be read from a physical register,
			 indicate this as so */
		      create_vector[sn][rs->onames[i]] = CVLINK_NULL;
		      create_vector_rt[sn][rs->onames[i]] = sim_cycle;
		   }
		   /* else, creator invalidated or there is another creator */
		}

	        /* walk output list, queue up ready operations */
	        for (olink=rs->odep_list[i]; olink; olink=olink_next)
		{
		   if (RSLINK_VALID(olink))
		   {
		      if (olink->rs->idep_ready[olink->x.opnum])
			panic(sn,"output dependence already satisfied");

		      /* input is now ready */
		      olink->rs->idep_ready[olink->x.opnum] = TRUE;

		      /* are all the register operands of target ready? */
		      if (OPERANDS_READY(olink->rs))
		      {
			 /* yes! enqueue instruction as ready, NOTE: stores
			     complete at dispatch, so no need to enqueue them */
			 if (!olink->rs->in_LSQ || ((MD_OP_FLAGS(olink->rs->op)&(F_MEM|F_STORE))
			       == (F_MEM|F_STORE)))
			    readyq_enqueue(olink->rs,sn);
			 /* else, ld op, issued when no mem conflict */
		      }
		   }

		   /* grab link to next element prior to free */
		   olink_next = olink->next;

		   /* free dependence link element */
		   RSLINK_FREE(olink);
		} /* for */
	        /* blow away the consuming op list */
	        rs->odep_list[i] = NULL;

	     } /* if (rs->onames[i] != NA) */

	 } /* for (i=0; i<MAX_ODEPS; i++) */

     } /* while ((rs = eventq_next_event(sn))) */

     /* to store the error integrity */
     aux_errno[sn] = errno;

  } /* for (sn=0;sn<process_num;sn++) */
}


/*
 *  LSQ_REFRESH() - memory access dependence checker/scheduler
 */

/* this function locates ready instructions whose memory dependencies have
   been satisfied, this is accomplished by walking the LSQ for loads, looking
   for blocking memory dependency condition (e.g., earlier store with an
   unknown address) */
#define MAX_STD_UNKNOWNS		64
static void
lsq_refresh(void)
{
   int i, j, sn, index, n_std_unknowns;
   md_addr_t std_unknowns[MAX_STD_UNKNOWNS];


   for (sn=0;sn<process_num;sn++)
   {
      /* to restore the error integrity */
      errno = aux_errno[sn]; 

      /* scan entire queue for ready loads: scan from oldest instruction
      (head) until we reach the tail or an unresolved store, after which no
      other instruction will become ready */
      for (i=0, index=LSQ_head[sn], n_std_unknowns=0;
          i < LSQ_num[sn]; i++, index=(index + 1) % LSQ_size)
      {
         /* terminate search for ready loads after first unresolved store,
	   as no later load could be resolved in its presence */
         if (/* store? */
	   (MD_OP_FLAGS(LSQ[sn][index].op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE))
	 {
	     if (!STORE_ADDR_READY(&LSQ[sn][index]))
	     {
	         /* FIXME: a later STD + STD known could hide the STA unknown */
	         /* sta unknown, blocks all later loads, stop search */
	         break;
	     }
	     else if (!OPERANDS_READY(&LSQ[sn][index]))
	          {
	              /* sta known, but std unknown, may block a later store, record
		        this address for later referral, we use an array here because
		        for most simulations the number of entries to search will be
		        very small */
	              if (n_std_unknowns == MAX_STD_UNKNOWNS)
		         fatal(sn,"STD unknown array overflow, increase MAX_STD_UNKNOWNS");
	              std_unknowns[n_std_unknowns++] = LSQ[sn][index].addr;
	          }
	          else /* STORE_ADDR_READY() && OPERANDS_READY() */
	          {
	             /* a later STD known hides an earlier STD unknown */
	             for (j=0; j<n_std_unknowns; j++)
		     {
		        if (std_unknowns[j] == /* STA/STD known */LSQ[sn][index].addr)
		           std_unknowns[j] = /* bogus addr */0;
		     }
	          }
	 }

         if (/* load? */
	    ((MD_OP_FLAGS(LSQ[sn][index].op) & (F_MEM|F_LOAD)) == (F_MEM|F_LOAD))
	    && /* queued? */!LSQ[sn][index].queued
	    && /* waiting? */!LSQ[sn][index].issued
	    && /* completed? */!LSQ[sn][index].completed
	    && /* regs ready? */OPERANDS_READY(&LSQ[sn][index]))
	 {
	    /* no STA unknown conflict (because we got to this check), check for
	     a STD unknown conflict */
	    for (j=0; j<n_std_unknowns; j++)
	    {
	       /* found a relevant STD unknown? */
	       if (std_unknowns[j] == LSQ[sn][index].addr)
		  break;
	    }
	    if (j == n_std_unknowns)
	    {
	       /* no STA or STD unknown conflicts, put load on ready queue */
	       readyq_enqueue(&LSQ[sn][index],sn);
	    }
	 }
      } /* for (i=0, index=LSQ_head[sn]... */

      /* to store the error integrity */
      aux_errno[sn] = errno; 

   } /* for (sn=0; sn<... */
}


/*
 *  RUU_ISSUE() - issue instructions to functional units
 */

/* attempt to issue all operations in the ready queue; insts in the ready
   instruction queue have all register dependencies satisfied, this function
   must then 1) ensure the instructions memory dependencies have been satisfied
   (see lsq_refresh() for details on this process) and 2) a function unit
   is available in this cycle to commence execution of the operation; if all
   goes well, the function unit is allocated, a writeback event is scheduled,
   and the instruction begins execution */
static void
ruu_issue(void)
{
   int i, load_lat, tlb_lat, n_issued, sn;
   struct RS_link *node[MAX_SLOTS], *next_node[MAX_SLOTS];
   struct res_template *fu;
   int not_done;
   static int next_slot = -1;
   int issued[MAX_SLOTS];

   /* tries issue for each slot */

/* fprintf(stderr, "\nentrou ruu_issue\n"); */

   for (sn=0; sn<process_num; sn++)
   {    issued[sn] = FALSE;
        node[sn] = ready_queue[sn];
        ready_queue[sn] = NULL;
   }

   next_slot++;
   next_slot = next_slot % process_num;
   sn = next_slot;
   not_done = process_num; 
   n_issued=0;

/* fprintf(stderr, "\nruu_issue passou 1 \n"); */

   while ( (not_done)
           && (n_issued < ruu_issue_width) )
   {
      /* to restore the error integrity */
      errno = aux_errno[sn];

/* fprintf(stderr, "\nruu_issue passou 2 \n"); */

      /* FIXME: could be a little more efficient when scanning the ready queue */

      /* copy and then blow away the ready list, NOTE: the ready list is
         always totally reclaimed each cycle, and instructions that are not
         issue are explicitly reinserted into the ready instruction queue,
         this management strategy ensures that the ready instruction queue
         is always properly sorted */

        /* visit all ready instructions (i.e., insts whose register input
        dependencies have been satisfied, stop issue when no more instructions
        are available or issue bandwidth is exhausted */
      if ( (node[sn]) &&
           (!issued[sn]) )
      {
         
/* fprintf(stderr, "\nruu_issue passou 3 \n"); */

         next_node[sn] = node[sn]->next;

         /* still valid? */
         if (RSLINK_VALID(node[sn]))
	 {
	    struct RUU_station *rs = RSLINK_RS(node[sn]);

/* fprintf(stderr, "\nruu_issue passou 4 \n"); */

	   /* issue operation, both reg and mem deps have been satisfied */
	   if (!OPERANDS_READY(rs) || !rs->queued
	      || rs->issued || rs->completed)
	      panic(sn,"issued inst !ready, issued, or completed");

	   /* node is now un-queued */
	   rs->queued = FALSE;

	   if (rs->in_LSQ
	      && ((MD_OP_FLAGS(rs->op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE)))
	   {
/* fprintf(stderr, "\nruu_issue passou 5 \n"); */

	        /* stores complete in effectively zero time, result is
		 written into the load/store queue, the actual store into
		 the memory system occurs when the instruction is retired
		 (see ruu_commit) */
	       rs->issued = TRUE;
	       rs->completed = TRUE;

	       if (rs->onames[0] || rs->onames[1])
		  panic(sn,"store creates result");

	       if (rs->recover_inst)
		  panic(sn,"mis-predicted store");

	       /* entered execute stage, indicate in pipe trace */
	       ptrace_newstage(rs->ptrace_seq, PST_WRITEBACK, 0);

	       /* one more inst issued */
	       n_issued++;
	   }
	   else
	   {

/* fprintf(stderr, "\nruu_issue passou 6 \n"); */

	      /* issue the instruction to a functional unit */
	      if (MD_OP_FUCLASS(rs->op) != NA)
	      {
		  fu = res_get(fu_pool, MD_OP_FUCLASS(rs->op));
		  if (fu)
		  {
/* fprintf(stderr, "\nruu_issue passou 8 \n"); */

		      /* got one! issue inst to functional unit */
		      rs->issued = TRUE;

		      /* reserve the functional unit */
		      if (fu->master->busy)
			 panic(sn,"functional unit already in use");

		      /* schedule functional unit release event */
		      fu->master->busy = fu->issuelat;

		      /* schedule a result writeback event */
		      if (rs->in_LSQ
			  && ((MD_OP_FLAGS(rs->op) & (F_MEM|F_LOAD))
		          == (F_MEM|F_LOAD)))
		      {
	      	          int events = 0;
                          md_addr_t addr, offset;

/* fprintf(stderr, "\nruu_issue passou 9 \n"); */

			  /* for loads, determine cache access latency:
			     first scan LSQ to see if a store forward is
			     possible, if not, access the data cache */
			  load_lat = 0;
			  i = (rs - LSQ[sn]);
			  if (i != LSQ_head[sn])
			  {

/* fprintf(stderr, "\nruu_issue passou 10 \n"); */

			     for (;;)
			     {
				 /* go to next earlier LSQ entry */
				 i = (i + (LSQ_size-1)) % LSQ_size;

			         /* FIXME: not dealing with partials! */
				 if ((MD_OP_FLAGS(LSQ[sn][i].op) & F_STORE)
				    && (LSQ[sn][i].addr == rs->addr))
				 {
				    /* hit in the LSQ */
				    load_lat = 1;
				    break;
				 }

				 /* scan file_analysed? */
				 if (i == LSQ_head[sn])
				    break;
			      }
			   }
                           /* "data_offset" puts an different offset in each */
                           /* address in order to simulate different initial */
                           /* memory pages allocated for different code. It must */
                           /* provides less misses in the cache during the start */

                           offset = (sn % (process_num / dl1_banks_num))*data_offset; 
                           addr = (rs->addr + offset)&~3;
     
			   /* was the value store forwared from the LSQ? */
			   if (!load_lat)
			   {
			      int valid_addr = MD_VALID_ADDR(rs->addr);

/* fprintf(stderr, "\nruu_issue passou 11 \n"); */

			      if (!spec_mode[sn] && !valid_addr)
				 sim_invalid_addrs[sn]++;

			      /* no! go to the data cache if addr is valid */
			      if (cache_dl1[0] /* there is dl1 */
                                  && valid_addr)
			      {   int dl1bank, dl2bank, start, finish, i;


                                  dl1bank = WHICHIS_DL1BANK_OF_SLOT(sn);

                                  /* access the cache if non-faulting */

/* fprintf(stderr, "\nruu_issue passou 12.1 \n"); */

     if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
        fprintf(stderr,"\n dl1 bank %d slot %d .....\n",dl1bank,sn);


				  load_lat =
				    cache_access(cache_dl1[dl1bank], Read,
						 addr, NULL, 4,
						 sim_cycle, NULL, NULL,sn);
				  if (load_lat > cache_dl1_lat)
				     events |= PEV_CACHEMISS;

/* fprintf(stderr, "\nruu_issue passou 12.2 \n"); */

                                  /* Broadcast bus_free for all dl1_banks those are connected in the same */
                                  /* l2 bank in order control the utilization of "dl1-dl2" bus by all dl1 */
                                  /* banks. It is considered that only one "dl1-dl2" bus is shared among */
                                  /* different dl1-banks. */
                                  /* Besides, there is another "il1-il2" bus for instructions, and, if */
                                  /* "il2=dl2", it is supposed there are 2 ports (2 bus) in this */
                                  /* unified level 2 cache. Also, if "il2!=dl2", it is considered 2 */
                                  /* ports (2 bus) in the memory. Otherwise, to simulates just 1 port in the */
                                  /* l2 cache or in the memory, it is necessary to set l1_bus_opt = 1 */

                                  dl2bank = WHICHIS_DL2BANK_OF_DL1(dl1bank);
                                  start=FIRST_DL1BANK_OF_DL2(dl2bank);
                                  finish=start + DL2_BANK_WIDTH;

                                  for (i=start;i<finish;i++)
                                      cache_dl1[i]->bus_free = cache_dl1[dl1bank]->bus_free;

/* fprintf(stderr, "\nruu_issue passou 12.3 \n"); */

                                  /* handle l1_bus_free */

                                  if (l1_bus_opt)
                                  {  start = FIRST_IL1BANK_OF_IL2(dl2bank); /* il2 & dl2 are unified */
                                     finish = start + IL2_BANK_WIDTH;
                                     for (i=start; i<finish; i++)
                                         cache_il1[i]->bus_free = cache_dl1[dl1bank]->bus_free;
                                  }

/* fprintf(stderr, "\nruu_issue passou 12.4 \n"); */

			       }
			       else
			       {
				  /* no caches defined, just use op latency */
				  load_lat = fu->oplat;
			       }
			    }

			    /* all loads and stores must to access D-TLB */
			    if (dtlb[0] && MD_VALID_ADDR(rs->addr))
			    {
/* fprintf(stderr, "\nruu_issue passou 13 \n"); */

			       /* access the D-DLB, NOTE: this code will
				 initiate speculative TLB misses */
			       tlb_lat =
				cache_access(dtlb[sn], Read, addr,
					     NULL, 4, sim_cycle, NULL, NULL,sn);
			       if (tlb_lat > 1)
				events |= PEV_TLBMISS;

			       /* D-cache/D-TLB accesses occur in parallel */
			       load_lat = MAX(tlb_lat, load_lat);
			     }

			     /* use computed cache access latency */
			     eventq_queue_event(rs, sim_cycle + load_lat,sn);

			     /* entered execute stage, indicate in pipe trace */
			     ptrace_newstage(rs->ptrace_seq, PST_EXECUTE,
					  ((rs->ea_comp ? PEV_AGEN : 0)
					   | events));
			 }
		         else /* !load && !store */
			 {
/* fprintf(stderr, "\nruu_issue passou 14 \n"); */

			    /* use deterministic functional unit latency */
			    eventq_queue_event(rs, sim_cycle + fu->oplat,sn);

			    /* entered execute stage, indicate in pipe trace */
			    ptrace_newstage(rs->ptrace_seq, PST_EXECUTE, 
					  rs->ea_comp ? PEV_AGEN : 0);
			 }

		         /* one more inst issued */
		         n_issued++;
		     }
		     else /* no functional unit */
		     {
		        /* insufficient functional unit resources, put operation
			 back onto the ready list, we'll try to issue it
			 again next cycle */
		       readyq_enqueue(rs,sn);
		     }
		 }
	         else /* does not require a functional unit! */
		 {

/* fprintf(stderr, "\nruu_issue passou 15 \n"); */

		   /* FIXME: need better solution for these */
		   /* the instruction does not need a functional unit */
		   rs->issued = TRUE;

		   /* schedule a result event */
		   eventq_queue_event(rs, sim_cycle + 1,sn);

		   /* entered execute stage, indicate in pipe trace */
		   ptrace_newstage(rs->ptrace_seq, PST_EXECUTE,
				  rs->ea_comp ? PEV_AGEN : 0);

		   /* one more inst issued */
		   n_issued++;
		 }
	     } /* !store */
	 }
         /* else, RUU entry was squashed */

         /* reclaim ready list entry, NOTE: this is done whether or not the
          instruction issued, since the instruction was once again reinserted
          into the ready queue if it did not issue, this ensures that the ready
          queue is always properly sorted */
         RSLINK_FREE(node[sn]);

         node[sn] = next_node[sn];

      }  /* if node[sn] ..*/
      else
      { 
/* fprintf(stderr, "\nruu_issue passou 16 \n"); */

        if (!issued[sn])
        {  issued[sn] = TRUE; /* it will not be issued */
           not_done--; /* less one to walk */
        }
      }

      sn++;
      sn = sn % process_num;

      /* to store the error integrity */
      aux_errno[sn] = errno;

   } /* while (not_done) ... */


      /* put any instruction not issued back into the ready queue, go through
        normal channels to ensure instruction stay ordered correctly */

   for (sn=0;sn<process_num;sn++)
      for (; node[sn]; node[sn] = next_node[sn])
      {
         next_node[sn] = node[sn]->next;

         /* still valid? */
         if (RSLINK_VALID(node[sn]))
         {
	   struct RUU_station *rs = RSLINK_RS(node[sn]);

           /* node is now un-queued */
           rs->queued = FALSE;

	   /* not issued, put operation back onto the ready list, we'll try to
	     issue it again next cycle */
           readyq_enqueue(rs,sn);
         }
         /* else, RUU entry was squashed */

         /* reclaim ready list entry, NOTE: this is done whether or not the
           instruction issued, since the instruction was once again reinserted
           into the ready queue if it did not issue, this ensures that the ready
           queue is always properly sorted */
         RSLINK_FREE(node[sn]);
      }
    
}


/*
 * routines for generating on-the-fly instruction traces with support
 * for control and data misspeculation modeling
 */

/* integer register file */
#define R_BMAP_SZ       (BITMAP_SIZE(MD_NUM_IREGS))
static BITMAP_TYPE(MD_NUM_IREGS, use_spec_R[MAX_SLOTS]);
static md_gpr_t spec_regs_R[MAX_SLOTS];

/* floating point register file */
#define F_BMAP_SZ       (BITMAP_SIZE(MD_NUM_FREGS))
static BITMAP_TYPE(MD_NUM_FREGS, use_spec_F[MAX_SLOTS]);
static md_fpr_t spec_regs_F[MAX_SLOTS];

/* miscellaneous registers */
#define C_BMAP_SZ       (BITMAP_SIZE(MD_NUM_CREGS))
static BITMAP_TYPE(MD_NUM_FREGS, use_spec_C[MAX_SLOTS]);
static md_ctrl_t spec_regs_C[MAX_SLOTS];

/* dump speculative register state */
static void
rspec_dump(FILE *stream,		/* output stream */
           int sn)                      /* slot's number */
{
  int i;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** speculative register contents **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode[sn] ? "t" : "f");

  /* dump speculative integer regs */
  for (i=0; i < MD_NUM_IREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_R[sn], R_BMAP_SZ, i))
	{
	  md_print_ireg(spec_regs_R[sn], i, stream);
	  fprintf(stream, "\n");
	}
    }

  /* dump speculative FP regs */
  for (i=0; i < MD_NUM_FREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, i))
	{
	  md_print_fpreg(spec_regs_F[sn], i, stream);
	  fprintf(stream, "\n");
	}
    }

  /* dump speculative CTRL regs */
  for (i=0; i < MD_NUM_CREGS; i++)
    {
      if (BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ, i))
	{
	  md_print_creg(spec_regs_C[sn], i, stream,sn);
	  fprintf(stream, "\n");
	}
    }
}


/* speculative memory hash table size, NOTE: this must be a power-of-two */
#define STORE_HASH_SIZE		32

/* speculative memory hash table definition, accesses go through this hash
   table when accessing memory in speculative mode, the hash table flush the
   table when recovering from mispredicted branches */
struct spec_mem_ent {
  struct spec_mem_ent *next;		/* ptr to next hash table bucket */
  md_addr_t addr;			/* virtual address of spec state */
  unsigned int data[2];			/* spec buffer, up to 8 bytes */
};

/* speculative memory hash table */
static struct spec_mem_ent *store_htable[MAX_SLOTS][STORE_HASH_SIZE];

/* speculative memory hash table bucket free list. See sim_init for initialization */
static struct spec_mem_ent *bucket_free_list[MAX_SLOTS] = {NULL};


/* program counter */
static md_addr_t pred_PC[MAX_SLOTS];
static md_addr_t recover_PC[MAX_SLOTS];

/* fetch unit next fetch address */
static md_addr_t fetch_regs_PC[MAX_SLOTS];
static md_addr_t fetch_pred_PC[MAX_SLOTS];

/* IFETCH -> DISPATCH instruction queue definition */
struct fetch_rec {
  md_inst_t IR;				/* inst register */
  md_addr_t regs_PC, pred_PC;		/* current PC, predicted next PC */
  struct bpred_update_t dir_update;	/* bpred direction update info */
  int stack_recover_idx;		/* branch predictor RSB index */
  unsigned int ptrace_seq;		/* print trace sequence id */
};

static struct fetch_rec *fetch_data[MAX_SLOTS];/* IFETCH -> DISPATCH inst queue */
static int fetch_num[MAX_SLOTS];	/* num entries in IF -> DIS queue */
static int fetch_tail[MAX_SLOTS], fetch_head[MAX_SLOTS];	/* head and tail pointers of queue */

/* recover instruction trace generator state to precise state state immediately
   before the first mis-predicted branch; this is accomplished by resetting
   all register value copied-on-write bitmasks are reset, and the speculative
   memory hash table is cleared */
static void
tracer_recover(int sn)     /* slot's number */
{
  int i;
  struct spec_mem_ent *ent, *ent_next;

  /* better be in mis-speculative trace generation mode */
  if (!spec_mode[sn])
    panic(sn,"cannot recover unless in speculative mode");

  /* reset to non-speculative trace generation mode */
  spec_mode[sn] = FALSE;

  /* reset copied-on-write register bitmasks back to non-speculative state */
  BITMAP_CLEAR_MAP(use_spec_R[sn], R_BMAP_SZ);
  BITMAP_CLEAR_MAP(use_spec_F[sn], F_BMAP_SZ);
  BITMAP_CLEAR_MAP(use_spec_C[sn], C_BMAP_SZ);

  /* reset memory state back to non-speculative state */
  /* FIXME: could version stamps be used here?!?!? */
  for (i=0; i<STORE_HASH_SIZE; i++)
    {
      /* release all hash table buckets */
      for (ent=store_htable[sn][i]; ent; ent=ent_next)
	{
	  ent_next = ent->next;
	  ent->next = bucket_free_list[sn];
	  bucket_free_list[sn] = ent;
	}
      store_htable[sn][i] = NULL;
    }

  /* if pipetracing, indicate squash of instructions in the inst fetch queue */
  if (ptrace_active)
    {
      while (fetch_num[sn] != 0)
	{
	  /* squash the next instruction from the IFETCH -> DISPATCH queue */
	  ptrace_endinst(fetch_data[sn][fetch_head[sn]].ptrace_seq);

	  /* consume instruction from IFETCH -> DISPATCH queue */
	  fetch_head[sn] = (fetch_head[sn]+1) & (ruu_ifq_size - 1);
	  fetch_num[sn]--;
	}
    }

  /* reset IFETCH state */
  fetch_num[sn] = 0;
  fetch_tail[sn] = fetch_head[sn] = 0;
  fetch_pred_PC[sn] = fetch_regs_PC[sn] = recover_PC[sn];
}

/* initialize the speculative instruction state generator state */
static void
tracer_init(int sn)  /* slot's number */
{
  int i;

  /* initially in non-speculative mode */
  spec_mode[sn] = FALSE;

  /* register state is from non-speculative state buffers */
  BITMAP_CLEAR_MAP(use_spec_R[sn], R_BMAP_SZ);
  BITMAP_CLEAR_MAP(use_spec_F[sn], F_BMAP_SZ);
  BITMAP_CLEAR_MAP(use_spec_C[sn], C_BMAP_SZ);

  /* memory state is from non-speculative memory pages */
  for (i=0; i<STORE_HASH_SIZE; i++)
    store_htable[sn][i] = NULL;
}


/* speculative memory hash table address hash function */
#define HASH_ADDR(ADDR)							\
  ((((ADDR) >> 24)^((ADDR) >> 16)^((ADDR) >> 8)^(ADDR)) & (STORE_HASH_SIZE-1))

/* this functional provides a layer of mis-speculated state over the
   non-speculative memory state, when in mis-speculation trace generation mode,
   the simulator will call this function to access memory, instead of the
   non-speculative memory access interfaces defined in memory.h; when storage
   is written, an entry is allocated in the speculative memory hash table,
   future reads and writes while in mis-speculative trace generation mode will
   access this buffer instead of non-speculative memory state; when the trace
   generator transitions back to non-speculative trace generation mode,
   tracer_recover() clears this table, returns any access fault */
static enum md_fault_type
spec_mem_access(struct mem_t *mem,		/* memory space to access */
		enum mem_cmd cmd,		/* Read or Write access cmd */
		md_addr_t addr,			/* virtual address of access */
		void *p,			/* input/output buffer */
		int nbytes,			/* number of bytes to access */
                int sn)                         /* slot's number */
{
  int i, index;
  struct spec_mem_ent *ent, *prev;

  /* FIXME: partially overlapping writes are not combined... */
  /* FIXME: partially overlapping reads are not handled correctly... */

  /* check alignments, even speculative this test should always pass */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    {
      /* no can do, return zero result */
      for (i=0; i < nbytes; i++)
	((char *)p)[i] = 0;

      return md_fault_none;
    }

  /* check permissions */
  if (!((addr >= ld_text_base[sn] && addr < (ld_text_base[sn]+ld_text_size[sn])
	 && cmd == Read)
	|| MD_VALID_ADDR(addr)))
    {
      /* no can do, return zero result */
      for (i=0; i < nbytes; i++)
	((char *)p)[i] = 0;

      return md_fault_none;
    }

  /* has this memory state been copied on mis-speculative write? */
  index = HASH_ADDR(addr);
  for (prev=NULL,ent=store_htable[sn][index]; ent; prev=ent,ent=ent->next)
    {
      if (ent->addr == addr)
	{
	  /* reorder chains to speed access into hash table */
	  if (prev != NULL)
	    {
	      /* not at head of list, relink the hash table entry at front */
	      prev->next = ent->next;
              ent->next = store_htable[sn][index];
              store_htable[sn][index] = ent;
	    }
	  break;
	}
    }

  /* no, if it is a write, allocate a hash table entry to hold the data */
  if (!ent && cmd == Write)
    {
      /* try to get an entry from the free list, if available */
      if (!bucket_free_list[sn])
	{
	  /* otherwise, call calloc() to get the needed storage */
	  bucket_free_list[sn] = calloc(1, sizeof(struct spec_mem_ent));
	  if (!bucket_free_list[sn])
	    fatal(sn,"out of virtual memory");
	}
      ent = bucket_free_list[sn];
      bucket_free_list[sn] = bucket_free_list[sn]->next;

      if (!bugcompat_mode)
	{
	  /* insert into hash table */
	  ent->next = store_htable[sn][index];
	  store_htable[sn][index] = ent;
	  ent->addr = addr;
	  ent->data[0] = 0; ent->data[1] = 0;
	}
    }

  /* handle the read or write to speculative or non-speculative storage */
  switch (nbytes)
    {
    case 1:
      if (cmd == Read)
	{
	  if (ent)
	    {
	      /* read from mis-speculated state buffer */
	      *((byte_t *)p) = *((byte_t *)(&ent->data[0]));
	    }
	  else
	    {
	      /* read from non-speculative memory state, don't allocate
	         memory pages with speculative loads */
	      *((byte_t *)p) = MEM_READ_BYTE(mem, addr);
	    }
	}
      else
	{
	  /* always write into mis-speculated state buffer */
	  *((byte_t *)(&ent->data[0])) = *((byte_t *)p);
	}
      break;
    case 2:
      if (cmd == Read)
	{
	  if (ent)
	    {
	      /* read from mis-speculated state buffer */
	      *((half_t *)p) = *((half_t *)(&ent->data[0]));
	    }
	  else
	    {
	      /* read from non-speculative memory state, don't allocate
	         memory pages with speculative loads */
	      *((half_t *)p) = MEM_READ_HALF(mem, addr);
	    }
	}
      else
	{
	  /* always write into mis-speculated state buffer */
	  *((half_t *)&ent->data[0]) = *((half_t *)p);
	}
      break;
    case 4:
      if (cmd == Read)
	{
	  if (ent)
	    {
	      /* read from mis-speculated state buffer */
	      *((word_t *)p) = *((word_t *)&ent->data[0]);
	    }
	  else
	    {
	      /* read from non-speculative memory state, don't allocate
	         memory pages with speculative loads */
	      *((word_t *)p) = MEM_READ_WORD(mem, addr);
	    }
	}
      else
	{
	  /* always write into mis-speculated state buffer */
	  *((word_t *)&ent->data[0]) = *((word_t *)p);
	}
      break;
    case 8:
      if (cmd == Read)
	{
	  if (ent)
	    {
	      /* read from mis-speculated state buffer */
	      *((word_t *)p) = *((word_t *)&ent->data[0]);
	      *(((word_t *)p)+1) = *((word_t *)&ent->data[1]);
	    }
	  else
	    {
	      /* read from non-speculative memory state, don't allocate
	         memory pages with speculative loads */
	      *((word_t *)p) = MEM_READ_WORD(mem, addr);
	      *(((word_t *)p)+1) =
		MEM_READ_WORD(mem, addr + sizeof(word_t));
	    }
	}
      else
	{
	  /* always write into mis-speculated state buffer */
	  *((word_t *)&ent->data[0]) = *((word_t *)p);
	  *((word_t *)&ent->data[1]) = *(((word_t *)p)+1);
	}
      break;
    default:
      panic(sn,"access size not supported in mis-speculative mode");
    }

  return md_fault_none;
}

/* dump speculative memory state */
static void
mspec_dump(FILE *stream,		/* output stream */
           int sn)                      /* slot's number */         
{
  int i;
  struct spec_mem_ent *ent;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** speculative memory contents **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode[sn] ? "t" : "f");

  for (i=0; i<STORE_HASH_SIZE; i++)
    {
      /* dump contents of all hash table buckets */
      for (ent=store_htable[sn][i]; ent; ent=ent->next)
	{
	  myfprintf(sn,stream, "[0x%08p]: %12.0f/0x%08x:%08x\n",
		    ent->addr, (double)(*((double *)ent->data)),
		    *((unsigned int *)&ent->data[0]),
		    *(((unsigned int *)&ent->data[0]) + 1));
	}
    }
}

/* default memory state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mem_obj(struct mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input/output buffer */
	      int nbytes,			/* size of access */
              int sn)
{
  enum mem_cmd cmd;

  if (!is_write)
    cmd = Read;
  else
    cmd = Write;

#if 0
  char *errstr;

  errstr = mem_valid(cmd, addr, nbytes, /* declare */FALSE,sn);
  if (errstr)
    return errstr;
#endif

  /* else, no error, access memory */
  if (spec_mode[sn])
    spec_mem_access(mem, cmd, addr, p, nbytes,sn);
  else
    mem_access(mem, cmd, addr, p, nbytes,sn);

  /* no error */
  return NULL;
}


/*
 *  RUU_DISPATCH() - decode instructions and allocate RUU and LSQ resources
 */

/* link RS onto the output chain number of whichever operation will next
   create the architected register value IDEP_NAME */
static INLINE void
ruu_link_idep(struct RUU_station *rs,		/* rs station to link */
	      int idep_num,			/* input dependence number */
	      int idep_name,			/* input register name */
              int sn)                           /* slot's number */
{
  struct CV_link head;
  struct RS_link *link;

  /* any dependence? */
  if (idep_name == NA)
    {
      /* no input dependence for this input slot, mark operand as ready */
      rs->idep_ready[idep_num] = TRUE;
      return;
    }

  /* locate creator of operand */
  head = CREATE_VECTOR(idep_name);

  /* any creator? */
  if (!head.rs)
    {
      /* no active creator, use value available in architected reg file,
         indicate the operand is ready for use */
      rs->idep_ready[idep_num] = TRUE;
      return;
    }
  /* else, creator operation will make this value sometime in the future */

  /* indicate value will be created sometime in the future, i.e., operand
     is not yet ready for use */
  rs->idep_ready[idep_num] = FALSE;

  /* link onto creator's output list of dependant operand */
  RSLINK_NEW(link, rs); link->x.opnum = idep_num;
  link->next = head.rs->odep_list[head.odep_num];
  head.rs->odep_list[head.odep_num] = link;
}

/* make RS the creator of architected register ODEP_NAME */
static INLINE void
ruu_install_odep(struct RUU_station *rs,	/* creating RUU station */
		 int odep_num,			/* output operand number */
		 int odep_name,			/* output register name */
                 int sn)                        /* slot's number */
{
  struct CV_link cv;

  /* any dependence? */
  if (odep_name == NA)
    {
      /* no value created */
      rs->onames[odep_num] = NA;
      return;
    }
  /* else, create a RS_NULL terminated output chain in create vector */

  /* record output name, used to update create vector at completion */
  rs->onames[odep_num] = odep_name;

  /* initialize output chain to empty list */
  rs->odep_list[odep_num] = NULL;

  /* indicate this operation is latest creator of ODEP_NAME */

  //CVLINK_INIT(cv, rs, odep_num); 

   cv.rs = rs; 
  cv.odep_num = odep_num;

  SET_CREATE_VECTOR(odep_name, cv);
}


/*
 * configure the instruction decode engine
 */

#define DNA			(0)

#if defined(TARGET_PISA)

/* general register dependence decoders */
#define DGPR(N)			(N)
#define DGPR_D(N)		((N) &~1)

/* floating point register dependence decoders */
#define DFPR_L(N)		(((N)+32)&~1)
#define DFPR_F(N)		(((N)+32)&~1)
#define DFPR_D(N)		(((N)+32)&~1)

/* miscellaneous register dependence decoders */
#define DHI			(0+32+32)
#define DLO			(1+32+32)
#define DFCC			(2+32+32)
#define DTMP			(3+32+32)

#elif defined(TARGET_ALPHA)

/* general register dependence decoders, $r31 maps to DNA (0) */
#define DGPR(N)			(31 - (N)) /* was: (((N) == 31) ? DNA : (N)) */

/* floating point register dependence decoders */
#define DFPR(N)			(((N) == 31) ? DNA : ((N)+32))

/* miscellaneous register dependence decoders */
#define DFPCR			(0+32+32)
#define DUNIQ			(1+32+32)
#define DTMP			(2+32+32)

#else
#error No ISA target defined...
#endif


/*
 * configure the execution engine
 */

/* next program counter */
#define SET_NPC(EXPR)           (regs[sn]->regs_NPC = (EXPR))

/* target program counter */
#undef  SET_TPC
#define SET_TPC(EXPR)		(target_PC[sn] = (EXPR))

/* current program counter */
#define CPC                   (regs[sn]->regs_PC)
#define SET_CPC(EXPR)         (regs[sn]->regs_PC = (EXPR))

/* general purpose register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see tracer_recover()
   for details on this process */
#define GPR(N)                (BITMAP_SET_P(use_spec_R[sn], R_BMAP_SZ, (N))\
				 ? spec_regs_R[sn][N]                       \
				 : regs[sn]->regs_R[N])
#define SET_GPR(N,EXPR)       (spec_mode[sn]				\
				 ? ((spec_regs_R[sn][N] = (EXPR)),		\
				    BITMAP_SET(use_spec_R[sn], R_BMAP_SZ, (N)),\
				    spec_regs_R[sn][N])			\
				 : (regs[sn]->regs_R[N] = (EXPR)))

#if defined(TARGET_PISA)

/* floating point register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see tracer_recover()
   for details on this process */
#define FPR_L(N)                 (BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, ((N)&~1))\
				 ? spec_regs_F[sn].l[(N)]                   \
				 : regs[sn]->regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)        (spec_mode[sn]				\
				 ? ((spec_regs_F[sn].l[(N)] = (EXPR)),	\
				    BITMAP_SET(use_spec_F[sn],F_BMAP_SZ,((N)&~1)),\
				    spec_regs_F[sn].l[(N)])			\
				 : (regs[sn]->regs_F.l[(N)] = (EXPR)))
#define FPR_F(N)                 (BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, ((N)&~1))\
				 ? spec_regs_F[sn].f[(N)]                   \
				 : regs[sn]->regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)        (spec_mode[sn]				\
				 ? ((spec_regs_F[sn].f[(N)] = (EXPR)),	\
				    BITMAP_SET(use_spec_F[sn],F_BMAP_SZ,((N)&~1)),\
				    spec_regs_F[sn].f[(N)])			\
				 : (regs[sn]->regs_F.f[(N)] = (EXPR)))
#define FPR_D(N)                 (BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, ((N)&~1))\
				 ? spec_regs_F[sn].d[(N) >> 1]              \
				 : regs[sn]->regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)        (spec_mode[sn]				\
				 ? ((spec_regs_F[sn].d[(N) >> 1] = (EXPR)),	\
				    BITMAP_SET(use_spec_F[sn],F_BMAP_SZ,((N)&~1)),\
				    spec_regs_F[sn].d[(N) >> 1])		\
				 : (regs[sn]->regs_F.d[(N) >> 1] = (EXPR)))

/* miscellanous register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see tracer_recover()
   for details on this process */
#define HI			(BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ, /*hi*/0)\
				 ? spec_regs_C[sn].hi			\
				 : regs[sn]->regs_C.hi)
#define SET_HI(EXPR)		(spec_mode[sn]				\
				 ? ((spec_regs_C[sn].hi = (EXPR)),		\
				    BITMAP_SET(use_spec_C[sn], C_BMAP_SZ,/*hi*/0),\
				    spec_regs_C[sn].hi)			\
				 : (regs[sn]->regs_C.hi = (EXPR)))
#define LO			(BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ, /*lo*/1)\
				 ? spec_regs_C[sn].lo			\
				 : regs[sn]->regs_C.lo)
#define SET_LO(EXPR)		(spec_mode[sn]				\
				 ? ((spec_regs_C[sn].lo = (EXPR)),		\
				    BITMAP_SET(use_spec_C[sn], C_BMAP_SZ,/*lo*/1),\
				    spec_regs_C[sn].lo)			\
				 : (regs[sn]->regs_C.lo = (EXPR)))
#define FCC			(BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ,/*fcc*/2)\
				 ? spec_regs_C[sn].fcc			\
				 : regs[sn]->regs_C.fcc)
#define SET_FCC(EXPR)   	(spec_mode[sn]				\
				 ? ((spec_regs_C[sn].fcc = (EXPR)),		\
				    BITMAP_SET(use_spec_C[sn],C_BMAP_SZ,/*fcc*/1),\
				    spec_regs_C[sn].fcc)			\
				 : (regs[sn]->regs_C.fcc = (EXPR)))

#elif defined(TARGET_ALPHA)

/* floating point register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see tracer_recover()
   for details on this process */
#define FPR_Q(N)		(BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, (N))\
				 ? spec_regs_F[sn].q[(N)]                   \
				 : regs[sn]->regs_F.q[(N)])
#define SET_FPR_Q(N,EXPR)	(spec_mode[sn]				\
				 ? ((spec_regs_F[sn].q[(N)] = (EXPR)),	\
				    BITMAP_SET(use_spec_F[sn],F_BMAP_SZ, (N)),\
				    spec_regs_F[sn].q[(N)])			\
				 : (regs[sn]->regs_F.q[(N)] = (EXPR)))
#define FPR(N)	    	        (BITMAP_SET_P(use_spec_F[sn], F_BMAP_SZ, (N))\
				 ? spec_regs_F[sn].d[(N)]			\
				 : regs[sn]->regs_F.d[(N)])
#define SET_FPR(N,EXPR) 	(spec_mode[sn]				\
				 ? ((spec_regs_F[sn].d[(N)] = (EXPR)),	\
				    BITMAP_SET(use_spec_F[sn],F_BMAP_SZ, (N)),\
				    spec_regs_F[sn].d[(N)])			\
				 : (regs[sn]->regs_F.d[(N)] = (EXPR)))

/* miscellanous register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see tracer_recover()
   for details on this process */
#define FPCR  		        (BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ,/*fpcr*/0)\
				 ? spec_regs_C[sn].fpcr			\
				 : regs[sn]->regs_C.fpcr)
#define SET_FPCR(EXPR)	        (spec_mode[sn]				\
			 	 ? ((spec_regs_C[sn].fpcr = (EXPR)),	\
				   BITMAP_SET(use_spec_C[sn],C_BMAP_SZ,/*fpcr*/0),\
				    spec_regs_C[sn].fpcr)			\
				 : (regs[sn]->regs_C.fpcr = (EXPR)))
#define UNIQ  		        (BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ,/*uniq*/1)\
			 	 ? spec_regs_C[sn].uniq			\
				 : regs[sn]->regs_C.uniq)
#define SET_UNIQ(EXPR)	        (spec_mode[sn]				\
				 ? ((spec_regs_C[sn].uniq = (EXPR)),	\
				   BITMAP_SET(use_spec_C[sn],C_BMAP_SZ,/*uniq*/1),\
				    spec_regs_C[sn].uniq)			\
				 : (regs[sn]->regs_C.uniq = (EXPR)))
#define FCC			(BITMAP_SET_P(use_spec_C[sn], C_BMAP_SZ,/*fcc*/2)\
				 ? spec_regs_C[sn].fcc			\
				 : regs[sn]->regs_C.fcc)
#define SET_FCC(EXPR)	        (spec_mode[sn]				\
				 ? ((spec_regs_C[sn].fcc = (EXPR)),		\
				    BITMAP_SET(use_spec_C[sn],C_BMAP_SZ,/*fcc*/1),\
				    spec_regs_C[sn].fcc)			\
				 : (regs[sn]->regs_C.fcc = (EXPR)))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros, NOTE: speculative copy on
   write storage provided for fast recovery during wrong path execute (see
   tracer_recover() for details on this process */
#define __READ_SPECMEM(SRC, SRC_V, FAULT)				\
  (addr = (SRC),							\
   (spec_mode[sn]								\
    ? ((FAULT) = spec_mem_access(mem[sn], Read, addr, &SRC_V, sizeof(SRC_V), sn))\
    : ((FAULT) = mem_access(mem[sn], Read, addr, &SRC_V, sizeof(SRC_V),sn))),	\
   SRC_V)

#define READ_BYTE(SRC, FAULT)	__READ_SPECMEM((SRC), temp_byte, (FAULT))
#define READ_HALF(SRC, FAULT)	__READ_SPECMEM((SRC), temp_half, (FAULT))
#define READ_WORD(SRC, FAULT)	__READ_SPECMEM((SRC), temp_word, (FAULT))
#ifdef HOST_HAS_QUAD
#define READ_QUAD(SRC, FAULT)	__READ_SPECMEM((SRC), temp_quad, (FAULT))
#endif /* HOST_HAS_QUAD */


#define __WRITE_SPECMEM(SRC, DST, DST_V, FAULT)				\
  (DST_V = (SRC), addr = (DST),						\
   (spec_mode[sn]								\
    ? ((FAULT) = spec_mem_access(mem[sn], Write, addr, &DST_V, sizeof(DST_V),sn))\
    : ((FAULT) = mem_access(mem[sn], Write, addr, &DST_V, sizeof(DST_V),sn))))

#define WRITE_BYTE(SRC, DST, FAULT)					\
  __WRITE_SPECMEM((SRC), (DST), temp_byte, (FAULT))
#define WRITE_HALF(SRC, DST, FAULT)					\
  __WRITE_SPECMEM((SRC), (DST), temp_half, (FAULT))
#define WRITE_WORD(SRC, DST, FAULT)					\
  __WRITE_SPECMEM((SRC), (DST), temp_word, (FAULT))
#ifdef HOST_HAS_QUAD
#define WRITE_QUAD(SRC, DST, FAULT)					\
  __WRITE_SPECMEM((SRC), (DST), temp_quad, (FAULT))
#endif /* HOST_HAS_QUAD */

/* system call handler macro */
#define SYSCALL(INST)							\
  (/* only execute system calls in non-speculative mode */		\
   (spec_mode[sn] ? panic(sn,"speculative syscall") : (void) 0),		\
   sys_syscall(regs[sn], mem_access, mem[sn], INST, TRUE,sn))

/* default register state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_reg_obj(struct regs_t *xregs,		/* registers to access */
	      int is_write,			/* access type */
	      enum md_reg_type rt,		/* reg bank to probe */
	      int reg,				/* register number */
	      struct eval_value_t *val,		/* input, output */
              int sn)				/* slot's number */
{
  switch (rt)
    {
    case rt_gpr:
      if (reg < 0 || reg >= MD_NUM_IREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = GPR(reg);
	}
      else
	SET_GPR(reg, eval_as_uint(*val,sn));
      break;

    case rt_lpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      /* FIXME: this is not portable... */
      abort();
#if 0
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = FPR_L(reg);
	}
      else
	SET_FPR_L(reg, eval_as_uint(*val,sn));
#endif
      break;

    case rt_fpr:
      /* FIXME: this is not portable... */
      abort();
#if 0
      if (!is_write)
	val->value.as_float = FPR_F(reg);
      else
	SET_FPR_F(reg, val->value.as_float);
#endif
      break;

    case rt_dpr:
      /* FIXME: this is not portable... */
      abort();
#if 0
      /* 1/2 as many regs in this mode */
      if (reg < 0 || reg >= MD_NUM_REGS/2)
	return "register number out of range";

      if (at == at_read)
	val->as_double = FPR_D(reg * 2);
      else
	SET_FPR_D(reg * 2, val->as_double);
#endif
      break;

      /* FIXME: this is not portable... */
#if 0
      abort();
    case rt_hi:
      if (at == at_read)
	val->as_word = HI;
      else
	SET_HI(val->as_word);
      break;
    case rt_lo:
      if (at == at_read)
	val->as_word = LO;
      else
	SET_LO(val->as_word);
      break;
    case rt_FCC:
      if (at == at_read)
	val->as_condition = FCC;
      else
	SET_FCC(val->as_condition);
      break;
#endif

    case rt_PC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs[sn]->regs_PC;
	}
      else
	regs[sn]->regs_PC = eval_as_addr(*val,sn);
      break;

    case rt_NPC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs[sn]->regs_NPC;
	}
      else
	regs[sn]->regs_NPC = eval_as_addr(*val,sn);
      break;

    default:
      panic(sn,"bogus register bank");
    }

  /* no error */
  return NULL;
}

/* these function return the total of instructions of the RRU and LSQ */
/* in order to allow simulate "shared" RUU/LSQ */

int
ruutotal(void)
{ int total=0, sn;

  for (sn=0;sn<process_num;sn++)
       total = total + RUU_num[sn];

  return total;
  
}

int
lsqtotal(void)
{ int total=0, sn;

  for (sn=0;sn<process_num;sn++)
       total = total + LSQ_num[sn];

  return total;
  
}


/* the last operation that ruu_dispatch() attempted to dispatch, for
   implementing in-order issue. See sim_init for initialization */

static struct RS_link last_op[MAX_SLOTS];


/* dispatch instructions from the IFETCH -> DISPATCH queue: instructions are
   first decoded, then they allocated RUU (and LSQ for load/stores) resources
   and input and output dependence chains are updated accordingly */
static void
ruu_dispatch(void)  
{
   int i;
   int sn;
   int n_dispatched;			/* total insts dispatched */
   md_inst_t inst;			/* actual instruction bits */
   enum md_opcode op;			/* decoded opcode enum */
   int out1, out2, in1, in2, in3;	/* output/input register names */
   md_addr_t target_PC[MAX_SLOTS];	        /* actual next/target PC address */
   md_addr_t addr;			/* effective address, if load/store */
   struct RUU_station *rs;		/* RUU station being allocated */
   struct RUU_station *lsq;		/* LSQ station for ld/st's */
   struct bpred_update_t *dir_update_ptr;/* branch predictor dir update ptr */
   int stack_recover_idx;		/* bpred retstack recovery index */
   unsigned int pseq;			/* pipetrace sequence number */
   int is_write;				/* store? */
   int made_check[MAX_SLOTS];			/* used to ensure DLite entry */
   int br_taken, br_pred_taken;		/* if br, taken?  predicted taken? */
   int fetch_redirected[MAX_SLOTS];
   byte_t temp_byte;			/* temp variable for spec mem access */
   half_t temp_half;			/* " ditto " */
   word_t temp_word;			/* " ditto " */
 #ifdef HOST_HAS_QUAD
   /* quad_T temp_quad; */			/* " ditto " */
 #endif /* HOST_HAS_QUAD */
   enum md_fault_type fault;

   int dispatched[MAX_SLOTS];

   static int next_slot = -1;
   int not_done = process_num;   

   for (sn=0;sn<process_num;sn++)
   {    dispatched[sn] = 0;   /* no instruction dispatched from slot sn */ 
        fetch_redirected[sn] = FALSE;
        made_check[sn] = FALSE;
   }

   /* Tries to dispatch from all IQ in a fashion like round-robin */
   /* decoding one instruction from each one per time until fill the */
   /* decode width. */
   /* The max number of instruction from each IQ is "decode_depth" */

   next_slot++;
   next_slot = next_slot % process_num;
   sn = next_slot;
   n_dispatched = 0;

   while ( (not_done) && 
           (n_dispatched < (ruu_decode_width * fetch_speed)) &&
           (ruutotal() < ruu_totalsize) &&
           (lsqtotal() < lsq_totalsize) )
   {
      /* to restore the error integrity */
      errno = aux_errno[sn];
       
      /* try dispatch one instruction from next process (sn) */

      if (  
           (dispatched[sn] < decode_depth)
	   /* RUU and LSQ not full? */
	   && RUU_num[sn] < RUU_size && LSQ_num[sn] < LSQ_size
	   /* insts still available from fetch unit? */
	   && fetch_num[sn] != 0
	   /* on an acceptable trace path */
	   && (ruu_include_spec || !spec_mode[sn]))
      {

         /* if issuing in-order, block until last op issues if inorder issue */
         if (ruu_inorder_issue
	    && (last_op[sn].rs && RSLINK_VALID(&last_op[sn])
	    && !OPERANDS_READY(last_op[sn].rs)))
	 {


	    /* stall until last operation is ready to issue */
            not_done--; /* less one to walk */
            dispatched[sn] = decode_depth; /* just to does not be more analysed */
	   
         }
         else
         {   int broke = FALSE; 

             /* get the next instruction from the IFETCH -> DISPATCH queue */
             inst = fetch_data[sn][fetch_head[sn]].IR;
             regs[sn]->regs_PC = fetch_data[sn][fetch_head[sn]].regs_PC;
             pred_PC[sn] = fetch_data[sn][fetch_head[sn]].pred_PC;
             dir_update_ptr = &(fetch_data[sn][fetch_head[sn]].dir_update);
             stack_recover_idx = fetch_data[sn][fetch_head[sn]].stack_recover_idx;
             pseq = fetch_data[sn][fetch_head[sn]].ptrace_seq;

             /* decode the inst */
             MD_SET_OPCODE(op, inst);

             /* compute default next PC */
             regs[sn]->regs_NPC = regs[sn]->regs_PC + sizeof(md_inst_t);

             /* drain RUU for TRAPs and system calls */
             if (MD_OP_FLAGS(op) & F_TRAP)
             {

	        if (RUU_num[sn] != 0)
	        {  

                   not_done--; /* less one to walk */
                   dispatched[sn] = decode_depth; /* just to does not be more analysed */
                   broke = TRUE;

                }
	             /* else, syscall is only instruction in the machine, */
                     /* at this point we should not be in (mis-)speculative mode */
	        if ( (!broke)
                     && (spec_mode[sn]) )
	           panic(sn,"drained and speculative");
	     }
             
             if (!broke)  /* continues */
             {

                /* maintain $r0 semantics (in spec and non-spec space) */
                regs[sn]->regs_R[MD_REG_ZERO] = 0; spec_regs_R[sn][MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
                regs[sn]->regs_F.d[MD_REG_ZERO] = 0.0; spec_regs_F[sn].d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

                if (!spec_mode[sn])
	        {
	           /* one more non-speculative instruction executed */
	           sim_num_insn[sn]++;
	        }

                /* default effective address (none) and access */
                addr = 0; is_write = FALSE;

                /* set default fault - none */
                fault = md_fault_none;

                /* more decoding and execution */
                switch (op)
                {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,O2,I1,I2,I3)		\
	          case OP:							\
	            /* compute output/input dependencies to out1-2 and in1-3 */	\
	            out1 = O1; out2 = O2;						\
	            in1 = I1; in2 = I2; in3 = I3;					\
	            /* execute the instruction */					\
	            SYMCAT(OP,_IMPL);						\
	            break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
	         case OP:							\
	           /* could speculatively decode a bogus inst, convert to NOP */	\
	           op = MD_NOP_OP;						\
	           /* compute output/input dependencies to out1-2 and in1-3 */	\
	           out1 = NA; out2 = NA;						\
	           in1 = NA; in2 = NA; in3 = NA;					\
	           /* no EXPR */							\
	           break;
#define CONNECT(OP)	/* nada... */
	           /* the following macro wraps the instruction fault declaration macro
	           with a test to see if the trace generator is in non-speculative
	           mode, if so the instruction fault is declared, otherwise, the
	           error is shunted because instruction faults need to be masked on
	           the mis-speculated instruction paths */
#define DECLARE_FAULT(FAULT)						\
	           {								\
	              if (!spec_mode[sn])						\
	                 fault = (FAULT);						\
	              /* else, spec fault, ignore it, always terminate exec... */	\
	              break;							\
	           }
#include "machine.def"
	         default:
	           /* can speculatively decode a bogus inst, convert to a NOP */
	           op = MD_NOP_OP;
	           /* compute output/input dependencies to out1-2 and in1-3 */	\
	           out1 = NA; out2 = NA;
	           in1 = NA; in2 = NA; in3 = NA;
	           /* no EXPR */
               } /* switch */

               /* operation sets next PC */

               if (fault != md_fault_none)
	         fatal(sn,"non-speculative fault (%d) detected @ 0x%08p",
	       fault, regs[sn]->regs_PC);

               /* update memory access stats */
               if (MD_OP_FLAGS(op) & F_MEM)
	       {

	         sim_total_refs[sn]++;
	         if (!spec_mode[sn])
	           sim_num_refs[sn]++;

	         if (MD_OP_FLAGS(op) & F_STORE)
	           is_write = TRUE;
	         else
	         {
	           sim_total_loads[sn]++;
	           if (!spec_mode[sn])
		      sim_num_loads[sn]++;
	         }
	       }

               br_taken = (regs[sn]->regs_NPC != (regs[sn]->regs_PC + sizeof(md_inst_t)));
               br_pred_taken = (pred_PC[sn] != (regs[sn]->regs_PC + sizeof(md_inst_t)));

               if ( (pred_forced)
                    &&(MD_OP_FLAGS(op) & F_CTRL) )
               {  int rate , reach;  

                 reach = (bpred_rate*RAND_MAX);
                 rate = 100*myrand();

                 /* Note: there is no predictor and default path is "not taken" */

                 if (rate <= reach)  /* forces good prediction */
                 { 

                   if (br_taken) /* it was wrong */
                   {   /* makes good */

                       fetch_pred_PC[sn] = regs[sn]->regs_NPC;

                       pred_PC[sn] = regs[sn]->regs_NPC;

                       fetch_head[sn] = (ruu_ifq_size-1);
	               fetch_num[sn] = 1;
	               fetch_tail[sn] = 0;

                    }
                    else; /* it was good: keep it */
                 }
                 else /* forces miss prediction */
                 {  

                   if (!br_taken) /* it was good */
                   {  /* makes bad. It receives a penalty in writeback */

        
                      /* forces target address in order to be computed */
                      /* correctly a miss predction */

                      pred_PC[sn] = target_PC[sn];

                      fetch_pred_PC[sn] = target_PC[sn] ; 

                      fetch_head[sn] = (ruu_ifq_size-1);
	              fetch_num[sn] = 1;
	              fetch_tail[sn] = 0;

                   }
                   else; /* was bad: keep it */              
	         }       
               }
        

               if ((pred_PC[sn] != regs[sn]->regs_NPC && pred_perfect)
	         || ((MD_OP_FLAGS(op) & (F_CTRL|F_DIRJMP)) == (F_CTRL|F_DIRJMP)
	         && target_PC[sn] != pred_PC[sn] && br_pred_taken))
	       {
	   
                 /* Either 1) we're simulating perfect prediction and are in a
                 mis-predict state and need to patch up, or 2) We're not simulating
                 perfect prediction, we've predicted the branch taken, but our
                 predicted target doesn't match the computed target (i.e.,
                 mis-fetch).  Just update the PC values and do a fetch squash.
                 This is just like calling fetch_squash() except we pre-anticipate
                 the updates to the fetch values at the end of this function.  If
                 case #2, also charge a mispredict penalty for redirecting fetch */

	         fetch_pred_PC[sn] = fetch_regs_PC[sn] = regs[sn]->regs_NPC;

	         /* was: if (pred_perfect) */
	         if (pred_perfect)
	            pred_PC[sn] = regs[sn]->regs_NPC;

	         fetch_head[sn] = (ruu_ifq_size-1);
	         fetch_num[sn] = 1;
	         fetch_tail[sn] = 0;

	         if (!pred_perfect)
                    ruu_slot_delay[sn] = ruu_branch_penalty;
 
	         fetch_redirected[sn] = TRUE;
	       }

               /* is this a NOP */
               if (op != MD_NOP_OP)
               {

	         /* for load/stores:
	            idep #0     - store operand (value that is store'ed)
	            idep #1, #2 - eff addr computation inputs (addr of access)

	            resulting RUU/LSQ operation pair:
	            RUU (effective address computation operation):
		    idep #0, #1 - eff addr computation inputs (addr of access)
	            LSQ (memory access operation):
		    idep #0     - operand input (value that is store'd)
		    idep #1     - eff addr computation result (from RUU op)

	            effective address computation is transfered via the reserved
	            name DTMP
	         */

	         /* fill in RUU reservation station */
	         rs = &RUU[sn][RUU_tail[sn]];

	         rs->IR = inst;
	         rs->op = op;
	         rs->PC = regs[sn]->regs_PC;
	         rs->next_PC = regs[sn]->regs_NPC; rs->pred_PC = pred_PC[sn];
	         rs->in_LSQ = FALSE;
	         rs->ea_comp = FALSE;
	         rs->recover_inst = FALSE;
                 rs->dir_update = *dir_update_ptr;
	         rs->stack_recover_idx = stack_recover_idx;
	         rs->spec_mode = spec_mode[sn];
	         rs->addr = 0;

	         /* rs->tag is already set */
	         rs->seq = ++inst_seq[sn];
	         rs->queued = rs->issued = rs->completed = FALSE;
	         rs->ptrace_seq = pseq;

	         /* split ld/st's into two operations: eff addr comp + mem access */
	         if (MD_OP_FLAGS(op) & F_MEM)
                 {

	           /* convert RUU operation from ld/st to an add (eff addr comp) */
	           rs->op = MD_AGEN_OP;
	           rs->ea_comp = TRUE;

	           /* fill in LSQ reservation station */
	           lsq = &LSQ[sn][LSQ_tail[sn]];

	           lsq->IR = inst;
	           lsq->op = op;
	           lsq->PC = regs[sn]->regs_PC;
	           lsq->next_PC = regs[sn]->regs_NPC; lsq->pred_PC = pred_PC[sn];
	           lsq->in_LSQ = TRUE;
	           lsq->ea_comp = FALSE;
	           lsq->recover_inst = FALSE;
	           lsq->dir_update.pdir1 = lsq->dir_update.pdir2 = NULL;
	           lsq->dir_update.pmeta = NULL;
	           lsq->stack_recover_idx = 0;
	           lsq->spec_mode = spec_mode[sn];
	           lsq->addr = addr;
	           /* lsq->tag is already set */
	           lsq->seq = ++inst_seq[sn];
	           lsq->queued = lsq->issued = lsq->completed = FALSE;
	           lsq->ptrace_seq = ptrace_seq[sn]++;

	           /* pipetrace this uop */
	           ptrace_newuop(lsq->ptrace_seq, "internal ld/st", lsq->PC, 0);
	           ptrace_newstage(lsq->ptrace_seq, PST_DISPATCH, 0);

	           /* link eff addr computation onto operand's output chains */
	           ruu_link_idep(rs, /* idep_ready[] index */0, NA,sn);
	           ruu_link_idep(rs, /* idep_ready[] index */1, in2,sn);
	           ruu_link_idep(rs, /* idep_ready[] index */2, in3,sn);

	           /* install output after inputs to prevent self reference */
	           ruu_install_odep(rs, /* odep_list[] index */0, DTMP,sn);
	           ruu_install_odep(rs, /* odep_list[] index */1, NA,sn);

	           /* link memory access onto output chain of eff addr operation */
	           ruu_link_idep(lsq,
			    /* idep_ready[] index */STORE_OP_INDEX/* 0 */,in1,sn);
	           ruu_link_idep(lsq,
			    /* idep_ready[] index */STORE_ADDR_INDEX/* 1 */, DTMP,sn);
	           ruu_link_idep(lsq, /* idep_ready[] index */2, NA,sn);

	           /* install output after inputs to prevent self reference */
	           ruu_install_odep(lsq, /* odep_list[] index */0, out1,sn);
	           ruu_install_odep(lsq, /* odep_list[] index */1, out2,sn);

	           /* install operation in the RUU and LSQ */
	           n_dispatched++;          
	           RUU_tail[sn] = (RUU_tail[sn] + 1) % RUU_size;
	           RUU_num[sn]++;
	           LSQ_tail[sn] = (LSQ_tail[sn] + 1) % LSQ_size;
	           LSQ_num[sn]++;

	           if (OPERANDS_READY(rs))
	           {
		      /* eff addr computation ready, queue it on ready list */
		      readyq_enqueue(rs,sn);
	           }

  	           /* issue may continue when the load/store is issued */
	           RSLINK_INIT(last_op[sn], lsq);

	           /* issue stores only, loads are issued by lsq_refresh() */
	           if (((MD_OP_FLAGS(op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE))
		      && OPERANDS_READY(lsq))
	           {
		      /* panic(sn,"store immediately ready"); */
		      /* put operation on ready list, ruu_issue() issue it later */
		      readyq_enqueue(lsq,sn);
	           }
                 }  
	         else /* !(MD_OP_FLAGS(op) & F_MEM) */
	         {

	            /* link onto producing operation */
	            ruu_link_idep(rs, /* idep_ready[] index */0, in1,sn);
	            ruu_link_idep(rs, /* idep_ready[] index */1, in2,sn);
	            ruu_link_idep(rs, /* idep_ready[] index */2, in3,sn);

	            /* install output after inputs to prevent self reference */
	            ruu_install_odep(rs, /* odep_list[] index */0, out1,sn);
	            ruu_install_odep(rs, /* odep_list[] index */1, out2,sn);

	            /* install operation in the RUU */
	            n_dispatched++;
	            RUU_tail[sn] = (RUU_tail[sn] + 1) % RUU_size;
	            RUU_num[sn]++;

	            /* issue op if all its reg operands are ready (no mem input) */
	            if (OPERANDS_READY(rs))
	            {
		      /* put operation on ready list, ruu_issue() issue it later */
		      readyq_enqueue(rs,sn);
	              /* issue may continue */
		      last_op[sn] = RSLINK_NULL;
	            }
	            else
	            {
		      /* could not issue this inst, stall issue until we can */
		      RSLINK_INIT(last_op[sn], rs);
	            }
	         }
               }
               else /*!(op != MD_NOP_OP) */
	       {
	          /* this is a NOP, no need to update RUU/LSQ state */
	          rs = NULL;

	       }

               /* one more instruction executed, speculative or otherwise */
               sim_total_insn[sn]++;

               if (MD_OP_FLAGS(op) & F_CTRL)
	         sim_total_branches[sn]++;

               if (!spec_mode[sn])
               {
#if 0 /* moved above for EIO trace file support */
	         /* one more non-speculative instruction executed */
	         sim_num_insn[sn]++;
#endif

	         /* if this is a branching instruction update BTB, i.e., only
	             non-speculative state is committed into the BTB */
	         if (MD_OP_FLAGS(op) & F_CTRL)
                 {

	            sim_num_branches[sn]++;
	            if (pred[sn] && bpred_spec_update == spec_ID)
	            {

		        bpred_update(pred[sn],
		          /* branch address */regs[sn]->regs_PC,
		          /* actual target address */regs[sn]->regs_NPC,
		          /* taken? */regs[sn]->regs_NPC != (regs[sn]->regs_PC +
						       sizeof(md_inst_t)),
		          /* pred taken? */pred_PC[sn] != (regs[sn]->regs_PC +
							sizeof(md_inst_t)),
		          /* correct pred? */pred_PC[sn] == regs[sn]->regs_NPC,
		          /* opcode */op,
		          /* predictor update ptr */&rs->dir_update);
	             }
                 }

	         /* is the trace generator trasitioning into mis-speculation mode? */
	         if (pred_PC[sn] != regs[sn]->regs_NPC && !fetch_redirected[sn])
	         {

	            /* entering mis-speculation mode, indicate this and save PC */
	            spec_mode[sn] = TRUE;

	            rs->recover_inst = TRUE;

	            recover_PC[sn] = regs[sn]->regs_NPC;

	         }
               }  /* if !(spec_mode[sn]) */

               /* entered decode/allocate stage, indicate in pipe trace */
               ptrace_newstage(pseq, PST_DISPATCH,
		   (pred_PC[sn] != regs[sn]->regs_NPC) ? PEV_MPOCCURED : 0);

               if (op == MD_NOP_OP)
	       { 
	         /* end of the line */
	         ptrace_endinst(pseq);
	       }

               /* update any stats tracked by PC */
               for (i=0; i<pcstat_nelt; i++)
	       {
	         counter_t newval;
	         int delta;

	         /* check if any tracked stats changed */
	         newval = STATVAL(pcstat_stats[sn][i]);
	         delta = newval - pcstat_lastvals[sn][i];
	         if (delta != 0)
	         {
	            stat_add_samples(pcstat_sdists[sn][i], regs[sn]->regs_PC, delta,sn);
	            pcstat_lastvals[sn][i] = newval;
	         }
	       }

               /* consume instruction from IFETCH -> DISPATCH queue */
               fetch_head[sn] = (fetch_head[sn]+1) & (ruu_ifq_size - 1);
               fetch_num[sn]--;

               dispatched[sn]++;

               if (dispatched[sn] == decode_depth)
                  not_done--;

               /* check for DLite debugger entry condition */
               made_check[sn] = TRUE;
               if (dlite_check_break(pred_PC[sn],
	           is_write ? ACCESS_WRITE : ACCESS_READ,
	           addr, sim_num_insn[sn], sim_cycle))
	         dlite_main(regs[sn]->regs_PC, pred_PC[sn], sim_cycle, regs[sn], mem[sn],  sn);

            } /* if !broke */
       
         } /* else from if ruu_inorder_issue ....*/

      } /* if  (dispatched[sn] < decode_depth) ... */
      else /* is not possible to dispatch from that IQ */
      { if (dispatched[sn] < decode_depth)
           {  not_done--; /* less one to walk */
              dispatched[sn] = decode_depth; /* just to does not be more analysed */
           }
        else; /* it was analysed */

      }

      /* need to enter DLite at least once per cycle */
      if (!made_check[sn])
      {
         if (dlite_check_break(/* no next PC */0,
		  is_write ? ACCESS_WRITE : ACCESS_READ,
	          addr, sim_num_insn[sn], sim_cycle))
	    dlite_main(regs[sn]->regs_PC, /* no next PC */0, sim_cycle, regs[sn], mem[sn],sn);
      }

      /* to store the error integrity */
      aux_errno[sn] = errno;

      /* goes try from next slot */
      sn++;
      sn = sn % process_num;

   } /* while ( (not_done) || .. */

/* fprintf(stderr,"\nRUU total = %d LSQ total = %d\n",ruutotal(), lsqtotal()); */

}


/*
 *  RUU_FETCH() - instruction fetch pipeline stage(s)
 */

/* initialize the instruction fetch pipeline stage */
static void
fetch_init(int sn)            /* slot's number */
{
  /* allocate the IFETCH -> DISPATCH instruction queue */
  fetch_data[sn] =
    (struct fetch_rec *)calloc(ruu_ifq_size, sizeof(struct fetch_rec));
  if (!fetch_data[sn])
    fatal(sn,"out of virtual memory");

  fetch_num[sn] = 0;
  fetch_tail[sn] = fetch_head[sn] = 0;
  IFQ_count[sn] = 0;
  IFQ_fcount[sn] = 0;
}

/* dump contents of fetch stage registers and fetch queue */
void
fetch_dump(FILE *stream,		/* output stream */
           int sn)                     /* slot's number */
{
  int num, head;

  if (!stream)
    stream = stderr;

  fprintf(stream, "** fetch stage state **\n");

  fprintf(stream, "spec_mode: %s\n", spec_mode[sn] ? "t" : "f");
  myfprintf(sn,stream, "pred_PC: 0x%08p, recover_PC: 0x%08p\n",
	    pred_PC[sn], recover_PC[sn]);
  myfprintf(sn,stream, "fetch_regs_PC: 0x%08p, fetch_pred_PC: 0x%08p\n",
	    fetch_regs_PC[sn], fetch_pred_PC[sn]);
  fprintf(stream, "\n");

  fprintf(stream, "** fetch queue contents **\n");
  fprintf(stream, "fetch_num: %d\n", fetch_num[sn]);
  fprintf(stream, "fetch_head: %d, fetch_tail[sn]: %d\n",
	  fetch_head[sn], fetch_tail[sn]);

  num = fetch_num[sn];
  head = fetch_head[sn];
  while (num)
    {
      fprintf(stream, "idx: %2d: inst: `", head);
      md_print_insn(fetch_data[sn][head].IR, fetch_data[sn][head].regs_PC, stream);
      fprintf(stream, "'\n");
      myfprintf(sn,stream, "         regs_PC: 0x%08p, pred_PC: 0x%08p\n",
		fetch_data[sn][head].regs_PC, fetch_data[sn][head].pred_PC);
      head = (head + 1) & (ruu_ifq_size - 1);
      num--;
    }
}

/* See initialization in sim_init */
static int last_inst_missed[MAX_SLOTS];
static int last_inst_tmissed[MAX_SLOTS];

/* fetch up as many instruction as one branch prediction and one cache line
   acess will support without overflowing the IFETCH -> DISPATCH QUEUE */
static void
ruu_fetch(int sn, int il1bank)  /* slot's number, i-cache bank */
{
  int i, lat, tlb_lat, done = FALSE;
  md_inst_t inst;
  int stack_recover_idx;
  int branch_cnt;


  if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
     fprintf(stderr,"\nbank = %d slot = %d\n",il1bank,sn);

  if (!ruu_fetch_width)
       ruu_fetch_width = ruu_decode_width * fetch_speed;
  for (i=0, branch_cnt=0;
       /* fetch up to as many instruction as the DISPATCH stage can decode */
       i < (ruu_fetch_width)
       /* fetch until IFETCH -> DISPATCH queue fills */
       && fetch_num[sn] < ruu_ifq_size
       /* and no IFETCH blocking condition encountered */
       && !done;
       i++)
    {
      /* fetch an instruction at the next predicted fetch address */
      fetch_regs_PC[sn] = fetch_pred_PC[sn];

      /* is this a bogus text address? (can happen on mis-spec path) */
      if (ld_text_base[sn] <= fetch_regs_PC[sn]
	  && fetch_regs_PC[sn] < (ld_text_base[sn]+ld_text_size[sn])
	  && !(fetch_regs_PC[sn] & (sizeof(md_inst_t)-1)))
	{ 
          md_addr_t addr;

          /* address is within program text, read instruction from memory */

          addr = fetch_regs_PC[sn];

/* fprintf(stderr,"\npassou 1 ....... \n"); */

	  /* in fact, read instruction from memory */
	  mem_access(mem[sn], Read, fetch_regs_PC[sn], &inst, sizeof(md_inst_t),sn);
    
	  /* simulates cache's access */
	  lat = cache_il1_lat;
	  if (cache_il1[0]) /* there is il1 caches */
	    {
/* fprintf(stderr,"\npassou 2 ....... \n"); */

	      /* access the I-cache */
	      lat =
		cache_access(cache_il1[il1bank], Read, IACOMPRESS(addr),
			     NULL, ISCOMPRESS(sizeof(md_inst_t)), sim_cycle,
			     NULL, NULL,sn);
	      if (lat > cache_il1_lat)
		last_inst_missed[sn] = TRUE;
	    }

	  if (itlb[0])
	    {
	      /* access the I-TLB, NOTE: this code will initiate
		 speculative TLB misses */

/* fprintf(stderr,"\npassou 3 ....... \n"); */

	      tlb_lat =
		cache_access(itlb[sn], Read, IACOMPRESS(addr),
			     NULL, ISCOMPRESS(sizeof(md_inst_t)), sim_cycle,
			     NULL, NULL,sn);
	      if (tlb_lat > 1)
		last_inst_tmissed[sn] = TRUE;

	      /* I-cache/I-TLB accesses occur in parallel */
	      lat = MAX(tlb_lat, lat);
	    }

	  /* I-cache/I-TLB miss? assumes I-cache hit >= I-TLB hit */
	  if (lat != cache_il1_lat)
	    {

              /* I-cache miss, block fetch until it is resolved */
	      ruu_slot_delay[sn] += lat - 1;
	      
              if (ruu_slot_delay[sn]>max_slot_delay)
              {  max_slot_delay = ruu_slot_delay[sn];
                 max_delayed_slot = sn;
              }  

	      break;
	    }
	  /* else, I-cache/I-TLB hit */
	}
      else
	{
	  /* fetch PC is bogus, send a NOP down the pipeline */
	  inst = MD_NOP_INST;
          sim_total_nops_insn[sn]++;
	}

      /* have a valid inst, here */

      /* possibly use the BTB target */
      if (pred[sn])
	{
	  enum md_opcode op;

	  /* pre-decode instruction, used for bpred stats recording */
	  MD_SET_OPCODE(op, inst);
	  
	  /* get the next predicted fetch address; only use branch predictor
	     result for branches (assumes pre-decode bits); NOTE: returned
	     value may be 1 if bpred can only predict a direction */
	  if (MD_OP_FLAGS(op) & F_CTRL)
	    fetch_pred_PC[sn] =
	      bpred_lookup(pred[sn],
			   /* branch address */fetch_regs_PC[sn],
			   /* target address *//* FIXME: not computed */0,
			   /* opcode */op,
			   /* call? */MD_IS_CALL(op),
			   /* return? */MD_IS_RETURN(op),
			   /* updt */&(fetch_data[sn][fetch_tail[sn]].dir_update),
			   /* RSB index */&stack_recover_idx,
                           /* slot's number */ sn);
	  else
	    fetch_pred_PC[sn] = 0;

	  /* valid address returned from branch predictor? */
	  if (!fetch_pred_PC[sn])
	    {
	      /* no predicted taken target, attempt not taken target */
	      fetch_pred_PC[sn] = fetch_regs_PC[sn] + sizeof(md_inst_t);
	    }
	  else
	    {
	      /* go with target, NOTE: discontinuous fetch, so terminate */
	      branch_cnt++;
	      if (branch_cnt >= fetch_speed)
		done = TRUE;
	    }
	}
      else
	{
	  /* no predictor, just default to predict not taken, and
	     continue fetching instructions linearly */
	  fetch_pred_PC[sn] = fetch_regs_PC[sn] + sizeof(md_inst_t);
	}
/* fprintf(stderr,"\npassou 4 ....... \n"); */

      /* commit this instruction to the IFETCH -> DISPATCH queue */
      fetch_data[sn][fetch_tail[sn]].IR = inst;
      fetch_data[sn][fetch_tail[sn]].regs_PC = fetch_regs_PC[sn];
      fetch_data[sn][fetch_tail[sn]].pred_PC = fetch_pred_PC[sn];
      fetch_data[sn][fetch_tail[sn]].stack_recover_idx = stack_recover_idx;
      fetch_data[sn][fetch_tail[sn]].ptrace_seq = ptrace_seq[sn]++;

      /* for pipe trace */
      ptrace_newinst(fetch_data[sn][fetch_tail[sn]].ptrace_seq,
		     inst, fetch_data[sn][fetch_tail[sn]].regs_PC,
		     0);
      ptrace_newstage(fetch_data[sn][fetch_tail[sn]].ptrace_seq,
		      PST_IFETCH,
		      ((last_inst_missed[sn] ? PEV_CACHEMISS : 0)
		       | (last_inst_tmissed[sn] ? PEV_TLBMISS : 0)));
      last_inst_missed[sn] = FALSE;
      last_inst_tmissed[sn] = FALSE;

      /* adjust instruction fetch queue */
      fetch_tail[sn] = (fetch_tail[sn] + 1) & (ruu_ifq_size - 1);
      fetch_num[sn]++;
    }
}

/* default machine state accessor, used by DLite */
static char *					/* err str, NULL for no err */
simoo_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* registers to access */
		 struct mem_t *mem,		/* memory space to access */
                 int sn)                        /* slot's number */
{
  if (!cmd || !strcmp(cmd, "help"))
    fprintf(stream,
"mstate commands:\n"
"\n"
"    mstate help   - show all machine-specific commands (this list)\n"
"    mstate stats  - dump all statistical variables\n"
"    mstate res    - dump current functional unit resource states\n"
"    mstate ruu    - dump contents of the register update unit\n"
"    mstate lsq    - dump contents of the load/store queue\n"
"    mstate eventq - dump contents of event queue\n"
"    mstate readyq - dump contents of ready instruction queue\n"
"    mstate cv     - dump contents of the register create vector\n"
"    mstate rspec  - dump contents of speculative regs\n"
"    mstate mspec  - dump contents of speculative memory\n"
"    mstate fetch  - dump contents of fetch stage registers and fetch queue\n"
"\n"
	    );
  else if (!strcmp(cmd, "stats"))
    {
      /* just dump intermediate stats */
      sim_print_stats(stream);
    }
  else if (!strcmp(cmd, "res"))
    {
      /* dump resource state */
      res_dump(fu_pool, stream);
    }
  else if (!strcmp(cmd, "ruu"))
    {
      /* dump RUU contents */
      ruu_dump(stream,sn);
    }
  else if (!strcmp(cmd, "lsq"))
    {
      /* dump LSQ contents */
      lsq_dump(stream,sn);
    }
  else if (!strcmp(cmd, "eventq"))
    {
      /* dump event queue contents */
      eventq_dump(stream,sn);
    }
  else if (!strcmp(cmd, "readyq"))
    {
      /* dump event queue contents */
      readyq_dump(stream,sn);
    }
  else if (!strcmp(cmd, "cv"))
    {
      /* dump event queue contents */
      cv_dump(stream,sn);
    }
  else if (!strcmp(cmd, "rspec"))
    {
      /* dump event queue contents */
      rspec_dump(stream,sn);
    }
  else if (!strcmp(cmd, "mspec"))
    {
      /* dump event queue contents */
      mspec_dump(stream,sn);
    }
  else if (!strcmp(cmd, "fetch"))
    {
      /* dump event queue contents */
      fetch_dump(stream,sn);
    }
  else
    return "unknown mstate command";

  /* no error */
  return NULL;
}

static int forward_number = 0;

void DESCARTA(void)
{ int sn; /* number of slots */
  enum md_fault_type fault;


  forward_number = fastfwd_count;

  for (sn=0;sn<process_num;sn++) /* Do it for all slots */
  {
    int icount;
    md_inst_t inst;		/* actual instruction bits */
    enum md_opcode op;	/* decoded opcode enum */
    md_addr_t target_PC[sn];	/*  actual next/target PC address */
    md_addr_t addr;		/* effective address, if load/store */
    int is_write;		/* store? */
    byte_t temp_byte;	/* temp variable for spec mem access */
    half_t temp_half;	/* " ditto " */
    word_t temp_word;	/* " ditto " */

    forward_number = forward_number + forward_step;

    fprintf(stderr, "SS_SMT: %d: ** fast forwarding %d insts **\n",sn, forward_number);
    
    for (icount=0; icount < forward_number; icount++)
    {
      /* maintain $r0 semantics */
      regs[sn]->regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs[sn]->regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      inst = __UNCHK_MEM_READ(mem[sn], regs[sn]->regs_PC, md_inst_t);
 
      /* set default reference address */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
      {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	 case OP:							\
	         SYMCAT(OP,_IMPL);						\
	         break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
	 case OP:							\
	         panic(sn,"attempted to execute a linking opcode");
#define CONNECT(OP)
#undef DECLARE_FAULT
#define DECLARE_FAULT(FAULT)						\
	         { fault = (FAULT); break; }
#include "machine.def"
	         default:
	         panic(sn,"attempted to execute a bogus opcode");
      } /* end of switch (op) */

      if (fault != md_fault_none)
	 fatal(sn,"fault (%d) detected @ 0x%08p", fault, regs[sn]->regs_PC);

      /* update memory access stats */
      if (MD_OP_FLAGS(op) & F_MEM)
      {
	 if (MD_OP_FLAGS(op) & F_STORE)
            is_write = TRUE;
      }

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs[sn]->regs_NPC,
	    is_write ? ACCESS_WRITE : ACCESS_READ,
	    addr, sim_num_insn[sn], sim_num_insn[sn]))
      dlite_main(regs[sn]->regs_PC, regs[sn]->regs_NPC, sim_num_insn[sn], regs[sn], mem[sn],sn);

      /* go to the next instruction */
      regs[sn]->regs_PC = regs[sn]->regs_NPC;
      regs[sn]->regs_NPC += sizeof(md_inst_t);
    } /* end of "for (icount=0 ...)" */
  } /* end of "for (sn=0;sn<process_num;sn++)" */
}

void CONCLUSAO(void)
{ ruu_commit(); }

void TERMINO(void)
{

  /* service function unit release events */
  ruu_release_fu();

  /* ==> may have ready queue entries carried over from previous cycles */

  /* service result completions, also readies dependent operations */
  /* ==> inserts operations into ready queue --> register deps resolved */
  ruu_writeback();

}

void REMESSA(void)
{

 /* try to locate memory operations that are ready to execute */
 /* ==> inserts operations into ready queue --> mem deps resolved */
 lsq_refresh();

 /* issue operations ready to execute from a previous cycle */
 /* <== drains ready queue <-- ready operations commence execution */
 ruu_issue();

}

void DECOD_DESPACHA(void)
{ ruu_dispatch(); }


/* next slot to fetch in a round robin fashion. Each i-cache bank has its current "next_slot". See sim_init_declarations */
int next_slot[MAX_SLOTS]; 

void BUSCA(void)
{ int module, /* number of i-cache module */
  sn, /* number of slot */
  ruu_ifq_min, /* less number of instrucoes inside of an ifq */
  slot_scheduled,
  not_done;   

  /* fetches from each i-cache module simultaneosly */
  for (module=0;module<il1_modules_num;module++)
  {
     /* First trial. It schedules a slot to fetch instructions. 
     The round robin fashion is used. It begines after the last slot fetched.*/

/* fprintf(stderr,"\nBUSCA : Module = %d previo slot = %d\n",module,next_slot[module]); */

     next_slot[module]++; 
     next_slot[module] = (next_slot[module] % (IL1_MODULE_WIDTH)) + FIRST_SLOT_OF_MOD(module);

     /* in this point, next_slot[module] contains the number of 
        first trial for the slot to fetch */
  
     ruu_ifq_min = ruu_ifq_size; /* receives a maximal value */
     slot_scheduled = FALSE;

     /* Checks if it is posible. Verifies all slots and just the first that is
        not blocked and its ifq queue has more free entries is scheduled */

     not_done = IL1_MODULE_WIDTH; 
     sn = next_slot[module]; /* start point to verify */

     while (not_done)
     {  if (!ruu_slot_delay[sn])          /* slot is not blocked */
        {  if (fetch_num[sn]<ruu_ifq_min) /* get that with more free entries */
           {  slot_scheduled = TRUE; 
              next_slot[module] = sn;
              ruu_ifq_min = fetch_num[sn];
           }
        }
        else { /* decreases one cycle. All walk here */      
               ruu_slot_delay[sn]--;
               if (ruu_slot_delay[sn]<0)
                  panic(sn,"slot com delay negativo");
              }
        
        not_done--;
        sn++;
        sn = (sn % (IL1_MODULE_WIDTH)) + (FIRST_SLOT_OF_MOD(module));
     }

     /* in this point, if there is slot scheduled it is "next_slot" */ 

     /* to restore the error integrity */
     errno = aux_errno[next_slot[module]];
 
     sn = next_slot[module];

     if ( (max_cycles) &&
          ( (sim_cycle < 100) ||
          ( (max_cycles - sim_cycle) < 100) ) )           
     {  int i;
        fprintf(stderr,"\n Module %d.............\n",module);
        for (i=0;i<IL1_MODULE_WIDTH;i++)
             fprintf(stderr," %d (%d)", fetch_num[i + FIRST_SLOT_OF_MOD(module)], 
                     ruu_slot_delay[i + FIRST_SLOT_OF_MOD(module)]);
         
        fprintf(stderr,"\n\n");
     }


     if (slot_scheduled)
     {  int il1bank, il2bank, start, finish, i;   /* number of "bank" inside of "module" */ 

        il1bank = WHICHIS_IL1BANK_OF_SLOT(sn);

        ruu_fetch(sn, il1bank);

        /* Broadcast bus_free for all il1_banks those are connected in the same */
        /* l2 bank in order control the utilization of "il1-il2" bus by all il1 */
        /* banks. It is considered that only one "il1-il2" bus is shared among */
        /* different il1-banks. */
        /* Besides, there is another "dl1-dl2" bus for data, but, and, if */
        /* "il2=dl2", it is supposed there are 2 ports (2 bus) in this */
        /* unified level 2 cache. Also, if "il2!=dl2", it is considered 2 */
        /* ports (2 bus) in the memory. Otherwise, to simulates just 1 port in the */
        /* l2 cache or memory, it is necessary to set l1_bus_opt = 1 */

        il2bank = WHICHIS_IL2BANK_OF_IL1(il1bank);
        start = FIRST_IL1BANK_OF_IL2(il2bank);
        finish = start + IL2_BANK_WIDTH;

        for (i=start; i<finish; i++)
            cache_il1[i]->bus_free = cache_il1[il1bank]->bus_free; 
        
        /* handle l1_bus_free */

        if (l1_bus_opt)
        { start = FIRST_DL1BANK_OF_DL2(il2bank); /* remember: il2 & dl2 are unified */
          finish = start + DL2_BANK_WIDTH; 
          for (i=start; i<finish; i++)
              cache_dl1[i]->bus_free = cache_il1[il1bank]->bus_free;
        }

        if ( (sim_cycle > 1000) &&
             (BOUND_POS(cache_il1[il1bank]->bus_free - sim_cycle) > 
              max_bank_delay))
              
        {   max_bank_delay = cache_il1[il1bank]->bus_free-sim_cycle;
            max_delayed_bank = il1bank;
            max_delayed_bank_cycle = sim_cycle;
        } 

     }
   
     /* to store the error integrity */
     aux_errno[next_slot[module]] = errno;

   }


}


/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{    int sn;  /* slot's number */

     int exit_code;


#ifdef HOST_HAS_QUAD
     /* quad_T temp_quad; */			/* " ditto " */
#endif /* HOST_HAS_QUAD */
      
fprintf(stderr, "\nStarting sim_main ....\n");

fprintf(stderr,"\nil1_banks_num = %d dl1_banks_num = %d l2_banks_num = %d\n",
       il1_banks_num, dl1_banks_num, l2_banks_num);

fprintf(stderr,"\nIL1WIDTH = %d IL2WIDTH = %d  DL1WIDTH = %d DL2WIDTH = %d\n",
       IL1_BANK_WIDTH,  IL2_BANK_WIDTH, DL1_BANK_WIDTH,  DL2_BANK_WIDTH);

  /* ignore any floating point exceptions, they may occur on mis-speculated
     execution paths */

    signal(SIGFPE, SIG_IGN); 

  /* Set up program entry state. Do it for all slots  */
  for (sn=0;sn<process_num;sn++)
  {  regs[sn]->regs_PC = ld_prog_entry[sn];
     regs[sn]->regs_NPC = regs[sn]->regs_PC + sizeof(md_inst_t);
  } 

  /* Check for DLite debugger entry condition. Do it for all slots */

  for (sn=0;sn<process_num;sn++)
   if (dlite_check_break(regs[sn]->regs_PC, /*no access*/ 0, /*addr*/ 0, 0, 0))
     dlite_main(regs[sn]->regs_PC, regs[sn]->regs_PC + sizeof(md_inst_t),
	         sim_cycle, regs[sn], mem[sn], sn); 
   
  /* fast forward simulator loop, performs functional simulation for
     FASTFWD_COUNT insts, then turns on performance (timing) simulation */
 
   if (fastfwd_count > 0)
      DESCARTA();  
  
    /* starting performance simulation for all slots */

    for (sn=0;sn<process_num;sn++)
    {  fprintf(stderr, "SS_SMT: %d: ** TEST DONE: starting performance simulation **\n",sn);

       /* set up timing simulation entry state */
       fetch_regs_PC[sn] = regs[sn]->regs_PC - sizeof(md_inst_t);
       fetch_pred_PC[sn] = regs[sn]->regs_PC;
       regs[sn]->regs_PC = regs[sn]->regs_PC - sizeof(md_inst_t);
    }

    /* set up a non-local exit point. When one instruction "exit" is executed */
    /* for an application, the computation return to here. From here on it is */
    /* posible those remainder slots to dispatch in this current cycle. Also, */
    /* it is posible to do the fetch (it is a parallel action) the last update */
    /* for the statistics. Note that instruction exit is reached in the routine */
    /* "ruu_dispatch" when the variable finished became TRUE too. */

    if ((exit_code = setjmp(sim_exit_buf)) != 0)
    {  /* special handling as longjmp cannot pass 0 
       exit_now(exit_code-1);   */
    } 

   
    /* main simulator loop, NOTE: the pipe stages are traverse in reverse order
       to eliminate this/next state synchronization and relaxation problems */

    if (!finished)
    for (;;)  /* Loop Forever that simulates the machine cycles */ 
    {
       /* Each cycle, here is done some integrity checks for all slots */

       for (sn=0;sn<process_num;sn++) 
       {                              
          /* to restore the error integrity */
          errno = aux_errno[sn]; 

          /* RUU/LSQ sanity checks */          
          if (RUU_num[sn] < LSQ_num[sn])
	     panic(sn,"RUU_num < LSQ_num");
          if (((RUU_head[sn] + RUU_num[sn]) % RUU_size) != RUU_tail[sn])
	     panic(sn,"RUU_head/RUU_tail wedged");
          if (((LSQ_head[sn] + LSQ_num[sn]) % LSQ_size) != LSQ_tail[sn])
	     panic(sn,"LSQ_head/LSQ_tail wedged");

          /* check if pipetracing is still active */
          ptrace_check_active(regs[sn]->regs_PC, sim_num_insn[sn], sim_cycle);

          /* indicate new cycle in pipetrace */
          ptrace_newcycle(sim_cycle);

          /* to store the error integrity */
          aux_errno[sn] = errno;
       } /* for */

       /* try commit entries from all RUU/LSQ to respective architected register file */


       CONCLUSAO();

 /* fprintf(stderr, "\n saiu conclusao ...\n"); */

       TERMINO();

/* fprintf(stderr, "\n saiu termino ...\n"); */
      
       if (!bugcompat_mode)
         REMESSA();

/* fprintf(stderr, "\n saiu remessa ...\n"); */
     
       /* decode and dispatch new operations */
       /* ==> insert ops w/ no deps or all regs ready --> reg deps resolved */
      
       DECOD_DESPACHA();

/* fprintf(stderr, "\n saiu decod_despa ...\n"); */


       if (bugcompat_mode)
          REMESSA();


       BUSCA();

/* fprintf(stderr, "\n saiu busca ...\n"); */


       /* update buffer occupancy stats for all slots */
       for (sn=0;sn<process_num;sn++) 
       {
          IFQ_count[sn] += fetch_num[sn];
          IFQ_fcount[sn] += ((fetch_num[sn] == ruu_ifq_size) ? 1 : 0);
          RUU_count[sn] += RUU_num[sn];
          RUU_fcount[sn] += ((RUU_num[sn] == RUU_size) ? 1 : 0);
          LSQ_count[sn] += LSQ_num[sn];
          LSQ_fcount[sn] += ((LSQ_num[sn] == LSQ_size) ? 1 : 0);
       }

       /* finish early? If all slots had finished, then the simulation finishes, but after all remainder slots had executed this same cycle */


      for (sn=0;sn<process_num;sn++)
        if ((max_insts && sim_total_insn[sn] >= max_insts))
	    {  finished = TRUE;
               break;
             } 

      for (sn=0;sn<process_num;sn++) 
        prev_sim_total_insn[sn] = sim_total_insn[sn];

       /* to keep the error integrity 
          aux_errno[sn] = errno; */

       sim_cycle++; /* computes this cycle */

       /* finish early? If there is one slot at least that had finished, then the simulation finishes, but after all remainder slots had executed this same cycle */

       if (max_cycles && sim_cycle >= max_cycles)
          finished = TRUE;  
         
       /* just after all slot have completed the same cycle, the simulation can be file_analysed */

       if (finished) 
          return;

/* if (sim_cycle > 100000)
    fprintf(stderr, "\n cycle = %d ...\n", (int)sim_cycle); */

    
    } /* end of "for (;;)" */

    if (finished) 
        return;

} /* end of sim_main */

/* To initialize all global variables from all banks, because after the replication they din't can be initialized on their declaration.*/

void
sim_init_declarations(void)
{
  int sn; /* slot's number */
 
 
  /* Initialize many variables */

  ptrace_nelt = 0;

  bimod_nelt = 1;
  bimod_config[0] = /*bimod tbl size*/ 2048;

  twolev_nelt = 4;
  twolev_config[0] = /*l1size*/ 1;
  twolev_config[1] = /*l2size*/ 1024;
  twolev_config[2] = /*hist*/ 8;
  twolev_config[3] = /*xor*/ FALSE;

  comb_nelt = 1;
  comb_config[0] =  /*meta_table_size*/ 1024 ;

  ras_size = 8;

  btb_nelt = 2;
  btb_config[0] = /*nsets*/ 512;
  btb_config[1] = /*assoc*/ 4;

  ruu_include_spec = TRUE;

  mem_nelt = 2;
  mem_lat[0] = /*lat first chunk*/ 18;
  mem_lat[1] = /*between remaining chunks*/ 2;

  pcstat_nelt = 0;
 

  for (sn=0; sn<process_num;sn++)
  {	
        /* from ss_smt.c */

        ruu_slot_delay[sn] = 0;
        next_slot[sn] = -1;

        aux_errno[sn] = 0;
        regs[sn] = NULL ;
        mem[sn] = NULL ;
        	
        sim_total_insn[sn]=0;
        prev_sim_total_insn[sn]=0;
        sim_total_nops_insn[sn]=0;
 	sim_num_refs[sn]=0;
 	sim_total_refs[sn]=0;
	sim_num_loads[sn]=0;
 	sim_total_loads[sn]=0;
 	sim_num_branches[sn]=0;
 	sim_total_branches[sn]=0;
        inst_seq[sn] = 0;
	ptrace_seq[sn] = 0;
	spec_mode[sn] = FALSE;
        sim_num_good_pred[sn] = 0;
	
        bucket_free_list[sn] = NULL; 

        /* Initialize with "RSLINK_NULL_DATA" */

        last_op[sn].next = NULL;
        last_op[sn].rs = NULL;
        last_op[sn].tag = 0;

      
        last_inst_missed[sn] = FALSE;
        last_inst_tmissed[sn] = FALSE;


        /* from ptrace.c */
        ptrace_outfd[sn] = NULL;
        ptrace_active[sn] = FALSE;
        ptrace_oneshot[sn] = FALSE;

        /* from main.c */
        sim_num_insn[sn]=0;
            
  } /* for */    

  /* from loader.c */
  loader_init_declarations();

  /* from eval.c */
  eval_init_declarations();

  /* from eio.c */
  eio_init_declarations();

  /* from symbol.c */
  symbol_init_declarations();

  /* from libexo.c */
  libexo_init_declarations();
  
}

/* initialize the simulator */
void
sim_init(void)
{ int sn; /* slot's number */
  char mem_name[128];
   
  /* allocate and initialize all register file and memory space and more */
  for (sn=0;sn<process_num;sn++) 
  { 
     regs[sn] = regs_create(sn) ; 
     regs_init(regs[sn]);
     sprintf(mem_name, "mem_%s", itoa[sn]);     
     mem[sn] = mem_create(mem_name,sn);
     mem_init(mem[sn]); 

     sim_num_refs[sn] = 0;

  }

 

}
