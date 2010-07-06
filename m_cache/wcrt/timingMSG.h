/*! This is a header file of the Chronos timing analyzer. */

/*! Functions for performing timing analysis of message sequence charts. */

#ifndef __CHRONOS_TIMING_MSG_H
#define __CHRONOS_TIMING_MSG_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Makes a clone of src.
 */
int copyTask( task_t *dst, task_t *src );

int mapDependency( task_t *tc, int *map );

/*
 * Adds tasks from msg[chartid] to chart ctx, maintaining the topological order.
 * For cyclic MSG: if a task from msg[chartid] is already in ctx, duplicate that task instance.
 * Assumption: each chart in the original MSG has unique tasks,
 * so just check if first task in msg[chartid] is already in ctx;
 * if so, it means the whole chart msg[chartid] is already in ctx.
 */
int addChartTasks( int chartid, chart_t *ctx );

int copyChart( chart_t *new, chart_t *src, int lastnode, char nextsucc );

/*
 * Returns the last node in bpStack and the corresponding nextSucc as (node, next).
 * The nextSucc value is incremented. If it reaches the end, the node is deleted and len is updated.
 */
int popNode( int *node, int *next, int **bpStack, int **nextSucc, int *len );

int countEdge( path_t *px, int src, int dst );

/*
 * Adds dependency between tasks of the same process occuring consecutively.
 * Assumes tasks are topologically sorted.
 * Records added dependencies in (preds, succs) pair.
 */
int addConcatDependency( chart_t *cx, int **predRecord, int **succRecord, int
    *recordLen );

/*
 * Removes dependency between tasks stated in records.
 * Note that removal may fail for added dependencies that also exist in the original chart.
 */
int recoverConcatDependency( int *predRecord, int *succRecord, int recordLen );

int initPath( path_t *px );

int copyPath( path_t *new, path_t *src, int lastnode, char nextsucc );

int getNextValidSuccId( path_t *px, int chartid, int from );

int timingEstimate_synch_acyclic();

/*
 * Handle cyclic MSG with bounds on back edges. 
 * Use implicit path enumeration with ILP.
 */
int timingEstimate_synch();

int timingEstimate_asynch_acyclic();

/*
 * This version has problem with backtracking, because nodes may repeat along a path.
 */
int timingEstimate_asynch_();

/*
 * Handle cyclic MSG with bounds on back edges.
 * REQUIRES THAT PATHS HAVE BEEN ENUMERATED in a file <dgname>.paths
 * Then for each path, construct the concatenated MSC and analyse.
 */
int timingEstimate_asynch();

int timingEstimate();


#endif
