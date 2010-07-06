/*
 * misc.c - miscellaneous routines
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
 * $Id: misc.c,v 1.1.1.1 2004/02/05 07:20:16 lixianfe Exp $
 *
 * $Log: misc.c,v $
 * Revision 1.1.1.1  2004/02/05 07:20:16  lixianfe
 * Initial version, simulate/esitmate straigh-line code
 *
 * Revision 1.5  1997/03/11  01:17:36  taustin
 * updated copyright
 * supported added for non-GNU C compilers
 *
 * Revision 1.4  1997/01/06  16:01:45  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "misc.h"

/* verbose output flag */
int verbose = FALSE;

#ifdef DEBUG
/* active debug flag */
int debugging = FALSE;
#endif /* DEBUG */

/* fatal function hook, this function is called just before an exit
   caused by a fatal error, used to spew stats, etc. */
static void (*hook_fn)(FILE *stream) = NULL;

/* register a function to be called when an error is detected */
void
fatal_hook(void (*fn)(FILE *stream))	/* fatal hook function */
{
  hook_fn = fn;
}

/* declare a fatal run-time error, calls fatal hook function */
#ifdef __GNUC__
void
_fatal(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
fatal(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "fatal: ");
  vfprintf(stderr, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  exit(1);
}

/* declare a panic situation, dumps core */
#ifdef __GNUC__
void
_panic(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
panic(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "panic: ");
  vfprintf(stderr, fmt, v);
#ifdef __GNUC__
  fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
  if (hook_fn)
    (*hook_fn)(stderr);
  abort();
}

/* declare a warning */
#ifdef __GNUC__
void
_warn(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
warn(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(stderr, "warning: ");
  vfprintf(stderr, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
}

/* print general information */
#ifdef __GNUC__
void
_info(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
info(const char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  vfprintf(stderr, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(stderr, "\n");
}

#ifdef DEBUG
/* print a debugging message */
#ifdef __GNUC__
void
_debug(const char *file, const char *func, int line, const char *fmt, ...)
#else /* !__GNUC__ */
void
debug(const char *fmt, ...)
#endif /* __GNUC__ */
{
    va_list v;
    va_start(v, fmt);

    if (debugging)
      {
        fprintf(stderr, "debug: ");
        vfprintf(stderr, fmt, v);
#ifdef __GNUC__
        fprintf(stderr, " [%s:%s, line %d]", func, file, line);
#endif
        fprintf(stderr, "\n");
      }
}
#endif /* DEBUG */


/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(char *s)		/* string to duplicate to heap storage */
{
  char *buf;

  if (!(buf = (char *)malloc(strlen(s)+1)))
    return NULL;
  strcpy(buf, s);
  return buf;
}

/* find the last occurrence of a character in a string */
char *
mystrrchr(char *s, char c)
{
  char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = s;
  } while (*s++);

  return rtnval;
}

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(char *s1, char *s2)	/* strings to compare, case insensitive */
{
  unsigned char u1, u2;

  for (;;)
    {
      u1 = (unsigned char)*s1++; u1 = tolower(u1);
      u2 = (unsigned char)*s2++; u2 = tolower(u2);

      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
}

/* return log of a number to the base 2 */
int
log_base2(int n)
{
  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
    panic("log2() only works for positive power of two values");

  while (n >>= 1)
    power++;

  return power;
}

/* return string describing elapsed time, passed in SEC in seconds */
char *
elapsed_time(long sec)
{
  static char tstr[256];
  char temp[256];

  if (sec <= 0)
    return "0s";

  tstr[0] = '\0';

  /* days */
  if (sec >= 86400)
    {
      sprintf(temp, "%ldD ", sec/86400);
      strcat(tstr, temp);
      sec = sec % 86400;
    }
  /* hours */
  if (sec >= 3600)
    {
      sprintf(temp, "%ldh ", sec/3600);
      strcat(tstr, temp);
      sec = sec % 3600;
    }
  /* mins */
  if (sec >= 60)
    {
      sprintf(temp, "%ldm ", sec/60);
      strcat(tstr, temp);
      sec = sec % 60;
    }
  /* secs */
  if (sec >= 1)
    {
      sprintf(temp, "%lds ", sec);
      strcat(tstr, temp);
    }
  tstr[strlen(tstr)-1] = '\0';
  return tstr;
}

/* assume bit positions numbered 31 to 0 (31 high order bit), extract num bits
   from word starting at position pos (with pos as the high order bit of those
   to be extracted), result is right justified and zero filled to high order
   bit, for example, extractl(word, 6, 3) w/ 8 bit word = 01101011 returns
   00000110 */
unsigned int
extractl(int word,		/* the word from which to extract */
         int pos,		/* bit positions 31 to 0 */
         int num)		/* number of bits to extract */
{
    return(((unsigned int) word >> (pos + 1 - num)) & ~(~0 << num));
}

#if 0 /* FIXME: not portable... */

static struct {
  char *type;
  char *ext;
  char *cmd;
} zfcmds[] = {
  /* type */	/* extension */		/* command */
  { "r",	".gz",			"gzip -dc < %s" },
  { "r",	".Z",			"uncompress -c < %s" },
  { "rb",	".gz",			"gzip -dc < %s" },
  { "rb",	".Z",			"uncompress -c < %s" },
  { "w",	".gz",			"gzip > %s" },
  { "w",	".Z",			"compress > %s" },
  { "wb",	".gz",			"gzip > %s" },
  { "wb",	".Z",			"compress > %s" }
};

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
ZFILE *
zfopen(char *fname, char *type)
{
  int i;
  char *cmd = NULL, *ext;
  ZFILE *zfd;

  /* get the extension */
  ext = mystrrchr(fname, '.');
  if (ext != NULL)
    {
      if (*ext != '\0')
	{
	  for (i=0; i < N_ELT(zfcmds); i++)
	    if (!strcmp(zfcmds[i].type, type) && !strcmp(zfcmds[i].ext, ext))
	      {
		cmd = zfcmds[i].cmd;
		break;
	      }
	}
    }

  zfd = (ZFILE *)calloc(1, sizeof(ZFILE));
  if (!zfd)
    fatal("out of virtual memory");
  
  if (cmd)
    {
      char str[2048];

      sprintf(str, cmd, fname);

      zfd->ft = ft_prog;
      zfd->fd = popen(str, type);
    }
  else
    {
      zfd->ft = ft_file;
      zfd->fd = fopen(fname, type);
    }

  if (!zfd->fd)
    {
      free(zfd);
      zfd = NULL;
    }
  return zfd;
}

int
zfclose(ZFILE *zfd)
{
  switch (zfd->ft)
    {
    case ft_file:
      fclose(zfd->fd);
      zfd->fd = NULL;
      break;

    case ft_prog:
      pclose(zfd->fd);
      zfd->fd = NULL;
      break;

    default:
      panic("bogus file type");
    }

  free(zfd);

  return TRUE;
}

#endif

