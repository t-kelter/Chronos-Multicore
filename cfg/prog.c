/*****************************************************************************
 *
 * $Id: prog.c,v 1.2 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: prog.c,v $
 * Revision 1.2  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#include "common.h"
#include "prog.h"

#include <stdlib.h>

//extern Prog prog;
#include "globals.h"

/* return the proc where pc falls in */
int
lookup_proc(SS_ADDR_TYPE pc)
{
    int		    lo, hi, i;
    SS_ADDR_TYPE    sa;

    lo = 0; hi = prog.nproc - 1; 

    while (hi > lo) {
	i = (lo + hi ) >> 1;
	sa = prog.procs[i].sa;
	if (pc > sa) {
	    if (pc < (sa + prog.procs[i].sz))
		return i;
	    else
		lo = i + 1;
	} else if (pc < sa)
	    hi = i - 1;
	else
	    return i;
    }

    if (lo > hi)
	return -1;  // pc is out of prog's scope
    return lo;
}



// decode instructions
void
decode_text()
{
    int		    i, num;
    SS_INST_TYPE    inst, *code = prog.code;
    decoded_inst_t  *dcode;

    num = prog.sz / SS_INST_SIZE;
    dcode = (decoded_inst_t *) calloc(num, sizeof(decoded_inst_t));
    CHECK_MEM(dcode);

    for (i=0; i<num; i++) {
	inst = code[i];
	dcode[i].op = SS_OPCODE(inst);
	switch(dcode[i].op) {
#define DEFINST(OP, MSK, NAME, FMT, FU, CLASS, O1,O2, IN1, IN2, IN3, EXPR) \
	case OP: \
	    dcode[i].in[0] = IN1; \
	    dcode[i].in[1] = IN2; \
	    dcode[i].in[2] = IN3; \
	    dcode[i].out[0] = O1; \
	    dcode[i].out[1] = O2; \
	    dcode[i].fu = FU; \
	    break;
#include "ss.def"
#undef DEFINST
	default:
	    dcode[i].in[0] = dcode[i].in[1] = dcode[i].in[2] = NA;
	    dcode[i].out[0] = dcode[i].out[1] = NA;
	    dcode[i].fu = FUClass_NA;
	}
    }
    prog.dcode = dcode;
}



// given a pointer to an instr, return its decoded instr
decoded_inst_t *
get_dcode(SS_INST_TYPE *inst)
{
    return (&prog.dcode[inst - prog.code]);
}
