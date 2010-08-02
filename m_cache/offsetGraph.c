// Include standard library headers
#include <assert.h>
#include <math.h>
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


/* Indicates which type of timing analysis to perform. */
enum ILPComputationType {
  ILP_TYPE_BCET,
  ILP_TYPE_WCET
};

// #########################################
// #### Declaration of static variables ####
// #########################################


// Whether to keep the temporary files generated during the analysis
static _Bool keepTemporaryFiles = 0;


// #########################################
// #### Definitions of static functions ####
// #########################################


/* Dumps a single node to the given file descriptor. */
static void dumpOffsetGraphNode( const offset_graph *og,
    const offset_graph_node *node, FILE *out )
{
  fprintf( out, "  %u: BCET %llu, WCET %llu\n", node->offset, node->bcet, node->wcet );

  uint j;
  fprintf( out, "    In-Edges : " );
  for ( j = 0; j < node->num_incoming_edges; j++ ) {
    const offset_graph_edge * const edge = &og->edges[node->incoming_edges[j]];
    fprintf( out, "%u (from node %u)", edge->edge_id, edge->start->offset );
    if ( j != node->num_incoming_edges - 1 ) {
      fprintf( out, ", " );
    }
  }
  fprintf( out, "\n" );

  fprintf( out, "    Out-Edges: " );
  for ( j = 0; j < node->num_outgoing_edges; j++ ) {
    const offset_graph_edge * const edge = &og->edges[node->outgoing_edges[j]];
    fprintf( out, "%u (to node %u)", edge->edge_id, edge->end->offset );
    if ( j != node->num_outgoing_edges - 1 ) {
      fprintf( out, ", " );
    }
  }
  fprintf( out, "\n" );
}


/* Prints the ILP-name of 'edge' for time 'time' to 'f'.
 * (Each edge has (loopbound) flow variables associated to it to represent
 *  the flow into that edge at the given time instant. This flow will then
 *  arrive at the target of the edge at the next time instant.)
 */
static inline void printILPEdgeName( FILE *f, const offset_graph_edge *edge,
    uint time )
{
  fprintf( f, "x%u_%u", edge->edge_id, time );
}


/* Returns the runtime of an edge for the dynamic flow in the offset graph. */
static inline uint getOffsetGraphEdgeRuntime( const offset_graph *og,
    const offset_graph_edge *edge )
{
  return 1;
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
  if ( keepTemporaryFiles ) {
    strcpy( tmpfilename, "offsetGraph.ilp" );
  } else {
    tmpnam( tmpfilename );
  }
  FILE *f = fopen( tmpfilename, "w" );
  if ( !f ) {
    prerr( "Unable to write to ILP file '%s'\n", tmpfilename );
  }
  DOUT( "Writing ILP to %s\n", tmpfilename );

  /* Write objective function. The objective is:
   *
   * sum_{e \in Edges} sum_{t = 0}^{loopbound-1} c(e)x(e,t)
   *
   * which is to minimize or maximize, depending on whether we
   * search BCET or WCET values. c(e) is then defined accordingly:
   *
   * c(e) = bcet(e) + bcet(e->start) if we search BCETs, or
   * c(e) = wcet(e) + wcet(e->start) if we search WCETs.
   */
  fprintf( f, "/*\n * Objective function\n */\n\n" );

  fprintf( f, ( type == ILP_TYPE_WCET ? "MAX: " : "MIN: " ) );

  // We have two extra time steps for the transition from the
  // supersource and to the supersink
  const uint num_time_steps = loopbound + 2;

  uint i, j;
  _Bool firstTerm = 1;
  for ( i = 0; i < og->num_edges; i++ ) {
    const offset_graph_edge * const edge = &og->edges[i];
    // Add the cost of the start node to account for that cost too
    const ull factor = ( type == ILP_TYPE_BCET
      ? edge->bcet + edge->start->bcet
      : edge->wcet + edge->start->wcet );

    if ( factor != 0 ) {
      for ( j = 0; j < num_time_steps; j++ ) {
        if ( !firstTerm ) {
          fprintf( f, " + " );
        } else {
          firstTerm = 0;
        }
        fprintf( f, "%llu ", factor );
        printILPEdgeName( f, edge, j );
      }
    }
  }

  fprintf( f, ";\n" );


  /* Write out constraints section. */
  fprintf( f, "\n/*\n * Constraints\n */\n\n" );

  /* Write out flow conservation constraints.
   *
   * We are workin gon a dynamic flow here, which may not buffer any flow
   * inside the nodes, so the flow conservation constraint for node 'n' is:
   *
   * sum_{e \in n->incoming} x(e, t - \tau(e)) =
   *                                        sum_{e \in n->outgoing} x(e, t)
   *
   * where \tau(e) is the runtime of the edges (the time it takes for the
   * flow to pass the edge). This is 1 for all edges.
   */
  fprintf( f, "/* Flow conservation constraints */\n" );
  for ( i = 0; i < og->num_nodes; i++ ) {
    offset_graph_node * const node = &og->nodes[i];

    // Skip empty restrictions
    if ( node->num_incoming_edges == 0 &&
         node->num_outgoing_edges == 0 ) {
      continue;
    }

    // Write comment
    fprintf( f, "/* Node %u: incoming edges from ", node->offset );
    uint k;
    if ( node->num_incoming_edges == 0 ) {
      fprintf( f, "nowhere" );
    } else {
      for ( k = 0; k < node->num_incoming_edges; k++ ) {
        fprintf( f, "%u", og->edges[node->incoming_edges[k]].start->offset );
        if ( k != node->num_incoming_edges - 1 ) {
          fprintf( f, ", " );
        }
      }
    }
    fprintf( f, " - outgoing edges to " );
    if ( node->num_outgoing_edges == 0 ) {
      fprintf( f, "nowhere" );
    } else {
      for ( k = 0; k < node->num_outgoing_edges; k++ ) {
        fprintf( f, "%u", og->edges[node->outgoing_edges[k]].end->offset );
        if ( k != node->num_outgoing_edges - 1 ) {
          fprintf( f, ", " );
        }
      }
    }
    fprintf( f, " */\n" );

    // Write conservation constraints (1 per time instant)
    for ( j = 0; j < num_time_steps; j++ ) {
      /* The flow that enters the edges at 'j - runtime(edge)'
       * arrives at our current node at time 'j' ... */
      firstTerm = 1;
      for ( k = 0; k < node->num_incoming_edges; k++ ) {
        const offset_graph_edge * const edge = &og->edges[node->incoming_edges[k]];
        const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

        if ( j >= runtime ) {
          if ( !firstTerm ) {
            fprintf( f, " + " );
          } else {
            firstTerm = 0;
          }
          printILPEdgeName( f, edge, j - runtime );
        }
      }

      /* If there are no  matching edges: Set to zero. */
      if ( firstTerm ) {
        fprintf( f, "0" );
      }

      fprintf( f, " = " );

      /* ... and must directly leave the node (no buffering). */
      firstTerm = 1;
      for ( k = 0; k < node->num_outgoing_edges; k++ ) {
        const offset_graph_edge * const edge = &og->edges[node->outgoing_edges[k]];

        if ( !firstTerm ) {
          fprintf( f, " + " );
        } else {
          firstTerm = 0;
        }
        printILPEdgeName( f, edge, j );
      }

      /* If there are no  matching edges: Set to zero. */
      if ( firstTerm ) {
        fprintf( f, "0" );
      }

      fprintf( f, ";\n" );
    }
  }


  /* Write out demand / supply values
   *
   * Only the supersource emits a single flow unit at time 0 and
   * only the supersink consumes a single flow unit at time 'num_time_steps'.
   */
  fprintf( f, "\n/* Demand / supply constraints */\n" );
  const offset_graph_node * const suso = &( og->supersource );
  for ( j = 0; j < num_time_steps; j++ ) {
    firstTerm = 1;
    uint k;
    for ( k = 0; k < suso->num_outgoing_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[suso->outgoing_edges[k]];

      if ( !firstTerm ) {
        fprintf( f, " + " );
      } else {
        firstTerm = 0;
      }
      printILPEdgeName( f, edge, j );
    }

    // Only at time '0', one flow unit may leave the source.
    fprintf( f, " = %u;\n", ( j == 0 ? 1 : 0 ) );
  }

  const offset_graph_node * const susi = &( og->supersink );
  for ( j = 0; j <= num_time_steps; j++ ) {
    firstTerm = 1;
    uint k;
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      if ( j >= runtime ) {
        if ( !firstTerm ) {
          fprintf( f, " + " );
        } else {
          firstTerm = 0;
        }
        printILPEdgeName( f, edge, j - runtime );
      }
    }

    // Only at time 'num_time_steps', one flow unit may enter the sink.
    // Only print this if any flow may arrive at this time instant
    if ( !firstTerm ) {
      fprintf( f, " = %u;\n", ( j == num_time_steps ? 1 : 0 ) );
    }
  }


  /* Write out bounds section.
   *
   * At each time instant only one flow unit may flow through each of
   * the edges, which represents the loop execution.
   */
  fprintf( f, "\n/*\n * Variable bounds\n */\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];

    for ( j = 0; j < num_time_steps; j++ ) {
      printILPEdgeName( f, edge, j );
      fprintf( f, " >= 0;\n" );
      printILPEdgeName( f, edge, j );
      fprintf( f, " <= %u;\n", 1 );
    }
  }


  /* Write out declarations section.
   *
   * See 'printILPEdgeName' for documentation about the flow variables.
   */
  fprintf( f, "\n/*\n * Variable declarations\n */\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];

    for ( j = 0; j < num_time_steps; j++ ) {
      fprintf( f, "/* Edge from %u to %u (time %u) */\n",
          edge->start->offset, edge->end->offset, j );
      fprintf( f, "int " );
      printILPEdgeName( f, edge, j );
      fprintf( f, ";\n");
    }
  }

  fclose( f );

  DRETURN( tmpfilename );
}


/* Solves a given ilp with lp_solve. */
ull solveILP( const offset_graph *og, const char *ilp_file )
{
  DSTART( "solveILP" );

  // Invoke external ILP solver.
  char commandline[500];
  char output_file[100];
  if ( keepTemporaryFiles ) {
    strcpy( output_file, "offsetGraph.result" );
  } else {
    tmpnam( output_file );
  }
  sprintf( commandline, "%s/lp_solve -presolve %s > %s\n", LP_SOLVE_PATH, ilp_file, output_file );
  DOUT( "Called lp_solve: %s", commandline );

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
  const uint line_size = 500;
  char result_file_line[line_size];
  char result_string[50];

  _Bool successfully_read = // Skip first line
                            fgets( result_file_line, line_size, result_file ) &&
                            // Read the result in the second line
                            fgets( result_file_line, line_size, result_file ) &&
                            sscanf( result_file_line, "%*s %*s %*s %*s %s", result_string );
  if ( successfully_read ) {
    // Parse the number (first try direct format)
    char *end_ptr = NULL;
    result = strtoull( result_string, &end_ptr, 10 );
    successfully_read = *end_ptr == '\0';
    // In case of failure: Try to parse the number in scientific format (2.81864e+06)
    if ( !successfully_read ) {
      double time_result_float = strtod( result_string, &end_ptr );
      successfully_read = *end_ptr == '\0';
      if ( successfully_read ) {
        assert( floor( time_result_float ) == time_result_float && "Invalid result!" );
        result = (ull)time_result_float;
      }
    }
  }
  fclose( result_file );

  // Delete output file
  if ( !keepTemporaryFiles ) {
    remove( output_file );
  }

  if ( successfully_read ) {
    DOUT( "Result was: %llu\n", result );
    DRETURN( result );
  } else {
    prerr( "Could not read output file!" );
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
    node->bcet = 0;
    node->wcet = 0;

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

    // Create new edge. Be aware that the addresses of all edges may
    // change here, thus rendering any pointer to an edge invalid.
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
    CALLOC_OR_REALLOC( start->outgoing_edges, uint*,
        start->num_outgoing_edges * sizeof( uint ),
        "start->outgoing_edges" );
    start->outgoing_edges[start->num_outgoing_edges - 1] = og->num_edges - 1;

    end->num_incoming_edges++;
    CALLOC_OR_REALLOC( end->incoming_edges, uint*,
        end->num_incoming_edges * sizeof( uint ),
        "end->incoming_edges" );
    end->incoming_edges[end->num_incoming_edges - 1] = og->num_edges - 1;

    // Return the new edge
    DRETURN( new_edge );
  }
}


/* Returns the edge with the given start and end nodes, or NULL if it doesn't exist. */
offset_graph_edge *getOffsetGraphEdge( const offset_graph *og,
    const offset_graph_node *start, const offset_graph_node *end )
{
  assert( og && start && end && "Invalid arguments!" );

  uint i;
  for ( i = 0; i < start->num_outgoing_edges; i++ ) {
    const offset_graph_edge * const edge = &og->edges[start->outgoing_edges[i]];
    if ( edge->end->offset == end->offset ) {
      return &og->edges[start->outgoing_edges[i]];
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
  dumpOffsetGraphNode( og, &og->supersource, out );
  dumpOffsetGraphNode( og, &og->supersink, out );
  for ( i = 0; i < og->num_nodes; i++ ) {
    const offset_graph_node * const node = &og->nodes[i];
    dumpOffsetGraphNode( og, node, out );
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
  const ull result = solveILP( og, tmpfile );

  if ( !keepTemporaryFiles ) {
    remove( tmpfile );
  }
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
  const ull result = solveILP( og, tmpfile );

  if ( !keepTemporaryFiles ) {
    remove( tmpfile );
  }
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
  uint i;
  for ( i = 0; i < og->num_nodes; i++ ) {
    const offset_graph_node * const node = &og->nodes[i];
    free( node->incoming_edges );
    free( node->outgoing_edges );
  }
  free( og->nodes );
  og->nodes = NULL;

  // Free the graph itself
  free( og );
}
