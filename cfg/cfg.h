/*****************************************************************************
 *
 * $Id: cfg.h,v 1.5 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: cfg.h,v $
 * Revision 1.5  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.4  2004/04/22 08:55:41  lixianfe
 * Version free of flush (except interprocedural flushes)
 *
 * Revision 1.3  2004/02/19 06:37:50  lixianfe
 * execution paths are created and their simulation/estimation times are obtained
 *
 * Revision 1.2  2004/02/17 04:02:17  lixianfe
 * implemented function of finding flush points
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#ifndef CFG_H
#define CFG_H

#include "ss.h"

#define	MAX_PROCS   256
#define	MAX_BB_IN	   102400
#define	INST_NUM(size)	    ((size) / sizeof(SS_INST_TYPE))

#define MAX_CALLS	    6400
#define MAX_BB_SIZE	    0x7fffffff	// split a block with instr > MAX_BB_SIZE

#define IS_CALL(inst)	    (inst_type((inst)) == CTRL_CALL)
#define IS_COND(inst)	    (inst_type((inst)) == CTRL_COND)
#define IS_UNCOND(inst)	    (inst_type((inst)) == CTRL_UNCOND)
#define IS_RETURN(inst)	    (inst_type((inst)) == CTRL_RET)
#define BB_LAST_INST(bb)    ((bb)->code + (bb)->sz / SS_INST_SIZE - 1)
#define BB_LAST_ADDR(bb)    ((bb)->sa + (bb)->sz - SS_INST_SIZE)
#define PROC_LAST_ADDR(p)   ((p)->sa + (p)->sz - SS_INST_SIZE)

enum {CTRL_SEQ, CTRL_COND, CTRL_UNCOND, CTRL_CALL, CTRL_RET};

 
typedef struct _basicblk    *BasicBlkPtr;
typedef struct _cfgedge	    *CfgEdgePtr;
typedef struct proc_t	    *ProcPtr;

typedef struct _basicblk {
    int		    id;		    // block id
    SS_ADDR_TYPE    sa;		    // start addr
    int		    sz;		    // size (in bytes)
    SS_INST_TYPE    *code;	    // first instruction

    int		    type;	    // branch/call/return ...
    CfgEdgePtr	    n, t;	    // non-taken/taken edges
    int		    num_in;	    // total in edges
    CfgEdgePtr	    in[MAX_BB_IN];  // in edges

    ProcPtr	    proc;	    // up-link (proc where it is in)

    int		    flags;
} BasicBlk;

typedef struct _cfgedge {
    BasicBlkPtr	    b1, b2;	    // b1 -> b2
} CfgEdge;


typedef struct call_t {
    SS_INST_TYPE    *caller;
    ProcPtr	    callee;
} Call;

typedef struct proc_t {
    int		    id;		    // proc id
    SS_ADDR_TYPE    sa;		    // start addr
    int		    sz;		    // size (in bytes)
    SS_INST_TYPE    *code;	    // first instruction

    int		    ncall;	    // number of calls
    Call	    calls[MAX_CALLS];

    int		    nbb;	    // number of basic blocks
    BasicBlk	    *bbs;	    // basic blocks

    int		    flags;
} Proc;

// decoded instr (try to align it 8-bytes)
typedef struct {
    unsigned short  op;
    unsigned char   in[3];
    unsigned char   out[2];
    unsigned char   fu;
} decoded_inst_t;

typedef struct prog_t {
    SS_ADDR_TYPE    sa;	    // program start address
    int		    sz;	    // program text size (in bytes)
    SS_ADDR_TYPE    main_sa;// start address of main (might be in middle of prog
    SS_INST_TYPE    *code;  // program text
    decoded_inst_t  *dcode; // decoded instructions

    Proc	    procs[MAX_PROCS];
    int		    nproc;  // number of procedures
    Proc	    *root;  // root = main
} Prog;


void
create_procs(Prog *prog);

void
create_bbs(Proc *proc);

SS_ADDR_TYPE
btarget_addr(SS_INST_TYPE *inst, SS_ADDR_TYPE pc);

SS_INST_TYPE *
lookup_inst(SS_INST_TYPE *code, SS_ADDR_TYPE sa, SS_ADDR_TYPE pc);

int
inst_type(SS_INST_TYPE *inst);

SS_ADDR_TYPE
inst_addr(SS_INST_TYPE *code, SS_ADDR_TYPE sa, SS_INST_TYPE *inst);

SS_ADDR_TYPE *
collect_blks_addr(Proc *proc);

Proc *
get_callee(Proc *proc, BasicBlk *bb);

BasicBlk *
lookup_bb(Proc *proc, SS_ADDR_TYPE addr);

int
bb_inst_num(BasicBlk *bb);

int
isolated_bb(BasicBlk *bb);

void
dump_cfg(FILE *fptr, Proc *proc);

void
dump_procs(Proc *procs, int nproc);

void
dump_bbs(Proc *proc);

#endif
