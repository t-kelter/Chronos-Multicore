/*! This is a header file of the Chronos timing analyzer. */

/*! Functions to test conflicts on paths */

#ifndef __CHRONOS_PATH_TEST_H
#define __CHRONOS_PATH_TEST_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########

int hasConflict(int effectNo);

void cancelEffect(int effectNo);

int isFeasible(int p[],int length);


#endif
