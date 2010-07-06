/*! This is a header file of the Chronos timing analyzer. */

/* This file declares the "main" function of the wcrt analyzer submodule. 
   The communication between the main analyzer and the wcrt analyzer 
   happens via files, but this could be changed in the future.

   The communication via files was only enforced because originally
   these were two different programs.
*/

#ifndef __CHRONOS_WCRT_H
#define __CHRONOS_WCRT_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int wcrt_analysis( char* filename_cf, char *filename_pd, char *filename_dg );

#endif
