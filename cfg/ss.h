/*
 * ss.h - simplescaler definitions
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
 * $Id: ss.h,v 1.2 2004/02/11 07:56:51 lixianfe Exp $
 *
 * $Log: ss.h,v $
 * Revision 1.2  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 * Revision 1.1.1.1  2004/02/05 07:20:16  lixianfe
 * Initial version, simulate/esitmate straigh-line code
 *
 * Revision 1.5  1997/03/11  01:38:10  taustin
 * updated copyrights
 * long/int tweaks made for ALPHA target support
 * IFAIL() hook now allows simulators to declare instruction faults
 * IDIV()/IMOD()/FDIV() hooks now support simulator fault masking
 * supported added for non-GNU C compilers
 *
 * Revision 1.4  1997/01/06  16:08:10  taustin
 * comments updated
 * functional unit definitions moved from ss.def
 *
 * Revision 1.3  1996/12/27  15:55:37  taustin
 * fixed system header collision with MAXINT
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 */

#ifndef SS_H
#define SS_H

#include <stdio.h>

/*
 * This file contains various definitions needed to decode, disassemble, and
 * execute SimpleScalar instructions.
 */

/* not applicable/available, usable in most definition contexts */
#define NA		0

/* basic SimpleScalar type definitions */
typedef double SS_DOUBLE_TYPE;
typedef float SS_FLOAT_TYPE;
typedef int SS_WORD_TYPE;
typedef short SS_HALF_TYPE;
typedef char SS_BYTE_TYPE;
typedef SS_WORD_TYPE SS_PTR_TYPE;

/* statistical counter types, note use of GNU GCC extensions */
#ifdef __GNUC__
typedef long long SS_COUNTER_TYPE;
typedef long long SS_TIME_TYPE;
#else
typedef double SS_COUNTER_TYPE;
typedef double SS_TIME_TYPE;
#endif

/* instruction/address formats */
typedef unsigned int SS_ADDR_TYPE;
typedef struct {
  unsigned int a;		/* simplescalar opcode (must be unsigned) */
  unsigned int b;		/* simplescalar unsigned immediate fields */
} SS_INST_TYPE;
#define SS_INST_SIZE		sizeof(SS_INST_TYPE)

/* virtual memory segment limits */
#define SS_TEXT_BASE		0x00400000
#define SS_DATA_BASE		0x10000000
#define SS_STACK_BASE 		0x7fffc000

/* virtual memory page size, this should be user configurable */
#define SS_PAGE_SIZE		4096

/* maximum size of argc+argv+envp environment */
#define SS_MAX_ENVIRON		16384

/* well known registers */
#define SS_GP_REGNO		28	/* global data section pointer */
#define SS_STACK_REGNO		29	/* stack pointer */
#define SS_FRAME_REGNO		30	/* frame pointer */

/* total number of registers in each register file (int and FP) */
#define SS_NUM_REGS		32

/* total number of register in processor 32I+32F+HI+LO+FCC+TMP+MEM+CTRL */
#define SS_TOTAL_REGS							\
  (SS_NUM_REGS+SS_NUM_REGS+/*HI*/1+/*LO*/1+/*FCC*/1+/*TMP*/1+		\
   /*MEM*/1+/*CTRL*/1)

/* returns the opcode field value of SimpleScalar instruction INST */
#define SS_OPCODE(INST)		((INST).a & 0xff)

/* returns pre/post-incr/decr operation field value */
#define SS_COMP_OP		((inst.a & 0xff00) >> 8)

/* pre/post-incr/decr operation field specifiers */
#define SS_COMP_NOP		0x00
#define SS_COMP_POST_INC	0x01
#define SS_COMP_POST_DEC	0x02
#define SS_COMP_PRE_INC		0x03
#define SS_COMP_PRE_DEC		0x04
#define SS_COMP_POST_DBL_INC	0x05	/* for double word accesses */
#define SS_COMP_POST_DBL_DEC	0x06
#define SS_COMP_PRE_DBL_INC	0x07
#define SS_COMP_PRE_DBL_DEC	0x08

/* the instruction expression modifications required for an expression to
   support pre/post-incr/decr operations is accomplished by the INC_DEC()
   macro, it looks so contorted to reduce the control complexity of the
   equation (and thus reducing the compilation time greatly with GNU GCC -
   the key is to only emit EXPR one time) */
#define INC_DEC(EXPR, REG, SIZE)					\
  (SET_GPR((REG), GPR(REG) + ss_fore_tab[(SIZE)-1][SS_COMP_OP]),	\
   (EXPR),								\
   SET_GPR((REG), GPR(REG) + ss_aft_tab[(SIZE)-1][SS_COMP_OP]))

/* INC_DEC expression step tables, they map (operation, size) -> step value */
extern int ss_fore_tab[8][5];
extern int ss_aft_tab[8][5];

/* integer register specifiers */
#undef  RS	/* defined in /usr/include/sys/syscall.h on HPUX boxes */
#define RS		(inst.b >> 24)			/* reg source #1 */
#define RT		((inst.b >> 16) & 0xff)		/* reg source #2 */
#define RD		((inst.b >> 8) & 0xff)		/* reg dest */

/* pre-defined registers */
#define Rgp		28		/* global data pointer */
#define Rsp		29		/* stack pointer */
#define Rfp		30		/* frame pointer */

/* returns shift amount field value */
#define SHAMT		(inst.b & 0xff)

/* floating point register field synonyms */
#define FS		RS
#define FT		RT
#define FD		RD

/* returns 16-bit signed immediate field value */
#define IMM		((int)((/* signed */short)(inst.b & 0xffff)))

/* returns 16-bit unsigned immediate field value */
#define UIMM		(inst.b & 0xffff)

/* returns 26-bit unsigned absolute jump target field value */
#define TARG		(inst.b & 0x3ffffff)

/* returns break code immediate field value */
#define BCODE		(inst.b & 0xfffff)

/* load/store 16-bit signed offset field value, synonym for imm field */
#define OFS		IMM		/* alias to IMM */

/* load/store base register specifier, synonym for RS field */
#define BS		RS		/* alias to rs */

/* lwl/swl defs */
#define WL_SIZE(ADDR)       (4-((ADDR) & 0x03))
#define WL_BASE(ADDR)       ((ADDR) & ~0x03)
#define WL_PROT_MASK(ADDR)  (ss_lr_masks[4-WL_SIZE(ADDR)])

/* lwr/swr defs */
#define WR_SIZE(ADDR)       (((ADDR) & 0x03)+1)
#define WR_BASE(ADDR)       ((ADDR) & ~0x03)
#define WR_PROT_MASK(ADDR)  (~(ss_lr_masks[WR_SIZE(ADDR)]))

/* mask table used to speed up LWL/LWR implementation */
extern unsigned int ss_lr_masks[];

/* FIXME: non-reentrant LWL/LWR implementation workspace */
extern SS_ADDR_TYPE ss_lr_temp;

/* FIXME: non-reentrant temporary variables */
extern SS_ADDR_TYPE temp_bs, temp_rd;

/* largest signed integer */
#define MAXINT_VAL	0x7fffffff

/* inst check macros, activated if NO_ICHECKS is not defined (default) */
#ifndef NO_ICHECKS

/* instruction failure notification macro, this can be defined by the
   target simulator if, for example, the simulator wants to handle the
   instruction fault in a machine specific fashion; a string describing
   the instruction fault is passed to the IFAIL() macro */
#ifndef IFAIL
#define IFAIL(S)	fatal(S)
#endif /* IFAIL */

/* check for overflow in X+Y, both signed */
#define OVER(X,Y)	(((((X) > 0) && ((Y) > 0)			\
			   && (MAXINT_VAL - (X) < (Y)))			\
			  ? IFAIL("+ overflow") : (void)0),		\
			 ((((X) < 0) && ((Y) < 0)			\
			   && (-MAXINT_VAL - (X) > (Y)))		\
			  ? IFAIL("+ underflow") : (void)0))

/* check for underflow in X-Y, both signed */
#define UNDER(X,Y)	(((((X) > 0) && ((Y) < 0)			\
			   && (MAXINT_VAL + (Y) < (X)))			\
			  ? IFAIL("- overflow") : (void)0),		\
			 ((((X) < 0) && ((Y) > 0)			\
			   && (-MAXINT_VAL + (Y) > (X)))		\
			  ? IFAIL("- underflow") : (void)0))

/* check for divide by zero error, N is denom */
#define DIV0(N)		(((N) == 0) ? IFAIL("divide by 0") : (void)0)

/* check reg specifier N for required double integer word alignment */
#define INTALIGN(N)	(((N) & 01)					\
			 ? IFAIL("bad INT register alignment") : (void)0)

/* check reg specifier N for required double FP word alignment */
#define FPALIGN(N)	(((N) & 01)					\
			 ? IFAIL("bad FP register alignment") : (void)0)

/* check target address TARG for required jump target alignment */
#define TALIGN(TARG)	(((TARG) & 0x7)					\
			 ? IFAIL("bad jump alignment") : (void)0)

#else /* NO_ICHECKS */

/* inst checks disables, change all checks to NOP expressions */
#define OVER(X,Y)	((void)0)
#define UNDER(X,Y)	((void)0)
#define DIV0(N)		((void)0)
#define INTALIGN(N)	((void)0)
#define FPALIGN(N)	((void)0)
#define TALIGN(TARG)	((void)0)

#endif /* NO_ICHECKS */

/* default division operator semantics, this operation is accessed through a
   macro because some simulators need to check for divide by zero faults
   before executing this operation */
#define IDIV(A, B)	((A) / (B))
#define IMOD(A, B)	((A) % (B))
#define FDIV(A, B)	((A) / (B))
#define FINT(A)		((int)A)

/* default target PC handling */
#ifndef SET_TPC
#define SET_TPC(PC)	0
#endif /* SET_TPC */

/* instruction flags */
#define F_ICOMP		0x00000001	/* integer computation */
#define F_FCOMP		0x00000002	/* FP computation */
#define F_CTRL		0x00000004	/* control inst */
#define F_UNCOND	0x00000008	/*   unconditional change */
#define F_COND		0x00000010	/*   conditional change */
#define F_MEM		0x00000020	/* memory access inst */
#define F_LOAD		0x00000040	/*   load inst */
#define F_STORE		0x00000080	/*   store inst */
#define F_DISP		0x00000100	/*   displaced (R+C) addr mode */
#define F_RR		0x00000200	/*   R+R addr mode */
#define F_DIRECT	0x00000400	/*   direct addressing mode */
#define F_TRAP		0x00000800	/* traping inst */
#define F_LONGLAT	0x00001000	/* long latency inst (for sched) */
#define F_DIRJMP	0x00002000	/* direct jump */
#define F_INDIRJMP	0x00004000	/* indirect jump */
#define F_CALL		0x00008000	/* function call */
#define F_FPCOND	0x00010000	/* FP conditional branch */

/* preferred nop instruction definition */
extern SS_INST_TYPE SS_NOP_INST;

/* global opcode names, these are returned by the decoder (SS_OP_ENUM()) */
enum ss_opcode {
  OP_NA = 0,	/* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR) OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) OP,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLDST
#undef DEFLINK
#undef CONNECT
  OP_MAX	/* number of opcodes + NA */
};

/* function unit classes, update ss_fu2name if you update this definition */
enum ss_fu_class {
  FUClass_NA = 0,	/* inst does not use a functional unit */
  IntALU,		/* integer ALU */
  IntMULT,		/* integer multiplier */
  IntDIV,		/* integer divider */
  FloatADD,		/* floating point adder/subtractor */
  FloatCMP,		/* floating point comparator */
  FloatCVT,		/* floating point<->integer converter */
  FloatMULT,		/* floating point multiplier */
  FloatDIV,		/* floating point divider */
  FloatSQRT,		/* floating point square root */
  RdPort,		/* memory read port */
  WrPort,		/* memory write port */
  NUM_FU_CLASSES	/* total functional unit classes */
};

/* largest opcode field value (currently upper 8-bit are used for pre/post-
    incr/decr operation specifiers */
#define SS_MAX_MASK		255

/* inst -> enum ss_opcode mapping, use this macro to decode insts */
#define SS_OP_ENUM(MSK)		(ss_mask2op[MSK])
extern enum ss_opcode ss_mask2op[];

/* enum ss_opcode -> description string */
#define SS_OP_NAME(OP)		(ss_op2name[OP])
extern char *ss_op2name[];

/* enum ss_opcode -> opcode operand format, used by disassembler */
#define SS_OP_FORMAT(OP)	(ss_op2format[OP])
extern char *ss_op2format[];

/* enum ss_opcode -> enum ss_fu_class, used by performance simulators */
#define SS_OP_FUCLASS(OP)	(ss_op2fu[OP])
extern enum ss_fu_class ss_op2fu[];

/* enum ss_opcode -> opcode flags, used by simulators */
#define SS_OP_FLAGS(OP)		(ss_op2flags[OP])
extern unsigned int ss_op2flags[];

/* enum ss_fu_class -> description string */
#define SS_FU_NAME(FU)		(ss_fu2name[FU])
extern char *ss_fu2name[];

/* intialize the inst decoder, this function builds the ISA decode tables */
void ss_init_decoder(void);

/* disassemble a SimpleScalar instruction */
void
ss_print_insn(SS_INST_TYPE inst,	/* instruction to disassemble */
	      SS_ADDR_TYPE pc,		/* addr of inst, used for PC-rels */
	      FILE *stream);		/* output stream */


// following macros are from sim-outorder.c -- by LXF
/* general register dependence decoders */
#define DGPR(N)			(N)
#define DCGPR(N)		(SS_COMP_OP != SS_COMP_NOP ? (N) : 0)
#define DGPR_D(N)		((N)&~1)

/* floating point register dependence decoders */
#define DFPR_L(N)		(((N)+32)&~1)
#define DFPR_F(N)		(((N)+32)&~1)
#define DFPR_D(N)		(((N)+32)&~1)

/* miscellaneous register dependence decoders */
#define DHI			(0+32+32)
#define DLO			(1+32+32)
#define DFCC			(2+32+32)
#define DTMP			(3+32+32)
#define DNA			(0)

#endif /* SS_H */
