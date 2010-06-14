/*****************************************************************************
 *
 * $Id: prog.h,v 1.4 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: prog.h,v $
 * Revision 1.4  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.3  2004/04/22 08:55:41  lixianfe
 * Version free of flush (except interprocedural flushes)
 *
 * Revision 1.2  2004/02/24 12:58:31  lixianfe
 * Initial ILP formulation is done
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#ifndef PROG_H
#define PROG_H

#include "ss.h"
#include "cfg.h"

#define	MAX_PROCS   256

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
decode_text();

decoded_inst_t *
get_dcode(SS_INST_TYPE	*inst);


#endif
