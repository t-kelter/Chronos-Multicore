#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analysisDAG_ET_alignment.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"


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


// ##################################################
// #### Forward declarations of static functions ####
// ##################################################


static combined_result analyze_block( block* bb, procedure* proc,
    loop* cur_lp, uint loop_context, const tdma_offset_bounds offsets );
static combined_result analyze_loop( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets );
static combined_result analyze_proc( procedure* proc, const tdma_offset_bounds start_offsets );


// #########################################
// #### Definitions of static functions ####
// #########################################


/* Returns the union of the given bounds. */
static tdma_offset_bounds mergeOffsetBounds( const tdma_offset_bounds *b1, const tdma_offset_bounds *b2 )
{
  assert( b1 && b2 && "Invalid arguments!" );

  tdma_offset_bounds result;
  result.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
  result.upper_bound = MAX( b1->upper_bound, b2->upper_bound );
  return result;
}

/* Returns the offset bounds for basic block 'bb' from procedure 'proc' where the
 * offset bound results for its predecessors have already been computed and are
 * stored in 'block_results'. */
static tdma_offset_bounds getStartOffsets( const block * bb, const procedure * proc,
                                           const combined_result *block_results )
{
  assert( bb && proc && block_results && "Invalid arguments!" );
  tdma_offset_bounds result = { 0, 0 };

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    const int pred_index = bb->incoming[i];
    assert( proc->bblist[pred_index] && "Missing basic block!" );

    result = mergeOffsetBounds( &result, &block_results[pred_index].offsets );
  }

  return result;
}


/* Returns the offset bounds for a given time range. */
static tdma_offset_bounds getOffsetBounds( ull minTime, ull maxTime )
{
  assert( minTime < maxTime && "Invalid arguments" );

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
  if ( factorDifference > 1 ) {
    tdma_offset_bounds result;
    result.lower_bound = 0;
    result.upper_bound = interval - 1;
    return result;
  /* b) The interval [minTime, maxTime] crosses exactly one TDMA interval boundary
   *    --> Not all offsets may be possible (depending on the remainders) but
   *        offset 0 and offset interval-1 are definitely possible and thus our
   *        offset representation does not allow a tighter bound than [0, interval-1] */
  } else if ( factorDifference == 1 ) {
    tdma_offset_bounds result;
    result.lower_bound = 0;
    result.upper_bound = interval - 1;
    return result;
  /* c) The interval [minTime, maxTime] lies inside a single TDMA interval
   *    --> Use the remainders as the offset bounds */
  } else {
    assert( minTime_factor == maxTime_factor &&
            minTime_remainder <= maxTime_remainder &&
            "Invalid internal state!" );
    tdma_offset_bounds result;
    result.lower_bound = minTime_remainder;
    result.upper_bound = maxTime_remainder;
    return result;
  }
}


/* Computes the BCET, WCET and offset bounds for the given block when starting from the given offset range. */
static combined_result analyze_block( block* bb, procedure* proc,
    loop* cur_lp, uint loop_context, const tdma_offset_bounds offsets )
{
  combined_result return_value;
  return return_value;
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from the given offset range. */
static combined_result analyze_loop( loop* lp, procedure* proc, uint loop_context,
    const tdma_offset_bounds start_offsets )
{
  combined_result return_value;
  return return_value;
}


/* Computes the BCET, WCET and offset bounds for the given procedure when starting from the given offset range. */
static combined_result analyze_proc( procedure* proc, const tdma_offset_bounds start_offsets )
{
  /* Preprocessing: Build CHMC classifications for the procedure */
  preprocess_chmc_WCET( proc );

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
        : getStartOffsets( bb, proc, block_results ) );
    block_results[i] = analyze_block( bb, proc, NULL, 0, block_start_bounds );
  }

  /* Now all BCETS, WCETs and offset bounds for individual blocks are finished */

  /* Compute final procedure BCET and WCET by propagating the values through the DAG. */
  ull *block_bcet_propagation_values = (ull*)CALLOC( block_bcet_propagation_values,
      proc->num_topo, sizeof( ull ), "block_bcet_propagation_values" );
  ull *block_wcet_propagation_values = (ull*)CALLOC( block_wcet_propagation_values,
        proc->num_topo, sizeof( ull ), "block_wcet_propagation_values" );
  for ( i = proc->num_topo - 1; i >= 0; i-- ) {

    block * const bb = proc->topo[i];
    assert(bb && "Missing basic block!");

    if ( i == proc->num_topo - 1 ) {
      // Head block: Take over block BCET/WCET
      block_bcet_propagation_values[i] = block_results[i].bcet;
      block_wcet_propagation_values[i] = block_results[i].wcet;
    } else {
      // Merge over predecessors
      int j;
      for ( j = 0; j < bb->num_incoming; j++ ) {

        const int pred_index = bb->incoming[j];
        assert( proc->bblist[pred_index] && "Missing basic block!" );

        block_bcet_propagation_values[i] = ( j == 0
            ? block_bcet_propagation_values[j] + block_results[j].bcet
            : MIN( block_bcet_propagation_values[i],
                block_bcet_propagation_values[j] + block_results[j].bcet ) );
        block_wcet_propagation_values[i] = MAX(
            block_wcet_propagation_values[i],
            block_wcet_propagation_values[j] + block_results[j].wcet );
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
  for ( i = 0; i < proc->num_topo; i++ ) {

    block * const bb = proc->topo[i];
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
  free( block_results );

  return result;
}

// #########################################
// #### Definitions of public functions ####
// #########################################


/* Analyze worst case execution time of all the tasks inside
 * a MSC. The MSC is given by the argument */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file )
{
  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

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

    /* Then compute and set the best and worst case cost of this task */
    combined_result main_result = analyze_proc( task_main,
        getOffsetBounds( earliest_start, latest_start ) );
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
    PRINT_PRINTF( "Earliest/Latest start time of the program = %Lu / %Lu cycles\n",
        earliest_start, latest_start );
    PRINT_PRINTF( "Earliest/Latest finish time of the task = %Lu / %Lu cycles\n",
        earliest_start + cur_task->bcet, latest_start + cur_task->wcet );
    PRINT_PRINTF( "BCET/WCET of the task %s shared bus = %Lu / %Lu cycles\n",
        g_shared_bus ? "with" : "without", cur_task->bcet, cur_task->wcet );
    PRINT_PRINTF( "**************************************************************\n\n" );
  }
}
