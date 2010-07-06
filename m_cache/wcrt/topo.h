/*! This is a header file of the Chronos timing analyzer. */

/*! Helper functions for topological sorting. */

#ifndef __CHRONOS_TOPO_H
#define __CHRONOS_TOPO_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Finds tasks without a predecessor and puts them into startTaskList.
 * Returns the number of start tasks.
 */
int collectStartTasks( int *taskIDList, int numChartTasks, 
                       int **startTaskList );

/*
 * Determine topological order of the taskList.
 * Updates topoArr with the ordered sequence, and returns the length of topoArr.
 */
int topoSort( int *taskIDList, int numChartTasks, int **topoArr );

/*
 * Sort topoLists by topological order.
 */
int topoTask();

/*
 * Sort MSG nodes by topological order.
 */
int topoGraph();

/*
 * Determine reverse topological order of the task dependency graph rooted at start.
 * Updates topoArr with the ordered sequence, and returns the length of topoArr.
 */
int topoSortSubgraph( int startid, int **topoArr );


#endif
