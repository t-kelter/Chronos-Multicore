#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "handler.h"
#include "header.h"
#include "wcrt/cycle_time.h"


/* Improper exit with formatted error message */
void prerr(char* msg, ...)
{
  va_list argument_list;
  va_start( argument_list, msg );

  vfprintf( stderr, msg, argument_list );
  va_end( argument_list );

  fflush(stderr);
  exit(-1);
}


/*
 * fopen with error check.
 * Assumes filename is the one given in the command line option.
 */
FILE* openfile( char *ext, char *mode ) {

  FILE *fptr;
  char fn[80];

  sprintf( fn, "%s.%s", filename, ext );
  fptr = fopen( fn, mode );
  if( !fptr ) {
    prerr( "Failed to open file: %s (handler.c:50)\n", fn);
  }

  return fptr;
}

int
indexOfProc(int num)
{
	int i;
	for( i = 0; i < num_procs; i ++)
		if(num == proc_cg[i])
			return i;
	return -1;
}

  
