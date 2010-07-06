/*! This is a header file of the Chronos timing analyzer. */

/*! Functions for parsing input files. */

#ifndef __CHRONOS_PARSE_H
#define __CHRONOS_PARSE_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int initSched( sched_t *sc );

int initChart( chart_t *cx );

/*
 * Returns the index in taskList which corresponds to the task with specified tname;
 * returns -1 if it is not found.
 */
int findTask( char tname[] );

/*
 * Returns the index in PEList which corresponds to the PE with specified id;
 * returns -1 if it is not found.
 */
int findPE( int id );

/*
 * Returns the index in topoList of the chart that contains taskList[idx].
 * i.e. finds the task chart that the task belongs to.
 */
int findChart( int idx );

/*
 * Recursively determines if task pred is a predecessor of succ.
 */
char isPredecessorOf( int pred, int succ );

/*
 * Reads task description file.
 * Format: one task per line
 * <taskID:int> <actorID:int> <peID:int> <priority:int> <period:time_t> <exectime:time_t>
 */
int readTasks();

/*
 * Reads the Message Sequence Graph.
 * Format:
 *
 * <nodeID_1> <numNodeSuccessors_1> <succNodeID_1.1> <succNodeID_1.1.2> ... <succNodeID_1.1.N>
 * <numTasksInChart_1>
 * <taskName_1.1> <numTaskSuccessors_1.1> <succTaskName_1.1.1> <succTaskName_1.1.2> ... <succTaskName_1.1.N>
 * <taskName_1.2> <numTaskSuccessors_1.2> <succTaskName_1.2.1> <succTaskName_1.2.2> ... <succTaskName_1.2.N>
 * ...
 * <nodeID_2> <numNodeSuccessors_2> <succNodeID_2.1> <succNodeID_2.1.2> ... <succNodeID_2.1.N>
 * <numTasksInChart_2>
 * <taskName_2.1> <numTaskSuccessors_2.1> <succTaskName_2.1.1> <succTaskName_2.1.2> ... <succTaskName_2.1.N>
 * <taskName_2.2> <numTaskSuccessors_2.2> <succTaskName_2.2.1> <succTaskName_2.2.2> ... <succTaskName_2.2.N>
 * ...
 */
int readMSG();

int readEdgeBounds();

int readTaskMemoryReq( task_t *tc );

/*
 * We require that for each task, there exist a file <tname>.cfg
 * containing the list of basic blocks to allocate into SPM.
 */
int readMemoryReq();

int readConfig();

int freeAlloc( alloc_t *ac );

int freeSched( sched_t *sc );

int freeTask( task_t *tx );

int freeChart( chart_t *cx );

int wcrt_freePath( path_t *px );

int freeAll();

#endif
