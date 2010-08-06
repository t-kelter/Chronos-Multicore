// Include standard library headers
#include <assert.h>
#include <limits.h>
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
#include "analysisDAG_ET_alignment.h"
#include "analysisDAG_common.h"
#include "block.h"
#include "busSchedule.h"
#include "dump.h"
#include "offsetGraph.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


/* Represents the result of the combined BCET/WCET/offset analysis */
typedef struct {
  tdma_offset_bounds offsets;
  ull bcet;
  ull wcet;
} combined_result;


// #########################################
// #### Declaration of static variables ####
// #########################################


/* The type of analysis that we use for the alignment tracking. */
static enum LoopAnalysisType currentLoopAnalysisType;
/* In this mode the alignment analysis "emulates" the purely structural
 * analysis, by assuming an offset of [0,0] at each loop / function head
 * and by adding the appropriate alignment penalty to the current
 * WCET/BCET. It only does this when this has a positive effect on the
 * WCET computation, and not in all cases as the purely structural
 * analysis. */
static _Bool tryPenalizedAlignment = 1;


// ##################################################
// #### Forward declarations of static functions ####
// ##################################################


static combined_result analyze_block( block* bb, procedure* proc,
    loop* cur_lp, uint loop_context, const tdma_offset_bounds offsets );
static combined_result analyze_loop( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets );
static combined_result analyze_proc( procedure* proc,
    const tdma_offset_bounds start_offsets );


// #########################################
// #### Definitions of static functions ####
// #########################################


/* A convenience macro to iterate over the offsets in an offset bound. */
#define ITERATE_OFFSET_BOUND( bound, iteration_variable ) \
  for ( iteration_variable  = bound.lower_bound; \
        iteration_variable <= bound.upper_bound; \
        iteration_variable++ )


/* Verifies that the bound information is valid. */
static _Bool checkOffsetBound( const tdma_offset_bounds *b )
{
  // TODO: This won't work correctly for segmented schedules
  const uint interval = getCoreSchedule( ncore, 0 )->interval;
  return b->lower_bound <= b->upper_bound &&
         b->lower_bound >= 0 && b->lower_bound < interval &&
         b->upper_bound >= 0 && b->upper_bound < interval;
}


/* Returns the union of the given offset bounds. */
static tdma_offset_bounds mergeOffsetBounds( const tdma_offset_bounds *b1, const tdma_offset_bounds *b2 )
{
  assert( b1 && checkOffsetBound( b1 ) && b2 && checkOffsetBound( b2 ) && 
          "Invalid arguments!" );

  tdma_offset_bounds result;
  result.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
  result.upper_bound = MAX( b1->upper_bound, b2->upper_bound );
  
  assert( checkOffsetBound( &result ) && "Invalid result!" );
  return result;
}


/* Returns
 * - a negative number : if 'lhs' is no subset of 'rhs', nor are they equal
 * - 0                 : if 'lhs' == 'rhs'
 * - a positive number : if 'lhs' is a subset of 'rhs' */
static int isOffsetBoundSubsetOrEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs )
{
  assert( lhs && checkOffsetBound( lhs ) && rhs && checkOffsetBound( rhs ) && 
          "Invalid arguments!" );

  // Check equality
  if ( lhs->lower_bound == rhs->lower_bound &&
       lhs->upper_bound == rhs->upper_bound ) {
    return 0;
  } else {
    // Check for subset
    if ( lhs->lower_bound >= rhs->lower_bound &&
         lhs->upper_bound <= rhs->upper_bound ) {
      return 1;
    } else {
      return -1;
    }
  }
}


/* Returns
 * - 1 : if 'lhs' == 'rhs'
 * - 0 : if 'lhs' != 'rhs' */
static _Bool isOffsetBoundEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs )
{
  assert( lhs && checkOffsetBound( lhs ) && rhs && checkOffsetBound( rhs ) &&
          "Invalid arguments!" );

  return lhs->lower_bound == rhs->lower_bound &&
         lhs->upper_bound == rhs->upper_bound;
}


/* Returns the offset bounds for basic block 'bb' from procedure 'proc' where the
 * offset bound results for its predecessors have already been computed and are
 * stored in 'dag_block_results'.
 *
 * 'dag_block_list' should be the list which contains the blocks of the currently
 * analyzed DAG in the same order in which the results are stored in 'block_results'.
 * This list is used to identify the index in the result array that belongs to
 * a certain block. 
 * 'dag_block_number' should be the number of blocks in 'dag_block_list'. */
static tdma_offset_bounds getStartOffsets( const block * bb, const procedure * proc,
                                           block ** dag_block_list,
                                           uint dag_block_number,
                                           const combined_result *dag_block_results )
{
  assert( bb && proc && dag_block_results && dag_block_list && "Invalid arguments!" );
  tdma_offset_bounds result;

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    const int pred_index = bb->incoming[i];
    const block * const pred = proc->bblist[pred_index];
    assert( pred && "Missing basic block!" );

    /* The pred_index is an index into the bblist of the procedure.
     * What we need is an index into the topologically_sorted_blocks.
     * Therefore we must convert that index into an index into the
     * topologically sorted list. For that purpose we use the block
     * id which identifies the block inside the function. */
    const int conv_pred_idx = getblock( pred->bbid, dag_block_list,
                                        0, dag_block_number - 1 );

    const tdma_offset_bounds * const pred_offsets = &dag_block_results[conv_pred_idx].offsets;
    if ( i == 0 ) {
      result = *pred_offsets;
    } else {
      result = mergeOffsetBounds( &result, pred_offsets );
    }
  }

  assert( checkOffsetBound( &result ) && "Invalid result!" );
  return result;
}


/* Returns the offset bounds for a given (absolute) time range. */
static tdma_offset_bounds getOffsetBounds( ull minTime, ull maxTime )
{
  assert( minTime <= maxTime && "Invalid arguments" );

  // Get schedule data
  const uint interval = getCoreSchedule( ncore, minTime )->interval;

  // Split up the time values into multiplier and remainder
  const uint minTime_factor    = minTime / interval;
  const uint minTime_remainder = minTime % interval;
  const uint maxTime_factor    = maxTime / interval;
  const uint maxTime_remainder = maxTime % interval;

  /* We want to find the offsets in the interval [minTime, maxTime]
   * To do that efficiently we must consider multiple cases:
   *
   * a) Between the two time values lies a full TDMA iteration
   *    --> all offsets are possible */
  const uint factorDifference = maxTime_factor - minTime_factor;
  tdma_offset_bounds result;
  if ( factorDifference > 1 ) {
    result.lower_bound = 0;
    result.upper_bound = interval - 1;
  /* b) The interval [minTime, maxTime] crosses exactly one TDMA interval boundary
   *    --> Not all offsets may be possible (depending on the remainders) but
   *        offset 0 and offset interval-1 are definitely possible and thus our
   *        offset representation does not allow a tighter bound than [0, interval-1] */
  } else if ( factorDifference == 1 ) {
    result.lower_bound = 0;
    result.upper_bound = interval - 1;
  /* c) The interval [minTime, maxTime] lies inside a single TDMA interval
   *    --> Use the remainders as the offset bounds */
  } else {
    assert( minTime_factor == maxTime_factor &&
            minTime_remainder <= maxTime_remainder &&
            "Invalid internal state!" );
    result.lower_bound = minTime_remainder;
    result.upper_bound = maxTime_remainder;
  }

  assert( checkOffsetBound( &result ) && "Invalid offset result!" );
  return result;
}


/* The per-block analysis only computes a block-BCET/-WCET and the resulting
 * offset bounds. Thus, to obtain a final BCET/WCET value for a DAG (loop or
 * procedure) one needs to propagate the individual BCET/WCET values through
 * the DAG and summarize + minimize/maximize them. This is done by this function.
 * Note that there is no need to propagate the offset results, because this
 * is done during the analysis via the "getStartOffsets" function.
 *
 * 'number_of_blocks' The number of blocks in the DAG to examine
 * 'topologically_sorted_blocks' The list of blocks in the DAG, sorted topologically,
 *                               indexed from '0' to 'number_of_blocks-1' where the
 *                               leaves of the DAG are the first array elements.
 * 'block_results' The BCET/WCET and offset results per basic block in the DAG
 *                 (indexed in the same way as 'topologically_sorted_blocks')
 * 'dag_proc' The procedure in which the DAG lies. This information is needed to
 *            obtain the predecessors of a block.
 */
static combined_result summarizeDAGResults( uint number_of_blocks,
    block ** topologically_sorted_blocks, const combined_result * block_results,
    const procedure *dag_proc )
{
  DSTART( "summarizeDAGResults" );
  assert( topologically_sorted_blocks && block_results && dag_proc &&
          "Invalid arguments!" );

  // Allocate space for the propagation values
  ull *block_bcet_propagation_values;
  CALLOC( block_bcet_propagation_values, ull*, number_of_blocks,
      sizeof( ull ), "block_bcet_propagation_values" );
  ull *block_wcet_propagation_values;
  CALLOC( block_wcet_propagation_values, ull*, number_of_blocks,
      sizeof( ull ), "block_wcet_propagation_values" );

  // Propagate the BCETs/WCETs through the DAG
  int i;
  for ( i = number_of_blocks - 1; i >= 0; i-- ) {

    const block * const bb = topologically_sorted_blocks[i];
    assert(bb && "Missing basic block!");

    if ( i == number_of_blocks - 1 ) {
      // Head block: Take over block BCET/WCET
      block_bcet_propagation_values[i] = block_results[i].bcet;
      block_wcet_propagation_values[i] = block_results[i].wcet;

      DOUT( "Processing head block: BCET %llu, WCET %llu (block 0x%s, index %d)\n",
          block_bcet_propagation_values[i],
          block_wcet_propagation_values[i], bb->instrlist[0]->addr, i );
    } else {
      // Merge over predecessors
      int j;
      for ( j = 0; j < bb->num_incoming; j++ ) {

        const int pred_index = bb->incoming[j];
        const block * const pred = dag_proc->bblist[pred_index];
        assert( pred && "Missing basic block!" );

        /* The pred_index is an index into the bblist of the procedure.
         * What we need is an index into the topologically_sorted_blocks.
         * Therefore we must convert that index into an index into the
         * topologically sorted list. For that purpose we use the block
         * id which identifies the block inside the function. */
        const int conv_pred_idx = getblock( pred->bbid, topologically_sorted_blocks,
                                            0, number_of_blocks - 1 );

        const ull bcet_via_pred = block_bcet_propagation_values[conv_pred_idx] +
                                  block_results[i].bcet;
        const ull wcet_via_pred = block_wcet_propagation_values[conv_pred_idx] +
                                  block_results[i].wcet;

        if ( j == 0 ) {
          block_bcet_propagation_values[i] = bcet_via_pred;
          block_wcet_propagation_values[i] = wcet_via_pred;

          DOUT( "Propagating from predecessor (0x%s, index %d): BCET %llu, "
            "WCET %llu (block 0x%s, index %d)\n", pred->instrlist[0]->addr,
            conv_pred_idx, block_bcet_propagation_values[i],
            block_wcet_propagation_values[i], bb->instrlist[0]->addr, i );
        } else {
          block_bcet_propagation_values[i] =
              MIN( block_bcet_propagation_values[i], bcet_via_pred );
          block_wcet_propagation_values[i] =
              MAX( block_wcet_propagation_values[i], wcet_via_pred );

          DOUT( "MIN/MAXed with predecessor (0x%s, index %d): BCET %llu, "
            "WCET %llu (block 0x%s, index %d)\n", pred->instrlist[0]->addr,
            conv_pred_idx, block_bcet_propagation_values[i],
            block_wcet_propagation_values[i], bb->instrlist[0]->addr, i );
        }
      }
    }
  }

  /* Create a result object */
  combined_result result;

  /* Extract the final BCET, WCET and offset bounds from the DAG leaves. */
  for ( i = 0; i < number_of_blocks; i++ ) {

    const block * const bb = topologically_sorted_blocks[i];
    assert(bb && "Missing basic block!");

    if ( bb->num_outgoing > 0 )
      break;

    if ( i == 0 ) {
      result.bcet = block_bcet_propagation_values[i];
      result.wcet = block_wcet_propagation_values[i];
      result.offsets = block_results[i].offsets;
    } else {
      result.bcet = MIN( result.bcet, block_bcet_propagation_values[i] );
      result.wcet = MAX( result.wcet, block_wcet_propagation_values[i] );
      result.offsets = mergeOffsetBounds( &result.offsets, &block_results[i].offsets );
    }
  }

  free( block_bcet_propagation_values );
  free( block_wcet_propagation_values );

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given block when starting from the given offset range. */
static combined_result analyze_block( block* bb, procedure* proc,
    loop* cur_lp, uint loop_context, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_block" );
  assert( bb && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  combined_result result;
  result.bcet = 0;
  result.wcet = 0;
  result.offsets = start_offsets;

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop * const inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    DOUT( "Block represents inner loop!\n" );
    result = analyze_loop( inlp, proc, loop_context, start_offsets );

  /* It's not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {

      instr * const inst = bb->instrlist[i];
      assert(inst);

      // Backup BCET/ WCET results at the beginning of each new instruction
      const ull old_bcet = result.bcet;
      const ull old_wcet = result.wcet;

      /* First handle instruction cache access. */
      const acc_type best_acc  = check_hit_miss( bb, inst, loop_context, 
                                                 ACCESS_SCENARIO_BCET );
      const acc_type worst_acc = check_hit_miss( bb, inst, loop_context, 
                                                 ACCESS_SCENARIO_WCET );

      uint j, min_latency, max_latency;
      for ( j  = result.offsets.lower_bound;
            j <= result.offsets.upper_bound; j++ ) {
        const uint bc_latency = determine_latency( bb, j, best_acc );
        const uint wc_latency = determine_latency( bb, j, worst_acc );
        if ( j == result.offsets.lower_bound ) {
          min_latency = bc_latency;
          max_latency = wc_latency;
        } else {
          min_latency = MIN( min_latency, bc_latency );
          max_latency = MAX( max_latency, wc_latency );
        }
      }
      result.bcet += min_latency;
      result.wcet += max_latency;

      /* Then add cost for executing the instruction. */
      result.bcet += getInstructionBCET( inst );
      result.wcet += getInstructionWCET( inst );

      /* Update the offset information. */
      result.offsets = getOffsetBounds(
          result.offsets.lower_bound + result.bcet - old_bcet,
          result.offsets.upper_bound + result.wcet - old_wcet );

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure * const callee = getCallee( inst, proc );

        /* For ignoring library calls */
        if ( callee ) {
          DOUT( "Block calls internal function %u\n", callee->pid );
          /* Compute the WCET of the callee procedure here.
           * We dont handle recursive procedure call chain */
          combined_result call_cost = analyze_proc( callee, result.offsets );
          result.bcet    += call_cost.bcet;
          result.wcet    += call_cost.wcet;
          result.offsets  = call_cost.offsets;
        }
      }

      DOUT( "  Instruction 0x%s: BCET %llu, WCET %llu\n", inst->addr,
          result.bcet - old_bcet, result.wcet - old_wcet );
    }
  }

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DOUT( "Accounting BCET %llu, WCET %llu for block 0x%s - 0x%s\n",
      result.bcet, result.wcet, bb->instrlist[0]->addr,
      bb->instrlist[bb->num_instr - 1]->addr );
  DRETURN( result );
}

/* Computes the BCET, WCET and offset bounds for a single iteration of the given loop
 * when starting from the given offset range. */
static combined_result analyze_single_loop_iteration( loop* lp, procedure* proc, uint loop_context,
    tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_single_loop_iteration" );
  assert( lp && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Loop iteration analysis starts with offsets [%u,%u]\n",
      start_offsets.lower_bound, start_offsets.upper_bound );

  /* Get an array for the result values per basic block. */
  combined_result *block_results;
  CALLOC( block_results, combined_result *, lp->num_topo, 
          sizeof( combined_result ), "block_result" );

  /* Iterate over the basic blocks in topological order */
  int i;
  for ( i = lp->num_topo - 1; i >= 0; i-- ) {

    block * const bb = lp->topo[i];
    assert(bb && "Missing basic block!");

    /* If this is the first block of the procedure then set the start interval
     * of this block to be the same with the start interval of the procedure itself */
    const tdma_offset_bounds block_start_bounds = ( i == lp->num_topo - 1
        ? start_offsets
        : getStartOffsets( bb, proc, lp->topo, lp->num_topo, block_results ) );
    block_results[i] = analyze_block( bb, proc, lp, loop_context, block_start_bounds );
  }

  /* Now all BCETS, WCETs and offset bounds for individual blocks are finished */

  /* Compute final procedure BCET and WCET by propagating the values through the DAG. */
  combined_result result = summarizeDAGResults( lp->num_topo, lp->topo,
                                                block_results, proc );

  free( block_results );

  DOUT( "Loop iteration analysis BCET / WCET result is %llu / %llu"
      " with offsets [%u,%u]\n", result.bcet, result.wcet,
      result.offsets.lower_bound, result.offsets.upper_bound );

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 *
 * This function iteratively computes an offset bound for the loop header until
 * the offset bound converges. This is less precise than the graph-based method
 * which tracks the development of the offset bounds.
 *
 * This function should not be called directly, only through its wrapper 'analyze_loop'. */
static combined_result analyze_loop_global_convergence( loop* lp, procedure* proc, 
    uint loop_context, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_loop_global_convergence" );
  assert( lp && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Loop %d.%d [lb %d] starts analysis with offsets [%u,%u]\n",
      lp->pid, lp->lpid, lp->loopbound, start_offsets.lower_bound, start_offsets.upper_bound );

  // This will store the offsets during convergence
  tdma_offset_bounds current_offsets = start_offsets;
  // Specifies the index of the currently analyzed iteration
  unsigned int current_iteration;
  // Holds the result from the iterations (if they were done)
  combined_result **results;
  CALLOC( results, combined_result**, lp->loopbound, sizeof( combined_result* ), "results" );

  // Perform single-iteration-analyses until the offset bound converges
  _Bool brokeOut = 0;
  for( current_iteration = 0; current_iteration < lp->loopbound; current_iteration++ ) {

    // Allocate new result slot
    MALLOC( results[current_iteration], combined_result*, sizeof( combined_result ), 
        "results[current_iteration]" );
    combined_result * const last_std_result = results[current_iteration];

    // Analyze the iteration
    const uint inner_context = getInnerLoopContext( lp, loop_context, 
                                   current_iteration == 0 );
    *last_std_result = analyze_single_loop_iteration( lp, proc, 
                       inner_context, current_offsets );

    // Compute new offsets if they changed
    if ( isOffsetBoundSubsetOrEqual( &last_std_result->offsets, &current_offsets ) < 0 ) {
      current_offsets = mergeOffsetBounds( &current_offsets, &last_std_result->offsets );
    // If they did not change, terminate the analysis
    } else {
      // We always need at least two iterations to fully exploit the cache analysis
      if ( current_iteration != 0 ) {
        brokeOut = 1;
        break;
      }
    }
  }

  // The number of iterations which were analyzed explicitly
  unsigned int analyzed_iterations = current_iteration + ( brokeOut ? 1 : 0 );

  // Compute the final result. The BCET and WCET values can be extracted from the
  // individual iterations, the result from the last analyzed iteration stays 
  // correct for all the remaining loop iterations.
  combined_result result;
  result.bcet = 0;
  result.wcet = 0;
  result.offsets = current_offsets;

  for( current_iteration = 0; current_iteration < analyzed_iterations; current_iteration++ ) {
    // The number of iterations for which the current result entry is valid
    const unsigned int multiplier = ( current_iteration == analyzed_iterations - 1 )
      ? lp->loopbound - analyzed_iterations + 1 
      : 1;
    
    const unsigned int bcet = results[current_iteration]->bcet;
    const unsigned int wcet = results[current_iteration]->wcet;

    result.bcet += bcet * multiplier;
    result.wcet += wcet * multiplier;
  }

  // Free the iteration results array
  for( current_iteration = 0; current_iteration < analyzed_iterations; current_iteration++ ) {
    free( results[current_iteration] );
    results[current_iteration] = 0;
  }
  free( results );

  DOUT( "Loop %d.%d BCET / WCET result is %llu / %llu"
      " with offsets [%u,%u]\n", lp->pid, lp->lpid, result.bcet, result.wcet,
      result.offsets.lower_bound, result.offsets.upper_bound );

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 *
 * This function builds a graph which models the possible development of the
 * loop header offset bounds and computes the final WCET by solving a maximum
 * cost flow problem on that graph.
 *
 * This function should not be called directly, only through its wrapper 'analyze_loop'. */
static combined_result analyze_loop_graph_tracking( loop * const lp, procedure * const proc,
    const uint loop_context, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_loop_graph_tracking" );
  assert( lp && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Starting graph-tracking analysis for loop %u.%u (loopbound %u) with "
      "offsets [%u,%u]\n", proc->pid, lp->lpid, lp->loopbound,
      start_offsets.lower_bound, start_offsets.upper_bound );

  // Get the current TDMA interval
  // TODO: This won't work correctly for segmented schedules
  const uint tdma_interval = getCoreSchedule( ncore, 0 )->interval;

  // The first iteration must be unrolled, because its overly big execution times
  // would disturb the ILP (it does not know that these execution times can only
  // occur at the first iteration and would try to apply them again).
  combined_result result;
  if ( lp->loopbound <= 0 ) {
    result.bcet = 0;
    result.wcet = 0;
    result.offsets = start_offsets;
    return result;
  } else {
    const uint first_context = getInnerLoopContext( lp, loop_context, 1 );
    result = analyze_single_loop_iteration( lp, proc, first_context, start_offsets );
    DOUT( "Analyzed first iteration: BCET %llu, WCET %llu, offsets [%u,%u]\n",
        result.bcet, result.wcet,
        result.offsets.lower_bound,
        result.offsets.upper_bound );

    if ( lp->loopbound == 1 ) {
      return result;
    }
  }

  // Create the offset graph with edges from the supersource to all initial offsets
  offset_graph *graph = createOffsetGraph( tdma_interval );
  uint i;
  ITERATE_OFFSET_BOUND( result.offsets, i ) {
    addOffsetGraphEdge( graph, &graph->supersource, getOffsetGraphNode( graph, i ), 0, 0 );
  }
  DOUT( "Created initial offset graph with %u nodes\n", tdma_interval );

  _Bool graphChanged = 0;             // Whether we changed any graph part in the current iteration
  tdma_offset_bounds current_offsets; // This will store the offsets
  combined_result iteration_result;   // Result from the last iteration analysis
  uint iteration_number = 1;          // The number of the currently analyzed iteration
                                      // (iteration 0 was peeled off above)
  do {
    // Get the current offsets
    if ( iteration_number == 1 ) {
      current_offsets = result.offsets;
    } else {
      current_offsets = iteration_result.offsets;
    }

    // Analyze the iteration
    const uint inner_context = getInnerLoopContext( lp, loop_context, 0 );
    iteration_result = analyze_single_loop_iteration( lp, proc, inner_context, current_offsets );
    DOUT( "Analyzed new iteration: BCET %llu, WCET %llu, offsets [%u,%u]\n",
        iteration_result.bcet, iteration_result.wcet,
        iteration_result.offsets.lower_bound,
        iteration_result.offsets.upper_bound );

    // Apply the needed changes to the graph
    graphChanged = 0;

    int j; // 'j' is the starting offset
    ITERATE_OFFSET_BOUND( current_offsets, j ) {
      offset_graph_node * const start = getOffsetGraphNode( graph, j );

      // Add the missing edges
      int k; // 'k' indexes the target offset
      ITERATE_OFFSET_BOUND( iteration_result.offsets, k ) {
        offset_graph_node * const end   = getOffsetGraphNode( graph, k );

        if ( getOffsetGraphEdge( graph, start, end ) == NULL ) {
          addOffsetGraphEdge( graph, start, end, 0, 0 );
          graphChanged = 1;

          assert( getOffsetGraphEdge( graph, start, end ) &&
                  "Internal error!" );
        }
      }

      // Update the BCET / WCET values of the nodes
      if ( start->bcet == 0 ||
           start->bcet > iteration_result.bcet ) {
        start->bcet = iteration_result.bcet;
        graphChanged = 1;
      }
      if ( start->wcet == 0 ||
           start->wcet < iteration_result.wcet ) {
        start->wcet = iteration_result.wcet;
        graphChanged = 1;
      }
    }

    // Update iteration number
    iteration_number++;

  } while( // Iterate until the graph converges
           ( graphChanged &&
             // But don't iterate again when the offsets have already converged,
             // because there can be no more changes to the graph then in the
             // next iteration.
             !isOffsetBoundEqual( &current_offsets, &iteration_result.offsets ) ) &&
           // In any case, stop when the loopbound is reached
           ( iteration_number < lp->loopbound ) );

  DOUT( "Analyzed %u iterations\n", iteration_number );

  // Add the edges from all offsets to the supersink
  for ( i = 0; i < tdma_interval; i++ ) {
    addOffsetGraphEdge( graph, getOffsetGraphNode( graph, i ), &graph->supersink, 0, 0 );
  }

  /* If the graph creation converged with the interval [0, tdma_interval - 1]
   * then it's not worth trying to solve the ILP, because the result can hardly
   * be more precise than the global convergence. */
  if ( iteration_result.offsets.lower_bound == 0 &&
       iteration_result.offsets.upper_bound == tdma_interval - 1 ) {
    result = analyze_loop_global_convergence( lp, proc, loop_context, start_offsets );
    DOUT( "Took over convergence result: BCET %llu, WCET %llu, offsets [%u, %u]\n", result.bcet,
        result.wcet, result.offsets.lower_bound, result.offsets.upper_bound );
  } else {
    // Solve flow problems
    // (Mind the peeled-off iteration of the loop, see beginning o this function)
    result.bcet += computeOffsetGraphLoopBCET( graph, lp->loopbound - 1 );
    result.wcet += computeOffsetGraphLoopWCET( graph, lp->loopbound - 1 );
    result.offsets = computeOffsetGraphLoopOffsets( graph, lp->loopbound - 1 );
    DOUT( "Loop results: BCET %llu, WCET %llu, offsets [%u, %u]\n", result.bcet,
            result.wcet, result.offsets.lower_bound, result.offsets.upper_bound );
  }

  freeOffsetGraph( graph );

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 * 
 * This function should not be called directly, only through its wrapper 'analyze_loop'. */
static combined_result analyze_loop_alignment_aware( loop* lp, procedure* proc,
    uint loop_context, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_loop_alignment_aware" );
  assert( lp && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  combined_result result;
  switch( currentLoopAnalysisType ) {
    case LOOP_ANALYSIS_GLOBAL_CONVERGENCE:
      result = analyze_loop_global_convergence( lp, proc, loop_context, start_offsets );
      break;
    case LOOP_ANALYSIS_GRAPH_TRACKING:
      result = analyze_loop_graph_tracking( lp, proc, loop_context, start_offsets );
      break;
    default:
      assert( 0 && "Unsupported analysis method!" );
  }

  DRETURN( result );
}

/* Computes the BCET, WCET and offset bounds for the given loop when starting from
 * the given offset range.
 *
 * This method is just an intelligent wrapper for the respective sub-methods. */
static combined_result analyze_loop( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_loop" );
  assert( lp && proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  // Get the result using our alignment-aware loop analysis
  DOUT( "Performing alignment-sensitive analysis\n" );
  combined_result result = analyze_loop_alignment_aware( lp, proc,
                             loop_context, start_offsets );

  /* If we are supposed to try the penalized alignment too, then we also compute the result
   * using the zero-alignment and add the appropriate alignment penalties. If this yields a
   * superior solution, then we pick that one instead of the previously computed result. */
  if ( tryPenalizedAlignment ) {
    // TODO: The alignments may be computed for a wrong segment in case of multi-segment
    //       schedules (see definitions of startAlign/endAlign)
    DOUT( "Attempting penalized alignment analysis\n" );
    const tdma_offset_bounds zero_offsets = { 0, 0 };
    combined_result pal_result = analyze_loop_alignment_aware( lp, proc,
                                   loop_context, zero_offsets );
    pal_result.wcet += startAlign( 0 );

    // If the result is better, then take this one
    if ( pal_result.wcet < result.wcet ) {
      DOUT( "Took over penalized alignment result!\n" );
      result = pal_result;
    }
  }

  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given procedure when starting from
 * the given offset range.
 *
 * This function should not be called directly, only through its wrapper 'analyze_proc'. */
static combined_result analyze_proc_alignment_aware( procedure* proc, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_proc_alignment_aware" );
  assert( proc && checkOffsetBound( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Analyzing procedure %d with offsets [%u,%u]\n",
        proc->pid, start_offsets.lower_bound, start_offsets.upper_bound );

  /* Get an array for the result values per basic block. */
  combined_result *block_results;
  CALLOC( block_results, combined_result *, proc->num_topo, 
          sizeof( combined_result ), "block_result" );

  /* Iterate over the basic blocks in topological order */
  int i;
  for ( i = proc->num_topo - 1; i >= 0; i-- ) {

    block * const bb = proc->topo[i];
    assert(bb && "Missing basic block!");

    /* If this is the first block of the procedure then set the start interval
     * of this block to be the same with the start interval of the procedure itself */
    const tdma_offset_bounds block_start_bounds = ( i == proc->num_topo - 1
        ? start_offsets
        : getStartOffsets( bb, proc, proc->topo, proc->num_topo, block_results ) );
    block_results[i] = analyze_block( bb, proc, NULL, 0, block_start_bounds );
  }

  /* Now all BCETS, WCETs and offset bounds for individual blocks are finished */

  /* Compute final procedure BCET and WCET by propagating the values through the DAG. */
  combined_result result = summarizeDAGResults( proc->num_topo, proc->topo,
                                                block_results, proc );

  free( block_results );

  DOUT( "Procedure %d WCET / BCET result is %llu / %llu"
      " with offsets [%u,%u]\n", proc->pid, result.bcet, result.wcet,
      result.offsets.lower_bound, result.offsets.upper_bound );

  assert( checkOffsetBound( &result.offsets ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given procedure when starting from
 * the given offset range.
 *
 * This method is just an intelligent wrapper for the respective sub-methods. */
static combined_result analyze_proc( procedure* proc, const tdma_offset_bounds start_offsets )
{
  DSTART( "analyze_proc" );

  // Get the result using our alignment-aware procedure analysis
  DOUT( "Performing alignment-sensitive analysis\n" );
  combined_result result = analyze_proc_alignment_aware( proc, start_offsets );

  /* If we are supposed to try the penalized alignment too, then we also compute the result
   * using the zero-alignment and add the appropriate alignment penalties. If this yields a
   * superior solution, then we pick that one instead of the previously computed result. */
  if ( tryPenalizedAlignment ) {
    // TODO: The alignments may be computed for a wrong segment in case of multi-segment
    //       schedules (see definitions of startAlign/endAlign)
    DOUT( "Attempting penalized alignment analysis\n" );
    const tdma_offset_bounds zero_offsets = { 0, 0 };
    combined_result pal_result = analyze_proc_alignment_aware( proc, zero_offsets );
    pal_result.wcet += startAlign( 0 );

    // If the result is better, then take this one
    if ( pal_result.wcet < result.wcet ) {
      DOUT( "Took over penalized alignment result!\n" );
      result = pal_result;
    }
  }

  DRETURN( result );
}


// #########################################
// #### Definitions of public functions ####
// #########################################


/* Analyze worst case execution time of all the tasks inside a MSC.
 *
 * 'msc' is the MSC to analyze.
 * 'tdma_bus_schedule_file' holds the filename of the TDMA bus specification to use
 * 'analysis_type_to_use' should be the type of loop analysis that shall be used
 * 'try_penalized_alignment' if this is true, then the alignment analysis will try to
 *                           work like the purely structural analysis by assuming an
 *                           offset range of [0,0] and adding the appropriate
 *                           alignment penalties. */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file,
   enum LoopAnalysisType analysis_type_to_use, _Bool try_penalized_alignment )
{
  DSTART( "compute_bus_ET_MSC_alignment" );
  assert( msc && tdma_bus_schedule_file && "Invalid arguments!" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Set the analysis options to use. */
  currentLoopAnalysisType = analysis_type_to_use;
  tryPenalizedAlignment = try_penalized_alignment;

  /* Reset the earliest/latest time of all cores */
  memset( earliest_core_time, 0, num_core * sizeof(ull) );
  memset( latest_core_time, 0, num_core * sizeof(ull) );

  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    DOUT( "Analyzing Task WCET %s (alignment-aware) ......\n", msc->taskList[k].task_name );

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* First get the earliest and latest start time of the current task. */
    const ull earliest_start = get_earliest_task_start_time( cur_task, ncore );
    const ull latest_start   = get_latest_task_start_time( cur_task, ncore );
    const tdma_offset_bounds initial_bounds = 
      getOffsetBounds( earliest_start, latest_start );
    DOUT( "Initial offset bounds: [%u,%u]\n",
        initial_bounds.lower_bound, initial_bounds.upper_bound );

    /* Then compute and set the best and worst case cost of this task */
    combined_result main_result = analyze_proc( task_main,
        initial_bounds );
    cur_task->bcet = main_result.bcet;
    cur_task->wcet = main_result.wcet;

    /* Now update the earliest/latest starting time in this core */
    earliest_core_time[ncore] = earliest_start + cur_task->bcet;
    latest_core_time[ncore]   = latest_start + cur_task->wcet;

    /* Since the interference file for a MSC was dumped in topological
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_task_earliest_start_time( msc, cur_task );
    update_succ_task_latest_start_time( msc, cur_task );

    DOUT( "**************************************************************\n" );
    DOUT( "Earliest / Latest start time of the program = %Lu / %Lu cycles\n",
        earliest_start, latest_start );
    DOUT( "Earliest / Latest finish time of the task = %Lu / %Lu cycles\n",
        earliest_start + cur_task->bcet, latest_start + cur_task->wcet );
    DOUT( "BCET / WCET of the task %s shared bus = %Lu / %Lu cycles\n",
        g_shared_bus ? "with" : "without", cur_task->bcet, cur_task->wcet );
    DOUT( "**************************************************************\n\n" );
  }

  DEND();
}
