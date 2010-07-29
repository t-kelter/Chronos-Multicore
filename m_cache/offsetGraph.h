/*! This is a header file of the Chronos timing analyzer. */

/*
 * A graph structure to represent an offset graph as needed in the
 * alignment-aware DAG-based WCET/BCET-analysis.
 */

#ifndef __CHRONOS_ANALYSIS_OFFSET_GRAPH_H
#define __CHRONOS_ANALYSIS_OFFSET_GRAPH_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########


// Forward declarations
typedef struct og_edge offset_graph_edge;
typedef struct og_node offset_graph_node;
typedef struct og      offset_graph;


/* Represents an edge in the offset graph (see below) */
struct og_edge {
  offset_graph_node *start;
  offset_graph_node *end;
  ull bcet;     /* An optional, additive BCET component which only applies when the edge is taken. */
  ull wcet;     /* An optional, additive WCET component which only applies when the edge is taken. */
  uint edge_id; /* A unique identifier for the edge. */
};

/* Represents a node in the offset graph (see below) */
struct og_node {
  uint offset;             /* The offset that is represented by the node. */
  ull bcet;                /* A fixed BCET that is needed for each invocation of the loop with offset 'offset'. */
  ull wcet;                /* A fixed WCET that is needed for each invocation of the loop with offset 'offset'. */
  uint *incoming_edges;    /* The indexes (into the edge array) of the incoming edges. */
  uint num_incoming_edges; /* The current size of 'num_incoming_edges'. */
  uint *outgoing_edges;    /* The indexes (into the edge array) of the outgoing edges. */
  uint num_outgoing_edges; /* The current size of 'num_outgoing_edges'. */
};

/* Represents a graph whose nodes represent a loop execution at a certain
 * offset. Additionally, it has two other nodes, a supersink 'si' and a
 * supersource 'so'. For all other nodes 'n' there are edges (so,n) and
 * (n,si). Those edges have cost 0, all other edges (u,v) have cost equal
 * to the WCET that occurs during an execution of the loop which starts
 * at offset(u) and ends at offset(v). All edges have capacity equal to
 * the loopbound.
 *
 * A minimum (maximum) cost flow through the graph thus yields its
 * alignment-aware BCET (WCET).
 */
struct og {
  offset_graph_node *nodes;
  uint num_nodes;
  offset_graph_edge *edges;
  uint num_edges;
  offset_graph_node supersource;
  offset_graph_node supersink;
};

// ######### Function declarations  ###########


/* Creates a new offset graph with 'number_of_nodes' nodes
 * (excluding the supersink and supersource). */
offset_graph *createOffsetGraph( uint number_of_nodes );

/* Returns the edge that was added or NULL if nothing was added. */
offset_graph_edge *addOffsetGraphEdge(
    offset_graph *og , offset_graph_node *start,
    offset_graph_node *end, ull bcet, ull wcet );

/* Gets the node which represents offset 'offset'. */
offset_graph_node *getOffsetGraphNode( offset_graph *og, uint offset );

/* Returns the edge with the given start and end nodes, or NULL if it doesn't exist. */
offset_graph_edge *getOffsetGraphEdge( const offset_graph *og,
    const offset_graph_node *start, const offset_graph_node *end );

/* Prints the offset graph to th given file descriptor. */
void dumpOffsetGraph( const offset_graph *og, FILE *out );

/* Solves a minimum cost flow problem to obtain the final BCET.
 *
 * 'loopbound_min' specifies the minimum number of iterations of the loop.
 */
ull computeOffsetGraphLoopBCET( const offset_graph *og, uint loopbound_min );

/* Solves a maximum cost flow problem to obtain the final WCET.
 *
 * 'loopbound_max' specifies the maximum number of iterations of the loop.
 */
ull computeOffsetGraphLoopWCET( const offset_graph *og, uint loopbound_max );

/* Deallocates an offset graph. */
void freeOffsetGraph( offset_graph *og );


#endif
