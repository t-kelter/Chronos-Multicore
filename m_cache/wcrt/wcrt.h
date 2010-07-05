/* This file declares the "main" function of the wcrt analyzer submodule. 
   The communication between the main analyzer and the wcrt analyzer 
   happens via files, but this could be changed in the future.

   The communication via files was only enforced because originally
   these were two different programs.
*/

int wcrt_analysis( char* filename_cf, char *filename_pd, char *filename_dg );
