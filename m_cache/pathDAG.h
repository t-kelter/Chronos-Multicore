/*! This is a header file of the Chronos timing analyzer. */

/*! Initializes data structures */

#ifndef __CHRONOS_PATH_DAG_H
#define __CHRONOS_PATH_DAG_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


static void
pathFunction(procedure * proc);

static void 
pathLoop(procedure *proc, loop *lp);

void 
pathDAG(MSC *msc);


#endif
