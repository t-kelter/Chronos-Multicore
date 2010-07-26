#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analysisDAG_ET_alignment.h"
#include "analysisDAG_common.h"
#include "block.h"
#include "busSchedule.h"
#include "dump.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


/* A data type to represent an offset range. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint lower_bound;
  uint upper_bound;
} tdma_offset_bounds;

/* Represents the result of the combined BCET/WCET/offset analysis */
typedef struct {
  tdma_offset_bounds offsets;
  ull bcet;
  ull wcet;
} combined_result;


// #########################################
// #### Declaration of static variables ####
// #########################################


static enum LoopAnalysisType currentLoopAnalysisType;


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


/* Verifies that the bound information is valid. */
static _Bool checkBound( const tdma_offset_bounds *b )
{
  return b->lower_bound <= b->upper_bound;
}


/* Returns the union of the given offset bounds. */
static tdma_offset_bounds mergeOffsetBounds( const tdma_offset_bounds *b1, const tdma_offset_bounds *b2 )
{
  assert( b1 && checkBound( b1 ) && b2 && checkBound( b2 ) && 
          "Invalid arguments!" );

  tdma_offset_bounds result;
  result.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
  result.upper_bound = MAX( b1->upper_bound, b2->upper_bound );
  
  assert( checkBound( &result ) && "Invalid result!" );
  return result;
}


/* Returns
 * - a negative number: if lhs is no subset of rhs, nor are they equal
 * - 0                : if lhs == rhs
 * - apositive number : if lhs is a subset of rhs */
static int isSubsetOrEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs )
{
  assert( lhs && checkBound( lhs ) && rhs && checkBound( rhs ) && 
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
  tdma_offset_bounds result = { 0, 0 };

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

    result = mergeOffsetBounds( &result, &dag_block_results[conv_pred_idx].offsets );
  }

  assert( checkBound( &result ) && "Invalid result!" );
  return result;
}


/* Returns the offset bounds for a given (absolute) time range. */
static tdma_offset_bounds getOffsetBounds( ull minTime, ull maxTime )
{
  assert( minTime <= maxTime && "Invalid arguments" );

  // Get schedule data
  const core_sched_p core_schedule = getCoreSchedule( ncore, minTime );
  const uint interval = core_schedule->interval;

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

  assert( result.lower_bound >= 0 && result.lower_bound < interval &&
          result.upper_bound >= 0 && result.upper_bound < interval &&
          checkBound( &result ) &&
          "Invalid offset result!" );
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
  assert( topologically_sorted_blocks && block_results && dag_proc &&
          "Invalid arguments!" );

  DEBUG_ALIGNMENT_PRINTF( "        DAG result summary\n" );

  // Allocate space for the propagation values
  ull *block_bcet_propagation_values = (ull*)CALLOC( block_bcet_propagation_values,
      number_of_blocks, sizeof( ull ), "block_bcet_propagation_values" );
  ull *block_wcet_propagation_values = (ull*)CALLOC( block_wcet_propagation_values,
      number_of_blocks, sizeof( ull ), "block_wcet_propagation_values" );

  // Propagate the BCETs/WCETs through the DAG
  int i;
  for ( i = number_of_blocks - 1; i >= 0; i-- ) {

    const block * const bb = topologically_sorted_blocks[i];
    assert(bb && "Missing basic block!");

    if ( i == number_of_blocks - 1 ) {
      // Head block: Take over block BCET/WCET
      block_bcet_propagation_values[i] = block_results[i].bcet;
      block_wcet_propagation_values[i] = block_results[i].wcet;
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
                                  block_results[conv_pred_idx].bcet;
        const ull wcet_via_pred = block_wcet_propagation_values[conv_pred_idx] +
                                  block_results[conv_pred_idx].wcet;

        block_bcet_propagation_values[i] = ( j == 0
            ? bcet_via_pred
            : MIN( block_bcet_propagation_values[i], bcet_via_pred ) );
        block_wcet_propagation_values[i] =
              MAX( block_wcet_propagation_values[i], wcet_via_pred );
      }
    }
  }

  /* Create a result object */
  combined_result result;
  result.bcet = ULLONG_MAX;
  result.wcet = 0;
  result.offsets.lower_bound = 0;
  result.offsets.upper_bound = 0;

  /* Extract the final BCET, WCET and offset bounds from the DAG leaves. */
  for ( i = 0; i < number_of_blocks; i++ ) {

    const block * const bb = topologically_sorted_blocks[i];
    assert(bb && "Missing basic block!");

    if ( bb->num_outgoing > 0 )
      break;

    // Compute BCET
    result.bcet = MIN( result.bcet, block_bcet_propagation_values[i] );
    // Compute WCET
    result.wcet = MAX( result.wcet, block_wcet_propagation_values[i] );
    // Compute offsets
    result.offsets = mergeOffsetBounds( &result.offsets, &block_results[i].offsets );
  }

  free( block_bcet_propagation_values );
  free( block_wcet_propagation_values );

  assert( checkBound( &result.offsets ) && "Invalid result!" );
  return result;
}


/* Computes the BCET, WCET and offset bounds for the given block when starting from the given offset range. */
static combined_result analyze_block( block* bb, procedure* proc,
    loop* cur_lp, uint loop_context, const tdma_offset_bounds start_offsets )
{
  assert( bb && proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );

  DEBUG_ALIGNMENT_PRINTF( "    Analyzing block %d.%d[%u] with offsets [%u,%u]\n",
      bb->bbid, proc->pid, loop_context, start_offsets.lower_bound,
      start_offsets.upper_bound );

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop * const inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    return analyze_loop( inlp, proc, loop_context, start_offsets );

  /* It's not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {

    combined_result result;
    result.bcet = 0;
    result.wcet = 0;
    result.offsets = start_offsets;

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
      uint min_latency = UINT_MAX;
      uint max_latency = 0;
      int j;
      for ( j =  result.offsets.lower_bound;
            j <= result.offsets.upper_bound;
            j++ ) {
        const uint best_latency  = determine_latency( bb, j, best_acc );
        const uint worst_latency = determine_latency( bb, j, worst_acc );
        min_latency = MIN( min_latency, best_latency );
        max_latency = MAX( max_latency, worst_latency );
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
          /* Compute the WCET of the callee procedure here.
           * We dont handle recursive procedure call chain */
          combined_result call_cost = analyze_proc( callee, result.offsets );
          result.bcet    += call_cost.bcet;
          result.wcet    += call_cost.wcet;
          result.offsets  = call_cost.offsets;
        }
      }

      DEBUG_ALIGNMENT_PRINTF( "      updated result after instruction "
          "%d: %llu / %llu [%u,%u]\n", i, result.bcet, result.wcet,
          result.offsets.lower_bound, result.offsets.upper_bound );
    }

    assert( checkBound( &result.offsets ) && "Invalid result!" );
    return result;
  }
}

/* Computes the BCET, WCET and offset bounds for a single iteration of the given loop
 * when starting from the given offset range. */
static combined_result analyze_single_loop_iteration( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets )
{
  assert( lp && proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );

  /* Get an array for the result values per basic block. */
  combined_result * const block_results = (combined_result *)CALLOC(
      block_results, lp->num_topo, sizeof( combined_result ), "block_result" );

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

  assert( checkBound( &result.offsets ) && "Invalid result!" );
  return result;
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 *
 * This function iteratively computes an offset bound for the loop header until
 * the offset bound converges. This is less precise than the graph-based method
 * which tracks the development of the offset bounds. */
static combined_result analyze_loop_global_convergence( loop* lp, procedure* proc, 
    uint loop_context, const tdma_offset_bounds start_offsets )
{
  assert( lp && proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );

  // This will store the offsets during convergence
  tdma_offset_bounds current_offsets = start_offsets;
  // Specifies the index of the currently analyzed iteration
  unsigned int current_iteration;
  // Holds the result from the iterations (if they were done)
  combined_result **results = (combined_result**)CALLOC( results,
      lp->loopbound, sizeof( combined_result* ), "results" );

  DEBUG_ALIGNMENT_PRINTF( "Loop {%d.%d} [lb %d] starts analysis with offsets [%u,%u]\n",
      lp->pid, lp->lpid, lp->loopbound, start_offsets.lower_bound, start_offsets.upper_bound );

  // Perform single-iteration-analyses until the offset bound converges
  for( current_iteration = 0; current_iteration < lp->loopbound; current_iteration++ ) {

    // Allocate new result slot
    results[current_iteration] = (combined_result*)MALLOC( results[current_iteration],
        sizeof( combined_result ), "results[current_iteration]" );
    combined_result * const last_result = results[current_iteration];

    // Analyze the iteration
    const uint inner_context = getInnerLoopContext( lp, loop_context, 
                                   current_iteration == 0 );
    *last_result = analyze_single_loop_iteration( lp, proc, 
                       inner_context, current_offsets );

    // Compute new offsets if they changed
    if ( isSubsetOrEqual( &last_result->offsets, &current_offsets ) < 0 ) {
      current_offsets = mergeOffsetBounds( &current_offsets, &last_result->offsets );

      DEBUG_ALIGNMENT_PRINTF( "Loop {%d.%d} has new offset bound [%u,%u]\n",
          lp->pid, lp->lpid, current_offsets.lower_bound, current_offsets.upper_bound );

    // If they did not change, terminate the analysis
    } else {
      // We always need at least two iterations to fully exploit the cache analysis
      if ( current_iteration != 0 ) {
        break;
      }
    }
  }

  // The number of iterations which were analyzed explicitly
  unsigned int analyzed_iterations = current_iteration + 1;
  DEBUG_ALIGNMENT_PRINTF( "  -- {%d.%d} Analyzed %u iterations explicitly\n",
      lp->pid, lp->lpid, analyzed_iterations );

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
    DEBUG_ALIGNMENT_PRINTF( "  Accounting BCET %d / WCET %d for iterations [%u,%u]\n",
      bcet, wcet, current_iteration, current_iteration + multiplier - 1 );

    result.bcet += bcet * multiplier;
    result.wcet += wcet * multiplier;
  }
  
  // Free the iteration results array
  for( current_iteration = 0; current_iteration < analyzed_iterations; current_iteration++ ) {
    free( results[current_iteration] );
    results[current_iteration] = 0;
  }
  free( results );

  assert( checkBound( &result.offsets ) && "Invalid result!" );
  return result;
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 *
 * This function builds a graph which models the possible development of the
 * loop header offset bounds and computes the final WCET by solving a maximum
 * cost flow problem on that graph. */
static combined_result analyze_loop_graph_tracking( loop* lp, procedure* proc, 
    uint loop_context, const tdma_offset_bounds start_offsets )
{
  assert( lp && proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );

  combined_result result;

  assert( checkBound( &result.offsets ) && "Invalid result!" );
  return result;
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 * 
 * This method just delegates the work to the currently active submethod. */
static combined_result analyze_loop( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets )
{
  assert( lp && proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );

  switch( currentLoopAnalysisType ) {
    case LOOP_ANALYSIS_GLOBAL_CONVERGENCE:
      return analyze_loop_global_convergence( lp, proc, loop_context, start_offsets );
    case LOOP_ANALYSIS_GRAPH_TRACKING:
      return analyze_loop_graph_tracking( lp, proc, loop_context, start_offsets );
    default:
      assert( 0 && "Unsupported analysis method!" );
      // To make compiler happy
      combined_result result;
      return result;
  }
}


/* Computes the BCET, WCET and offset bounds for the given procedure when starting from the given offset range. */
static combined_result analyze_proc( procedure* proc, const tdma_offset_bounds start_offsets )
{
  assert( proc && checkBound( &start_offsets ) &&
          "Invalid arguments!" );
    
  DEBUG_ALIGNMENT_PRINTF( "  Analyzing procedure %d with offsets [%u,%u]\n",
        proc->pid, start_offsets.lower_bound, start_offsets.upper_bound );

  /* Get an array for the result values per basic block. */
  combined_result * const block_results = (combined_result *)CALLOC(
      block_results, proc->num_topo, sizeof( combined_result ), "block_result" );

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

  DEBUG_ALIGNMENT_PRINTF( "  Procedure %d WCET / BCET result is %llu / %llu"
      " with offsets [%u,%u]\n", proc->pid, result.bcet, result.wcet, 
      result.offsets.lower_bound, result.offsets.upper_bound );

  assert( checkBound( &result.offsets ) && "Invalid result!" );
  return result;
}

// #########################################
// #### Definitions of public functions ####
// #########################################


/* Analyze worst case execution time of all the tasks inside
 * a MSC. The MSC is given by the argument */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file,
   enum LoopAnalysisType analysisTypeToUse )
{
  assert( msc && tdma_bus_schedule_file && "Invalid arguments!" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Set the loop analysis to use. */
  currentLoopAnalysisType = analysisTypeToUse;

  /* Reset the earliest/latest time of all cores */
  memset( earliest_core_time, 0, num_core * sizeof(ull) );
  memset( latest_core_time, 0, num_core * sizeof(ull) );

  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    PRINT_PRINTF( "Analyzing Task WCET %s (alignment-aware) ......\n", msc->taskList[k].task_name );

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* First get the earliest and latest start time of the current task. */
    const ull earliest_start = get_earliest_task_start_time( cur_task, ncore );
    const ull latest_start   = get_latest_task_start_time( cur_task, ncore );
    const tdma_offset_bounds initial_bounds = 
      getOffsetBounds( earliest_start, latest_start );
    DEBUG_ALIGNMENT_PRINTF( "  Initial offset bounds: [%u,%u]\n",
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

    PRINT_PRINTF( "\n**************************************************************\n" );
    PRINT_PRINTF( "Earliest / Latest start time of the program = %Lu / %Lu cycles\n",
        earliest_start, latest_start );
    PRINT_PRINTF( "Earliest / Latest finish time of the task = %Lu / %Lu cycles\n",
        earliest_start + cur_task->bcet, latest_start + cur_task->wcet );
    PRINT_PRINTF( "BCET / WCET of the task %s shared bus = %Lu / %Lu cycles\n",
        g_shared_bus ? "with" : "without", cur_task->bcet, cur_task->wcet );
    PRINT_PRINTF( "**************************************************************\n\n" );
  }
}
