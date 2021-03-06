// Include standard library headers
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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


/* IDs of the built-in nodes. */
#define SUPERSINK_ID ( UINT_MAX - 10U )
#define SUPERSOURCE_ID ( UINT_MAX - 9U )
#define UNKOWN_OFFSET_NODE_ID ( UINT_MAX - 8U )

/* Indicates which type of timing analysis to perform. */
enum ILPComputationType {
  ILP_COMP_TYPE_BCET,    /* An ILP for determining the final BCET of the loop. */
  ILP_COMP_TYPE_WCET,    /* An ILP for determining the final WCET of the loop. */
  ILP_COMP_TYPE_OFFSETS  /* An ILP for determining the final offsets after the execution of the loop. */
};

/* Indicates which type of ILP solver is used. */
enum ILPSolver {
  ILP_CPLEX,   /* The CPLEX solver. */
  ILP_LPSOLVE  /* The lp_solve solver. */
};


// #########################################
// #### Declaration of static variables ####
// #########################################


// Whether to keep the temporary files generated during the analysis
static _Bool keepTemporaryFiles = 0;

/* The ILP solver to use. */
static enum ILPSolver ilpSolver = ILP_CPLEX;


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


/* The prefix used for the x_active variables. */
#define X_ACTIVE_PREFIX "x_active_"

/* Prints the ILP-name of 'x_active(e)' variable which indicates whether
 * x(e,num_time_steps - runtime(e)) is bigger than zero. 'e' must be an edge
 * which ends at the supersink.
 * */
static inline void fprintILPXActive( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "%s%u", X_ACTIVE_PREFIX, edge->edge_id );
}


/* Prints the ILP-name of 'x_active_helper(e)' variable which is used during
 * the x_active(e) determination.
 * */
static inline void fprintILPXActiveHelper( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x_active_helper_%u", edge->edge_id );
}


/* Prints the ILP-name of the 'min_selector(e)' variable which is used during
 * the computation of 'min_offset' in the offset ILP.
 * */
static inline void fprintILPMinSelector( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x_min_selector_%u", edge->edge_id );
}


/* Prints the ILP-name of the 'max_selector(e)' variable which is used during
 * the computation of 'max_offset' in the offset ILP.
 * */
static inline void fprintILPMaxSelector( FILE *f, const offset_graph_edge *edge )
{
  fprintf( f, "x_max_selector_%u", edge->edge_id );
}


/* Returns the runtime of an edge for the dynamic flow in the offset graph. */
static inline uint getOffsetGraphEdgeRuntime( const offset_graph *og,
    const offset_graph_edge *edge )
{
  return 1;
}


/* Writes out a flow conservation constraint for the given node the the given file. */
static void writeFlowConservationConstraint( FILE * const f,
    const offset_graph * const og, const offset_graph_node * const node,
    const uint num_time_steps )
{
  DSTART( "writeFlowConservationConstraint" );

  uint j, k;

  // Skip empty restrictions
  if ( node->num_incoming_edges == 0 &&
       node->num_outgoing_edges == 0 ) {
    DRETURN();
  }

  // Write comment
  DACTION(
    fprintf( f, "  \\ Node %u: incoming edges from ", node->offset );
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
    fprintf( f, "\n" );
  );

  // Write conservation constraints (1 per time instant)
  for ( j = 0; j < num_time_steps; j++ ) {
    fprintf( f, "  " );
    /* The flow that enters the edges at 'j - runtime(edge)'
     * arrives at our current node at time 'j' ... */
    _Bool firstTerm = 1;
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

    /* ... minus the flow which leaves the node .... */
    for ( k = 0; k < node->num_outgoing_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[node->outgoing_edges[k]];

      fprintf( f, " - " );
      fprintILPEdgeName( f, edge, j );
    }

    /* ... must be zero to conserve the flow (no buffering). */
    fprintf( f, " = 0\n" );
  }

  DEND();
}


/* Writes the ILP in CPLEX format. */
static void writeCPLEXILP( FILE *f, const offset_graph *og, uint loopbound,
    enum ILPComputationType computation_type )
{
  DSTART( "writeCPLEXILP" );

  /* ASSUMPTION: Some constraints in the ILP build upon the assumption,
   *             that the number of nodes in the graph is equal to the
   *             TDMA cycle length.
   */

  /* ILP hints:
   * - CPLEX demands constraints where all variables are on the left, and all
   *   constants on the right side of the constraint
   * - lp_solve treats '<' and '>' like '<=' and '>=' !!! Thus only use the
   *   latter in the ILP for clarification.
   */

  /* Helper variables. */
  uint i, j, k;
  _Bool firstTerm = 1;

  const offset_graph_node * const suso = &( og->supersource );
  const offset_graph_node * const susi = &( og->supersink );

  /* We have two extra time steps in the dynamic flow, one for the
   * transition from the supersource and one for the transition to
   * the supersink. */
  const uint num_time_steps = loopbound + 2;

  /* Write objective function. */
  switch ( computation_type ) {

    case ILP_COMP_TYPE_BCET:
    case ILP_COMP_TYPE_WCET:
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
      fprintf( f, ( computation_type == ILP_COMP_TYPE_WCET
          ? "MAXIMIZE\n\n\n  " : "MINIMIZE\n\n\n  " ) );

      for ( i = 0; i < og->num_edges; i++ ) {
        const offset_graph_edge * const edge = &og->edges[i];
        // Add the cost of the start node to account for that cost too
        const ull factor = ( computation_type == ILP_COMP_TYPE_BCET
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

    case ILP_COMP_TYPE_OFFSETS:
      /* This ILP has a different objective function. It emits (number_of_nodes)
       * flow units from the supersource and tries to reach as many offsets as
       * possible when the loop ends. */
      fprintf( f, "MAXIMIZE\n\n\n  " );

      for ( i = 0; i < susi->num_incoming_edges; i++ ) {
        const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[i]];

        if ( i != 0 ) {
          fprintf( f, " + " );
        }
        if ( edge->start->offset == UNKOWN_OFFSET_NODE_ID ) {
          fprintf( f, "%u ", og->num_nodes );
        }
        fprintILPXActive( f, edge );
      }

      break;

    default:
      assert( 0 && "Unknown ILP type!" );
      break;
  }

  fprintf( f, "\n" );

  /* Write out constraints section. */
  fprintf( f, "\n\nSUBJECT TO\n\n\n" );

  /* Write out flow conservation constraints.
   *
   * We are working on a dynamic flow here, which may not buffer any flow
   * inside the nodes, so the flow conservation constraint for node 'n' is:
   *
   * sum_{e \in n->incoming} x(e, t - \tau(e)) =
   *                                        sum_{e \in n->outgoing} x(e, t)
   *
   * where \tau(e) is the runtime of the edges (the time it takes for the
   * flow to pass the edge). This is 1 for all edges.
   */
  fprintf( f, "  \\ Flow conservation constraints\n" );
  for ( i = 0; i < og->num_nodes; i++ ) {
    offset_graph_node * const node = &og->nodes[i];
    writeFlowConservationConstraint( f, og, node, num_time_steps );
  }
  writeFlowConservationConstraint( f, og, &og->unknown_offset_node,
                                   num_time_steps );

  /* The number of flow units in transit. */
  const uint flow_units = ( computation_type == ILP_COMP_TYPE_OFFSETS
                            ? og->num_nodes : 1 );

  /* Write out demand / supply values
   *
   * Only the supersource emits 'flow_units' flow units at time 0 and
   * only the supersink consumes 'flow_units' units at time 'num_time_steps'.
   */
  fprintf( f, "\n  \\ Demand / supply constraints\n" );
  for ( j = 0; j < num_time_steps; j++ ) {
    firstTerm = 1;
    for ( k = 0; k < suso->num_outgoing_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[suso->outgoing_edges[k]];

      if ( !firstTerm ) {
        fprintf( f, " + " );
      } else {
        fprintf( f, "  " );
        firstTerm = 0;
      }
      fprintILPEdgeName( f, edge, j );
    }

    // Only at time '0', flow units may leave the source.
    fprintf( f, " = %u\n", ( j == 0 ? flow_units : 0 ) );
  }

  for ( j = 0; j <= num_time_steps; j++ ) {
    firstTerm = 1;
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      if ( j >= runtime ) {
        if ( !firstTerm ) {
          fprintf( f, " + " );
        } else {
          fprintf( f, "  " );
          firstTerm = 0;
        }
        fprintILPEdgeName( f, edge, j - runtime );
      }
    }

    // Only at time 'num_time_steps', flow units may enter the sink.
    // Only print this if any flow may arrive at this time instant
    if ( !firstTerm ) {
      fprintf( f, " = %u\n", ( j == num_time_steps ? flow_units : 0 ) );
    }
  }


  /* For the offset ILP write out the definitions of the active exit edges. */
  if ( computation_type == ILP_COMP_TYPE_OFFSETS ) {
    /* \forall_{e=(o,supersink) \in Edges}:
     *   x_active(e) = 1 <=> x(e,num_time_steps - runtime(e)) > 0  */
    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      /* The constraints are:
       *
       * for the "=>" direction
       *
       *     x_active(e) = 1 => x(e,num_time_steps - runtime(e)) > 0
       * <=> x(e,num_time_steps - runtime(e)) + (1 - x_active(e)) > 0
       * <=> x(e,num_time_steps - runtime(e)) + 1 > x_active(e)
       * <=> x(e,num_time_steps - runtime(e)) \geq x_active(e)
       * <=> x(e,num_time_steps - runtime(e)) x_active(e) \geq 0
       *
       * and for the "<=" direction
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
       * <=> x(e,num_time_steps - runtime(e)) - flow_units*y \leq 0
       *     x_active(e) - y \geq 0
       *
       * where 'y' is the helper variable.
       * */
      fprintf( f, "  " );
      fprintILPEdgeName( f, edge, num_time_steps - runtime );
      fprintf( f, " - " );
      fprintILPXActive( f, edge );
      fprintf( f, " >= 0\n" );

      fprintf( f, "  " );
      fprintILPEdgeName( f, edge, num_time_steps - runtime );
      fprintf( f, " - %u ", flow_units );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, " <= 0\n" );

      fprintf( f, "  " );
      fprintILPXActive( f, edge );
      fprintf( f, " - " );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, " >= 0\n" );
    }
  }


  /* Write out bounds section.
   *
   * At each time instant only 'flow_units' units may flow through
   * each of the edges, which represents the loop execution.
   */
  fprintf( f, "\n\nBOUNDS\n\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];

    for ( j = 0; j < num_time_steps; j++ ) {
      fprintf( f, "  %u <= ", 0 );
      fprintILPEdgeName( f, edge, j );
      fprintf( f, " <= %u\n", flow_units );
    }
  }


  /* Write out declarations section.
   *
   * See 'printILPEdgeName' for documentation about the flow variables.
   */
  fprintf( f, "\n\nGENERAL\n\n\n" );

  for ( i = 0; i < og->num_edges; i++ ) {
    offset_graph_edge * const edge = &og->edges[i];

    for ( j = 0; j < num_time_steps; j++ ) {
      DACTION(
          fprintf( f, "  \\ Edge from %u to %u (time %u)\n",
            edge->start->offset, edge->end->offset, j );
      );
      fprintf( f, "  " );
      fprintILPEdgeName( f, edge, j );
      fprintf( f, "\n");
    }
  }

  // Offset-mode specific declarations
  if ( computation_type == ILP_COMP_TYPE_OFFSETS ) {

    fprintf( f, "\n\nBINARY\n\n\n" );

    for ( k = 0; k < susi->num_incoming_edges; k++ ) {
      const offset_graph_edge * const edge = &og->edges[susi->incoming_edges[k]];
      const uint runtime = getOffsetGraphEdgeRuntime( og, edge );

      DACTION(
          fprintf( f, "  \\ Indicator whether " );
          fprintILPEdgeName( f, edge, num_time_steps - runtime );
          fprintf( f, " > 0\n" );
      );
      fprintf( f, "  " );
      fprintILPXActive( f, edge );
      fprintf( f, "\n");

      DACTION(
          fprintf( f, "  \\ Helper variable for determining " );
          fprintILPXActive( f, edge );
          fprintf( f, "\n" );
      );
      fprintf( f, "  " );
      fprintILPXActiveHelper( f, edge );
      fprintf( f, "\n");
    }
  }

  fprintf( f, "\n\nEND\n" );

  DEND();
}


/* Generate an ILP. The function can generate various ILPs which differ
 * only marginally. The type of ILP which is generated is determined by the parameter
 * 'computation_type'.
 *
 * Returns the name of the file to which the ILP was written. The caller must free
 * the returned string and delete the file if he does not want to keep it.
 * */
static char *generateOffsetGraphILP( const offset_graph *og, uint loopbound,
    enum ILPComputationType computation_type, enum ILPSolver format )
{
  DSTART( "generateOffsetGraphILP" );

  // Get output file
  FILE *f = NULL;
  char *tmpfilename;
  MALLOC( tmpfilename, char*, 50 * sizeof( char ), "tmpfilename" );
  if ( keepTemporaryFiles ) {
    strcpy( tmpfilename, "offsetGraph.ilp" );
    f = fopen( tmpfilename, "w" );
  } else {
    strcpy( tmpfilename, "/tmp/chronos_ilp_XXXXXX" );
    int posix_file_descriptor = mkstemp( tmpfilename );
    // Yes, this allows race conditions again, but I have no
    // idea about the POSIX I/O functions, therefore this is okay.
    if ( posix_file_descriptor != -1 ) {
      close( posix_file_descriptor );
      f = fopen( tmpfilename, "w" );
    }
  }
  if ( !f ) {
    prerr( "Unable to write to ILP file '%s'\n", tmpfilename );
  }
  DOUT( "Writing ILP to %s\n", tmpfilename );

  if ( format == ILP_LPSOLVE ) {
    /* We also write CPLEX ILPS for lp_solve, because lp_solve
     * can read them via the external language interface. */
    writeCPLEXILP( f, og, loopbound, computation_type );
  } else if ( format == ILP_CPLEX ) {
    writeCPLEXILP( f, og, loopbound, computation_type );
  } else {
    assert( 0 && "Unknown ILP solver!" );
  }

  fclose( f );

  DRETURN( tmpfilename );
}


/* Invokes the solver for a given ILP file and returns the output
 * file's path. The caller must free this string then. */
static char* invokeILPSolver( const char *ilp_file,
    enum ILPSolver solver )
{
  DSTART( "invokeILPSolver" );

  // Invoke external ILP solver.
  char commandline[1024];
  char *output_file;
  MALLOC( output_file, char*, 100 * sizeof( char ), "output_file" );
  if ( keepTemporaryFiles ) {
    strcpy( output_file, "offsetGraph.result" );
  } else {
    strcpy( output_file, "/tmp/chronos_ilp_result_XXXXXX" );
    int posix_file_descriptor = mkstemp( output_file );
    if ( posix_file_descriptor != -1 ) {
      close( posix_file_descriptor );
    }
  }
  remove( output_file );

  if ( solver == ILP_LPSOLVE ) {
    sprintf( commandline, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s/lp_solve"
        " -presolve -rxli xli_CPLEX %s > %s\n", LP_SOLVE_PATH, LP_SOLVE_PATH,
        ilp_file, output_file );
    DOUT( "Called lp_solve: %s\n", commandline );

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

  } else if ( solver == ILP_CPLEX ) {
    sprintf( commandline, "printf \""
        "read %s lp\\n"
        "mipopt\\n"
        "set output results n %s\\n"
        "display solution objective\\n"
        "display solution variable -\\n"
        "quit\\n"
        "\" | cplex > /dev/null 2>&1", ilp_file, output_file );
    DOUT( "Called CPLEX: %s\n", commandline );

    const int ret = system( commandline );

    if ( ret == -1 ) {
      prerr( "Failed to invoke CPLEX with: %s", commandline );
    } else
    if ( ret != 0 ) {
      prerr( "Failed to solve ILP. Maybe solver not available." );
    }

  } else {
    assert( 0 && "Unknown ILP solver!" );
  }

  DOUT( "Solved ILP, output is in %s\n", output_file );
  DRETURN( output_file );
}


/* Parses a result value given in scientific of standard number
 * notation in the "number_string". */
static ull parseResultValue( const char * const number_string )
{
  // Parse the number (first try direct format)
  char *end_ptr = NULL;
  ull result = strtoull( number_string, &end_ptr, 10 );
  _Bool successfully_read = *end_ptr == '\0';
  /* In case of failure:
   * Try to parse the number in scientific format (2.81864e+06) */
  if ( !successfully_read ) {
    double time_result_float = strtod( number_string, &end_ptr );
    successfully_read = *end_ptr == '\0';
    if ( successfully_read ) {
      assert( floor( time_result_float ) == time_result_float &&
          "Invalid result!" );
      result = (ull)time_result_float;
    } else {
      prerr( "Invalid result value format!" );
    }
  }
  return result;
}


/* Solves a given ilp with lp_solve. */
static ull solveET_ILP( const offset_graph *og, const char *ilp_file,
    enum ILPSolver solver )
{
  DSTART( "solveET_ILP" );

  const char * const output_file = invokeILPSolver( ilp_file, solver );

  // Parse result file
  FILE *result_file = fopen( output_file, "r" );
  ull result;
  const uint line_size = 500;
  char result_file_line[line_size];
  char result_string[50];
  _Bool successfully_read = 0;

  if ( solver == ILP_LPSOLVE ) {
    successfully_read =
      // Skip first two lines
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      // Read the result in the third line
      fgets( result_file_line, line_size, result_file ) &&
      sscanf( result_file_line, "%*s %*s %*s %*s %s", result_string );

    if ( successfully_read ) {
      result = parseResultValue( result_string );
    }

  } else if ( solver == ILP_CPLEX ) {
    successfully_read =
      // Skip first four lines
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      fgets( result_file_line, line_size, result_file ) &&
      // Read the result in the fifth line
      fgets( result_file_line, line_size, result_file ) &&
      sscanf( result_file_line, "%*s %*s %*s %*s %*s %*s %*s %s", result_string );

    if ( successfully_read ) {
      result = parseResultValue( result_string );
    }

  } else {
    assert( 0 && "Unknown ILP solver!" );
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
    prerr( "Could not read ET ILP output file!" );
    DRETURN( result );
  }
}


/* Solves a given ilp with lp_solve. The result is returned in the given
 * offset data representation. */
static offset_data solveOffset_ILP( const offset_graph *og, const char *ilp_file,
    enum ILPSolver solver, enum OffsetDataType offsetType )
{
  DSTART( "solveOffset_ILP" );

  const char * const output_file = invokeILPSolver( ilp_file, solver );

  // Parse result file
  FILE *result_file = fopen( output_file, "r" );

  const uint line_size = 500;
  char result_file_line[line_size];
  char var_name[50];
  uint var_num;
  uint var_value;
  _Bool skipped = 0;

  if ( solver == ILP_LPSOLVE ) {
    // Skip first 4 lines
    skipped =
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file );

  } else if ( solver == ILP_CPLEX ) {
    // Skip first 7 lines
    skipped =
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file ) &&
        fgets( result_file_line, line_size, result_file );

  } else {
    assert( 0 && "Unknown ILP solver!" );
  }

  offset_data result;
  _Bool foundAnyOffset = FALSE;

  if ( skipped ) {
    // Read until all offset variables were found
    while( fgets( result_file_line, line_size, result_file ) ) {
      sscanf( result_file_line, "%s %u", var_name, &var_value );
      if ( sscanf( var_name, X_ACTIVE_PREFIX "%u", &var_num ) == 1 ) {
        const uint edge_index = var_num - 1;
        const uint active_offset = og->edges[edge_index].start->offset;

        // Update the result object with the new offset
        if ( !foundAnyOffset ) {
          if ( active_offset != UNKOWN_OFFSET_NODE_ID ) {
            result = createOffsetDataFromOffsetBounds( offsetType,
                       active_offset, active_offset );
          } else {
            setOffsetDataMaximal( &result );
          }
          foundAnyOffset = TRUE;
        } else {
          assert( active_offset != UNKOWN_OFFSET_NODE_ID && "Invalid result!" );
          updateOffsetData( &result, &result, active_offset,
                              active_offset, TRUE );
        }
      }
    }
  }

  fclose( result_file );

  // Delete output file
  if ( !keepTemporaryFiles ) {
    remove( output_file );
  }
  free( (void*)output_file );

  if ( foundAnyOffset ) {
    DOUT( "Result was: %s\n", getOffsetDataString( &result ) );
    DRETURN( result );
  } else {
    prerr( "Could not read offset ILP output file!" );
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

  result->supersource.offset = SUPERSOURCE_ID;
  result->supersink.offset = SUPERSINK_ID;
  result->unknown_offset_node.offset = UNKOWN_OFFSET_NODE_ID;

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
  dumpOffsetGraphNode( og, &og->unknown_offset_node, out );
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

  char * const tmpfile = generateOffsetGraphILP( og,
      loopbound_min, ILP_COMP_TYPE_BCET, ilpSolver );
  const ull result = solveET_ILP( og, tmpfile, ilpSolver );

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
      loopbound_max, ILP_COMP_TYPE_WCET, ilpSolver );
  const ull result = solveET_ILP( og, tmpfile, ilpSolver );

  if ( !keepTemporaryFiles ) {
    remove( tmpfile );
  }
  free( tmpfile );

  return result;
}


/* Solves a flow problem to obtain the final offsets. The result is
 * returned in the given offset data type.
 *
 * 'loopbound_max' specifies the maximum number of iterations of the loop.
 */
offset_data computeOffsetGraphLoopOffsets( const offset_graph *og,
    uint loopbound_max, enum OffsetDataType offsetType )
{
  assert( og && "Invalid arguments!" );

  char * const tmpfile = generateOffsetGraphILP( og, loopbound_max,
      ILP_COMP_TYPE_OFFSETS, ilpSolver );
  const offset_data result = solveOffset_ILP( og, tmpfile, ilpSolver,
                                              offsetType );

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
  free( og->supersource.incoming_edges );
  free( og->supersource.outgoing_edges );
  free( og->supersink.incoming_edges );
  free( og->supersink.outgoing_edges );
  free( og->unknown_offset_node.incoming_edges );
  free( og->unknown_offset_node.outgoing_edges );

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
