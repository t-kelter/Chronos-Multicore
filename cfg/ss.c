/*
 * ss.c - simplescalar definition routines
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
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
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: ss.c,v 1.1.1.1 2004/02/05 07:20:16 lixianfe Exp $
 *
 * $Log: ss.c,v $
 * Revision 1.1.1.1  2004/02/05 07:20:16  lixianfe
 * Initial version, simulate/esitmate straigh-line code
 *
 * Revision 1.4  1997/03/11  01:40:38  taustin
 * updated copyrights
 * long/int tweaks made for ALPHA target support
 * supported added for non-GNU C compilers
 * ss_print_insn() can now tolerate bogus insts (for DLite!)
 *
 * Revision 1.3  1997/01/06  16:07:24  taustin
 * comments updated
 * functional unit definitions moved from ss.def
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include "misc.h"
#include "ss.h"

/* INC_DEC expression step tables, they map (operation, size) -> step value,
   and speed up pre/post-incr/desc handling */

/* force a nasty address */
#define XX		0x6bababab

/* before increment */
int ss_fore_tab[/* operand size */8][/* operation */5] = {
             /* NOP   POSTI POSTD  PREI   PRED */
/* byte */    {  0,    0,    0,     1,     -1,  },
/* half */    {  0,    0,    0,     2,     -2,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* word */    {  0,    0,    0,     4,     -4,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* dword */   {  0,    0,    0,     8,     -8,  },
};

/* after increment */
int ss_aft_tab[/* operand size */8][/* operation */5] = {
             /* NOP   POSTI POSTD  PREI   PRED */
/* byte */    {  0,    1,    -1,    0,     0,   },
/* half */    {  0,    2,    -2,    0,     0,   },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* word */    {  0,    4,    -4,    0,     0,   },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* invalid */ {  XX,   XX,   XX,    XX,    XX,  },
/* dword */   {  0,    8,    -8,    0,     0,   },
};

/* lwl/lwr/swl/swr masks */
unsigned int ss_lr_masks[] = {
#ifdef BYTES_BIG_ENDIAN
  0x00000000,
  0x000000ff,
  0x0000ffff,
  0x00ffffff,
  0xffffffff,
#else
  0xffffffff,
  0x00ffffff,
  0x0000ffff,
  0x000000ff,
  0x00000000,
#endif
};

/* LWL/LWR implementation workspace */
SS_ADDR_TYPE ss_lr_temp;

/* temporary variables */
SS_ADDR_TYPE temp_bs, temp_rd;

/* opcode mask -> enum ss_opcodem, used by decoder (SS_OP_ENUM()) */
enum ss_opcode ss_mask2op[SS_MAX_MASK+1];

/* preferred nop instruction definition */
SS_INST_TYPE SS_NOP_INST = { NOP, 0 };

/* intialize the inst decoder, this function builds the ISA decode tables */
void
ss_init_decoder(void)
{
  /* FIXME: CONNECT defined? */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR)	\
  if (ss_mask2op[(MSK)]) fatal("doubly defined mask value");		\
  if ((MSK) > SS_MAX_MASK) fatal("mask value is too large");		\
  ss_mask2op[(MSK)]=(OP);
#include "ss.def"
#undef DEFINST
}

/* enum ss_opcode -> description string */
char *ss_op2name[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR) NAME,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NAME,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
};

/* enum ss_opcode -> opcode operand format, used by disassembler */
char *ss_op2format[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR) OPFORM,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NULL,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
};

/* enum ss_opcode -> enum ss_fu_class, used by performance simulators */
enum ss_fu_class ss_op2fu[OP_MAX] = {
  FUClass_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR) RES,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) FUClass_NA,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
};

/* enum ss_opcode -> opcode flags, used by simulators */
unsigned int ss_op2flags[OP_MAX] = {
  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR) FLAGS,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NA,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
};

/* enum ss_fu_class -> description string */
char *ss_fu2name[NUM_FU_CLASSES] = {
  NULL, /* NA */
  "fu-int-ALU",
  "fu-int-multiply",
  "fu-int-divide",
  "fu-FP-add/sub",
  "fu-FP-comparison",
  "fu-FP-conversion",
  "fu-FP-multiply",
  "fu-FP-divide",
  "fu-FP-sqrt",
  "rd-port",
  "wr-port"
};

/* disassemble a SimpleScalar instruction */
void
ss_print_insn(SS_INST_TYPE inst,	/* instruction to disassemble */
	      SS_ADDR_TYPE pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  enum ss_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  op = SS_OPCODE(inst); /* use: SS_OP_ENUM(SS_OPCODE(inst)); if not predec */

  /* disassemble the instruction */
  if (op >= OP_MAX)
    {
      /* bogus instruction */
      fprintf(stream, "<invalid inst: 0x%08x:%08x>\n", inst.a, inst.b);
    }
  else
    {
      char *s;

      fprintf(stream, "%-10s", SS_OP_NAME(op));

      s = SS_OP_FORMAT(op);
      while (*s) {
	switch (*s) {
	case 'd':
	  fprintf(stream, "r%d", RD);
	  break;
	case 's':
	  fprintf(stream, "r%d", RS);
	  break;
	case 't':
	  fprintf(stream, "r%d", RT);
	  break;
	case 'b':
	  fprintf(stream, "r%d", BS);
	  break;
	case 'D':
	  fprintf(stream, "f%d", FD);
	  break;
	case 'S':
	  fprintf(stream, "f%d", FS);
	  break;
	case 'T':
	  fprintf(stream, "f%d", FT);
	  break;
	case 'j':
	  fprintf(stream, "0x%x", (pc + 8 + (OFS << 2)));
	  break;
	case 'o':
	case 'i':
	  fprintf(stream, "%d", IMM);
	  break;
	case 'H':
	  fprintf(stream, "%d", SHAMT);
	  break;
	case 'u':
	  fprintf(stream, "%u", UIMM);
	  break;
	case 'U':
	  fprintf(stream, "0x%x", UIMM);
	  break;
	case 'J':
	  fprintf(stream, "0x%x", ((pc & 036000000000) | (TARG << 2)));
	  break;
	case 'B':
	  fprintf(stream, "0x%x", BCODE);
	  break;
	case ')':
	  /* handle pre- or post-inc/dec */
	  if (SS_COMP_OP == SS_COMP_NOP)
	    fprintf(stream, ")");
	  else if (SS_COMP_OP == SS_COMP_POST_INC)
	    fprintf(stream, ")+");
	  else if (SS_COMP_OP == SS_COMP_POST_DEC)
	    fprintf(stream, ")-");
	  else if (SS_COMP_OP == SS_COMP_PRE_INC)
	    fprintf(stream, ")^+");
	  else if (SS_COMP_OP == SS_COMP_PRE_DEC)
	    fprintf(stream, ")^-");
	  else if (SS_COMP_OP == SS_COMP_POST_DBL_INC)
	    fprintf(stream, ")++");
	  else if (SS_COMP_OP == SS_COMP_POST_DBL_DEC)
	    fprintf(stream, ")--");
	  else if (SS_COMP_OP == SS_COMP_PRE_DBL_INC)
	    fprintf(stream, ")^++");
	  else if (SS_COMP_OP == SS_COMP_PRE_DBL_DEC)
	    fprintf(stream, ")^--");
	  else
	    panic("bogus SS_COMP_OP");
	  break;
	default:
	  /* anything unrecognized, e.g., '.' is just passed through */
	  fputc(*s, stream);
	}
	s++;
      }
      fprintf(stream, "\n");
    }
}

/* Added by Tulika */
/* disassemble a SimpleScalar instruction */
void
tulika_ss_print_insn(SS_INST_TYPE inst,	/* instruction to disassemble */
	      SS_ADDR_TYPE pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  enum ss_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  op = SS_OPCODE(inst); /* use: SS_OP_ENUM(SS_OPCODE(inst)); if not predec */

  /* disassemble the instruction */
  if (op >= OP_MAX)
    {
      /* bogus instruction */
      fprintf(stream, "<invalid inst: 0x%08x:%08x>\n", inst.a, inst.b);
    }
  else
    {
      char *s;

      fprintf(stream, "%-10s", SS_OP_NAME(op));

      s = SS_OP_FORMAT(op);
      while (*s) {
	switch (*s) {
	case 'd':
	  fprintf(stream, "r%d", RD);
	  break;
	case 's':
	  fprintf(stream, "r%d", RS);
	  break;
	case 't':
	  fprintf(stream, "r%d", RT);
	  break;
	case 'b':
	  fprintf(stream, "r%d", BS);
	  break;
	case 'D':
	  fprintf(stream, "f%d", FD);
	  break;
	case 'S':
	  fprintf(stream, "f%d", FS);
	  break;
	case 'T':
	  fprintf(stream, "f%d", FT);
	  break;
	case 'j':
	  fprintf(stream, "0x%x", (pc + 8 + (OFS << 2)));
      	  break;
	case 'o':
	case 'i':
	  fprintf(stream, "%d", IMM);
	  break;
	case 'H':
	  fprintf(stream, "%d", SHAMT);
	  break;
	case 'u':
	  fprintf(stream, "%u", UIMM);
	  break;
	case 'U':
	  fprintf(stream, "0x%x", UIMM);
	  break;
	case 'J':
	  fprintf(stream, "0x%x", ((pc & 036000000000) | (TARG << 2)));
	  break;
	case 'B':
	  fprintf(stream, "0x%x", BCODE);
	  break;
	case ')':
	  /* handle pre- or post-inc/dec */
	  if (SS_COMP_OP == SS_COMP_NOP)
	    fprintf(stream, ")");
	  else if (SS_COMP_OP == SS_COMP_POST_INC)
	    fprintf(stream, ")+");
	  else if (SS_COMP_OP == SS_COMP_POST_DEC)
	    fprintf(stream, ")-");
	  else if (SS_COMP_OP == SS_COMP_PRE_INC)
	    fprintf(stream, ")^+");
	  else if (SS_COMP_OP == SS_COMP_PRE_DEC)
	    fprintf(stream, ")^-");
	  else if (SS_COMP_OP == SS_COMP_POST_DBL_INC)
	    fprintf(stream, ")++");
	  else if (SS_COMP_OP == SS_COMP_POST_DBL_DEC)
	    fprintf(stream, ")--");
	  else if (SS_COMP_OP == SS_COMP_PRE_DBL_INC)
	    fprintf(stream, ")^++");
	  else if (SS_COMP_OP == SS_COMP_PRE_DBL_DEC)
	    fprintf(stream, ")^--");
	  else
	    panic("bogus SS_COMP_OP");
	  break;
	default:
	  /* anything unrecognized, e.g., '.' is just passed through */
	  if (*s != ',')
	    fputc(*s, stream);
	  else
	    fprintf(stream, " ");
	}
	s++;
      }
      fprintf(stream, "\n");
    }
}
/* End added by Tulika */
