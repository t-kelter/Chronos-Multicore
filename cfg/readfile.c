/*****************************************************************************
 *
 * $Id: readfile.c,v 1.1.1.1 2004/02/05 07:20:16 lixianfe Exp $
 *
 * $Log: readfile.c,v $
 * Revision 1.1.1.1  2004/02/05 07:20:16  lixianfe
 * Initial version, simulate/esitmate straigh-line code
 *
 *
 ****************************************************************************/

#include <stdio.h>
#include "ecoff.h"
#include "readfile.h"


SS_INST_TYPE * readcode(char *fname, int *start, int *end)
{
    FILE		    *pf;
    long		    pos;
    struct ecoff_filehdr    fhdr;
    struct ecoff_aouthdr    ahdr;
    struct ecoff_scnhdr	    shdr;
    unsigned		    text_offset;
    unsigned		    text_size;
    unsigned		    text_entry;
    int			    i, j;
    SS_INST_TYPE	    inst, *code;

    pf = fopen(fname, "r");
    if (pf == NULL) {
      fprintf(stderr, "Failed to open file: %s\n", fname);
      exit (1);
    }

    ss_init_decoder();

    // locate the text section
    fseek(pf, 0, SEEK_SET);
    fread(&fhdr, sizeof fhdr, 1, pf);
    fread(&ahdr, sizeof ahdr, 1, pf);
    for (i=0; i<fhdr.f_nscns; i++) {
	fread(&shdr, sizeof shdr, 1, pf);
	if (shdr.s_flags != ECOFF_STYP_TEXT)
	    continue;
	text_size = shdr.s_size;
	text_offset = shdr.s_scnptr;
	text_entry = shdr.s_vaddr;
    }
    if (*start < text_entry || *start >= (text_entry + text_size))
	*start = text_entry;
    if (*end <= text_entry || *end > (text_entry + text_size))
	*end = text_entry + text_size;
    text_offset += *start - text_entry;
    text_size = *end - *start;
    text_entry = *start;

    // alloc mem for the text to be read
    code = (SS_INST_TYPE *) malloc(text_size);
    if (code == NULL) {
	fprintf(stderr, "fail malloc!\n");
	exit(1);
    }

    // read the text
    fseek(pf, text_offset, SEEK_SET);
    for (i=0, j=0; i<text_size; i += SS_INST_SIZE) {
	fread(&inst, sizeof inst, 1, pf);
	inst.a = (inst.a & ~0xff) | (unsigned int)SS_OP_ENUM(SS_OPCODE(inst));
	code[j++] = inst;
    }

    //print_code(code, text_size, text_entry);
    fclose(pf);
    return code;
}


void print_code(SS_INST_TYPE *code, int size, unsigned entry)
{
    int		    i;
    SS_INST_TYPE    inst;
    enum ss_opcode  op;
    int		    in1, in2, in3, out1, out2;

    for (i=0; i<size; i += SS_INST_SIZE) {
	inst = *code;
	op = SS_OPCODE(inst);
	switch(op) {
#define DEFINST(OP, MSK, NAME, FMT, FU, CLASS, O1,O2, IN1, IN2, IN3, EXPR) \
	case OP: \
	    in1 = IN1; in2 = IN2; in3 = IN3; \
	    out1 = O1; out2 = O2; \
	    break;
#include "ss.def"
#undef DEFINST
	default:
	    in1 = in2 = in3 = NA;
	    out1 = out2 = NA;
	}
	printf("%x %8s %15s\tIN(%d, %d, %d)\tOUT(%d, %d)\n", (entry + i), SS_OP_NAME(op),
		SS_FU_NAME(SS_OP_FUCLASS(op)), in1, in2, in3, out1, out2);
	
	++code;
    }
}
