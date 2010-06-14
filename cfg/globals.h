/*****************************************************************************
 * $Id: globals.h,v 1.1 2004/07/29 02:18:10 lixianfe Exp $
 *
 * define global variables
 *
 ****************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H


#ifdef DEF_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

#include "prog.h"

EXTERN Prog	    prog;
EXTERN FILE	    *filp, *fvar, *fusr;


#endif
