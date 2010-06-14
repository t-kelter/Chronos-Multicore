/*
 * modified misc.h -  miscellaneous routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original misc.h file from SimpleScalar 3.0 Tool.
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

 * The original misc.h file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The SimpleScalar suite is currently maintained by Doug Burger and Todd M. Austin.
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
 * Adaptation 1.0  1999/11/10  Ronaldo A. L. Goncalves
 * The original misc.h was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 2) Many function calls were modified to pass the slot's number argument
 * 3) The routine "get_slot_number" was included to get the number that must
 *    be concatened in the end of a string. It is used to handle both option 
 *    and stat names.
 * 4) The define "itoa" was included to return a string for a integer. Its
 *    reach must be enlarged if MAX_SLOTS increases besides 32
 *
 *
 * Revision 1.5  1998/08/27 15:45:24  taustin
 * implemented host interface description in host.h
 * added target interface support
 * implemented a more portable random() interface
 * disabled calls to sbrk() under malloc(), this breaks some
 *       malloc() implementation (e.g., newer Linux releases)
 * added myprintf() and myatoq() routines for printing and reading
 *       quadword's, respectively
 * added gzopen() and gzclose() routines for reading and writing
 *       compressed files, updated sysprobe to search for GZIP, if found
 *       support is enabled
 * moved host-dependent definitions to host.h
 *
 * Revision 1.4  1997/03/11  01:18:24  taustin
 * updated copyright
 * supported added for non-GNU C compilers
 * ANSI C compiler check added
 * long/int tweaks made for ALPHA target support
 * hacks added to make SYMCAT() portable
 *
 * Revision 1.3  1997/01/06  16:02:01  taustin
 * comments updated
 * system prototypes deleted (-Wall flag no longer a clean compile)
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

/* return a string for an integer. This definition must has as elements as MAX_SLOTS (machine.h). If MAX_SLOTS is increased then this definition must to be grown too. Note that: if MAX_SLOTS can be greater than 99, the conversion must use 3 or more characters between "", for example, "001" or "0023" */

extern char *itoa[32];

/* these variables defines the first and last command line to be executed */

extern int start_command_line;
extern int last_command_line;

/* this function is used to know if an function was called early in accord with an specific mask. If the bit from mask that is pointed by index is 1 then the function was acessed early. The function return the result of comparation and update the mask for the next time */

extern int first_time(int index, int *mask);

/* it tries to identify the slot number. If there is one, this function return it and "okay" return true, otherwise this function return zero and "okay" return false */


extern int get_slot_number(char * name, int *okay);

/* boolean value defs */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* various useful macros */
#ifndef MAX
#define MAX(a, b)    (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

/* for printing out "long long" vars */
#define LLHIGH(L)		((int)(((L)>>32) & 0xffffffff))
#define LLLOW(L)		((int)((L) & 0xffffffff))

/* size of an array, in elements */
#define N_ELT(ARR)   (sizeof(ARR)/sizeof((ARR)[0]))

/* rounding macros, assumes ALIGN is a power of two */
#define ROUND_UP(N,ALIGN)	(((N) + ((ALIGN)-1)) & ~((ALIGN)-1))
#define ROUND_DOWN(N,ALIGN)	((N) & ~((ALIGN)-1))

/* verbose output flag */
extern int verbose;

#ifdef DEBUG
/* active debug flag */
extern int debugging;
#endif /* DEBUG */

/* register a function to be called when an error is detected */
void
fatal_hook(void (*hook_fn)(FILE *stream));	/* fatal hook function */

#ifdef __GNUC__
/* declare a fatal run-time error, calls fatal hook function */
#define fatal(sn, fmt, args...)	\
  _fatal(sn, __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_fatal(int sn, char *file, char *func, int line, char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
fatal(int sn, char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a panic situation, dumps core */
#define panic(sn, fmt, args...)	\
  _panic(sn, __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_panic(int sn, char *file, char *func, int line, char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
panic(int sn, char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a warning */
#define warn(sn, fmt, args...)	\
  _warn(sn, __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_warn(int sn, char *file, char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
warn(int sn, char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* print general information */
#define info(fmt, args...)	\
  _info(sn,__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_info(int sn, char *file, char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
info(int sn, char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef DEBUG

#ifdef __GNUC__
/* print a debugging message */
#define debug(sn, fmt, args...)	\
    do {                        \
        if (debugging)         	\
            _debug(sn, __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
    } while(0)

void
_debug(int sn, char *file, char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
debug(int sn, char *fmt, ...);
#endif /* !__GNUC__ */

#else /* !DEBUG */

#ifdef __GNUC__
#define debug(sn, fmt, args...)
#else /* !__GNUC__ */
/* the optimizer should eliminate this call! */
static void debug(int sn, char *fmt, ...) {}
#endif /* !__GNUC__ */

#endif /* !DEBUG */

/* seed the random number generator */
void
mysrand(unsigned int seed);	/* random number generator seed */

/* get a random number */
int myrand(void);		/* returns random number */

/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(char *s);		/* string to duplicate to heap storage */

/* find the last occurrence of a character in a string */
char *
mystrrchr(char *s, char c);

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(char *s1, char *s2);	/* strings to compare, case insensitive */

/* allocate some core, this memory has overhead no larger than a page
   in size and it cannot be released. the storage is returned cleared */
void *getcore(int nbytes);

/* return log of a number to the base 2 */
int log_base2(int n, int sn);

/* return string describing elapsed time, passed in SEC in seconds */
char *elapsed_time(long sec);

/* assume bit positions numbered 31 to 0 (31 high order bit), extract num bits
   from word starting at position pos (with pos as the high order bit of those
   to be extracted), result is right justified and zero filled to high order
   bit, for example, extractl(word, 6, 3) w/ 8 bit word = 01101011 returns
   00000110 */
unsigned int
extractl(int word,		/* the word from which to extract */
         int pos,		/* bit positions 31 to 0 */
         int num);		/* number of bits to extract */

#if defined(sparc) && !defined(__svr4__)
#define strtoul strtol
#endif

/* portable 64-bit I/O package */

/* portable vsprintf with quadword support, returns end pointer */
char *myvsprintf(char *obuf, char *format, va_list v, int sn);

/* portable sprintf with quadword support, returns end pointer */
char *mysprintf(int sn, char *obuf, char *format, ...);

/* portable vfprintf with quadword support, returns end pointer */
void myvfprintf(FILE *stream, char *format, va_list v, int sn);

/* portable fprintf with quadword support, returns end pointer */
void myfprintf(int sn, FILE *stream, char *format, ...);

#ifdef HOST_HAS_QUAD

/* convert a string to a signed result */
squad_T myatosq(char *nptr, char **endp, int base, int sn);

/* convert a string to a unsigned result */
quad_T myatoq(char *nptr, char **endp, int base, int sn);

#endif /* HOST_HAS_QUAD */

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
FILE *gzopen(char *fname, char *type);

/* close compressed stream */
void gzclose(FILE *fd);

#endif /* MISC_H */
