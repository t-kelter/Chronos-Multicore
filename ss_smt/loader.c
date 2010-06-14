/*
 * modified loader.c - program loader routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original loader.c file from SimpleScalar 3.0 Tool.
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
 * The original loader.c file is a part of the SimpleScalar tool suite written by
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
 * The original loader.c was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 *
 * $Id: loader.c,v 1.7 1998/08/27 16:57:46 taustin Exp taustin $
 *
 * $Log: loader.c,v $
 * Revision 1.7  1998/08/27 16:57:46  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 * external event I/O (EIO) tracing and checkpointing added
 * improved loader error messages, e.g., loading Alpha binaries on
 *       PISA-configured simulator indicates such
 * fixed a BFD/non-BFD loader problem where tail padding in the text
 *       segment was not correctly allocated in simulator memory; when
 *       padding region happened to cross a page boundary the final text
 *       page has a NULL pointer in mem_table, resulting in a SEGV when
 *       trying to access it in intruction predecoding
 * instruction predecoding moved to loader module
 *
 * Revision 1.6  1997/04/16  22:09:05  taustin
 * added standalone loader support
 *
 * Revision 1.5  1997/03/11  01:12:39  taustin
 * updated copyright
 * swapping supported disabled until it can be tested further
 *
 * Revision 1.4  1997/01/06  15:59:22  taustin
 * stat_reg calls now do not initialize stat variable values
 * ld_prog_fname variable exported
 *
 * Revision 1.3  1996/12/27  15:51:28  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "eio.h"
#include "loader.h"

#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */
#include "ecoff.h"
#endif /* BFD_LOADER */

/* amount of tail padding added to all loaded text segments */
#define TEXT_TAIL_PADDING 128

/* program text (code) segment base. See sim_init for initialization */
md_addr_t ld_text_base[MAX_SLOTS];

/* program text (code) size in bytes. See sim_init for initialization  */
unsigned int ld_text_size[MAX_SLOTS];

/* program initialized data segment base. See sim_init for initialization  */
md_addr_t ld_data_base[MAX_SLOTS];

/* program initialized ".data" and uninitialized ".bss" size in bytes. See sim_init for initialization */
unsigned int ld_data_size[MAX_SLOTS];

/* top of the data segment. See sim_init for initialization */
md_addr_t ld_brk_point[MAX_SLOTS];

/* program stack segment base (highest address in stack). See sim_init for initialization */
md_addr_t ld_stack_base[MAX_SLOTS];

/* program initial stack size. See sim_init for initialization  */
unsigned int ld_stack_size[MAX_SLOTS];

/* lowest address accessed on the stack. See sim_init for initialization */
md_addr_t ld_stack_min[MAX_SLOTS];

/* program file name. See sim_init for initialization */
char *ld_prog_fname[MAX_SLOTS] = {NULL};

/* program entry point (initial PC). See sim_init for initialization */
md_addr_t ld_prog_entry[MAX_SLOTS];

/* program environment base address address. See sim_init for initializatio */
md_addr_t ld_environ_base[MAX_SLOTS];

/* target executable endian-ness, non-zero if big endian */
int ld_target_big_endian[MAX_SLOTS];

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb)	/* stats data base */
{ char stat_name[128];
  int sn; /* slot number */

  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_text_base_%s", itoa[sn]); 
     stat_reg_addr(sdb, stat_name,
	   	"program text (code) segment base",
		&ld_text_base[sn], ld_text_base[sn], "  0x%08p",sn); 
  }
 
  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_text_size_%s", itoa[sn]); 
     stat_reg_uint(sdb, stat_name,
		"program text (code) size in bytes",
		&ld_text_size[sn], ld_text_size[sn], NULL,sn);
  }

  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_data_base_%s", itoa[sn]);
     stat_reg_addr(sdb, stat_name,
		"program initialized data segment base",
		&ld_data_base[sn], ld_data_base[sn], "  0x%08p",sn);
  }

  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_data_size_%s", itoa[sn]);
     stat_reg_uint(sdb, stat_name,
		"program init'ed `.data' and uninit'ed `.bss' size in bytes",
		&ld_data_size[sn], ld_data_size[sn], NULL,sn);
  }

  for (sn=0;sn<process_num;sn++) { 
     sprintf(stat_name, "ld_stack_base_%s", itoa[sn]);;
     stat_reg_addr(sdb, stat_name,
		"program stack segment base (highest address in stack)",
		&ld_stack_base[sn], ld_stack_base[sn], "  0x%08p",sn);
  }
  
  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_stack_size_%s", itoa[sn]);
     stat_reg_uint(sdb, stat_name,
		"program initial stack size",
		&ld_stack_size[sn], ld_stack_size[sn], NULL,sn);
  }

#if 0 /* FIXME: broken... */
  stat_reg_addr(sdb, "ld_stack_min_%s",
		"program stack segment lowest address",
		&ld_stack_min[sn], ld_stack_min[sn], "  0x%08p",sn);
#endif  

  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_prog_entry_%s", itoa[sn]);
     stat_reg_addr(sdb, stat_name,
		"program entry point (initial PC)",
		&ld_prog_entry[sn], ld_prog_entry[sn], "  0x%08p",sn);
  }

  for (sn=0;sn<process_num;sn++) { 
     sprintf(stat_name, "ld_environ_base_%s", itoa[sn]);
     stat_reg_addr(sdb, stat_name,
		"program environment base address address",
		&ld_environ_base[sn], ld_environ_base[sn], "  0x%08p",sn);
  }

  for (sn=0;sn<process_num;sn++) {
     sprintf(stat_name, "ld_target_big_endian_%s", itoa[sn]);
     stat_reg_int(sdb, stat_name,
	       "target executable endian-ness, non-zero if big endian",
	       &ld_target_big_endian[sn], ld_target_big_endian[sn], NULL,sn);
  }
}


/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,		/* program to load */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     struct regs_t *regs,	/* registers to initialize for load */
	     struct mem_t *mem,		/* memory space to load prog into */
	     int zero_bss_segs,		/* zero uninit data segment? */
             int sn)                    /* slot number */
{
  int i;
  word_t temp;
  md_addr_t sp, data_break = 0, null_ptr = 0, argv_addr, envp_addr;


  if (eio_valid(fname))
    {
      if (argc != 1)
	{
	  fprintf(stderr, "error: EIO file has arguments\n");
	  exit(1);
	}

      fprintf(stderr, "sim: loading EIO file: %s\n", fname);

      sim_eio_fname[sn] = mystrdup(fname);

      /* open the EIO file stream */
      sim_eio_fd[sn] = eio_open(fname,sn);

      /* load initial state checkpoint */
      if (eio_read_chkpt(regs, mem, sim_eio_fd[sn],sn) != -1)
	fatal(sn,"bad initial checkpoint in EIO file");

      /* load checkpoint? */
      if (sim_chkpt_fname != NULL)
	{
	  counter_t restore_icnt;

	  FILE *chkpt_fd;

	  fprintf(stderr, "sim: loading checkpoint file: %s\n",
		  sim_chkpt_fname);

	  if (!eio_valid(sim_chkpt_fname))
	    fatal(sn,"file `%s' does not appear to be a checkpoint file",
		  sim_chkpt_fname);

	  /* open the checkpoint file */
	  chkpt_fd = eio_open(sim_chkpt_fname,sn);

	  /* load the state image */
	  restore_icnt = eio_read_chkpt(regs, mem, chkpt_fd,sn);

	  /* fast forward the baseline EIO trace to checkpoint location */
	  fprintf(stderr, "sim: fast forwarding to instruction %d\n",
		  (int)restore_icnt);
	  eio_fast_forward(sim_eio_fd[sn], restore_icnt,sn);
	}

      /* computed state... */
      ld_environ_base[sn] = regs->regs_R[MD_REG_SP];
      ld_prog_entry[sn] = regs->regs_PC;

      /* fini... */
      return;
    }

  if (sim_chkpt_fname != NULL)
    fatal(sn,"checkpoints only supported while EIO tracing");

#ifdef BFD_LOADER

  {
    bfd *abfd;
    asection *sect;
  
    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base[sn] = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size[sn] = ld_stack_base[sn] - sp;

    /* initial stack pointer value */
    ld_environ_base[sn] = sp;

    /* load the program into memory, try both endians */
    if (!(abfd = bfd_openr(argv[0], "ss-coff-big")))
      if (!(abfd = bfd_openr(argv[0], "ss-coff-little")))
	fatal(sn,"cannot open executable `%s'", argv[0]);

    /* this call is mainly for its side effect of reading in the sections.
       we follow the traditional behavior of `strings' in that we don't
       complain if we don't recognize a file to be an object file.  */
    if (!bfd_check_format(abfd, bfd_object))
      {
	bfd_close(abfd);
	fatal(sn,"cannot open executable `%s'", argv[0]);
      }

    /* record profile file name */
    ld_prog_fname[sn] = argv[0];

    /* record endian of target */
    ld_target_big_endian[sn] = abfd->xvec->byteorder_big_p;

    debug(sn,"processing %d sections in `%s'...",
	  bfd_count_sections(abfd), argv[0]);

    /* read all sections in file */
    for (sect=abfd->sections; sect; sect=sect->next)
      {
	char *p;

	debug(sn,"processing section `%s', %d bytes @ 0x%08x...",
	      bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
	      bfd_section_vma(abfd, sect));

	/* read the section data, if allocated and loadable and non-NULL */
	if ((bfd_get_section_flags(abfd, sect) & SEC_ALLOC)
	    && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
	    && bfd_section_vma(abfd, sect)
	    && bfd_section_size(abfd, sect))
	  {
	    /* allocate a section buffer */
	    p = calloc(bfd_section_size(abfd, sect), sizeof(char));
	    if (!p)
	      fatal(sn,"cannot allocate %d bytes for section `%s'",
		    bfd_section_size(abfd, sect),
		    bfd_section_name(abfd, sect));

	    if (!bfd_get_section_contents(abfd, sect, p, (file_ptr)0,
					  bfd_section_size(abfd, sect)))
	      fatal(sn,"could not read entire `%s' section from executable",
		    bfd_section_name(abfd, sect));

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, bfd_section_vma(abfd, sect),
		      p, bfd_section_size(abfd, sect),sn);

	    /* release the section buffer */
	    free(p);
	  }
	/* zero out the section if it is loadable but not allocated in exec */
	else if (zero_bss_segs
		 && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
		 && bfd_section_vma(abfd, sect)
		 && bfd_section_size(abfd, sect))
	  {
	    /* zero out the section region */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect),
		      bfd_section_size(abfd, sect),sn);
	  }
	else
	  {
	    /* else do nothing with this section, it's probably debug data */
	    debug(sn,"ignoring section `%s' during load...",
		  bfd_section_name(abfd, sect));
	  }

	/* expected text section */
	if (!strcmp(bfd_section_name(abfd, sect), ".text"))
	  {
	    /* .text section processing */

	    ld_text_size[sn] =
	      ((bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect))
	       - MD_TEXT_BASE)
		+ /* for speculative fetches/decodes */TEXT_TAIL_PADDING;

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect)
		      + bfd_section_size(abfd, sect),
		      TEXT_TAIL_PADDING,sn);
	  }
	/* expected data sections */
	else if (!strcmp(bfd_section_name(abfd, sect), ".rdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".data")
		 || !strcmp(bfd_section_name(abfd, sect), ".sdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".bss")
		 || !strcmp(bfd_section_name(abfd, sect), ".sbss"))
	  {
	    /* data section processing */
	    if (bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect) >
		data_break)
	      data_break = (bfd_section_vma(abfd, sect) +
			    bfd_section_size(abfd, sect));
	  }
	else
	  {
	    /* what is this section??? */
	    fatal(sn,"encountered unknown section `%s', %d bytes @ 0x%08x",
		  bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
		  bfd_section_vma(abfd, sect));
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base[sn] = MD_TEXT_BASE;
    ld_data_base[sn] = MD_DATA_BASE;
    ld_prog_entry[sn] = bfd_get_start_address(abfd);
    ld_data_size[sn] = data_break - ld_data_base[sn];

    /* done with the executable, close it */
    if (!bfd_close(abfd))
      fatal(sn,"could not close executable `%s'", argv[0]);
  }

#else /* !BFD_LOADER, i.e., standalone loader */

  {
    FILE *fobj;
    long floc;
    struct ecoff_filehdr fhdr;
    struct ecoff_aouthdr ahdr;
    struct ecoff_scnhdr shdr;
    md_addr_t data_base;


    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base[sn] = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size[sn] = ld_stack_base[sn] - sp;

    /* initial stack pointer value */
    ld_environ_base[sn]= sp;

    /* record profile file name */
    ld_prog_fname[sn] = argv[0];

    /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
    fobj = fopen(argv[0], "rb");
#else
    fobj = fopen(argv[0], "r");
#endif
    if (!fobj)
      fatal(sn,"cannot open executable `%s'", argv[0]);

    if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
      fatal(sn,"cannot read header from executable `%s'", argv[0]);

    /* record endian of target */
    if (fhdr.f_magic == ECOFF_EB_MAGIC)
      ld_target_big_endian[sn] = TRUE;
    else if (fhdr.f_magic == ECOFF_EL_MAGIC)
      ld_target_big_endian[sn] = FALSE;
    else if (fhdr.f_magic == ECOFF_EB_OTHER || fhdr.f_magic == ECOFF_EL_OTHER)
      fatal(sn,"PISA binary `%s' has wrong endian format", argv[0]);
    else if (fhdr.f_magic == ECOFF_ALPHAMAGIC)
      fatal(sn,"PISA simulator cannot run Alpha binary `%s'", argv[0]);
    else
      fatal(sn,"bad magic number in executable `%s' (not an executable?)",
	    argv[0]);

    if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
      fatal(sn,"cannot read AOUT header from executable `%s'", argv[0]);

    data_base = MD_DATA_BASE;

    data_break = data_base + ahdr.dsize + ahdr.bsize;

#if 0
    Data_start = ahdr.data_start;
    Data_size = ahdr.dsize;
    Bss_size = ahdr.bsize;
    Bss_start = ahdr.bss_start;
    Gp_value = ahdr.gp_value;
    Text_entry = ahdr.entry;
#endif

    /* seek to the beginning of the first section header, the file header comes
       first, followed by the optional header (this is the aouthdr), the size
       of the aouthdr is given in Fdhr.f_opthdr */
    fseek(fobj, sizeof(struct ecoff_filehdr) + fhdr.f_opthdr, 0);

    debug(sn,"processing %d sections in `%s'...", fhdr.f_nscns, argv[0]);

    /* loop through the section headers */
    floc = ftell(fobj);
    for (i = 0; i < fhdr.f_nscns; i++)
      {
	char *p;

	if (fseek(fobj, floc, 0) == -1)
	  fatal(sn,"could not reset location in executable");
	if (fread(&shdr, sizeof(struct ecoff_scnhdr), 1, fobj) < 1)
	  fatal(sn,"could not read section %d from executable", i);
	floc = ftell(fobj);

	switch (shdr.s_flags)
	  { 
	  case ECOFF_STYP_TEXT:

	    ld_text_size[sn] = ((shdr.s_vaddr + shdr.s_size) - MD_TEXT_BASE) 
	      + TEXT_TAIL_PADDING;

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal(sn,"out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal(sn,"could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal(sn,"could not read text section from executable");

	    /* copy program section into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size,sn);

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      shdr.s_vaddr + shdr.s_size, TEXT_TAIL_PADDING,sn);
  
	    /* release the section buffer */
	    free(p);

#if 0
	    Text_seek = shdr.s_scnptr;
	    Text_start = shdr.s_vaddr;
	    Text_size = shdr.s_size / 4;
	    /* there is a null routine after the supposed end of text */
	    Text_size += 10;
	    Text_end = Text_start + Text_size * 4;
	    /* create_text_reloc(shdr.s_relptr, shdr.s_nreloc); */
#endif
	    break;

	  case ECOFF_STYP_RDATA:
	    /* The .rdata section is sometimes placed before the text
	     * section instead of being contiguous with the .data section.
	     */
#if 0
	    Rdata_start = shdr.s_vaddr;
	    Rdata_size = shdr.s_size;
	    Rdata_seek = shdr.s_scnptr;
#endif
	    /* fall through */
	  case ECOFF_STYP_DATA:
#if 0
	    Data_seek = shdr.s_scnptr;
#endif
	    /* fall through */
	  case ECOFF_STYP_SDATA:
#if 0
	    Sdata_seek = shdr.s_scnptr;
#endif

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal(sn,"out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal(sn,"could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal(sn,"could not read text section from executable");

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size,sn);

	    /* release the section buffer */
	    free(p);

	    break;

	  case ECOFF_STYP_BSS:
	    break;

	  case ECOFF_STYP_SBSS:
	    break;
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base[sn] = MD_TEXT_BASE;
    ld_data_base[sn] = MD_DATA_BASE;
    ld_prog_entry[sn] = ahdr.entry;
    ld_data_size[sn] = data_break - ld_data_base[sn];

    /* done with the executable, close it */
    if (fclose(fobj))
      fatal(sn,"could not close executable `%s'", argv[0]);
  }
#endif /* BFD_LOADER */

  /* perform sanity checks on segment ranges */
  if (!ld_text_base[sn] || !ld_text_size[sn])
    fatal(sn,"executable is missing a `.text' section");
  if (!ld_data_base[sn] || !ld_data_size[sn])
    fatal(sn,"executable is missing a `.data' section");
  if (!ld_prog_entry[sn])
    fatal(sn,"program entry point not specified");

  /* determine byte/words swapping required to execute on this host */
  sim_swap_bytes[sn] = (endian_host_byte_order(sn) != endian_target_byte_order(sn));
  if (sim_swap_bytes[sn])
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping bytes to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal(sn,"binary endian does not match host endian");
#endif
    }
  sim_swap_words[sn] = (endian_host_word_order() != endian_target_word_order(sn));
  if (sim_swap_words[sn])
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping words to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal(sn,"binary endian does not match host endian");
#endif
    }


  /* write [argc] to stack */
  temp = SWAP_WORD(argc);
  mem_access(mem, Write, sp, &temp, sizeof(word_t),sn);
  sp += sizeof(word_t);

  /* skip past argv array and NULL */
  argv_addr = sp;
  sp = sp + (argc + 1) * sizeof(md_addr_t);

  /* save space for envp array and NULL */
  envp_addr = sp;
  for (i=0; envp[i]; i++)
    sp += sizeof(md_addr_t);
  sp += sizeof(md_addr_t);


  /* fill in the argv pointer array and data */
  for (i=0; i<argc; i++)
    {
      /* write the argv pointer array entry */
      temp = SWAP_WORD(sp);
      mem_access(mem, Write, argv_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t),sn);
      /* and the data */
      mem_strcpy(mem_access, mem, Write, sp, argv[i],sn);
      sp += strlen(argv[i]) + 1;

    }


  /* terminate argv array with a NULL */
  mem_access(mem, Write, argv_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t),sn);


  /* write envp pointer array and data to stack */
  for (i = 0; envp[i]; i++)
    {
      /* write the envp pointer array entry */
      temp = SWAP_WORD(sp);
      mem_access(mem, Write, envp_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t),sn);
      /* and the data */
      mem_strcpy(mem_access, mem, Write, sp, envp[i],sn);
      sp += strlen(envp[i]) + 1;
    }


  /* terminate the envp array with a NULL */
  mem_access(mem, Write, envp_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t),sn);

  /* did we tromp off the stop of the stack? */
  if (sp > ld_stack_base[sn])
    {
      /* we did, indicate to the user that MD_MAX_ENVIRON must be increased,
	 alternatively, you can use a smaller environment, or fewer
	 command line arguments */
      fatal(sn,"environment overflow, increase MD_MAX_ENVIRON in ss.h");
    }

  /* initialize the bottom of heap to top of data segment */
  ld_brk_point[sn] = ROUND_UP(ld_data_base[sn] + ld_data_size[sn], MD_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  ld_stack_min[sn] = regs->regs_R[MD_REG_SP];

  /* set up initial register state */
  regs->regs_R[MD_REG_SP] = ld_environ_base[sn];
  regs->regs_PC = ld_prog_entry[sn];

  debug(sn,"ld_text_base: 0x%08x  ld_text_size: 0x%08x",
	ld_text_base[sn], ld_text_size[sn]);
  debug(sn,"ld_data_base: 0x%08x  ld_data_size: 0x%08x",
	ld_data_base[sn], ld_data_size[sn]);
  debug(sn,"ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
	ld_stack_base[sn], ld_stack_size[sn]);
  debug(sn,"ld_prog_entry: 0x%08x", ld_prog_entry[sn]);

  /* finally, predecode the text segment... */
  {
    md_addr_t addr;
    md_inst_t inst;
    enum md_fault_type fault;

    if (OP_MAX > 255)
      fatal(sn,"cannot perform fast decoding, too many opcodes");


    debug(sn,"sim: decoding text segment...");
    for (addr=ld_text_base[sn];
	 addr < (ld_text_base[sn]+ld_text_size[sn]);
	 addr += sizeof(md_inst_t))
      {
	fault = mem_access(mem, Read, addr, &inst, sizeof(inst),sn);
	if (fault != md_fault_none)
	  fatal(sn,"could not read instruction memory");
	inst.a = (inst.a & ~0xff) | (word_t)MD_OP_ENUM(MD_OPFIELD(inst));
	fault = mem_access(mem, Write, addr, &inst, sizeof(inst),sn);
	if (fault != md_fault_none)
	  fatal(sn,"could not write instruction memory");
      }

  }
}

void loader_init_declarations(void) {
   int sn;

   for (sn=0;sn<process_num;sn++) {
        ld_text_base[sn] = 0;
        ld_text_size[sn] = 0;
        ld_data_base[sn] = 0;
        ld_data_size[sn] = 0;
        ld_brk_point[sn] = 0;
        ld_stack_base[sn] = MD_STACK_BASE;
        ld_stack_size[sn] = 0;
        ld_stack_min[sn] = (md_addr_t)-1;
        ld_prog_entry[sn] = 0;
        ld_environ_base[sn] = 0;
        ld_prog_fname[sn] = NULL;    
   }
}
