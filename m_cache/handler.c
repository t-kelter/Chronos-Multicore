#include <stdlib.h>

#include "handler.h"
#include "header.h"
#include "wcrt/cycle_time.h"

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
    fprintf(stderr, "Failed to open file: %s (handler.c:50)\n", fn);
    exit(1);
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

  
