/*! This is a header file of the Chronos timing analyzer. */

/*! Helper functions for graph coloring */

#ifndef __CHRONOS_GRAPHCOLORING_H
#define __CHRONOS_GRAPHCOLORING_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Performs greedy heuristic for the graph coloring problem.
 * Returns the number of colors used, and updates colorAssg with the assigned colors.
 */
int graphColoring( int numNodes, int *outdegree, int **outedges, char
    **colorAssg );

/*
 * Wrapper for graphColoring.
 * Transforms task interactions into interference graph and calls graphColoring.
 */
int taskColoring( overlay_t *ox, char **colorAssg );

/*
 * Partitions SPM to 'colors', based on total utilization of tasks assigned to each color.
 * Calls external SPM partitioning routine and stores the result in colorShare.
 */
int colorPartition( int *memberList, int numMembers, char *colorAssg, int
    **colorShare, int capacity );

/*
 * Calls spm allocation that considers colors.
 */
int colorAllocation( chart_t *msc, overlay_t *ox, char *colorAssg, int
    numColors, int capacity );


#endif
