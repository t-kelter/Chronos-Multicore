/*! This is a header file of the Chronos timing analyzer. */

/*
 * File and memory handling functions.
 */

#ifndef __CHRONOS_HANDLER_H
#define __CHRONOS_HANDLER_H

#include <stdio.h>

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * fopen with error check.
 * Assumes filename is the one given in the command line option.
 */
FILE* openfile( char *ext, char *mode );

int
indexOfProc(int num);


#endif
