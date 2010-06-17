/*
 * File and memory handling functions.
 */

#include <stdio.h>

#include "cycle_time.h"
#include "cycle_time.c"

/* 
 * Time-keeping:
 * Change CYCLES_PER_MSEC according to CPU speed where this is run.
 * e.g. 3 GHz = 3E+9 cyc/sec = 3E+6 cyc/millisec
 * CLOCKS_PER_SEC is defined in <time.h>
 */

#define CYCLES_PER_MSEC 3000000
#define STARTTIME cycle_time(0)
#define STOPTIME t = cycle_time(1); printf( "=== %f ms\n", t/CYCLES_PER_MSEC ); fflush( stdout )

#define NAMELEN 40


/* Memory allocation with error check. */

#define MALLOC( ptr, size, msg ) \
  malloc( (size) ); \
  if( !(ptr) ) printf( "\nError: malloc %d bytes failed for %s.\n\n", (size), (msg) ), exit(1)

#define CALLOC( ptr, len, size, msg ) \
  calloc( (len), (size) ); \
  if( !(ptr) ) printf( "\nError: calloc %d bytes failed for %s.\n\n", (size), (msg) ), exit(1)

#define REALLOC( ptr, size, msg ) \
  realloc( (ptr), (size) ); \
  if( !(ptr) ) printf( "\nError: realloc %d bytes failed for %s.\n\n", (size), (msg) ), exit(1)


/*
 * fopen with error check.
 */
FILE* openfile( char *filename, char *mode ) {

  FILE *fptr;

  fptr = fopen( filename, mode );
  if( !fptr ) {
    fprintf( stderr, "Failed to open file %s.\n", filename );
    exit(1);
  }

  return fptr;
}


/*
 * Opens file <filename>.<ext>
 */
FILE* openfext( char *filename, char *ext, char *mode ) {

  char fn[80];

  sprintf( fn, "%s.%s", filename, ext );
  return openfile( fn, mode );
}


char testopen( char *filename ) {

  FILE *fptr;

  fptr = fopen( filename, "r" );
  if( fptr != NULL ) {
    fclose( fptr );
    return 1;
  }
  return 0;
}


char testopenfext( char *filename, char *ext ) {

  char fn[80];

  sprintf( fn, "%s.%s", filename, ext );
  return testopen( fn );
}
