/* sim-safe.c - sample functional simulator implementation */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

// Added by LXF.begin

extern int    *inst_cnts;

#define CALL_STACK_SIZE       32
#define TAKEN 1
#define NOT_TAKEN 0
#define rshift(x, b)          (((unsigned) (x)) >> (b))
#define CACHE_LINE(addr)      (rshift(((addr) << CACHE_TAG_BITS), \
	                                     (CACHE_TAG_BITS + CACHE_LINE_BITS)))
#define CACHE_LINE_TAG(addr)  (rshift((addr), (32 - CACHE_TAG_BITS)))
#define IS_CACHE_MISS(addr)   (cache_table[CACHE_LINE(addr)] != \
	                                 CACHE_LINE_TAG(addr))
#define SPC		      (spec_PC)
#define SET_SPC(x)	      (SPC = (x) ? ((CPC + 8 + (OFS << 2))) : \
				 (CPC + sizeof(md_inst_t)))

extern md_addr_t    start_addr, end_addr;

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type;

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
static int twolev_nelt = 4;
static int twolev_config[4] =
  { /* l1size */1, /* l2size */128, /* hist */3, /* xor */TRUE};

enum {perfect, GAg, gshare, local};
static int bpred_scheme;
static int bpred_table_bits = 0;
static int history_bits;
static int BPRED_TABLE_SIZE;
static int P_MASK, H_MASK;
static int BPENALTY = 10;

static int *bpred_miss;
static unsigned char *bpred_table;
static unsigned int history_register = 0;

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il1_opt;

static int enable_icache = 0;
static int CACHE_LINE_SIZE;
static int CACHE_SET_NUM;
static unsigned int CACHE_LINE_BITS;
static unsigned int CACHE_TAG_BITS;
static int CPENALTY = 50;

static int *cache_miss;
static unsigned int *cache_table;
static int cache_blocking_count;
static int *cache_blocking;
static int cache_blocking_delay;


static int code_insts = 0;
static int my_insn_count = 0;

extern int  effect_cycles;
extern int  effect_bpred_count;
extern int  effect_bpred_misses;
extern int  effect_icache_misses;

#define CALL_STACK_SIZE	      32
md_addr_t call_stack[CALL_STACK_SIZE];
static int call_level = 0;

unsigned int int_log2(unsigned int i)
{
    unsigned int b;

    for (b = 0; i; b++)
	i >>= 1;
    return b;
}



unsigned int mask(int n)
{
    unsigned int i = 0;

    if ((n < 0) || (n >= 32))
	return 0;

    while (n > 0) {
	i = (i << 1) + 1; n--;
    }
    return i;
}




void create_my_bpred(void)
{

    int i;

    my_insn_count = 0;
    effect_bpred_misses = 0;
    history_register = 0;
    bpred_miss = (int *) calloc(1024, sizeof(int));
    bpred_table = (unsigned char *)calloc(BPRED_TABLE_SIZE,
	    sizeof(unsigned char));
    /* Initialize to NOT-TAKEN */
    for (i=0; i<BPRED_TABLE_SIZE; i++)
	bpred_table[i] = 0;
    for (i=0; i<1024; i++)
	bpred_miss[i] = 0;
#if 0
    for (i=0; i<MAX_CODE_SIZE; i++)
	inst_cnts[i] = 0;
#endif
}



int lookup_my_bpred(md_addr_t current, md_addr_t target)
{
  unsigned char outcome, prediction;
  unsigned int index;
 

  effect_bpred_count++;
  if (target == current + sizeof(md_inst_t))
    outcome = NOT_TAKEN;
  else
    outcome = TAKEN;

  switch (bpred_scheme) {
     case GAg:
	index = history_register; 
	break;
     case gshare:
	//index = (history_register << 2) ^ (rshift(current, 3));
	// XOR history with the MSBs fo address
	index = (history_register << (bpred_table_bits - history_bits))
	   ^ (rshift(current, 3));
	break;
     case local:
	index = rshift(current, 3);
	break;
  }
  index &= P_MASK;

  prediction = bpred_table[index];
  if (prediction != outcome){
    effect_bpred_misses++;
    bpred_miss[rshift(current - start_addr, 3)]++;
    bpred_table[index] = outcome;
  }
  history_register = ((history_register << 1) | outcome) & H_MASK;

  return (prediction != outcome);
}



void spec_exec(md_inst_t inst, int x)
{
    register md_addr_t spec_PC;
    enum md_opcode op;
    int clocks_left = BPENALTY;
    int s_call_level = call_level;
    int len, i;
    md_addr_t backup_call_stack[BPENALTY];


    /* backup part of the call stack as the spec session may screw up them */
    len = (BPENALTY >= call_level) ? call_level : BPENALTY;
    for (i = 0; i < len; i++)
	backup_call_stack[i] = call_stack[call_level - len + i];

    SET_SPC(x);
    while (clocks_left > 0) {
	if ((spec_PC >= start_addr) && (spec_PC < end_addr)) {
	    /* if cache miss */
	    /* 06.Nov.2002
	     */
	    if (enable_icache && IS_CACHE_MISS(spec_PC)) {
		cache_blocking[rshift(regs.regs_PC - start_addr, 3)]++;
		cache_blocking_count++;
		cache_blocking_delay += (CPENALTY - clocks_left);
		cache_table[CACHE_LINE(spec_PC)] = CACHE_LINE_TAG(spec_PC);
		clocks_left = 0;
	    }
	}
	else {
	    clocks_left = 0;
	}

        MD_FETCH_INST(inst, mem, spec_PC);
	/* decode the instruction */
        MD_SET_OPCODE(op, inst);
	if (MD_OP_FLAGS(op) & F_CTRL) {     /* control instruction */
	    switch (op) {
	    case JAL:
		call_stack[s_call_level] = spec_PC + sizeof(md_inst_t);
		s_call_level++;
	    case JUMP:
		spec_PC = ((SPC & 036000000000) | (TARG << 2));
		clocks_left--;
		break;
	    case JR:
		s_call_level--;
		spec_PC = call_stack[s_call_level];
		clocks_left--;
		break;
	    default:	   /* branch inst, spec session stops here */
		clocks_left = 0;
	    }
	}
	else {	  /* other inst, pretend to execute them */
	    spec_PC += sizeof(md_inst_t);
	    clocks_left--;
	}
    }

    // recover part of the call stack as the spec session may screw up them
    for (i = 0; i < len; i++)
	call_stack[call_level - len + i] = backup_call_stack[i];

}


void init_cache()
{
   effect_icache_misses = 0;
   cache_blocking_count = 0;
   cache_blocking_delay = 0;
   CACHE_LINE_BITS = int_log2(CACHE_LINE_SIZE) - 1;
   CACHE_TAG_BITS = 32 - (int_log2(CACHE_SET_NUM) - 1) - CACHE_LINE_BITS;

   cache_miss = (int *) calloc(code_insts, sizeof(int));
   cache_blocking = (int *) calloc(code_insts, sizeof(int));
   cache_table = (unsigned int *)calloc(CACHE_SET_NUM, sizeof(unsigned int));
   /* Initialize to EMPTY */
}

// Added by LXF.end


/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-safe: This simulator implements a functional simulator.  This\n"
"functional simulator is the simplest, most user-friendly simulator in the\n"
"simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
"for all instruction errors, and the implementation is crafted for clarity\n"
"rather than speed.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  // Added by LXF.begin
  opt_reg_string(odb, "-bpred",
		 "branch predictor type {nottaken|taken|perfect|bimod|2lev|comb}",
                 &pred_type, /* default */"2lev",
                 /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2-level predictor config "
		   "(<l1size> <l2size> <hist_size> <xor>)",
                   twolev_config, twolev_nelt, &twolev_nelt,
		   /* default */twolev_config,
                   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
		 &cache_il1_opt, "il1:32:32:1:l",
		 /* print */TRUE, NULL);
#if 0
printf("bpred_type: %s\n", pred_type);
printf("-bpred:2lev: %d %d %d %d\n", twolev_config[0], twolev_config[1],
	twolev_config[2], twolev_config[3]);
printf("-cache:il1: %s\n", cache_il1_opt);
#endif

  // Added by LXF.end
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
  // Added by LXF.begin
    int	    assoc;
    char    name[128], c;

    if (!mystricmp(pred_type, "perfect")) {
        /* perfect predictor */
        bpred_scheme = perfect;
    } else if (!mystricmp(pred_type, "2lev")) {
	bpred_table_bits = int_log2(twolev_config[1]) - 1;
	history_bits = twolev_config[2];
	if (history_bits == 0) {
	    bpred_scheme = local;
	} else if (twolev_config[3] == TRUE) {
	    bpred_scheme = gshare;
	} else {
	    bpred_scheme = GAg;
	    bpred_table_bits = history_bits;
	}
	BPRED_TABLE_SIZE = twolev_config[1];
    }
    P_MASK = mask(bpred_table_bits);
    H_MASK = mask(history_bits);

    printf("bpred_scheme: %d\n", bpred_scheme);
    printf("bpred_table_bits: %d\n", bpred_table_bits);
    printf("history_bits: %d\n", history_bits);

    if (!mystricmp(cache_il1_opt, "none")) {
        enable_icache = 0;
    } else {
	enable_icache = 1;
        if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &CACHE_SET_NUM, &CACHE_LINE_SIZE, &assoc, &c) != 5) {
	    fprintf(stderr, "wrong cache configuration!\n");
	    exit(1);
        }
    }

    printf("CACHE_SET_NUM: %d\n", CACHE_SET_NUM);
    printf("CACHE_LINE_SIZE: %d\n", CACHE_LINE_SIZE);

  // Added by LXF.end
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

extern void
sym_loadsyms(char *fname,	/* file name containing symbols */
	     int load_locals);	/* load local symbols */

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  // Added by LXF.begin

  sym_loadsyms(ld_prog_fname, TRUE);
  code_insts = (end_addr - start_addr) / sizeof(md_inst_t);
  inst_cnts = (int *) calloc(code_insts, sizeof(int));

  if (bpred_scheme != perfect)
      create_my_bpred();
  if (enable_icache)
      init_cache();
  fprintf(stderr, "enable_icache: %d\n", enable_icache);

  // Added by LXF.end

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	       regs.regs_PC, sim_num_insn, &regs, mem);

  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */
//fprintf(stderr, "regs_PC: %x\n", regs.regs_PC);
      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
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
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("attempted to execute a bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n [xor: 0x%08x] @ 0x%08p: ",
		    sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  if (MD_OP_FLAGS(op) & F_MEM)
	    myfprintf(stderr, "  mem: 0x%08p", addr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      // Added by LXF.begin
      if ((regs.regs_PC >= start_addr) && (regs.regs_PC < end_addr)) {
	 my_insn_count++;
	 inst_cnts[rshift(regs.regs_PC - start_addr, 3)]++;

	 /* process cache access */
	 if (enable_icache && IS_CACHE_MISS(regs.regs_PC)) {
	    cache_table[CACHE_LINE(regs.regs_PC)] = CACHE_LINE_TAG(regs.regs_PC);
	    cache_miss[rshift(regs.regs_PC - start_addr, 3)]++;
	    effect_icache_misses++;
	 }

	 /* process branch prediction */
	 if ((bpred_scheme != perfect) && (MD_OP_FLAGS(op) & F_COND)) {
	    if (lookup_my_bpred(regs.regs_PC, regs.regs_NPC)) /* mispredict */
	       spec_exec(inst, (regs.regs_NPC == (regs.regs_PC + sizeof(md_inst_t))));
	 }

	 /* if is function call, maintain the call stack */
	 if (op == JAL) {
	    call_stack[call_level] = regs.regs_PC + sizeof(md_inst_t);
            if ((regs.regs_NPC >= start_addr) && (regs.regs_NPC < end_addr))
               call_level++;
	 }
	 else {
	    if (op == JR) call_level--;
	 }
      }
      // Added by LXF.end

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);
    
#if 1
      // Added by LXF.begin

      effect_cycles = my_insn_count;
      effect_cycles += effect_bpred_misses * BPENALTY;
      effect_cycles += effect_icache_misses * CPENALTY;

      // Added by LXF.end
#endif

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
