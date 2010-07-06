/*! This is a header file of the Chronos timing analyzer. */

/*! File and memory handler functions. */

#ifndef __CHRONOS_HANDLER_H
#define __CHRONOS_HANDLER_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * fopen with error check.
 */
FILE* wcrt_openfile( char *filename, char *mode );

/*
 * Opens file <filename>.<ext>
 */
FILE* openfext( char *filename, char *ext, char *mode );

char testopen( char *filename );

char testopenfext( char *filename, char *ext );


#endif
