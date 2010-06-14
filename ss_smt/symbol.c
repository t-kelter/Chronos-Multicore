/*
 * modified symbol.c - program symbol and line data routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original routines.c file from SimpleScalar 3.0 Tool.
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

 * The original routines.c file is a part of the SimpleScalar tool suite written by
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
 * The original symbol.c was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 *  
 * $Id: symbol.c,v 1.3 1998/08/27 17:06:45 taustin Exp taustin $
 *
 * $Log: symbol.c,v $
 * Revision 1.3  1998/08/27 17:06:45  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for MS VC++ and CYGWIN32 hosts
 *
 * Revision 1.2  1997/04/16  22:11:50  taustin
 * added standalone loader support
 *
 * Revision 1.1  1997/03/11  01:34:45  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */
#include "ecoff.h"
#endif /* BFD_LOADER */
#include "loader.h"
#include "symbol.h"

/* #define PRINT_SYMS */

/* symbol database in no particular order. See symbol_init */
struct sym_sym_t *sym_db[MAX_SLOTS];

/* all symbol sorted by address. See symbol_init  */
int sym_nsyms[MAX_SLOTS];
struct sym_sym_t **sym_syms[MAX_SLOTS];

/* all symbols sorted by name. See symbol_init  */
struct sym_sym_t **sym_syms_by_name[MAX_SLOTS];

/* text symbols sorted by address. See symbol_init  */
int sym_ntextsyms[MAX_SLOTS];
struct sym_sym_t **sym_textsyms[MAX_SLOTS];

/* text symbols sorted by name. See symbol_init  */
struct sym_sym_t **sym_textsyms_by_name[MAX_SLOTS];

/* data symbols sorted by address. See symbol_init  */
int sym_ndatasyms[MAX_SLOTS];
struct sym_sym_t **sym_datasyms[MAX_SLOTS];

/* data symbols sorted by name. See symbol_init  */
struct sym_sym_t **sym_datasyms_by_name[MAX_SLOTS];

/* symbols loaded?. See symbol_init  */
static int syms_loaded[MAX_SLOTS];

#ifdef PRINT_SYMS
/* convert BFD symbols flags to a printable string */
static char *			/* symbol flags string */
flags2str(unsigned int flags,	/* bfd symbol flags */
          int sn)               /* slot number */
{
  static char buf[MAX_SLOTS][256];
  char *p;

  if (!flags)
    return "";

  p = buf[sn];
  *p = '\0';

  if (flags & BSF_LOCAL)
    {
      *p++ = 'L';
      *p++ = '|';
    }
  if (flags & BSF_GLOBAL)
    {
      *p++ = 'G';
      *p++ = '|';
    }
  if (flags & BSF_DEBUGGING)
    {
      *p++ = 'D';
      *p++ = '|';
    }
  if (flags & BSF_FUNCTION)
    {
      *p++ = 'F';
      *p++ = '|';
    }
  if (flags & BSF_KEEP)
    {
      *p++ = 'K';
      *p++ = '|';
    }
  if (flags & BSF_KEEP_G)
    {
      *p++ = 'k'; *p++ = '|';
    }
  if (flags & BSF_WEAK)
    {
      *p++ = 'W';
      *p++ = '|';
    }
  if (flags & BSF_SECTION_SYM)
    {
      *p++ = 'S'; *p++ = '|';
    }
  if (flags & BSF_OLD_COMMON)
    {
      *p++ = 'O';
      *p++ = '|';
    }
  if (flags & BSF_NOT_AT_END)
    {
      *p++ = 'N';
      *p++ = '|';
    }
  if (flags & BSF_CONSTRUCTOR)
    {
      *p++ = 'C';
      *p++ = '|';
    }
  if (flags & BSF_WARNING)
    {
      *p++ = 'w';
      *p++ = '|';
    }
  if (flags & BSF_INDIRECT)
    {
      *p++ = 'I';
      *p++ = '|';
    }
  if (flags & BSF_FILE)
    {
      *p++ = 'f';
      *p++ = '|';
    }

  if (p == buf[sn])
    panic(sn,"no flags detected");

  *--p = '\0';
  return buf[sn];
}
#endif /* PRINT_SYMS */

/* qsort helper function */
static int
acmp(struct sym_sym_t **sym1, struct sym_sym_t **sym2)
{
  return (int)((*sym1)->addr - (*sym2)->addr);
}

/* qsort helper function */
static int
ncmp(struct sym_sym_t **sym1, struct sym_sym_t **sym2)
{
  return strcmp((*sym1)->name, (*sym2)->name);
}

#define RELEVANT_SCOPE(SYM)						\
(/* global symbol */							\
 ((SYM)->flags & BSF_GLOBAL)						\
 || (/* local symbol */							\
     (((SYM)->flags & (BSF_LOCAL|BSF_DEBUGGING)) == BSF_LOCAL)		\
     && (SYM)->name[0] != '$')						\
 || (/* compiler local */						\
     load_locals							\
     && ((/* basic block idents */					\
	  ((SYM)->flags&(BSF_LOCAL|BSF_DEBUGGING))==(BSF_LOCAL|BSF_DEBUGGING)\
	  && (SYM)->name[0] == '$')					\
	 || (/* local constant idents */				\
	     ((SYM)->flags & (BSF_LOCAL|BSF_DEBUGGING)) == (BSF_LOCAL)	\
	     && (SYM)->name[0] == '$'))))
     

/* load symbols out of FNAME */
void
sym_loadsyms(char *fname,	/* file name containing symbols */
	     int load_locals,	/* load local symbols */
             int sn)            /* slot number */
{
  int i, debug_cnt;
#ifdef BFD_LOADER
  bfd *abfd;
  asymbol **syms;
  int storage, i, nsyms, debug_cnt;
#else /* !BFD_LOADER */
  int len;
  FILE *fobj;
  struct ecoff_filehdr fhdr;
  struct ecoff_aouthdr ahdr;
  struct ecoff_symhdr_t symhdr;
  char *strtab = NULL;
  struct ecoff_EXTR *extr;
#endif /* BFD_LOADER */

  if (syms_loaded[sn])
    {
      /* symbols are already loaded */
      /* FIXME: can't handle symbols from multiple files */
      return;
    }

#ifdef BFD_LOADER

  /* load the program into memory, try both endians */
  if (!(abfd = bfd_openr(fname, "ss-coff-big")))
    if (!(abfd = bfd_openr(fname, "ss-coff-little")))
      fatal(sn,"cannot open executable `%s'", fname);

  /* this call is mainly for its side effect of reading in the sections.
     we follow the traditional behavior of `strings' in that we don't
     complain if we don't recognize a file to be an object file.  */
  if (!bfd_check_format(abfd, bfd_object))
    {
      bfd_close(abfd);
      fatal(sn,"cannot open executable `%s'", fname);
    }

  /* sanity check, endian should be the same as loader.c encountered */
  if (abfd->xvec->byteorder_big_p != (unsigned)ld_target_big_endian[sn])
    panic(sn,"binary endian changed");

  if ((bfd_get_file_flags(abfd) & (HAS_SYMS|HAS_LOCALS)))
    {
      /* file has locals, read them in */
      storage = bfd_get_symtab_upper_bound(abfd);
      if (storage <= 0)
	fatal(sn,"HAS_SYMS is set, but `%s' still lacks symbols", fname);

      syms = (asymbol **)calloc(storage, 1);
      if (!syms)
	fatal(sn,"out of virtual memory");

      nsyms = bfd_canonicalize_symtab (abfd, syms);
      if (nsyms <= 0)
	fatal(sn,"HAS_SYMS is set, but `%s' still lacks symbols", fname);

      /*
       * convert symbols to local format
       */

      /* first count symbols */
      sym_ndatasyms[sn] = 0; sym_ntextsyms[sn] = 0;
      for (i=0; i < nsyms; i++)
	{
	  asymbol *sym = syms[i];

	  /* decode symbol type */
	  if (/* from the data section */
	      (!strcmp(sym->section->name, ".rdata")
	       || !strcmp(sym->section->name, ".data")
	       || !strcmp(sym->section->name, ".sdata")
	       || !strcmp(sym->section->name, ".bss")
	       || !strcmp(sym->section->name, ".sbss"))
	      /* from a scope we are interested in */
	      && RELEVANT_SCOPE(sym))
	    {
	      /* data segment symbol */
	      sym_ndatasyms[sn]++;
#ifdef PRINT_SYMS
	      fprintf(stderr,
		      "+sym: %s  sect: %s  flags: %s  value: 0x%08lx\n",
		      sym->name, sym->section->name, flags2str(sym->flags,sn),
		      sym->value + sym->section->vma);
#endif /* PRINT_SYMS */
	    }
	  else if (/* from the text section */
		   !strcmp(sym->section->name, ".text")
		   /* from a scope we are interested in */
		   && RELEVANT_SCOPE(sym))
	    {
	      /* text segment symbol */
	      sym_ntextsyms[sn]++;
#ifdef PRINT_SYMS
	      fprintf(stderr,
		      "+sym: %s  sect: %s  flags: %s  value: 0x%08lx\n",
		      sym->name, sym->section->name, flags2str(sym->flags,sn),
		      sym->value + sym->section->vma);
#endif /* PRINT_SYMS */
	    }
	  else
	    {
	      /* non-segment sections */
#ifdef PRINT_SYMS
	      fprintf(stderr,
		      "-sym: %s  sect: %s  flags: %s  value: 0x%08lx\n",
		      sym->name, sym->section->name, flags2str(sym->flags,sn),
		      sym->value + sym->section->vma);
#endif /* PRINT_SYMS */
	    }
	}
      sym_nsyms[sn] = sym_ntextsyms[sn] + sym_ndatasyms[sn];
      if (sym_nsyms[sn] <= 0)
	fatal(sn,"`%s' has no text or data symbols", fname);

      /* allocate symbol space */
      sym_db[sn] = (struct sym_sym_t *)calloc(sym_nsyms[sn], sizeof(struct sym_sym_t));
      if (!sym_db[sn])
	fatal(sn,"out of virtual memory");

      /* convert symbols to internal format */
      for (debug_cnt=0, i=0; i < nsyms; i++)
	{
	  asymbol *sym = syms[i];

	  /* decode symbol type */
	  if (/* from the data section */
	      (!strcmp(sym->section->name, ".rdata")
	       || !strcmp(sym->section->name, ".data")
	       || !strcmp(sym->section->name, ".sdata")
	       || !strcmp(sym->section->name, ".bss")
	       || !strcmp(sym->section->name, ".sbss"))
	      /* from a scope we are interested in */
	      && RELEVANT_SCOPE(sym))
	    {
	      /* data segment symbol, insert into symbol database */
	      sym_db[sn][sn][debug_cnt].name = mystrdup((char *)sym->name);
	      sym_db[sn][debug_cnt].seg = ss_data;
	      sym_db[sn][debug_cnt].initialized =
		(!strcmp(sym->section->name, ".rdata")
		 || !strcmp(sym->section->name, ".data")
		 || !strcmp(sym->section->name, ".sdata"));
	      sym_db[sn][debug_cnt].pub = (sym->flags & BSF_GLOBAL);
	      sym_db[sn][debug_cnt].local = (sym->name[0] == '$');
	      sym_db[sn][debug_cnt].addr = sym->value + sym->section->vma;

	      debug_cnt++;
	    }
	  else if (/* from the text section */
		   !strcmp(sym->section->name, ".text")
		   /* from a scope we are interested in */
		   && RELEVANT_SCOPE(sym))
	    {
	      /* text segment symbol, insert into symbol database */
	      sym_db[sn][debug_cnt].name = mystrdup((char *)sym->name);
	      sym_db[sn][debug_cnt].seg = ss_text;
	      sym_db[sn][debug_cnt].initialized = /* seems reasonable */TRUE;
	      sym_db[sn][debug_cnt].pub = (sym->flags & BSF_GLOBAL);
	      sym_db[sn][debug_cnt].local = (sym->name[0] == '$');
	      sym_db[sn][debug_cnt].addr = sym->value + sym->section->vma;

	      debug_cnt++;
	    }
	  else
	    {
	      /* non-segment sections */
	    }
	}
      /* sanity check */
      if (debug_cnt != sym_nsyms[sn])
	panic(sn,"could not locate all counted symbols");

      /* release bfd symbol storage */
      free(syms);
    }

  /* done with file, close if */
  if (!bfd_close(abfd))
    fatal(sn,"could not close executable `%s'", fname);

#else /* !BFD_LOADER */

  /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
  fobj = fopen(fname, "rb");
#else
  fobj = fopen(fname, "r");
#endif
  if (!fobj)
    fatal(sn,"cannot open executable `%s'", fname);

  if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
    fatal(sn,"cannot read header from executable `%s'", fname);

  /* record endian of target */
  if (fhdr.f_magic != ECOFF_EB_MAGIC && fhdr.f_magic != ECOFF_EL_MAGIC)
    fatal(sn,"bad magic number in executable `%s'", fname);

  if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
    fatal(sn,"cannot read AOUT header from executable `%s'", fname);

  /* seek to the beginning of the symbolic header */
  fseek(fobj, fhdr.f_symptr, 0);

  if (fread(&symhdr, sizeof(struct ecoff_symhdr_t), 1, fobj) < 1)
    fatal(sn,"could not read symbolic header from executable `%s'", fname);

  if (symhdr.magic != ECOFF_magicSym)
    fatal(sn,"bad magic number (0x%x) in symbolic header", symhdr.magic);

  /* allocate space for the string table */
  len = symhdr.issMax + symhdr.issExtMax;
  strtab = (char *)calloc(len, sizeof(char));
  if (!strtab)
    fatal(sn,"out of virtual memory");

  /* read all the symbol names into memory */
  fseek(fobj, symhdr.cbSsOffset, 0);
  if (fread(strtab, len, 1, fobj) < 0)
    fatal(sn,"error while reading symbol table names");

  /* allocate symbol space */
  len = symhdr.isymMax + symhdr.iextMax;
  if (len <= 0)
    fatal(sn,"`%s' has no text or data symbols", fname);
  sym_db[sn] = (struct sym_sym_t *)calloc(len, sizeof(struct sym_sym_t));
  if (!sym_db[sn])
    fatal(sn,"out of virtual memory");

  /* allocate space for the external symbol entries */
  extr =
    (struct ecoff_EXTR *)calloc(symhdr.iextMax, sizeof(struct ecoff_EXTR));
  if (!extr)
    fatal(sn,"out of virtual memory");

  fseek(fobj, symhdr.cbExtOffset, 0);
  if (fread(extr, sizeof(struct ecoff_EXTR), symhdr.iextMax, fobj) < 0)
    fatal(sn,"error reading external symbol entries");

  sym_nsyms[sn] = 0; sym_ndatasyms[sn] = 0; sym_ntextsyms[sn] = 0;

  /* convert symbols to internal format */
  for (i=0; i < symhdr.iextMax; i++)
    {
      int str_offset;

      str_offset = symhdr.issMax + extr[i].asym.iss;

#if 0
      printf("ext %2d: ifd = %2d, iss = %3d, value = %8x, st = %3x, "
	     "sc = %3x, index = %3x\n",
	     i, extr[i].ifd,
	     extr[i].asym.iss, extr[i].asym.value,
	     extr[i].asym.st, extr[i].asym.sc,
	     extr[i].asym.index);
      printf("       %08x %2d %2d %s\n",
	     extr[i].asym.value,
	     extr[i].asym.st,
	     extr[i].asym.sc,
	     &strtab[str_offset]);
#endif

      switch (extr[i].asym.st)
	{
	case ECOFF_stGlobal:
	case ECOFF_stStatic:
	  /* from data segment */
	  sym_db[sn][sym_nsyms[sn]].name = mystrdup(&strtab[str_offset]);
	  sym_db[sn][sym_nsyms[sn]].seg = ss_data;
	  sym_db[sn][sym_nsyms[sn]].initialized = /* FIXME: ??? */TRUE;
	  sym_db[sn][sym_nsyms[sn]].pub = /* FIXME: ??? */TRUE;
	  sym_db[sn][sym_nsyms[sn]].local = /* FIXME: ??? */FALSE;
	  sym_db[sn][sym_nsyms[sn]].addr = extr[i].asym.value;
	  sym_nsyms[sn]++;
	  sym_ndatasyms[sn]++;
	  break;

	case ECOFF_stProc:
	case ECOFF_stStaticProc:
	case ECOFF_stLabel:
	  /* from text segment */
	  sym_db[sn][sym_nsyms[sn]].name = mystrdup(&strtab[str_offset]);
	  sym_db[sn][sym_nsyms[sn]].seg = ss_text;
	  sym_db[sn][sym_nsyms[sn]].initialized = /* FIXME: ??? */TRUE;
	  sym_db[sn][sym_nsyms[sn]].pub = /* FIXME: ??? */TRUE;
	  sym_db[sn][sym_nsyms[sn]].local = /* FIXME: ??? */FALSE;
	  sym_db[sn][sym_nsyms[sn]].addr = extr[i].asym.value;
	  sym_nsyms[sn]++;
	  sym_ntextsyms[sn]++;
	  break;

	default:
	  /* FIXME: ignored... */;
	}
    }
  free(extr);

  /* done with the executable, close it */
  if (fclose(fobj))
    fatal(sn,"could not close executable `%s'", fname);

#endif /* BFD_LOADER */

  /*
   * generate various sortings
   */

  /* all symbols sorted by address and name */
  sym_syms[sn] =
    (struct sym_sym_t **)calloc(sym_nsyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_syms[sn])
    fatal(sn,"out of virtual memory");

  sym_syms_by_name[sn] =
    (struct sym_sym_t **)calloc(sym_nsyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_syms_by_name[sn])
    fatal(sn,"out of virtual memory");

  for (debug_cnt=0, i=0; i<sym_nsyms[sn]; i++)
    {
      sym_syms[sn][debug_cnt] = &sym_db[sn][i];
      sym_syms_by_name[sn][debug_cnt] = &sym_db[sn][i];
      debug_cnt++;
    }
  /* sanity check */
  if (debug_cnt != sym_nsyms[sn])
    panic(sn,"could not locate all symbols");

  /* sort by address */
  qsort(sym_syms[sn], sym_nsyms[sn], sizeof(struct sym_sym_t *), (void *)acmp);

  /* sort by name */
  qsort(sym_syms_by_name[sn], sym_nsyms[sn], sizeof(struct sym_sym_t *), (void *)ncmp);

  /* text segment sorted by address and name */
  sym_textsyms[sn] =
    (struct sym_sym_t **)calloc(sym_ntextsyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_textsyms[sn])
    fatal(sn,"out of virtual memory");

  sym_textsyms_by_name[sn] =
    (struct sym_sym_t **)calloc(sym_ntextsyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_textsyms_by_name[sn])
    fatal(sn,"out of virtual memory");

  for (debug_cnt=0, i=0; i<sym_nsyms[sn]; i++)
    {
      if (sym_db[sn][i].seg == ss_text)
	{
	  sym_textsyms[sn][debug_cnt] = &sym_db[sn][i];
	  sym_textsyms_by_name[sn][debug_cnt] = &sym_db[sn][i];
	  debug_cnt++;
	}
    }
  /* sanity check */
  if (debug_cnt != sym_ntextsyms[sn])
    panic(sn,"could not locate all text symbols");

  /* sort by address */
  qsort(sym_textsyms[sn], sym_ntextsyms[sn], sizeof(struct sym_sym_t *), (void *)acmp);

  /* sort by name */
  qsort(sym_textsyms_by_name[sn], sym_ntextsyms[sn],
	sizeof(struct sym_sym_t *), (void *)ncmp);

  /* data segment sorted by address and name */
  sym_datasyms[sn] =
    (struct sym_sym_t **)calloc(sym_ndatasyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_datasyms[sn])
    fatal(sn,"out of virtual memory");

  sym_datasyms_by_name[sn] =
    (struct sym_sym_t **)calloc(sym_ndatasyms[sn], sizeof(struct sym_sym_t *));
  if (!sym_datasyms_by_name[sn])
    fatal(sn,"out of virtual memory");

  for (debug_cnt=0, i=0; i<sym_nsyms[sn]; i++)
    {
      if (sym_db[sn][i].seg == ss_data)
	{
	  sym_datasyms[sn][debug_cnt] = &sym_db[sn][i];
	  sym_datasyms_by_name[sn][debug_cnt] = &sym_db[sn][i];
	  debug_cnt++;
	}
    }
  /* sanity check */
  if (debug_cnt != sym_ndatasyms[sn])
    panic(sn,"could not locate all data symbols");
      
  /* sort by address */
  qsort(sym_datasyms[sn], sym_ndatasyms[sn], sizeof(struct sym_sym_t *), (void *)acmp);

  /* sort by name */
  qsort(sym_datasyms_by_name[sn], sym_ndatasyms[sn],
	sizeof(struct sym_sym_t *), (void *)ncmp);

  /* compute symbol sizes */
  for (i=0; i<sym_ntextsyms[sn]; i++)
    {
      sym_textsyms[sn][i]->size =
	(i != (sym_ntextsyms[sn] - 1)
	 ? (sym_textsyms[sn][i+1]->addr - sym_textsyms[sn][i]->addr)
	 : ((ld_text_base[sn] + ld_text_size[sn]) - sym_textsyms[sn][i]->addr));
    }
  for (i=0; i<sym_ndatasyms[sn]; i++)
    {
      sym_datasyms[sn][i]->size =
	(i != (sym_ndatasyms[sn] - 1)
	 ? (sym_datasyms[sn][i+1]->addr - sym_datasyms[sn][i]->addr)
	 : ((ld_data_base[sn] + ld_data_size[sn]) - sym_datasyms[sn][i]->addr));
    }

  /* symbols are now available for use */
  syms_loaded[sn] = TRUE;
}

/* dump symbol SYM to output stream FD */
void
sym_dumpsym(struct sym_sym_t *sym,	/* symbol to display */
	    FILE *fd)			/* output stream */
{
  fprintf(fd,
    "sym `%s': %s seg, init-%s, pub-%s, local-%s, addr=0x%08x, size=%d\n",
	  sym->name,
	  sym->seg == ss_data ? "data" : "text",
	  sym->initialized ? "y" : "n",
	  sym->pub ? "y" : "n",
	  sym->local ? "y" : "n",
	  sym->addr,
	  sym->size);
}

/* dump all symbols to output stream FD */
void
sym_dumpsyms(FILE *fd, int sn)			/* output stream */
{
  int i;

  for (i=0; i < sym_nsyms[sn]; i++)
    sym_dumpsym(sym_syms[sn][i], fd);
}

/* dump all symbol state to output stream FD */
void
sym_dumpstate(FILE *fd, int sn)			/* output stream */
{
  int i;

  if (fd == NULL)
    fd = stderr;

  fprintf(fd, "** All symbols sorted by address:\n");
  for (i=0; i < sym_nsyms[sn]; i++)
    sym_dumpsym(sym_syms[sn][i], fd);

  fprintf(fd, "\n** All symbols sorted by name:\n");
  for (i=0; i < sym_nsyms[sn]; i++)
    sym_dumpsym(sym_syms_by_name[sn][i], fd);

  fprintf(fd, "** Text symbols sorted by address:\n");
  for (i=0; i < sym_ntextsyms[sn]; i++)
    sym_dumpsym(sym_textsyms[sn][i], fd);

  fprintf(fd, "\n** Text symbols sorted by name:\n");
  for (i=0; i < sym_ntextsyms[sn]; i++)
    sym_dumpsym(sym_textsyms_by_name[sn][i], fd);

  fprintf(fd, "** Data symbols sorted by address:\n");
  for (i=0; i < sym_ndatasyms[sn]; i++)
    sym_dumpsym(sym_datasyms[sn][i], fd);

  fprintf(fd, "\n** Data symbols sorted by name:\n");
  for (i=0; i < sym_ndatasyms[sn]; i++)
    sym_dumpsym(sym_datasyms_by_name[sn][i], fd);
}

/* bind address ADDR to a symbol in symbol database DB, the address must
   match exactly if EXACT is non-zero, the index of the symbol in the
   requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *			/* symbol found, or NULL */
sym_bind_addr(md_addr_t addr,		/* address of symbol to locate */
	      int *pindex,		/* ptr to index result var */
	      int exact,		/* require exact address match? */
	      enum sym_db_t db,		/* symbol database to search */
              int sn)                   /* slot number */
{
  int nsyms, low, high, pos;
  struct sym_sym_t **syms;

  switch (db)
    {
    case sdb_any:
      syms = sym_syms[sn];
      nsyms = sym_nsyms[sn];
      break;
    case sdb_text:
      syms = sym_textsyms[sn];
      nsyms = sym_ntextsyms[sn];
      break;
    case sdb_data:
      syms = sym_datasyms[sn];
      nsyms = sym_ndatasyms[sn];
      break;
    default:
      panic(sn,"bogus symbol database");
    }

  /* any symbols to search? */
  if (!nsyms)
    {
      if (pindex)
	*pindex = -1;
      return NULL;
    }

  /* binary search symbol database (sorted by address) */
  low = 0;
  high = nsyms-1;
  pos = (low + high) >> 1;
  while (!(/* exact match */
	   (exact && syms[pos]->addr == addr)
	   /* in bounds match */
	   || (!exact
	       && syms[pos]->addr <= addr
	       && addr < (syms[pos]->addr + MAX(1, syms[pos]->size)))))
    {
      if (addr < syms[pos]->addr)
	high = pos - 1;
      else
	low = pos + 1;
      if (high >= low)
	pos = (low + high) >> 1;
      else
	{
	  if (pindex)
	    *pindex = -1;
	  return NULL;
	}
    }

  /* bound! */
  if (pindex)
    *pindex = pos;
  return syms[pos];
}

/* bind name NAME to a symbol in symbol database DB, the index of the symbol
   in the requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *				/* symbol found, or NULL */
sym_bind_name(char *name,			/* symbol name to locate */
	      int *pindex,			/* ptr to index result var */
	      enum sym_db_t db,			/* symbol database to search */
              int sn)
{
  int nsyms, low, high, pos, cmp;
  struct sym_sym_t **syms;

  switch (db)
    {
    case sdb_any:
      syms = sym_syms_by_name[sn];
      nsyms = sym_nsyms[sn];
      break;
    case sdb_text:
      syms = sym_textsyms_by_name[sn];
      nsyms = sym_ntextsyms[sn];
      break;
    case sdb_data:
      syms = sym_datasyms_by_name[sn];
      nsyms = sym_ndatasyms[sn];
      break;
    default:
      panic(sn,"bogus symbol database");
    }

  /* any symbols to search? */
  if (!nsyms)
    {
      if (pindex)
	*pindex = -1;
      return NULL;
    }

  /* binary search symbol database (sorted by name) */
  low = 0;
  high = nsyms-1;
  pos = (low + high) >> 1;
  while (!(/* exact string match */!(cmp = strcmp(syms[pos]->name, name))))
    {
      if (cmp > 0)
	high = pos - 1;
      else
	low = pos + 1;
      if (high >= low)
	pos = (low + high) >> 1;
      else
	{
	  if (pindex)
	    *pindex = -1;
	  return NULL;
	}
    }

  /* bound! */
  if (pindex)
    *pindex = pos;
  return syms[pos];
}

/* to intialize the replicated global variables from sim_init */

void symbol_init_declarations(void) {
   int sn;

   for (sn=0;sn<process_num;sn++) {
       sym_db[sn] = NULL;
       sym_nsyms[sn] = 0;
       sym_syms[sn] = NULL;
       sym_syms_by_name[sn] = NULL;
       sym_ntextsyms[sn] = 0;
       sym_textsyms[sn] = NULL;
       sym_textsyms_by_name[sn] = NULL;
       sym_ndatasyms[sn] = 0;
       sym_datasyms[sn] = NULL;
       sym_datasyms_by_name[sn] = NULL;
       syms_loaded[sn] = FALSE;
   }
}
