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
  ILP_TYPE_BCET,    /* An ILP for determining the final BCET of the loop. */
  ILP_TYPE_WCET,    /* An ILP for determining the final WCET of the loop. */
  ILP_TYPE_OFFSETS  /* An ILP for determining the final offsets after the execution of the loop. */
};

// #########################################
// #### Declaration of static variables ####
// #########################################


// Whether to keep the temporary files generated during the analysis
static _Bool keepTemporaryFiles = 0;


/* Two variable names for the offset ILP. */
static const char * const min_offset_var = "x_min";
static const char * const max_offset_var = "x_max";


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
 * */
static inline void fprintILPEdgeName( FILE *f, const offset_graph_edge *edge,
    uint time )
{
  fprintf( f, "x%u_%u", edge->edge_id, time );
}


/* Prints the ILP-name of 'x_{active}(e)' variable which indicates whether
 * x(e,num_time_steps - runtime(e)) is bigger than zero. 'e' must be an edge
 * which ends at the supersink.
 * */
static inline void fprintILPXActive( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x_active_%u", edge->edge_id );
}


/* Prints the ILP-name of 'x_{active_helper}(e)' variable which is used during
 * the x_{active}(e) determination.
 * */
static inline void fprintILPXActiveHelper( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x_active_helper_%u", edge->edge_id );
}


/* Returns the runtime of an edge for the dynamic flow in the offset graph. */
static inline uint getOffsetGraphEdgeRuntime( const offset_graph *og,
    const offset_graph_edge *edge )
{
  return 1;
}


/* Generate an ILP for lp_solve. The function can generate various ILPs which differ
 * only marginally. The type of ILP which is generated is determined by the parameter
 * 'type'.
 *
 * Returns the name of the file to which the ILP was written. The caller must free
 * the returned string and delete the file if he does not want to keep it.
 * */
static char *generateOffsetGraphILP( const offset_graph *og, uint loopbound,
    enum ILPComputationType type )
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

  /* We have two extra time steps in the dynamic flow, one for the
   * transition from the supersource and one for the transition to
   * the supersink. */
  const uint num_time_steps = loopbound + 2;

  /* Write objective function. */
  fprintf( f, "/*\n * Objective function\n */\n\n" );

  uint i, j, k;
  _Bool firstTerm = 1;

  switch ( type ) {

    case ILP_TYPE_BCET:
    case ILP_TYPE_WCET:
       /* The objective is:
        *
        * sum_{e \in Edges} sum_{t = 0}^{num_time_steps-1} c(e)x(e,t)
        *
        * which is to minimize or maximize, depending on whether we
        * search BCET or WCET values. c(e) is then defined accordingly:
        *
        * c(e) = bcet(e) + bcet(e->start) if we search BCETs, or
        * c(e) = wcet(e) + wcet(e->start) if we search WCETs.
        */
      fprintf( f, ( type == ILP_TYPE_WCET ? "MAX: " : "MIN: " ) );

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
            fprintILPEdgeName( f, edge, j );
          }
        }
      }

      break;

    case ILP_TYPE_OFFSETS:
      /* This ILP has a different objective function. It emits two flow units
       * from the supersource and tries to maximize the difference between the
       * offsets from which the flow units arrive at the supersink. */
      fprintf( f, "MAX: %s - %s", max_offset_var, min_offset_var );
      break;

    default:
      assert( 0 && "Unknown ILP type!" );
      break;
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
          fprintILPEdgeName( f, edge, j - runtime );
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
        fprintILPEdgeName( f, edge, j );
      }

      /* If there are no  matching edges: Set to zero. */
      if ( firstTerm ) {
        fprintf( f, "0" );
      }

      fprintf( f, ";\n" );
    }
  }


  /* The number of flow units in transit. */
  const uint flow_units = ( type == ILP_TYPE_OFFSETS ? 2 : 1 );

  /* Write out demand / supply values
   *
   * Only the supersource emits 'flow_units' flow units at time 0 and
   * only the supersink consumes 'flow_units' units at time 'num_time_steps'.
   */
  fprintf( f, "\n/* Demand / supply constraints */\n" );
  const offset_graph_node * const suso = &( og->supersource );
  for ( j = 0; j < num_time_steps; j++ ) {
    firstTerm = 1;
    for ( k = 0; k < suso->num_outgoing_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[suso->outgoing_edges[k]];

      if ( !firstTerm ) {
        fprintf( f, " + " );
      } else {
        firstTerm = 0;
      }
      fprintILPEdgeName( f, edge, j );
    }

    // Only at time '0', flow units may leave the source.
    fprintf( f, " = %u;\n", ( j == 0 ? flow_units : 0 ) );
  }

  const offset_graph_node * const susi = &( og->supersink );
  for ( j = 0; j <= num_time_steps; j++ ) {
    firstTerm = 1;
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      if ( j >= runtime ) {
        if ( !firstTerm ) {
          fprintf( f, " + " );
        } else {
          firstTerm = 0;
        }
        fprintILPEdgeName( f, edge, j - runtime );
      }
    }

    // Only at time 'num_time_steps', flow units may enter the sink.
    // Only print this if any flow may arrive at this time instant
    if ( !firstTerm ) {
      fprintf( f, " = %u;\n", ( j == num_time_steps ? flow_units : 0 ) );
    }
  }


  /* For the offset ILP write out the definitions of the min_offset
   * and max_offset variables. */
  if ( type == ILP_TYPE_OFFSETS ) {
    /* \forall_{e=(o,supersink) \in Edges}:
     *   x_active(e) = 1 <=> x(e,num_time_steps - runtime(e)) > 0  */
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      /* The constraints are:
       *
       *     x_active(e) = 1 => x(e,num_time_steps - runtime(e)) > 0
       * <=> x(e,num_time_steps - runtime(e)) + (1 - x_active(e)) > 0
       * <=> x(e,num_time_steps - runtime(e)) + 1 > x_active(e)
       * <=> x(e,num_time_steps - runtime(e)) \geq x_active(e)
       *
       * for the "=>" direction and
       *
       *     x(e,num_time_steps - runtime(e)) > 0 => x_active(e) = 1
       * <=> x(e,num_time_steps - runtime(e)) > 0 => x_active(e) > 0
       *     (x_active(e) <= 1 is guaranteed by the bounds below)
       * <=> x(e,num_time_steps - runtime(e)) < 1 \or x_active(e) > 0
       * <=> x(e,num_time_steps - runtime(e)) < 1 + flow_units*y
       *     x_active(e) > -1*(1-y)
       * <=> x(e,num_time_steps - runtime(e)) < 1 + flow_units*y
       *     x_active(e) + 1 > y
       * <=> x(e,num_time_steps - runtime(e)) \leq flow_units*y
       *     x_active(e) \geq y
       *
       * for the "<=" direction
       * */
      fprintILPEdgeName( f, edge, num_time_steps - runtime );
      fprintf( f, " >= " );
      fprintILPXActive( f, edge );
      fprintf( f, ";\n" );

      fprintILPEdgeName( f, edge, num_time_steps - runtime );
      fprintf( f, " <= %u ", flow_units );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, ";\n" );

      fprintILPXActive( f, edge );
      fprintf( f, " >= " );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, ";\n" );
    }

    /* \forall_{e=(o,supersink) \in Edges}:
     *     x_active(e) = 1 --> min_offset >= offset(o)
     * <=> min_offset >= offset(o) * x_active(e)
     */
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      fprintf( f, "%s >= %u ", min_offset_var, edge->start->offset );
      fprintILPXActive( f, edge );
      fprintf( f, ";\n" );
    }

    /* \forall_{e=(o,supersink) \in Edges}:
     *     x_active(e) = 1 --> max_offset <= offset(o)
     * <=> max_offset <= offset(o) * x_active(e) + og->num_nodes * (1-x_active(e))
     * <=> max_offset + og->num_nodes * x_active(e)
     *       <= offset(o) * x_active(e) + og->num_nodes
     */
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      fprintf( f, "%s + %u ", max_offset_var, og->num_nodes );
      fprintILPXActive( f, edge );
      fprintf( f, " <= %u ", edge->start->offset );
      fprintILPXActive( f, edge );
      fprintf( f, " + %u;\n", og->num_nodes );
    }
  }


  /* Write out bounds section.
   *
   * At each time instant only 'flow_units' units may flow through
   * each of the edges, which represents the loop execution.
   */
  fprintf( f, "\n/*\n * Variable bounds\n */\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];

    for ( j = 0; j < num_time_steps; j++ ) {
      fprintILPEdgeName( f, edge, j );
      fprintf( f, " >= %u;\n", 0 );
      fprintILPEdgeName( f, edge, j );
      fprintf( f, " <= %u;\n", flow_units );
    }
  }
  // Offset-mode specific bounds
  if ( type == ILP_TYPE_OFFSETS ) {
    fprintf( f, "%s >= %u;\n", min_offset_var, 0 );
    fprintf( f, "%s <= %u;\n", min_offset_var, og->num_nodes - 1 );
    fprintf( f, "%s >= %u;\n", max_offset_var, 0 );
    fprintf( f, "%s <= %u;\n", max_offset_var, og->num_nodes - 1 );

    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];

      fprintILPXActive( f, edge );
      fprintf( f, " >= %u;\n", 0 );
      fprintILPXActive( f, edge );
      fprintf( f, " <= %u;\n", 1 );

      fprintILPXActiveHelper( f, edge );
      fprintf( f, " >= %u;\n", 0 );
      fprintILPXActiveHelper( f, edge );
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
      fprintILPEdgeName( f, edge, j );
      fprintf( f, ";\n");
    }
  }
  // Offset-mode specific declarations
  if ( type == ILP_TYPE_OFFSETS ) {
    fprintf( f, "/* Minimum final offset. */\n" );
    fprintf( f, "int %s;\n", min_offset_var );
    fprintf( f, "/* Maximum final offset. */\n" );
    fprintf( f, "int %s;\n", max_offset_var );

    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      fprintf( f, "/* Indicator whether " );
      fprintILPEdgeName( f, edge, num_time_steps - runtime );
      fprintf( f, " > 0 */\n" );
      fprintf( f, "int " );
      fprintILPXActive( f, edge );
      fprintf( f, ";\n");

      fprintf( f, "/* Helper variable for determining " );
      fprintILPXActive( f, edge );
      fprintf( f, " */\n" );
      fprintf( f, "int " );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, ";\n");
    }
  }

  fclose( f );

  DRETURN( tmpfilename );
}


/* Invokes the solver for a given ILP file and returns the output
 * file's path. The caller must free this string then. */
static char* invokeILPSolver( const char *ilp_file )
{
  DSTART( "invokeILPSolver" );

  // Invoke external ILP solver.
  char commandline[500];
  char *output_file;
  MALLOC( output_file, char*, 100 * sizeof( char ), "output_file" );
  if ( keepTemporaryFiles ) {
    strcpy( output_file, "offsetGraph.result" );
  } else {
    tmpnam( output_file );
  }
  sprintf( commandline, "%s/lp_solve -presolve %s > %s\n", LP_SOLVE_PATH,
      ilp_file, output_file );
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
  DRETURN( output_file );
}


/* Solves a given ilp with lp_solve. */
static ull solveET_ILP( const offset_graph *og, const char *ilp_file )
{
  DSTART( "solveET_ILP" );

  const char * const output_file = invokeILPSolver( ilp_file );

  // Parse result file
  FILE *result_file = fopen( output_file, "r" );
  ull result;
  const uint line_size = 500;
  char result_file_line[line_size];
  char result_string[50];

  _Bool successfully_read =
    // Skip first line
    fgets( result_file_line, line_size, result_file ) &&
    // Read the result in the second line
    fgets( result_file_line, line_size, result_file ) &&
    sscanf( result_file_line, "%*s %*s %*s %*s %s", result_string );

  if ( successfully_read ) {
    // Parse the number (first try direct format)
    char *end_ptr = NULL;
    result = strtoull( result_string, &end_ptr, 10 );
    successfully_read = *end_ptr == '\0';
    /* In case of failure:
     * Try to parse the number in scientific format (2.81864e+06) */
    if ( !successfully_read ) {
      double time_result_float = strtod( result_string, &end_ptr );
      successfully_read = *end_ptr == '\0';
      if ( successfully_read ) {
        assert( floor( time_result_float ) == time_result_float &&
            "Invalid result!" );
        result = (ull)time_result_float;
      }
    }
  }

  fclose( result_file );

  // Delete output file
  if ( !keepTemporaryFiles ) {
    remove( output_file );
  }
  free( (void*)output_file );

  if ( successfully_read ) {
    DOUT( "Result was: %llu\n", result );
    DRETURN( result );
  } else {
    prerr( "Could not read output file!" );
    DRETURN( result );
  }
}


/* Solves a given ilp with lp_solve. */
static tdma_offset_bounds solveOffset_ILP( const offset_graph *og,
    const char *ilp_file )
{
  DSTART( "solveOffset_ILP" );

  const char * const output_file = invokeILPSolver( ilp_file );

  // Parse result file
  FILE *result_file = fopen( output_file, "r" );
  tdma_offset_bounds result;
  const uint line_size = 500;
  char result_file_line[line_size];
  char var_name[50];
  uint var_value;

  // Skip first 4 lines
  _Bool skipped =
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file );
  // Read until the bound variables were found
  _Bool foundMin = 0;
  _Bool foundMax = 0;
  while( fgets( result_file_line, line_size, result_file ) &&
         ( !foundMax || !foundMin ) ) {

    sscanf( result_file_line, "%s %u", var_name, &var_value );
    if ( strcmp( var_name, min_offset_var ) ) {
      result.lower_bound = var_value;
      foundMin = 1;
    } else
    if ( strcmp( var_name, max_offset_var ) ) {
      result.upper_bound = var_value;
      foundMax = 1;
    }
  }

  fclose( result_file );

  // Delete output file
  if ( !keepTemporaryFiles ) {
    remove( output_file );
  }
  free( (void*)output_file );

  if ( skipped && foundMax && foundMin ) {
    DOUT( "Result was: [%u, %u]\n", result.lower_bound, result.upper_bound );
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
  const ull result = solveET_ILP( og, tmpfile );

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

  char * const tmpfile = generateOffsetGraphILP( og,
      loopbound_max, ILP_TYPE_WCET );
  const ull result = solveET_ILP( og, tmpfile );

  if ( !keepTemporaryFiles ) {
    remove( tmpfile );
  }
  free( tmpfile );

  return result;
}
/* Solves a flow problem to obtain the final offsets.
 *
 * 'loopbound_max' specifies the maximum number of iterations of the loop.
 */
tdma_offset_bounds computeOffsetGraphLoopOffsets( const offset_graph *og,
    uint loopbound_max )
{
  assert( og && "Invalid arguments!" );

  char * const tmpfile = generateOffsetGraphILP( og,
      loopbound_max, ILP_TYPE_OFFSETS );
  const tdma_offset_bounds result = solveOffset_ILP( og, tmpfile );

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
