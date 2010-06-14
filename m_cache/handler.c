/*
 * File and memory handling functions.
 */

#include <stdio.h>


#include "cycle_time.h"
//#include "cycle_time.c"

/* 
 * Time-keeping:
 * Change CYCLES_PER_MSEC according to CPU speed where this is run.
 * e.g. 3 GHz = 3E+9 cyc/sec = 3E+6 cyc/millisec
 * CLOCKS_PER_SEC is defined in <time.h>
 */

#define CYCLES_PER_MSEC 2992898
#define STARTTIME cycle_time(0)
#define STOPTIME t = cycle_time(1); printf( "=== %lf msec\n", t/CYCLES_PER_MSEC ); fflush( stdout )


/* Memory allocation with error check. */

#define MALLOC( ptr, size, msg ) \
  malloc( (size) ); \
  if( !(ptr) ) printf( "\nError: malloc failed for %s.\n\n", (msg) ), exit(1)

#define CALLOC( ptr, len, size, msg ) \
  calloc( (len), (size) ); \
  if( !(ptr) ) printf( "\nError: calloc failed for %s.\n\n", (msg) ), exit(1)

#define REALLOC( ptr, size, msg ) \
  realloc( (ptr), (size) ); \
  if( !(ptr) ) printf( "\nError: realloc failed for %s.\n\n", (msg) ), exit(1)


/*
 * fopen with error check.
 * Assumes filename is the one given in the command line option.
 */
FILE* openfile( char *ext, char *mode ) {

  FILE *fptr;
  char fn[80];

  sprintf( fn, "%s.%s", filename, ext );
  fptr = fopen( fn, mode );
  if( !fptr )
    printf( "Error opening file %s.\n", fn ), exit(1);

  return fptr;
}

static int
indexOfProc(int num)
{
	int i;
	for( i = 0; i < num_procs; i ++)
		if(num == proc_cg[i])
			return i;
	return -1;
}

  
