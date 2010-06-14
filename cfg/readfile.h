/*****************************************************************************
 *
 * $Id: readfile.h,v 1.1.1.1 2004/02/05 07:20:16 lixianfe Exp $
 *
 * $Log: readfile.h,v $
 * Revision 1.1.1.1  2004/02/05 07:20:16  lixianfe
 * Initial version, simulate/esitmate straigh-line code
 *
 *
 ****************************************************************************/

#ifndef READ_FILE_H
#define READ_FILE_H


#include "ss.h"

SS_INST_TYPE * readcode(char *fname, int *start, int *end);
void print_code(SS_INST_TYPE *code, int size, unsigned entry);


#endif
