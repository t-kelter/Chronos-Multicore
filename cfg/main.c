/*****************************************************************************
 *
 * $Id: main.c,v 1.4 2004/04/22 08:55:41 lixianfe Exp $
 *
 ****************************************************************************/


#include <stdio.h>
#include "readfile.h"
#include "prog.h"
#include "cfg.h"

#define DEF_GLOBALS
#include "globals.h"



static void
do_cfg(int argc, char ** argv)
{
    int	    start, end, main_sa, i;
    Proc    *proc;
    FILE    *fptr;
    char    fn[80];

    sscanf(argv[2], "%x", &start);
    sscanf(argv[3], "%x", &end);
    sscanf(argv[4], "%x", &main_sa);

    if ((start == 0 || end == 0) || (start >= end)) {
	fprintf(stderr, "please give correct start/end addresses of the program\n");
	fprintf(stderr, "usage: %s <objfile> start(hex) end(hex) main(hex)\n", argv[0]);
	exit(1);
    }
    if (main_sa < start || main_sa > end) {
	fprintf(stderr, "please give correct main address of the program\n");
	fprintf(stderr, "usage: %s <objfile> start(hex) end(hex) main(hex)\n", argv[0]);
	exit(1);
    }

    prog.main_sa = main_sa;
    prog.code = (SS_INST_TYPE *) readcode(argv[1], &start, &end);
    //fprintf(stderr, "start=%x; end=%x\n", start, end);
    prog.sa = start;
    prog.sz = end - start;
    decode_text();

    sprintf( fn, "%s.cfg", argv[1] );
    fptr = fopen( fn, "w" );
    if (fptr == NULL) {
      fprintf(stderr, "Failed to open file: %s\n", fn);
      exit (1);
    }

    create_procs(&prog);
    for (i=0; i<prog.nproc; i++) {
	proc = &(prog.procs[i]);
	create_bbs(proc);
	dump_cfg(fptr,proc);
    }
}
int
main(int argc, char **argv)
{
    do_cfg(argc, argv);
}
