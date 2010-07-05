/*****************************************************************************
 *
 * $Id: cfg.c,v 1.8 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: cfg.c,v $
 * Revision 1.8  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.7  2004/04/22 08:55:41  lixianfe
 * Version free of flush (except interprocedural flushes)
 *
 * Revision 1.6  2004/03/02 02:58:14  lixianfe
 * results of sim/est doesn't match, this is due to the procedure call and sim
 * dosen't get the right path
 *
 * Revision 1.5  2004/02/26 08:00:46  lixianfe
 * sim/est results are generated
 *
 * Revision 1.4  2004/02/24 12:58:31  lixianfe
 * Initial ILP formulation is done
 *
 * Revision 1.3  2004/02/22 08:24:43  lixianfe
 * execution paths & their times are obtained
 *
 * Revision 1.2  2004/02/17 04:02:17  lixianfe
 * implemented function of finding flush points
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#include "cfg.h"
#include "prog.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

SS_INST_TYPE	INST_NOP;

static int
is_ctrl(SS_INST_TYPE *inst);
static SS_ADDR_TYPE
callee_addr(SS_INST_TYPE *inst, SS_ADDR_TYPE pc);
static void
build_call_edges(Prog *prog);
static int
cmp_proc_sa(const void *key, const void *datum);
static void
reset_procs_flags(Prog *prog);
void
dump_procs(Proc *procs, int nproc);
void
dump_bbs(Proc *proc);




static int
scan_procs(Prog *prog, SS_ADDR_TYPE *psa);

static void
create_procs_basic(Prog *prog, SS_ADDR_TYPE *psa, int nproc);

static void
build_call_edges(Prog *prog);

/* create procedures for a program */
void create_procs(Prog *prog)
{
    SS_ADDR_TYPE    psa[1024];
    int		    nproc;

    nproc = scan_procs(prog, psa);
    create_procs_basic(prog, psa, nproc);
    build_call_edges(prog);   
}


/* identify procedures' start addresses and store them in psa
 * XXX: if a callee addr is out of range, replace this call instr with a nop
 */
static int
scan_procs(Prog *prog, SS_ADDR_TYPE *psa)
{
    SS_INST_TYPE    *inst;
    int		    nproc = 0;
    SS_ADDR_TYPE    pc, ea = prog->sa + prog->sz, x, *y;
    
#if SHOW_PROGRESS
    fprintf(stderr, "scan_procs()...\n");
#endif

    inst = prog->code;
    for (pc = prog->sa; pc < ea; pc += SS_INST_SIZE, inst++) {
	if (!IS_CALL(inst))
	    continue;

	x = callee_addr(inst, pc);
	if (x < prog->sa || x > (prog->sa + prog->sz)) {
	    /* callee out of range, change this instr into nop */
	    INST_NOP.a = 0; INST_NOP.b = 0;
	    *inst = SS_NOP_INST;
	    continue;
	}
	y = (SS_ADDR_TYPE *) my_bsearch(&x, psa, nproc, sizeof(SS_ADDR_TYPE),
		NULL);
	if (x != *y)
	    /* callee address not found, insert it in front of pca */
	    my_insert(&x, psa, y, &nproc, sizeof(SS_ADDR_TYPE));
    }

#if SHOW_PROGRESS
    fprintf(stderr, "done\n\n");
#endif

    return nproc;
}


/* create procedures' basic information (excluding connections between procedures), 
 * according to results from scan_procs()
 */
static void
create_procs_basic(Prog *prog, SS_ADDR_TYPE *psa, int nproc)
{
    SS_ADDR_TYPE    *x;
    Proc	    *proc;
    int		    i;

#if SHOW_PROGRESS
    fprintf(stderr, "create_procs()...\n");
#endif

    /* insert main's start address into approporate place */
    x = (SS_ADDR_TYPE *)my_bsearch(&(prog->main_sa), psa, nproc, sizeof(SS_ADDR_TYPE), NULL);
    my_insert(&(prog->main_sa), psa, x, &nproc, sizeof(SS_ADDR_TYPE));

    prog->nproc = nproc;
    /* create proc array */
    psa[nproc] = prog->sa + prog->sz;	/* for convenience of following part */
    for (i = 0; i < nproc; i++) {
	proc = &(prog->procs[i]);
	proc->id = i;
	proc->sa = psa[i];
	proc->sz = psa[i+1] - psa[i];
	proc->code = lookup_inst(prog->code, prog->sa, psa[i]);
	proc->ncall = 0;
	proc->flags = 0;
    }
    prog->root = prog->procs + (x - psa);
    
#if SHOW_PROGRESS
    fprintf(stderr, "done\n\n");
#endif
}


/* build call edges among processes */
static void
build_call_edges(Prog *prog)
{
    SS_INST_TYPE    *inst;
    SS_ADDR_TYPE    pc, ea, x;
    Proc	    *proc, *callee;
    int		    i;

#if SHOW_PROGRESS
    fprintf(stderr, "build_call_edges()...\n");
#endif

    for (i = 0; i < prog->nproc; i++) {
	proc = &(prog->procs[i]);
	inst = proc->code;
	ea = proc->sa + proc->sz;
	for (pc = proc->sa; pc < ea; pc += SS_INST_SIZE) {
	    if (IS_CALL(inst)) {
		x = callee_addr(inst, pc);
		callee = (Proc *) my_bsearch(&x, prog->procs, prog->nproc,
			sizeof(Proc), cmp_proc_sa);
		assert(x == callee->sa);
		proc->calls[proc->ncall].caller = inst;
		proc->calls[proc->ncall].callee = callee;
		proc->ncall++;
	    }
	    inst++;
	}
    }

#if SHOW_PROGRESS
    fprintf(stderr, "done\n\n");
#endif
}


/* compare addr to proc's start address, return <0 if less; =0 if equal; >0 if great
 * the parameters should be: cmp_proc_sa(Proc *proc, SS_ADDR_TYPE addr)
 */
static int
cmp_proc_sa(const void *key, const void *datum)
{
    const Proc	*proc = (const Proc *) datum;
    const SS_ADDR_TYPE	addr = *((const SS_ADDR_TYPE *) key);

    return (addr - proc->sa);
}




static int
scan_bbs(Proc *proc, SS_ADDR_TYPE *bsa);

static void
create_bbs_basic(Proc *proc, SS_ADDR_TYPE *bsa, int nbb);

static void
build_cfg_edges(Proc *proc);

/* create basic blocks for a procedure */
void
create_bbs(Proc *proc)
{
    SS_ADDR_TYPE    bsa[4096];
    int		    nbb;

    nbb = scan_bbs(proc, bsa);
    create_bbs_basic(proc, bsa, nbb);
    build_cfg_edges(proc);
    //dump_cfg(proc);
}


/* find block boundaries inside a procedure, split a very big block into smaller
 * blocks, which are connected by fall-through edges
 * return (a) the blocks start addresses; (b) number of blocks */
static int
scan_bbs(Proc *proc, SS_ADDR_TYPE *bsa)
{
    SS_INST_TYPE    *inst;
    int		    nbb = 0, bb_size;
    SS_ADDR_TYPE    pc, ea, x, *y;
    
#if SHOW_PROGRESS
    fprintf(stderr, "scan_bbs()...\n");
#endif
    
    /* XXX: last instr is always a return, don't process it */
    ea =  proc->sa + proc->sz - SS_INST_SIZE;	
    inst = proc->code;
    my_insert(&(proc->sa), bsa, bsa, &nbb, sizeof(SS_ADDR_TYPE));
    bb_size = 0;
    for (pc = proc->sa; pc < ea; pc += SS_INST_SIZE, inst++) {
	if ((!is_ctrl(inst)) && (++bb_size < MAX_BB_SIZE))
	    continue;

	/* a control transfer instruction, first there is a boundary between current
	 * and next instr; second, if it is a branch, the target defines another
	 * boundary (if it is call, just connect current block and next block; if it
	 * is return, no connection is made from this block. When traverse the CFG,
	 * determine where to go by the combination of: (a) block type
	 * (call/return/others); (b) target block if block type is others
	 */

	/* first boundary */
	x = pc + SS_INST_SIZE;
	y = (SS_ADDR_TYPE *) my_bsearch(&x, bsa, nbb, sizeof(SS_ADDR_TYPE), NULL);
	if (x != *y)
	    /* new block boundary, insert it */
	    my_insert(&x, bsa, y, &nbb, sizeof(SS_ADDR_TYPE));
	
	if (!is_ctrl(inst)) {
	    //fprintf(stderr, "%x\n", pc+SS_INST_SIZE);
	    bb_size = 0;
	    continue;
	}
	if (IS_CALL(inst) || IS_RETURN(inst))
	    continue;

	/*second boundary (for only branches */
	x = btarget_addr(inst, pc);
	y = (SS_ADDR_TYPE *) my_bsearch(&x, bsa, nbb, sizeof(SS_ADDR_TYPE), NULL);
	if (x != *y)
	    /* new block boundary, insert it */
	    my_insert(&x, bsa, y, &nbb, sizeof(SS_ADDR_TYPE));
    }

#if SHOW_PROGRESS
    fprintf(stderr, "done\n\n");
#endif
    return nbb;
}


/* create basic blocks according to results from scan_bbs() */
void
create_bbs_basic(Proc *proc, SS_ADDR_TYPE *bsa, int nbb)
{
    int		i;
    BasicBlk	*bb;

    /* allocate memory for blocks */
    proc->bbs = (BasicBlk *) calloc(nbb, sizeof(BasicBlk));
    if (proc->bbs == NULL) {
	fprintf(stderr, "out of memory (%s:%d)\n", __FILE__, __LINE__);
	exit(1);
    }
    proc->nbb = nbb;

    /* fill blocks with information execept edges */
    bsa[nbb] = proc->sa + proc->sz;	/* for convenience of calculation */
    for (i = 0; i < nbb; i++) {
	bb = &(proc->bbs[i]);
	bb->id = i;
	bb->sa = bsa[i];
	bb->sz = bsa[i+1] - bsa[i];
	bb->code = lookup_inst(proc->code, proc->sa, bsa[i]);
	/* block type */
	bb->type = inst_type(BB_LAST_INST(bb));
	bb->proc = proc;
    }
}



// remove out edges of a block, bcoz this block doesn't have in edges
static void
remove_out_edges(BasicBlk *bb)
{
    BasicBlk	*target;
    int		i;

    if (bb->n != NULL) {
	target = bb->n->b2;
	if (target->num_in == 1) {
	    target->num_in = 0;
	    remove_out_edges(target);
	} else {
	    for (i = 0; i < target->num_in; i++) {
		if (target->in[i] == bb->n)
		    break;
	    }
	    if (i < target->num_in - 1) {
		memmove(&target->in[i], &target->in[i+1],
			(target->num_in - i - 1) * sizeof(CfgEdge *));
	    }
	    target->num_in--;
	}
	free(bb->n);
	bb->n = NULL;
    }

    if (bb->t != NULL) {
	target = bb->t->b2;
	if (target->num_in == 1) {
	    target->num_in = 0;
	    remove_out_edges(target);
	} else {
	    for (i = 0; i < target->num_in; i++) {
		if (target->in[i] == bb->t)
		    break;
	    }
	    if (i < target->num_in - 1) {
		memmove(&target->in[i], &target->in[i+1],
			(target->num_in - i - 1) * sizeof(CfgEdge *));
	    }
	    target->num_in--;
	}
	free(bb->t);
	bb->t = NULL;
    }

}



static void
create_cfg_edge(BasicBlk *b1, BasicBlk *b2, CfgEdge **out);

BasicBlk *
lookup_bb(Proc *proc, SS_ADDR_TYPE addr);
    
static void
build_cfg_edges(Proc *proc)
{
    SS_INST_TYPE    *inst;
    SS_ADDR_TYPE    pc;
    BasicBlk	    *bb, *target;
    int		    i;

#if SHOW_PROGRESS
    fprintf(stderr, "build_cfg_edges()...\n");
#endif

    for (i = 0; i < proc->nbb - 1; i++) {
	bb = &proc->bbs[i];
	if (bb->type == CTRL_SEQ || bb->type == CTRL_COND || bb->type == CTRL_CALL) {
	    /* create fall-through edge */
	    target = &(proc->bbs[i+1]);
	    create_cfg_edge(bb, target, &(bb->n));
	}
	if (bb->type == CTRL_COND || bb->type == CTRL_UNCOND) {
	    /* create branch edge */
	    inst = BB_LAST_INST(bb);
	    pc = BB_LAST_ADDR(bb);
	    pc = btarget_addr(inst, pc);
	    target = lookup_bb(proc, pc);
	    assert(target != NULL);
	    create_cfg_edge(bb, target, &(bb->t));
	}
    }

    // XXX: benchmark minver has a block 2.55, which no blocks reach;
    // current solution: try finding such blocks and remove their out edges
    for (i = 1; i < proc->nbb - 1; i++) {
	bb = &proc->bbs[i];
	if (bb->num_in == 0)
	    remove_out_edges(bb);
    }

#if SHOW_PROGRESS
    fprintf(stderr, "done\n\n");
#endif

}


static void 
create_cfg_edge(BasicBlk *b1, BasicBlk *b2, CfgEdge **out)
{
    CfgEdge	*edge;

    if ((edge = (CfgEdge *) malloc(sizeof(CfgEdge))) == NULL) {
	fprintf(stderr, "out of memory (%s:%d)\n", __FILE__, __LINE__);
	exit(1);
    }
    edge->b1 = b1; edge->b2 = b2;
    *out = edge;		    /* source out */
    b2->in[b2->num_in++] = edge;    /* target in */
}


BasicBlk *
lookup_bb(Proc *proc, SS_ADDR_TYPE addr)
{
    int		i;
    BasicBlk	*bb;

    for (i = 0; i < proc->nbb; i++) {
	bb = &(proc->bbs[i]);
	if ((addr >= bb->sa) && (addr <= BB_LAST_ADDR(bb)))
	    return bb;
    }
    return NULL;
}


/* reset flags of procedures in the program */
static void
reset_procs_flags(Prog *prog)
{
    int	    i;

    for (i = 0; i < prog->nproc; i++)
	prog->procs[i].flags = 0;
}


SS_INST_TYPE *
lookup_inst(SS_INST_TYPE *code, SS_ADDR_TYPE sa, SS_ADDR_TYPE pc)
{
    return (code + (pc - sa) / SS_INST_SIZE);
}


SS_ADDR_TYPE 
inst_addr(SS_INST_TYPE *code, SS_ADDR_TYPE sa, SS_INST_TYPE *inst)
{
    return (sa + (inst - code) * SS_INST_SIZE);
}



int
bb_inst_num(BasicBlk *bb)
{
    return INST_NUM(bb->sz);
}



int
isolated_bb(BasicBlk *bb)
{
    if ((bb->num_in == 0) && (bb->id != 0)) {
	assert((bb->n == NULL) && (bb->t == NULL));
	return 1;
    } else
	return 0;
}



/*=============================================================================
 * functions extracting info from SimpleScalar instructions
 */

int
inst_type(SS_INST_TYPE *inst)
{
    int	    flags = SS_OP_FLAGS(SS_OPCODE(*inst));

    if (flags & F_CTRL) {
	if (flags & F_COND)
	    return CTRL_COND;
	if (flags & F_CALL)
	    return CTRL_CALL;
	if (flags & F_INDIRJMP)
	    return CTRL_RET;
	return CTRL_UNCOND;
    } else {
	return CTRL_SEQ;
    }
}



/* returns block's type according to the last instr */
int
bb_type(BasicBlk *bb)
{
    SS_INST_TYPE    *inst;

    inst = lookup_inst(bb->code, bb->sa, bb->sa + bb->sz - SS_INST_SIZE);
    return inst_type(inst);
}


static int
is_ctrl(SS_INST_TYPE *inst)
{
    int	    flags = SS_OP_FLAGS(SS_OPCODE(*inst));
    return (flags & F_CTRL) ? 1 : 0;
}
 

/*
static int
is_call(SS_INST_TYPE *inst)
{
    unsigned	flags = SS_OP_FLAGS(SS_OPCODE(*inst));
    return (flags & F_CALL) ? 1 : 0;
}
*/


/* return the first instruction's address of the callee */
static SS_ADDR_TYPE
callee_addr(SS_INST_TYPE *inst, SS_ADDR_TYPE pc)
{
    int	    offset;

    offset = (inst->b & 0x3ffffff) << 2;
    return ((pc & 0xf0000000) | offset);
}


/* return the branch target address, return 0 if not branch (conditional or
 * unconditional */
SS_ADDR_TYPE
btarget_addr(SS_INST_TYPE *inst, SS_ADDR_TYPE pc)
{
    int	    offset;

    if (IS_COND(inst)) {
	offset = ((int)((short)((inst)->b & 0xffff))) << 2;
	return (pc + SS_INST_SIZE + offset);
    } else if (IS_UNCOND(inst)) {
	offset = (inst->b & 0x3ffffff) << 2;
	return ((pc & 0xf0000000) | offset);
    } else
	return 0;
}



/* collect start addresses of procs and store them in an allocated memory */
SS_ADDR_TYPE *
collect_procs_addr(Prog *prog)
{
    SS_ADDR_TYPE    *addrs;
    int		    i;

    addrs = (SS_ADDR_TYPE *) malloc(prog->nproc * sizeof(SS_ADDR_TYPE));
    if (addrs == NULL) {
	fprintf(stderr, "out of memory (%s:%d)\n", __FILE__, __LINE__);
	exit(1);
    }

    for (i=0; i<prog->nproc; i++)
	addrs[i] = prog->procs[i].sa;

    return addrs;
}



/* collect start addresses of blocks in a procedure */
SS_ADDR_TYPE *
collect_blks_addr(Proc *proc)
{
    SS_ADDR_TYPE    *addrs;
    int		    i;

    addrs = (SS_ADDR_TYPE *) malloc(proc->nbb * sizeof(SS_ADDR_TYPE));
    if (addrs == NULL) {
	fprintf(stderr, "out of memory (%s:%d)\n", __FILE__, __LINE__);
	exit(1);
    }

    for (i=0; i<proc->nbb; i++)
	addrs[i] = proc->bbs[i].sa;

    return addrs;
}



/* if a block calls a proc at end, return the called procedure */
Proc *
get_callee(Proc *proc, BasicBlk *bb)
{
    int	cmp_caller();

    SS_INST_TYPE    *caller;
    Call	    *call;

    if (bb_type(bb) != CTRL_CALL)
	return NULL;
    
    caller = lookup_inst(bb->code, bb->sa, bb->sa + bb->sz - SS_INST_SIZE);
    call = (Call *) my_bsearch(caller, proc->calls, proc->ncall, sizeof(Call),
	    cmp_caller);

    return call->callee;
}



/* compare instruction to caller */
int
cmp_caller(const void *k, const void *datum)
{
    SS_INST_TYPE    *inst = (SS_INST_TYPE *) k;
    Call	    *call = (Call *) datum;

    return (inst - call->caller);
}



/*=============================================================================
 * dump functions
 */

/* dump addresses of vector va */
void
dump_vaddr(SS_ADDR_TYPE *va, int n)
{
    int	    i;

    for (i=0; i<n; i++) {
	fprintf(stderr, "%2d: %x\n", i, va[i]);
    }
}


void
dump_procs(Proc *procs, int nproc)
{
    int	    i;

    for (i=0; i<nproc; i++) {
	fprintf(stderr, "id=%d; sa=%x; sz=%x, inst=%x\n",
		procs[i].id, procs[i].sa, procs[i].sz, (unsigned int)(intptr_t)procs[i].code);
    }
}


void
dump_call_graph(Prog *prog)
{
    int	    i, j;
    Proc    *p;

    for (i=0; i<prog->nproc; i++) {
	p = &(prog->procs[i]);
	for (j=0; j<p->ncall; j++)
	    fprintf(stderr, "%x => %d(%x)\n",
		    inst_addr(prog->code, prog->sa, p->calls[j].caller),
		    p->calls[j].callee->id, p->calls[j].callee->sa);
	printf("\n");
    }
}


void
dump_bbs(Proc *proc)
{
    BasicBlk	*bb;
    int		i;

    fprintf(stderr, "proc[%d]'s basic blocks:\n", proc->id);
    for (i = 0; i < proc->nbb; i++) {
	bb = &(proc->bbs[i]);
	fprintf(stderr, "id=%d, sa=%x, sz=%x, code=%x, type=%d\n",
		bb->id, bb->sa, bb->sz, (unsigned int)(intptr_t)bb->code, bb->type);
    }
}


void
dump_cfg(FILE *fptr, Proc *proc)
{
    BasicBlk	*bb;
    Proc	*callee;
    int		i;

    fprintf(stderr, "\nproc[%d] cfg:\n", proc->id);
    for (i = 0; i < proc->nbb; i++) {
	bb = &(proc->bbs[i]);
	fprintf(fptr, "%2d %3d %08x", proc->id, bb->id, bb->sa);
	fprintf(stderr, "%2d : %08x : [", bb->id, bb->sa);
	if (bb->n != NULL) {
	    fprintf(fptr, " %3d", bb->n->b2->id);
	    fprintf(stderr, "%2d", bb->n->b2->id);
	}
	else {
	    fprintf(fptr, "  -1");
	    fprintf(stderr, "  ");
	}
	fprintf(stderr, ", ");
	if (bb->t != NULL) {
	    fprintf(fptr, " %3d", bb->t->b2->id);
	    fprintf(stderr, "%2d", bb->t->b2->id);
	}
	else {
	    fprintf(fptr, "  -1");
	    fprintf(stderr, "  ");
	}
	fprintf(stderr, "]");

	if (bb_type(bb) == CTRL_CALL) {
	    callee = get_callee(proc, bb);
	    fprintf(fptr, " %2d", callee->id);
	    fprintf(stderr, " P%d", callee->id);
	}
	else
	    fprintf(fptr, " -1");

	fprintf(fptr, "\n");
	fprintf(stderr, "\n");
    }
}


void
dump_inst(SS_INST_TYPE inst, SS_ADDR_TYPE pc)
{
    enum ss_opcode op;

    /* decode the instruction, assumes predecoded text segment */
    op = SS_OPCODE(inst); /* use: SS_OP_ENUM(SS_OPCODE(inst)); if not predec */

    /* disassemble the instruction */
    if (op >= OP_MAX) {
	/* bogus instruction */
	fprintf(stderr, "<invalid inst: 0x%08x:%08x>\n", inst.a, inst.b);
    } else {
	char *s;

	fprintf(stderr, "%x\t%-10s", pc, SS_OP_NAME(op));

	s = SS_OP_FORMAT(op);
	if (s == NULL)
	    goto done;
	while (*s) {
	    switch (*s) {
	    case 'd':
		fprintf(stderr, "r%d", RD);
		break;
	    case 's':
		fprintf(stderr, "r%d", RS);
		break;
	    case 't':
		fprintf(stderr, "r%d", RT);
		break;
	    case 'b':
		fprintf(stderr, "r%d", BS);
		break;
	    case 'D':
		fprintf(stderr, "f%d", FD);
		break;
	    case 'S':
		fprintf(stderr, "f%d", FS);
		break;
	    case 'T':
		fprintf(stderr, "f%d", FT);
		break;
	    case 'j':
		fprintf(stderr, "0x%x", (pc + 8 + (OFS << 2)));
		break;
	    case 'o':
	    case 'i':
		fprintf(stderr, "%d", IMM);
		break;
	    case 'H':
		fprintf(stderr, "%d", SHAMT);
		break;
	    case 'u':
		fprintf(stderr, "%u", UIMM);
		break;
	    case 'U':
		fprintf(stderr, "0x%x", UIMM);
		break;
	    case 'J':
		fprintf(stderr, "0x%x", ((pc & 036000000000) | (TARG << 2)));
		break;
	    case 'B':
		fprintf(stderr, "0x%x", BCODE);
		break;
	    case ')':
		/* handle pre- or post-inc/dec */
		if (SS_COMP_OP == SS_COMP_NOP)
		    fprintf(stderr, ")");
		else if (SS_COMP_OP == SS_COMP_POST_INC)
		    fprintf(stderr, ")+");
		else if (SS_COMP_OP == SS_COMP_POST_DEC)
		    fprintf(stderr, ")-");
		else if (SS_COMP_OP == SS_COMP_PRE_INC)
		    fprintf(stderr, ")^+");
		else if (SS_COMP_OP == SS_COMP_PRE_DEC)
		    fprintf(stderr, ")^-");
		else if (SS_COMP_OP == SS_COMP_POST_DBL_INC)
		    fprintf(stderr, ")++");
		else if (SS_COMP_OP == SS_COMP_POST_DBL_DEC)
		    fprintf(stderr, ")--");
		else if (SS_COMP_OP == SS_COMP_PRE_DBL_INC)
		    fprintf(stderr, ")^++");
		else if (SS_COMP_OP == SS_COMP_PRE_DBL_DEC)
		    fprintf(stderr, ")^--");
		else {
		    fprintf(stderr, "bogus SS_COMP_OP");
		    exit(1);
		}
		break;
	    default:
		/* anything unrecognized, e.g., '.' is just passed through */
		if (*s != ',')
		    fputc(*s, stderr);
		else
		    fprintf(stderr, " ");
	    }
	    s++;
	}
done:
	fprintf(stderr, "\n");
    }
}
