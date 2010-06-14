/*
 * modified libexo.c - EXO library main line routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original libexo.c file from SimpleScalar 3.0 Tool.
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

 * The original libexo.c file is a part of the SimpleScalar tool suite written by
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
 * The original libexo.c was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 *
 * $Id: libexo.c,v 1.1 1998/08/27 08:55:06 taustin Exp taustin $
 *
 * $Log: libexo.c,v $
 * Revision 1.1  1998/08/27 08:55:06  taustin
 * Initial revision
 *
 * Revision 1.1  1998/08/27 08:51:36  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "libexo.h"

/* EXO term classes print strings */
char *exo_class_str[ec_NUM] = {
  "integer",
  "address",
  "float",
  "char",
  "string"
  "list",
  "array",
  "token",
  "blob"
};

/* return the value of an escape sequence, ESCAPE is a pointer to the first
   character following '\', sets NEXT to first character after escape */
/* A2.5.2 */
static int
intern_escape(char *esc, char **next, int sn)
{
  int c, value, empty, count;

  switch (c = *esc++) {
  case 'x':
    /* \xhh hex value */
    value = 0;
    empty = TRUE;
    while (1)
      {
        c = *esc++;
        if (!(c >= 'a' && c <= 'f')
            && !(c >= 'A' && c <= 'F')
            && !(c >= '0' && c <= '9'))
          {
            esc--;
            break;
          }
        value *=16;
        if (c >= 'a' && c <= 'f')
          value += c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
          value += c - 'A' + 10;
        if (c >= '0' && c <= '9')
          value += c - '0';
        empty = FALSE;
      }
    if (empty)
      fatal(sn,"\\x used with no trailing hex digits");
    break;

  case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7':
    /* \ooo octal value */
    value = 0;
    count = 0;
    while ((c <= '7') && (c >= '0') && (count++ < 3))
      {
        value = (value * 8) + (c - '0');
        c = *esc++;
      }
    esc--;
    break;

  case '\\':
  case '\'':
  case '"':
    value = c;
    break;

  case 'n':
    value = '\n';
    break;

  case 't':
    value = '\t';
    break;

  case 'r':
    value = '\r';
    break;

  case 'f':
    value = '\f';
    break;

  case 'b':
    value = '\b';
    break;

  case 'a':
    value = '\a';
    break;

  case 'v':
    value = '\v';
    break;

  case '?':
    value = c;
    break;

  case '(':
  case '{':
  case '[':
  case '%':
    value = c;
    warn(sn,"non-ANSI escape sequence `\\%c'", c);
    break;

  default:
    fatal(sn,"unknown escape, '\\' followed by char %x (`%c')", (int)c, c);
  }

  if (*next)
    *next = esc;

  return value;
}

/* return the value of an character literal sequence */
/* A2.5.2 */
static int
intern_char(char *s, char **next, int sn)
{
  unsigned char value;

  if (s[0] != '\'' || s[strlen(s)-1] != '\'')
    panic(sn,"mal-formed string constant");

  if (s[1] != '\\')
    {
      value = (unsigned)s[1];
      if (s[2] != '\'')
	panic(sn,"mal-formed string constant");
      if (next)
	*next = s + 2;
    }
  else
    {
      /* escaped char constant */
      value = intern_escape(s+2, next,sn);
    }

  /* map to a signed char value */
  value = (signed int)((unsigned char)((unsigned char)((unsigned int)value)));

  if (UCHAR_MAX < value)
    fatal(sn,"character constant out of range");

  return value;
}

static void
print_char(unsigned char c, FILE *stream)
{
  switch (c)
    {
    case '\n':
      fprintf(stream, "\\n");
      break;

    case '\\':
      fprintf(stream, "\\\\");
      break;

    case '\'':
      fprintf(stream, "\\'");
      break;

    case '\t':
      fprintf(stream, "\\t");
      break;

    case '\r':
      fprintf(stream, "\\r");
      break;

    case '\f':
      fprintf(stream, "\\f");
      break;

    case '\b':
      fprintf(stream, "\\b");
      break;

    case '\a':
      fprintf(stream, "\\a");
      break;

    case '\v':
      fprintf(stream, "\\v");
      break;

    default:
      if (isprint(c))
	fprintf(stream, "%c", c);
      else
	fprintf(stream, "\\x%02x", c);
    }
}

/* expand all escapes in string STR, return pointer to allocation w/ result */
static char *
intern_string(char *str, int sn)
{
  char *s, *istr;

  /* resulting string cannot be longer than STR */
  s = istr = malloc(strlen(str)+1);

  if (!str || !*str || *str != '\"') /* " */
    panic(sn,"mal-formed string constant");

  /* skip `"' */ /* " */
  str++;

  while (*str)
    {
      if (*str == '\\')
        *s++ = intern_escape(str+1, &str,sn);
      else
        {
          /* A2.6 */
          if (*str == '\n')
            warn(sn,"ANSI C forbids newline in character constant");
          /* A2.6 */
          if (*str == '"' && str[1] != '\0')
            panic(sn,"encountered `\"' embedded in string constant");

          if (*str != '\"') /* " */
            *s++ = *str;
          str++;
        }
    }
  *s = '\0';
  return istr;
}

static void
print_string(unsigned char *s, FILE *stream)
{
  while (*s)
    {
      print_char(*s, stream);
      s++;
    }
}

/* bogus token value */
#define TOKEN_BOGON		0

/* see libexo_init_declarations */

static int token_id[MAX_SLOTS];

#define TOKEN_HASH_SIZE		1024
struct exo_token_t *token_hash[MAX_SLOTS][TOKEN_HASH_SIZE];

/* hash a string */
static unsigned long
hash_str(char *s)
{
  unsigned h = 0;
  while (*s)
    h = (h << 1) + *s++;
  return (h % TOKEN_HASH_SIZE);
}

/* intern token TOKEN_STR */
struct exo_token_t *
exo_intern(char *token_str, int sn)		/* string to intern */
{
  int index;
  struct exo_token_t *ent;

  index = hash_str(token_str);

  for (ent=token_hash[sn][index]; ent != NULL; ent=ent->next)
    {
      if (!strcmp(token_str, ent->str))
	{
	  /* got a match, return token entry */
	  return ent;
	}
    }

  /* not found, create a new entry */
  ent = (struct exo_token_t *)calloc(1, sizeof(struct exo_token_t));
  if (!ent)
    fatal(sn,"out of virtual memory");

  ent->str = mystrdup(token_str);
  ent->token = token_id[sn]++;
  ent->next = token_hash[sn][index];
  token_hash[sn][index] = ent;

  return ent;
}

/* intern token TOKEN_STR as value TOKEN */
struct exo_token_t *
exo_intern_as(char *token_str,		/* string to intern */
	      int token, 		/* internment value */
              int sn)                   /* slot number */
{
  struct exo_token_t *ent;

#if 0
  if (token_id[sn] > token)
    fatal(sn,"token value is already in use");
#endif

  ent = exo_intern(token_str,sn);
  /* overide the default value */
  ent->token = token;

#if 0
  if (ent->token != token)
    fatal(sn,"symbol `%s' was previously interned", token_str);
#endif

  return ent;
}

/* allocate an EXO node, fill in its type */
static struct exo_term_t *
exo_alloc(enum exo_class_t ec, int sn)
{
  struct exo_term_t *exo;

  exo = (struct exo_term_t *)calloc(1, sizeof(struct exo_term_t));
  if (!exo)
    fatal(sn,"out of virtual memory");
  exo->next = NULL;
  exo->ec = ec;

  return exo;
}


/*
 * create a new EXO term, usage:
 *
 *	exo_new(sn,ec_integer, (exo_integer_t)<int>);
 *	exo_new(sn,ec_address, (exo_integer_t)<int>);
 *	exo_new(sn,ec_float, (exo_float_t)<float>);
 *	exo_new(sn,ec_char, (int)<char>);
 *      exo_new(sn,ec_string, "<string>");
 *      exo_new(sn,ec_list, <list_ent>..., NULL);
 *      exo_new(sn,ec_array, <size>, <array_ent>..., NULL);
 *	exo_new(sn,ec_token, "<token>"); 
*/
struct exo_term_t *
exo_new(int sn, enum exo_class_t ec, ...)
{
  struct exo_term_t *exo;
  va_list v;
  va_start(v, ec);

  exo = exo_alloc(ec,sn);
  switch (ec)
    {
    case ec_integer:
      exo->as_integer.val = va_arg(v, exo_integer_t);
      break;

    case ec_address:
      exo->as_address.val = va_arg(v, exo_address_t);
      break;

    case ec_float:
      exo->as_float.val = va_arg(v, exo_float_t);
      break;

    case ec_char:
      exo->as_char.val = va_arg(v, int);
      break;

    case ec_string:
      {
	char *str;

	str = va_arg(v, char *);
	exo->as_string.str = (unsigned char *)mystrdup(str);
      }
      break;

    case ec_list:
      {
	struct exo_term_t *ent;

	exo->as_list.head = NULL;
	do {
	  ent = va_arg(v, struct exo_term_t *);
	  exo->as_list.head = exo_chain(exo->as_list.head, ent);
	} while (ent != NULL);
      }
      break;

    case ec_array:
      {
	int i;
	struct exo_term_t *ent;

	exo->as_array.size = va_arg(v, int);
	exo->as_array.array = (struct exo_term_t **)
	  calloc(exo->as_array.size, sizeof(struct exo_term_t *));
	if (!exo->as_array.array)
	  fatal(sn,"out of virtual memory");
	i = 0;
	do {
	  ent = va_arg(v, struct exo_term_t *);
	  if (ent != NULL)
	    {
	      if (i == exo->as_array.size)
		fatal(sn,"array constructor overflow");
	      SET_EXO_ARR(exo, i, ent);
	    }
	  i++;
	} while (ent != NULL);
      }
      break;

    case ec_token:
      {
	char *str;

	str = va_arg(v, char *);
	exo->as_token.ent = exo_intern(str,sn);
      }
      break;

    case ec_blob:
      {
	unsigned size;
	unsigned char *data;

	size = va_arg(v, unsigned);
	data = va_arg(v, unsigned char *);

	exo->as_blob.size = size;
	exo->as_blob.data = malloc(size);
	if (data != NULL)
	  memcpy(exo->as_blob.data, data, size);
	else
	  memset(exo->as_blob.data, 0, size);
      }
      break;

    case ec_null:
      break;

    default:
      panic(sn,"bogus EXO class");
    }

  va_end(v);
  return exo;
}

/* release an EXO term */
void
exo_delete(struct exo_term_t *exo, int sn)
{
  exo->next = NULL;

  switch (exo->ec)
    {
    case ec_integer:
      /* no extra storage */
      exo->as_integer.val = 0;
      break;

    case ec_address:
      /* no extra storage */
      exo->as_address.val = 0;
      break;

    case ec_float:
      /* no extra storage */
      exo->as_float.val = 0.0;
      break;

    case ec_char:
      /* no extra storage */
      exo->as_char.val = '\0';
      break;

    case ec_string:
      free(exo->as_string.str);
      exo->as_string.str = NULL;
      break;

    case ec_list:
      {
	struct exo_term_t *ent, *next_ent;

	for (ent=exo->as_list.head; ent != NULL; ent = next_ent)
	  {
	    next_ent = ent->next;
	    exo_delete(ent,sn);
	  }
	exo->as_list.head = NULL;
      }
      break;

    case ec_array:
      {
	int i;

	for (i=0; i < exo->as_array.size; i++)
	  {
	    if (exo->as_array.array[i] != NULL)
	      exo_delete(exo->as_array.array[i],sn);
	  }
	free(exo->as_array.array);
	exo->as_array.array = NULL;
	exo->as_array.size = 0;
      }
      break;

    case ec_token:
      /* no extra storage */
      exo->as_token.ent = NULL;
      break;

    case ec_blob:
      /* free the blob data */
      free(exo->as_blob.data);
      exo->as_blob.data = NULL;
      break;

    case ec_null:
      /* no extra storage */
      break;

    default:
      panic(sn,"bogus EXO class");
    }
  exo->ec = (enum exo_class_t)0;

  /* release the node */
  free(exo);
}

/* chain two EXO lists together, FORE is attached on the end of AFT */
struct exo_term_t *
exo_chain(struct exo_term_t *fore, struct exo_term_t *aft)
{
  struct exo_term_t *exo, *prev;

  if (!fore && !aft)
    return NULL;

  if (!fore)
    return aft;

  /* find the tail of FORE */
  for (prev=NULL,exo=fore; exo != NULL; prev=exo,exo=exo->next)
    /* nada */;
  assert(prev);

  /* link onto the tail of FORE */
  prev->next = aft;

  return fore;
}

/* copy an EXO node */
struct exo_term_t *
exo_copy(struct exo_term_t *exo, int sn)
{
  struct exo_term_t *new_exo;

  /* NULL copy */
  if (!exo)
    return NULL;

  new_exo = exo_alloc(exo->ec,sn);
  *new_exo = *exo;

  /* the next link is always blown away on a copy */
  new_exo->next = NULL;

  switch (new_exo->ec)
    {
    case ec_integer:
    case ec_address:
    case ec_float:
    case ec_char:
    case ec_string:
    case ec_list:
    case ec_token:
      /* no internal parts to copy */
      break;

    case ec_array:
      {
	int i;

	/* copy the array */
	new_exo->as_array.array = (struct exo_term_t **)
	  calloc(new_exo->as_array.size, sizeof(struct exo_term_t *));

	for (i=0; i<new_exo->as_array.size; i++)
	  {
	    SET_EXO_ARR(new_exo, i, EXO_ARR(exo, i));
	  }
      }
      break;

    case ec_blob:
      new_exo->as_blob.data = malloc(new_exo->as_array.size);
      memcpy(new_exo->as_blob.data, exo->as_blob.data, new_exo->as_array.size);
      break;

    default:
      panic(sn,"bogus EXO class");
    }

  return new_exo;
}

/* deep copy an EXO structure */
struct exo_term_t *
exo_deepcopy(struct exo_term_t *exo, int sn)
{
  struct exo_term_t *new_exo;

  /* NULL copy */
  if (!exo)
    return NULL;

  new_exo = exo_copy(exo,sn);
  switch (new_exo->ec)
    {
    case ec_integer:
    case ec_address:
    case ec_float:
    case ec_char:
    case ec_token:
      /* exo_copy() == exo_deepcopy() for these node classes */
      break;

    case ec_string:
      /* copy the referenced string */
      new_exo->as_string.str =
	(unsigned char *)mystrdup((char *)exo->as_string.str);
      break;

    case ec_list:
      /* copy all list elements */
      {
	struct exo_term_t *elt, *new_elt, *new_list;

	new_list = NULL;
	for (elt=new_exo->as_list.head; elt != NULL; elt=elt->next)
	  {
	    new_elt = exo_deepcopy(elt,sn);
	    new_list = exo_chain(new_list, new_elt);
	  }
	new_exo->as_list.head = new_list;
      }
      break;

    case ec_array:
      /* copy all array elements */
      {
	int i;

	for (i=0; i<new_exo->as_array.size; i++)
	  {
	    SET_EXO_ARR(new_exo, i, exo_deepcopy(EXO_ARR(exo, i),sn));
	  }
      }
      break;

    case ec_blob:
      new_exo->as_blob.data = malloc(new_exo->as_array.size);
      memcpy(new_exo->as_blob.data, exo->as_blob.data, new_exo->as_array.size);
      break;

    default:
      panic(sn,"bogus EXO class");
    }

  return new_exo;
}

/* print an EXO term */
void
exo_print(struct exo_term_t *exo, FILE *stream, int sn)
{
  if (!stream)
    stream = stderr;

  switch (exo->ec)
    {
    case ec_integer:
      if (sizeof(exo_integer_t) == 4)
	myfprintf(sn,stream, "%u", exo->as_integer.val);
      else
	myfprintf(sn,stream, "%lu", exo->as_integer.val);
      break;

    case ec_address:
      if (sizeof(exo_address_t) == 4)
	myfprintf(sn,stream, "0x%x", exo->as_integer.val);
      else
	myfprintf(sn,stream, "0x%lx", exo->as_integer.val);
      break;

    case ec_float:
      fprintf(stream, "%f", exo->as_float.val);
      break;

    case ec_char:
      fprintf(stream, "'");
      print_char(exo->as_char.val, stream);
      fprintf(stream, "'");
      break;

    case ec_string:
      fprintf(stream, "\"");
      print_string(exo->as_string.str, stream);
      fprintf(stream, "\"");
      break;

    case ec_list:
      {
	struct exo_term_t *ent;

	fprintf(stream, "(");
	for (ent=exo->as_list.head; ent != NULL; ent=ent->next)
	  {
	    exo_print(ent, stream,sn);
	    if (ent->next)
	      fprintf(stream, ", ");
	  }
	fprintf(stream, ")");
      }
      break;

    case ec_array:
      {
	int i, last;

	/* search for last first non-NULL entry */
	for (last=exo->as_array.size-1; last >= 0; last--)
	  {
	    if (EXO_ARR(exo, last) != NULL)
	      break;
	  }
	/* LAST == index of last non-NULL array entry */

	fprintf(stream, "{%d}[", exo->as_array.size);
	for (i=0; i<exo->as_array.size && i <= last; i++)
	  {
	    if (exo->as_array.array[i] != NULL)
	      exo_print(exo->as_array.array[i], stream,sn);
	    else
	      fprintf(stream, " ");
	    if (i != exo->as_array.size-1 && i != last)
	      fprintf(stream, ", ");
	  }
	fprintf(stream, "]");
      }
      break;

    case ec_token:
      fprintf(stream, "%s", exo->as_token.ent->str);
      break;

    case ec_blob:
      {
	int i, cr;

	fprintf(stream, "{%d}<\n", exo->as_blob.size);
	for (i=0; i < exo->as_blob.size; i++)
	  {
	    cr = FALSE;
	    if (i != 0 && (i % 38) == 0)
	      {
		fprintf(stream, "\n");
		cr = TRUE;
	      }
	    fprintf(stream, "%02x", exo->as_blob.data[i]);
	  }
	if (!cr)
	  fprintf(stream, "\n");
	fprintf(stream, ">");
      }
      break;

    default:
      panic(sn,"bogus EXO class");
    }
}

/* (f)lex external defs */
extern int yylex(void);
extern int yy_nextchar(void);
extern char *yytext;
extern FILE *yyin;

static void
exo_err(char *err)
{
  extern int line;

  fprintf(stderr, "EXO parse error: line %d: %s\n", line, err);
  exit(1);
}

/* read one EXO term from STREAM */
struct exo_term_t *
exo_read(FILE *stream, int sn)
{
  int tok;
  char tok_buf[1024], *endp;
  struct exo_term_t *ent = NULL;
  extern int errno;
  extern void yy_setstream(int sn, FILE *);

  /* make sure we have a valid stream */
  if (!stream)
    stream = stdin;
  yy_setstream(sn, stream);

  /* make local copies of everything, allows arbitrary recursion */
  tok = yylex();
  strcpy(tok_buf, yytext);

  switch (tok)
    {
    case lex_integer:
      {
	exo_integer_t int_val;

	/* attempt integer conversion */
	errno = 0;
#ifdef HOST_HAS_QUAD
	int_val = myatoq(tok_buf, &endp, /* parse base */10,sn);
#else /* !HOST_HAS_QUAD */
	int_val = strtoul(tok_buf, &endp, /* parse base */10);
#endif /* HOST_HAS_QUAD */
	if (!errno && !*endp)
	  {
	    /* good conversion */
	    ent = exo_new(sn, ec_integer, int_val);
	  }
	else
	  exo_err("cannot parse integer literal");
      }
      break;

    case lex_address:
      {
	exo_address_t addr_val;

	/* attempt address conversion */
	errno = 0;
#ifdef HOST_HAS_QUAD
	addr_val = myatoq(tok_buf, &endp, /* parse base */16,sn);
#else /* !HOST_HAS_QUAD */
	addr_val = strtoul(tok_buf, &endp, /* parse base */16);
#endif /* HOST_HAS_QUAD */
	if (!errno && !*endp)
	  {
	    /* good conversion */
	    ent = exo_new(sn, ec_address, addr_val);
	  }
	else
	  exo_err("cannot parse address literal");
      }
      break;

    case lex_float:
      {
	exo_float_t float_val;

	/* attempt double conversion */
	errno = 0;
	float_val = strtod(tok_buf, &endp);
	if (!errno && !*endp)
	  {
	    /* good conversion */
	    ent = exo_new(sn, ec_float, float_val);
	  }
	else
	  exo_err("cannot parse floating point literal");
      }
      break;

    case lex_char:
      {
	int c;

	c = intern_char(tok_buf, &endp,sn);
	if (!endp)
	  exo_err("cannot convert character literal");
	ent = exo_new(sn, ec_char, c);
      }
      break;

    case lex_string:
      {
	char *s;

	s = intern_string(tok_buf,sn);
	ent = exo_new(sn, ec_string, s);
	free(s);
      }
      break;

    case lex_token:
      ent = exo_new(sn, ec_token, tok_buf);
      break;

    case lex_byte:
      exo_err("unexpected blob byte encountered");
      break;

    case '(':
      {
	struct exo_term_t *elt;

	ent = exo_new(sn, ec_list, NULL);

	if (yy_nextchar() != ')')
	  {
	    /* not an empty list */
	    do {
	      elt = exo_read(stream,sn);
	      if (!elt)
		exo_err("unexpected end-of-file");
	      ent->as_list.head =
		exo_chain(ent->as_list.head, elt);

	      /* consume optional commas */
	      if (yy_nextchar() == ',')
		yylex();
	    } while (yy_nextchar() != ')');
	  }

	/* read tail delimiter */
	tok = yylex();
	if (tok != ')')
	  exo_err("expected ')'");
      }
      break;

    case ')':
      exo_err("unexpected ')' encountered");
      break;

    case '<':
      exo_err("unexpected '<' encountered");
      break;

    case '>':
      exo_err("unexpected '>' encountered");
      break;

    case '{':
      {
	int cnt, size;
	struct exo_term_t *elt;

	/* get the size */
	elt = exo_read(stream,sn);
	if (!elt || elt->ec != ec_integer)
	  exo_err("badly formed array size");

	/* record the size of the array/blob */
	size = (int)elt->as_integer.val;

	/* done with the EXO integer */
	exo_delete(elt,sn);

	/* read the array delimiters */
	tok = yylex();
	if (tok != '}')
	  exo_err("expected '}'");

	tok = yylex();
	switch (tok)
	  {
	  case '[': /* array definition */
	    /* allocate an array definition */
	    ent = exo_new(sn,ec_array, size, NULL);

	    /* read until array is full or tail delimiter encountered */
	    if (yy_nextchar() != ']')
	      {
		/* not an empty array */
		cnt = 0;
		do {
		  if (cnt == ent->as_array.size)
		    exo_err("too many initializers for array");

		  /* NULL element? */
		  if (yy_nextchar() == ',')
		    {
		      elt = NULL;
		    }
		  else
		    {
		      elt = exo_read(stream,sn);
		      if (!elt)
			exo_err("unexpected end-of-file");
		    }
		  SET_EXO_ARR(ent, cnt, elt);
		  cnt++;

		  /* consume optional commas */
		  if (yy_nextchar() == ',')
		    yylex();
		} while (yy_nextchar() != ']');
	      }

	    /* read tail delimiter */
	    tok = yylex();
	    if (tok != ']')
	      exo_err("expected ']'");
	    break;

	  case '<': /* blob definition */
	    /* allocate an array definition */
	    ent = exo_new(sn,ec_blob, size, /* zero contents */NULL);

	    /* read until blob is full */
	    if (yy_nextchar() != '>')
	      {
		unsigned int byte_val;

		/* not an empty array */
		cnt = 0;
		for (;;) {
		  /* read next blob byte */
		  tok = yylex();

		  if (tok == lex_byte)
		    {
		      if (cnt == ent->as_blob.size)
			exo_err("too many initializers for blob");

		      /* attempt hex conversion */
		      errno = 0;
		      byte_val = strtoul(yytext, &endp, /* parse base */16);
		      if (errno != 0 || *endp != '\0')
			exo_err("cannot parse blob byte literal");
		      if (byte_val > 255)
			panic(sn,"bogus byte value");
		      ent->as_blob.data[cnt] = byte_val;
		      cnt++;
		    }
		  else if (tok == '>')
		    break;
		  else
		    exo_err("unexpected character in blob");
		}
	      }

#if 0 /* zero tail is OK... */
	    if (cnt != ent->as_blob.size)
	      exo_err("not enough initializers for blob");
#endif
	    break;

	  default:
	    exo_err("expected '[' or '<'");
	  }
      }
      break;

    case '}':
      exo_err("unexpected '}' encountered");
      break;

    case ',':
      exo_err("unexpected ',' encountered");
      break;

    case '[':
      {
	int i, cnt;
	struct exo_term_t *list, *elt, *next_elt;

	/* compute the array size */
	list = NULL;
	if (yy_nextchar() == ']')
	  exo_err("unsized array has no initializers");

	cnt = 0;
	do {
	  /* NULL element? */
	  if (yy_nextchar() == ',')
	    {
	      elt = exo_new(sn,ec_null);
	    }
	  else
	    {
	      elt = exo_read(stream,sn);
	      if (!elt)
		exo_err("unexpected end-of-file");
	    }
	  cnt++;
	  list = exo_chain(list, elt);

	  /* consume optional commas */
	  if (yy_nextchar() == ',')
	    yylex();
	} while (yy_nextchar() != ']');

	/* read tail delimiter */
	tok = yylex();
	if (tok != ']')
	  exo_err("expected ']'");

	/* create the array */
	assert(cnt > 0);
	ent = exo_new(sn,ec_array, cnt, NULL);

	/* fill up the array */
	for (i=0,elt=list; i<cnt; i++,elt=next_elt)
	  {
	    assert(elt != NULL);
	    next_elt = elt->next;
	    if (elt->ec == ec_null)
	      {
		SET_EXO_ARR(ent, cnt, NULL);
		exo_delete(ent,sn);
	      }
	    else
	      {
		SET_EXO_ARR(ent, cnt, elt);
		elt->next = NULL;
	      }
	  }
      }
      break;

    case ']':
      exo_err("unexpected ']' encountered");
      break;

    case lex_eof:
      /* nothing to read */
      ent = NULL;
      break;

    default:
      panic(sn,"bogus token");
    }

  return ent;
}

void libexo_init_declarations(void)
{  int sn;

   for (sn=0;sn<process_num;sn++)
       token_id[sn] = TOKEN_BOGON + 1;

}
