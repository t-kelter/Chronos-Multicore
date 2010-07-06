#include <stdio.h>

#include "handler.h"

/*
 * fopen with error check.
 */
FILE* wcrt_openfile( char *filename, char *mode ) {

  FILE *fptr;

  fptr = fopen( filename, mode );
  if( !fptr ) {
    fprintf( stderr, "Failed to open file %s. (handler.c:48)\n", filename );
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
  return wcrt_openfile( fn, mode );
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
