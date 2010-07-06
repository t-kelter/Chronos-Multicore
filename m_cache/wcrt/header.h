/*! This is a header file of the Chronos timing analyzer. */

/*! This file holds all the globally needed declarations */

#ifndef __CHRONOS_HEADER_H
#define __CHRONOS_HEADER_H

#include <stdio.h>

// ######### Macros #########

#define uint unsigned int
#define time_t unsigned long long

#define MAXSTEP 20
#define MAXLEN  512
#define MAXCR  100      // max rounds for CR interference reduction
#define MAX_UNFOLD 100  // max unfolding count for MSG edges, if not specified

#define INSN_EXEC_TIME 1

#define INSN_SIZE   8
#define FETCH_SIZE 16

#define DEFAULT_SPM_LATENCY 1
#define DEFAULT_OFF_LATENCY 10

#define DOWNTIME_TOLERANCE  5
// how many iterations to proceed while no better result is encountered

#define DEFAULT_ALLOCWEIGHT 0.4
// base weight of a task in allocation content selection

#define DEFAULT_LIMITSOLN 2

// choices of chart concatenation method
#define SYNCH  0
#define ASYNCH 1

// choices of allocation methods
#define NONE                 0  // no allocation, just timing analysis
#define PROFILE_KNAPSACK     1  // WCET-profile based knapsack (static allocation)
#define INTERFERENCE_CLUSTER 2  // partition SPM among all tasks interfering (transitively) at any point
#define GRAPH_COLORING       3  // graph coloring of tasks in INTERFERENCE CLUSTER; partition SPM among colors
#define SLACK_CRITICAL       4  // GRAPH_COLORING combined with slack insertion to improve critical paths

#define CLUSTER_BASED ( allocmethod != NONE && allocmethod != PROFILE_KNAPSACK )

#define NAMELEN 40

/* Memory allocation with error check. */

#define MALLOC( ptr, size, msg ) \
  malloc( (size) ); \
  if( !(ptr) ) printf( "\nError: malloc %d bytes failed for %s.\n\n", (int)(size), (msg) ), exit(1)

#define CALLOC( ptr, len, size, msg ) \
  calloc( (len), (size) ); \
  if( !(ptr) ) printf( "\nError: calloc %d bytes failed for %s.\n\n", (int)(size), (msg) ), exit(1)

#define REALLOC( ptr, size, msg ) \
  realloc( (ptr), (size) ); \
  if( !(ptr) ) printf( "\nError: realloc %d bytes failed for %s.\n\n", (int)(size), (msg) ), exit(1)


// ######### Datatype declarations  ###########


typedef struct {
  time_t tmStart;
  time_t tmEnd;
  int numOwnerTasks;
  int *ownerTaskList;
} overlay_t;

typedef struct {
  int numOverlays;
  overlay_t **overlayList;
} alloc_t;

typedef struct {
  int numAssigned;
  int *assignedList;      // task index (not pid!) of assigned tasks, not ordered
  alloc_t* spm;
} sched_t;

typedef struct {
  int fnid, bbid;         // for analysis
  int addr;
  int size;
  int realsize;           // in the case that jumps are inserted because of allocation
  int freq;               // access frequency
} mem_t;

typedef struct {
  char   tname[MAXLEN];
  int    actorID;
  int    peID;
  int    priority;
  time_t period;
  time_t ctimeLo;          // lower bound of computation time
  time_t ctimeHi;          // upper bound of computation time
  int    numPreds;
  int    *predList;        // task index (not pid!) of predecessors
  int    numSuccs;
  int    *succList;        // task index (not pid!) of successors
  int    numMemBlocks;
  mem_t  **memBlockList;
  char   *allocated;
} task_t;

typedef struct {
  int  numSuccs;
  int  *succList;
  int  topoListLen;
  int  *topoList;
  int  *timeTopoList;      // updating order based on topology and current scheduling, for safe slack insertion
  time_t wcrt;             // WCRT of the chart
  sched_t **PEList;
  char *msc_name; // The file from which the chart was read
} chart_t;

typedef struct {
  int src;
  int dst;
  int bound;
} edge_t;

typedef struct {
  chart_t *msc;
  char **edgeBounds;
} path_t;


// ######### Global variables declarations / definitions ###########

#ifdef DEF_GLOBALS
#define EXTERN 
#else
#define EXTERN extern
#endif

EXTERN char *cfname; // <filename>.cf: processor configuration file
EXTERN char *pdname; // <filename>.pd: task description file
EXTERN char *dgname; // <filename>.dg: dependency graph file
extern int times_iteration; // This is given by the main program
EXTERN char allocmethod;
EXTERN char concat;

EXTERN int totalcapacity;

EXTERN int spm_latency;
EXTERN int off_latency;

EXTERN int numPEs;
EXTERN int *peID;
EXTERN int *spmCapacity;

EXTERN int numCharts;            // number of MSC nodes in the MSG
EXTERN chart_t *msg;
EXTERN int *topoMSG;             // MSG with nodes topologically sorted, for traversal

EXTERN int startNode;            // index of start node of the MSG
EXTERN int endNode;              // index of end node of the MSG (assumed single; may still have outgoing edges if MSG is cyclic)

EXTERN edge_t *edgeBounds;       // bounds of back edges in the MSG
EXTERN int numEdgeBounds;


EXTERN int *pChart;              // pChart[i]: index of topoLists of the MSC containing taskList[i]

EXTERN int numTasks;
EXTERN task_t **taskList;        // global task list


EXTERN time_t *earliestReq;
EXTERN time_t *latestReq;
EXTERN time_t *latestReq_copy;

EXTERN time_t *earliestFin;
EXTERN time_t *latestFin;


EXTERN char **interfere;
EXTERN char **peers;

// interfere[i][j] = interfere[j][i] = 1 if tasks i and j interfere, 0 otherwise.
// For non-greedy graph-coloring method, maintain that interference does not increase.
// By right only needed for tasks assigned on the same PE,
// but it is more efficient to address this array by task id.

EXTERN char *isCritical;         // isCritical[i] = 1 if taskList[i] is in the critical path of its MSC, 0 otherwise

EXTERN double allocweight;
//int limitsoln;


//TODO: These should be defines instead
#ifdef EXTERN
  EXTERN char ALLOCHEUR;
  EXTERN char GREEDY;
  EXTERN char DEBUG;
#else
  EXTERN char ALLOCHEUR = 0;
  EXTERN char GREEDY = 1;
  EXTERN char DEBUG = 0;
#endif

EXTERN FILE *timefp;
EXTERN char resultFileBaseName[200];

#endif
