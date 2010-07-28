// Include standard library headers
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "offsetGraph.h"
#include "header.h"
#include "handler.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


enum ILPComputationType {
  ILP_TYPE_BCET,
  ILP_TYPE_WCET
};


// #########################################
// #### Declaration of static variables ####
// #########################################


// #########################################
// #### Definitions of static functions ####
// #########################################


/* Dumps a single node to the given file descriptor. */
static void dumpOffsetGraphNode( const offset_graph_node *node, FILE *out )
{
  fprintf( out, "  %u:\n", node->offset );

  uint j;
  fprintf( out, "    In-Edges : " );
  for ( j = 0; j < node->num_incoming_edges; j++ ) {
    const offset_graph_edge * const edge = node->incoming_edges[j];
    fprintf( out, "%u (from node %u)", edge->edge_id, edge->start->offset );
    if ( j != node->num_incoming_edges - 1 ) {
      fprintf( out, ", " );
    }
  }
  fprintf( out, "\n" );

  fprintf( out, "    Out-Edges: " );
  for ( j = 0; j < node->num_outgoing_edges; j++ ) {
    const offset_graph_edge * const edge = node->outgoing_edges[j];
    fprintf( out, "%u (to node %u)", edge->edge_id, edge->end->offset );
    if ( j != node->num_outgoing_edges - 1 ) {
      fprintf( out, ", " );
    }
  }
  fprintf( out, "\n" );
}


/* Prints the ILP-name of 'edge' to 'f'. */
static inline void printILPEdgeName( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x%u", edge->edge_id );
}


/* Generate an ILP for lp_solve.
 *
 * The caller must free the returned string.
 * */
static char *generateOffsetGraphILP( const offset_graph *og, uint loopbound, enum ILPComputationType type )
{
  DSTART( "generateOffsetGraphILP" );

  // Get output file
  char *tmpfilename;
  MALLOC( tmpfilename, char*, 50 * sizeof( char ), "tmpfilename" );
  strcpy( tmpfilename, "test.ilp" );
  //tmpnam( tmpfilename );
  FILE *f = fopen( tmpfilename, "w" );
  if ( !f ) {
    prerr( "Unable to write to ILP file '%s'\n", tmpfilename );
  }
  DOUT( "Writing ILP to %s\n", tmpfilename );

  // In the ILP we map each edge to a variable 'x<edge_id>'
  // This variable represents the flow through the edge.

  // Write objective function
  fprintf( f, "/*\n * Objective function\n */\n\n" );

  fprintf( f, ( type == ILP_TYPE_WCET ? "MAX: " : "MIN: " ) );

  uint i;
  for ( i = 0; i < og->num_edges; i++ ) {

    offset_graph_edge * const edge = &og->edges[i];
    const ull factor = ( type == ILP_TYPE_BCET ? edge->bcet : edge->wcet );

    if ( factor != 0 ) {
      fprintf( f, "%llu ", factor );
    }
    printILPEdgeName( f, edge );
    if ( i != og->num_edges - 1 ) {
      fprintf( f, " + " );
    }
  }

  fprintf( f, ";\n" );


  // Write out constraints section.
  fprintf( f, "\n/*\n * Constraints\n */\n\n" );

  // Write out flow conservation constraints
  fprintf( f, "/* Flow conservation constraints */\n" );
  for ( i = 0; i < og->num_nodes; i++ ) {
    offset_graph_node * const node = &og->nodes[i];

    if ( node->num_incoming_edges == 0 ) {
      fprintf( f, "0" );
    } else {
      uint j;
      for ( j = 0; j < node->num_incoming_edges; j++ ) {
        printILPEdgeName( f, node->incoming_edges[j] );
        if ( j != node->num_incoming_edges - 1 ) {
          fprintf( f, " + " );
        }
      }
    }

    fprintf( f, " = " );

    if ( node->num_outgoing_edges == 0 ) {
      fprintf( f, "0" );
    } else {
      uint j;
      for ( j = 0; j < node->num_outgoing_edges; j++ ) {
        printILPEdgeName( f, node->outgoing_edges[j] );
        if ( j != node->num_outgoing_edges - 1 ) {
          fprintf( f, " + " );
        }
      }
    }

    fprintf( f, ";\n" );
  }

  // Write out demand / supply values
  fprintf( f, "\n/* Demand / supply constraints */\n" );
  const offset_graph_node * const suso = &( og->supersource );
  for ( i = 0; i < suso->num_outgoing_edges; i++ ) {
    printILPEdgeName( f, suso->outgoing_edges[i] );
    if ( i != suso->num_outgoing_edges - 1 ) {
      fprintf( f, " + " );
    }
  }
  fprintf( f, " = %u;\n", loopbound );

  const offset_graph_node * const susi = &( og->supersink );
  for ( i = 0; i < susi->num_incoming_edges; i++ ) {
    printILPEdgeName( f, susi->incoming_edges[i] );
    if ( i != susi->num_incoming_edges - 1 ) {
      fprintf( f, " + " );
    }
  }
  fprintf( f, " = %u;\n", loopbound );


  // Write out bounds section.
  fprintf( f, "\n/*\n * Variable bounds\n */\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];
    printILPEdgeName( f, edge );
    fprintf( f, " >= 0;\n" );
    printILPEdgeName( f, edge );
    fprintf( f, " <= %u;\n", loopbound );
  }


  // Write out declarations section.
  fprintf( f, "\n/*\n * Variable declarations\n */\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];
    fprintf( f, "int " );
    printILPEdgeName( f, edge );
    fprintf( f, ";\n");
  }

  fclose( f );

  DRETURN( tmpfilename );
}


/* Solves a given ilp with lp_solve. */
ull solveILP( const char *ilp_file )
{
  DSTART( "solveILP" );

  // Invoke external ILP solver.
  char commandline[500];
  char output_file[100];
  tmpnam( output_file );
  sprintf( commandline, "%s/lp_solve %s > %s", LP_SOLVE_PATH, ilp_file, output_file );

  const int ret = system( commandline );

  if ( ret == -1 ) {
    prerr( "Failed to invoke lp_solve with: %s", commandline );
  } else
  if ( ret == 1 ) {
    prerr( "Timeout occurred, though no timeout option was given!" );
  } else
  if ( ret == 2 ) {
    prerr( "Problem was infeasible!" );
  } else
  if ( ret == 3 ) {
    prerr( "Problem was unbounded!" );
  } else
  if ( ret == 7 ) {
    prerr( "lp_solve found no solution!" );
  } else
  if ( ret == 255 ) {
    prerr( "ILP was erroneous!" );
  } else
  if ( ret != 0 ) {
    prerr( "Failed to solve ILP. Maybe solver not available." );
  }

  DOUT( "Solved ILP, output is in %s\n", output_file );

  // Parse result file
  FILE *result_file = fopen( output_file, "r" );
  ull result;
  int read_count = 0;
  read_count = fscanf( result_file, "\n" );
  read_count = fscanf( result_file, "Value of objective function %llu", &result );
  fclose( result_file );

  // Delete output file
  remove( output_file );

  if ( read_count != 1 ) {

    prerr( "Could not read output file!" );
    DRETURN( 0 );

  } else {

    DOUT( "Result was: %llu\n", result );
    DRETURN( result );

  }
}


// #########################################
// #### Definitions of public functions ####
// #########################################


/* Creates a new offset graph with 'number_of_nodes' nodes
 * (excluding the supersink and supersource). */
offset_graph *createOffsetGraph( uint number_of_nodes )
{
  offset_graph *result;
  CALLOC( result, offset_graph*, 1, sizeof( offset_graph ), "result" );

  result->supersource.offset = 4000000;
  result->supersink.offset = 3000000;

  // Create the nodes
  CALLOC( result->nodes, offset_graph_node*, number_of_nodes, 
      sizeof( offset_graph_node ), "result->nodes" );
  result->num_nodes = number_of_nodes;
  result->num_edges = 0;
  uint i;
  for ( i = 0; i < result->num_nodes; i++ ) {
    offset_graph_node * const node = &( result->nodes[i] );
    node->offset = i;
    // Initialize all incident edge lists with size 0
    node->incoming_edges = 0;
    node->outgoing_edges = 0;
    node->num_incoming_edges = 0;
    node->num_outgoing_edges = 0;
  }

  return result;
}


/* Returns the edge that was added or NULL if nothing was added. */
offset_graph_edge *addOffsetGraphEdge(
    offset_graph *og, offset_graph_node *start,
    offset_graph_node *end, ull bcet, ull wcet )
{
  DSTART( "addOffsetGraphEdge" );
  assert( og && start && end && "Invalid arguments!" );

  DOUT( "Requesting creation of edge %u --> %u with BCET %llu"
      " and WCET %llu\n", start->offset, end->offset, bcet, wcet );

  if ( getOffsetGraphEdge( og, start, end ) != NULL ) {
    DOUT( "Edge existed, returning NULL!\n" );
    DRETURN( NULL );
  } else {

    // Create new edge
    og->num_edges++;
    CALLOC_OR_REALLOC( og->edges, offset_graph_edge*,
        og->num_edges * sizeof( offset_graph_edge ), "og->edges" );
    offset_graph_edge *new_edge = &( og->edges[og->num_edges - 1] );

    new_edge->start   = start;
    new_edge->end     = end;
    new_edge->bcet    = bcet;
    new_edge->wcet    = wcet;
    new_edge->edge_id = og->num_edges;

    // Register with the nodes
    start->num_outgoing_edges++;
    CALLOC_OR_REALLOC( start->outgoing_edges, offset_graph_edge**,
        start->num_outgoing_edges * sizeof( offset_graph_edge* ),
        "start->outgoing_edges" );
    start->outgoing_edges[start->num_outgoing_edges - 1] = new_edge;

    end->num_incoming_edges++;
    CALLOC_OR_REALLOC( end->incoming_edges, offset_graph_edge**,
        end->num_incoming_edges * sizeof( offset_graph_edge* ),
        "end->incoming_edges" );
    end->incoming_edges[end->num_incoming_edges - 1] = new_edge;

    // Dump edges
    DACTION( dumpOffsetGraph( og, stdout ) );

    // Return the new edge
    DRETURN( new_edge );
  }
}


/* Returns the edge with the given start and end nodes, or NULL if it doesn't exist. */
offset_graph_edge *getOffsetGraphEdge( offset_graph *og,
    const offset_graph_node *start, const offset_graph_node *end )
{
  assert( og && start && end && "Invalid arguments!" );

  uint i;
  for ( i = 0; i < start->num_outgoing_edges; i++ ) {
    if ( start->outgoing_edges[i]->end->offset == end->offset ) {
      return start->outgoing_edges[i];
    }
  }

  return NULL;
}

/* Gets the node which represents offset 'offset'. */
offset_graph_node *getOffsetGraphNode( offset_graph *og, uint offset )
{
  assert( og && "Invalid arguments!" );

  if ( offset >= og->num_nodes ) {
    return NULL;
  } else {
    return &( og->nodes[offset] );
  }
}


/* Prints the offset graph to th given file descriptor. */
void dumpOffsetGraph( const offset_graph *og, FILE *out )
{
  fprintf( out, "Offset graph with %u nodes and %u edges\n",
      og->num_nodes, og->num_edges );
  fprintf( out, "\n");

  uint i;
  fprintf( out, "Nodes: \n" );
  dumpOffsetGraphNode( &og->supersource, out );
  dumpOffsetGraphNode( &og->supersink, out );
  for ( i = 0; i < og->num_nodes; i++ ) {
    const offset_graph_node * const node = &og->nodes[i];
    dumpOffsetGraphNode( node, out );
  }

  fprintf( out, "\n");

  fprintf( out, "Edges: \n" );
  for ( i = 0; i < og->num_edges; i++ ) {
    const offset_graph_edge * const edge = &og->edges[i];

    fprintf( out, "  %u: node %u --> node %u with BCET %llu, WCET %llu \n",
        edge->edge_id, edge->start->offset, edge->end->offset, edge->bcet,
        edge->wcet );
  }
}


/* Solves a minimum cost flow problem to obtain the final BCET.
 *
 * 'loopbound_min' specifies the minimum number of iterations of the loop.
 */
ull computeOffsetGraphLoopBCET( const offset_graph *og, uint loopbound_min )
{
  assert( og && "Invalid arguments!" );

  char * const tmpfile = generateOffsetGraphILP( og, loopbound_min, ILP_TYPE_BCET );
  const ull result = solveILP( tmpfile );

  //remove( tmpfile );
  free( tmpfile );

  return result;
}
/* Solves a maximum cost flow problem to obtain the final WCET.
 *
 * 'loopbound_max' specifies the maximum number of iterations of the loop.
 */
ull computeOffsetGraphLoopWCET( const offset_graph *og, uint loopbound_max )
{
  assert( og && "Invalid arguments!" );

  char * const tmpfile = generateOffsetGraphILP( og, loopbound_max, ILP_TYPE_WCET );
  const ull result = solveILP( tmpfile );

  //remove( tmpfile );
  free( tmpfile );

  return result;
}


/* Deallocates an offset graph. */
void freeOffsetGraph( offset_graph *og )
{
  assert( og && "Invalid arguments!" );

  // Free the edges
  free( og->edges );
  og->edges = NULL;

  // Free the nodes
  free( og->nodes );
  og->nodes = NULL;

  // Free the graph itself
  free( og );
}
