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
#include "loopdetect.h"
#include "offsetGraph.h"
#include "wcrt/cycle_time.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


/* Represents the result of the combined BCET/WCET/offset analysis */
typedef struct {
  offset_data offsets;
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
/* The currently used offset representation. */
static enum OffsetDataType currentOffsetRepresentation;

/* This is a multi-level array for buffering the computed results
 * per block. In an access like
 *
 * block_results[p][b][lc][ol][ou]
 *
 * p = procedure id
 * b = block id
 * lc = loop context
 * ol = lower offset bound
 * ou = upper offset bound */
static combined_result ******block_results = NULL;
/* This is a multi-level array for buffering the computed results
 * per loop. In an access like
 *
 * loop_results[p][l][lc][ol][ou]
 *
 * p = procedure id
 * l = loop id
 * lc = loop context
 * ol = lower offset bound
 * ou = upper offset bound */
static combined_result ******loop_results = NULL;
/* This is a multi-level array for buffering the computed results
 * per procedure. In an access like
 *
 * proc_results[p][ol][ou]
 *
 * p = procedure id
 * ol = lower offset bound
 * ou = upper offset bound */
static combined_result ****proc_results = NULL;


// ##################################################
// #### Forward declarations of static functions ####
// ##################################################


static combined_result analyze_block( const block * const bb,
    const procedure * const proc, const loop * const cur_lp,
    const uint loop_context, const offset_data start_offsets );
static combined_result analyze_loop( const loop * const lp,
    const procedure * const proc, const uint loop_context,
    const offset_data start_offsets );
static combined_result analyze_proc( const procedure * const proc,
    const offset_data start_offsets );


// #########################################
// #### Definitions of static functions ####
// #########################################



/* Verifies that the result information is valid. */
static _Bool checkResult( const combined_result *r )
{
  return r->bcet <= r->wcet && isOffsetDataValid( &r->offsets );
}


/* Initializes the buffers to store intermediate results for the given task. */
static void initResultBuffers( const task_t * const task )
{
  int i, j, k, offset;
  const uint tdma_interval = getCoreSchedule( ncore, 0 )->interval;

  #define INIT_TDMA_GRID( ptr ) \
    CALLOC( ptr, combined_result***, tdma_interval, \
      sizeof( combined_result** ), #ptr ); \
    for ( offset = 0; offset < tdma_interval; offset++ ) { \
      CALLOC( ptr[offset], combined_result**, tdma_interval, \
        sizeof( combined_result* ), #ptr "[offset]" ); \
    }

  CALLOC( block_results, combined_result******, task->num_proc,
      sizeof( combined_result***** ), "block_results" );
  CALLOC( loop_results, combined_result******, task->num_proc,
        sizeof( combined_result***** ), "loop_results" );
  CALLOC( proc_results, combined_result****, task->num_proc,
        sizeof( combined_result*** ), "proc_results" );

  for ( i = 0; i < task->num_proc; i++ ) {
    const procedure * const proc = task->procs[i];

    INIT_TDMA_GRID( proc_results[i] );

    // One slot for each context + 1 for context zero
    const int contextSlots = getMaximumLoopContext( proc ) + 1;

    CALLOC( block_results[i], combined_result*****, proc->num_bb,
          sizeof( combined_result**** ), "block_results[i]" );
    for ( j = 0; j < proc->num_bb; j++ ) {
      CALLOC( block_results[i][j], combined_result****, contextSlots,
            sizeof( combined_result*** ), "block_results[i][j]" );
      for ( k = 0; k < contextSlots; k++ ) {
        INIT_TDMA_GRID( block_results[i][j][k] );
      }
    }

    CALLOC( loop_results[i], combined_result*****, proc->num_loops,
          sizeof( combined_result**** ), "loop_results[i]" );
    for ( j = 0; j < proc->num_loops; j++ ) {
      CALLOC( loop_results[i][j], combined_result****, contextSlots,
            sizeof( combined_result*** ), "loop_results[i][j]" );
      for ( k = 0; k < contextSlots; k++ ) {
        INIT_TDMA_GRID( loop_results[i][j][k] );
      }
    }
  }
}
/* Frees the buffers to store intermediate results for the given task. */
static void freeResultBuffers( const task_t * const task )
{
  int i, j, k, offset_1, offset_2;
  const uint tdma_interval = getCoreSchedule( ncore, 0 )->interval;

  #define FREE_TDMA_GRID( ptr ) \
    for ( offset_1 = 0; offset_1 < tdma_interval; offset_1++ ) { \
      for ( offset_2 = 0; offset_2 < tdma_interval; offset_2++ ) { \
        if ( ptr[offset_1][offset_2] != NULL ) { \
          free( ptr[offset_1][offset_2] ); \
        } \
      } \
      free( ptr[offset_1] ); \
    } \
    free( ptr );

  for ( i = 0; i < task->num_proc; i++ ) {
    const procedure * const proc = task->procs[i];

    FREE_TDMA_GRID( proc_results[i] );

    const int maxLoopContext = getMaximumLoopContext( proc );

    for ( j = 0; j < proc->num_bb; j++ ) {
      for ( k = 0; k < maxLoopContext; k++ ) {
        FREE_TDMA_GRID( block_results[i][j][k] );
      }
      free( block_results[i][j] );
    }
    free( block_results[i] );

    for ( j = 0; j < proc->num_loops; j++ ) {
      for ( k = 0; k < maxLoopContext; k++ ) {
        FREE_TDMA_GRID( loop_results[i][j][k] );
      }
      free( loop_results[i][j] );
    }
    free( loop_results[i] );
  }

  free( block_results );
  free( loop_results );
  free( proc_results );
}


/* Given a block 'bb' and one of its predecessors 'pred', this function computes
 * the effective predecessor of 'bb' which represents 'pred' in the DAG (DAG nodes
 * are given by the 'dag_block_list' of size 'dag_block_numer'). In case of nested
 * loops, the effective predecessor is the loop head of the preceding loop for example.
 * Additionally this function requires the procedure 'proc' in which the DAG is
 * situated.
 *
 * If an effective predecessor and its index in the dag_block_list was found,
 * then the index is returned, else -1 is returned.
 */
static int getEffectiveDAGPredecessorIndex( const block * const bb,
                                            const block * const pred,
                                            const procedure * const proc,
                                            block ** const dag_block_list,
                                            const uint dag_block_number )
{
  DSTART( "getEffectiveDAGPredecessorIndex" );

  DOUT( "Getting effective DAG predecessor for pred %u of "
      "block %u in function %u\n", pred->bbid, bb->bbid, proc->pid );

  /* The pred_index is an index into the bblist of the procedure.
   * What we need is an index into the dag_block_list.
   * Therefore we must convert that index into an index into the
   * DAG block list. For that purpose we use the block
   * id which identifies the block inside the function. */
  int conv_pred_idx = getblock( pred->bbid, dag_block_list,
                                    0, dag_block_number - 1 );

  /* The predecessor is not in this DAG. */
  if ( conv_pred_idx < 0 ) {
    /* Then we have two possibilities: Either the predecessor
     * is in a surrounding loop, in which case we have a loop
     * with multiple entries which is not supported. Or the
     * predecessor is in a nested loop, but its not the loop
     * header (he would be part of the current DAG). Then we
     * set the predecessor id to be the id of the loop header
     * manually. */
    const loop * const pred_loop = ( pred->loopid >= 0 ?
                                     proc->loops[pred->loopid] : NULL );
    const loop * const bb_loop   = ( bb->loopid >= 0 ?
                                     proc->loops[bb->loopid] : NULL );

    // This will store the block which will be the effective predecessor
    const block *placeholder = NULL;

    if ( bb_loop != NULL && pred_loop != NULL ) {
      /* For loops with multiple exits, the loop exits are sometimes
       * not part of the loop itself, cause they unconditionally jump
       * to the loop's successor block). In such a case we must take
       * the exited loop's head as the placeholder, else we must take
       * the loop header of the loop in which pred resides. */
      loop * const exitedLoop = isLoopExit( pred, proc );
      if ( exitedLoop != NULL ) {
        placeholder = exitedLoop->loophead;
      } else {
        assert( bb_loop->lpid != pred_loop->lpid &&
          "Invalid DAG: Predecessor in same loop, but not in DAG!" );
        assert( bb_loop->level <= pred_loop->level &&
          "Found loop with multiple entrypoints!" );
        placeholder = pred_loop->loophead;
      }
    } else if ( bb_loop != NULL ) {
      assert( 0 && "Found loop with multiple entrypoints!" );
    } else if ( pred_loop != NULL ) {
      placeholder = pred_loop->loophead;
    } else {
      /* Both blocks are not in a loop, but the predecessor is not
       * in the DAG. This may happen for loops with multiple exits,
       * (the exit blocks themselves are sometimes not part of the
       * loop itself, cause they might unconditionally jump to the
       * loop's successor block). Check whether such a case is
       * present here. */
      loop * const exitedLoop = isLoopExit( pred, proc );
      assert( exitedLoop && "Expected loop exit here!" );
      placeholder = exitedLoop->loophead;
    }

    conv_pred_idx = getblock( placeholder->bbid, dag_block_list,
                              0, dag_block_number - 1 );
    assert( conv_pred_idx >= 0 && "Missing block!" );
  }

  DRETURN( conv_pred_idx );
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
static offset_data getStartOffsets( const block * const bb,
                                    const procedure * const proc,
                                    block ** const dag_block_list,
                                    const uint dag_block_number,
                                    const combined_result * const dag_block_results )
{
  DSTART( "getStartOffsets" );
  assert( bb && proc && dag_block_results && dag_block_list && "Invalid arguments!" );
  offset_data result;

  DOUT( "Getting start offset for block %u.%u (0x%s)\n",
      bb->pid, bb->bbid, bb->instrlist[0]->addr );

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    const int pred_index = bb->incoming[i];
    const block * const pred = proc->bblist[pred_index];
    assert( pred && "Missing basic block!" );

    DOUT( "Accounting for predecessor information from bb %u.%u (0x%s)\n",
        pred->pid, pred->bbid, pred->instrlist[0]->addr );

    /* The predecessor may be a loop exit, which must be mapped to its
     * loop head to obtain the attached lop information. This and the
     * mapping of the predecessor block to the respective index in the
     * dag_block_list is done by the following call. */
    const int conv_pred_idx = getEffectiveDAGPredecessorIndex( bb, pred,
                                proc, dag_block_list, dag_block_number );
    assert( conv_pred_idx >= 0 && "Invalid DAG predecessor!" );

    const offset_data * const pred_offsets = &dag_block_results[conv_pred_idx].offsets;
    if ( i == 0 ) {
      assert( isOffsetDataValid( pred_offsets ) && "Invalid argument!" );
      result = *pred_offsets;
    } else {
      result = mergeOffsetData( &result, pred_offsets );
    }
  }

  assert( isOffsetDataValid( &result ) && "Invalid result!" );
  DRETURN( result );
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

        /* The predecessor may be a loop exit, which must be mapped to its
             * loop head to obtain the attached lop information. This and the
             * mapping of the predecessor block to the respective index in the
             * dag_block_list is done by the following call. */
        const int conv_pred_idx = getEffectiveDAGPredecessorIndex( bb, pred,
            dag_proc, topologically_sorted_blocks, number_of_blocks );
        assert( conv_pred_idx >= 0 && "Invalid DAG predecessor!" );

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
      result.offsets = mergeOffsetData( &result.offsets, &block_results[i].offsets );
    }
  }

  free( block_bcet_propagation_values );
  free( block_wcet_propagation_values );

  assert( checkResult( &result ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given block when starting from the given offset range. */
static combined_result analyze_block( const block * const bb,
    const procedure * const proc, const loop * const cur_lp,
    const uint loop_context, const offset_data start_offsets )
{
  DSTART( "analyze_block" );
  assert( bb && proc && isOffsetDataValid( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Starting block analysis for block %u.%u [context %u] with "
      "offsets %s\n", proc->pid, bb->bbid, loop_context,
      getOffsetDataString( &start_offsets ) );

  /* Buffering currently only works for the "RANGE" data type. */
  combined_result **bufferLocation = NULL;
  if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_RANGE ) {
    /* Check whether we have already computed the requested result. */
    bufferLocation = &block_results[proc->pid][bb->bbid]
      [loop_context][getOffsetDataMinimumOffset( &start_offsets )]
                    [getOffsetDataMaximumOffset( &start_offsets )];
    if ( *bufferLocation != NULL ) {
      DRETURN( **bufferLocation );
    }
  }

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

    /* During the block analysis we may encounter bus accesses which
     * force us to wait for the next TDMA slot. Normally we only update
     * the current offset range after each instruction, but here we
     * can use more information: After the first wait for a new TDMA
     * slot we know, that in our worst-case scenario we are now aligned
     * (access duration) cycles after the begin of our TDMA slot.
     * Therefore we can use fixed offsets for the next accesses for
     * the next instructions until a call occurs or the block ends.
     *
     * The "useFixed..Offset" variables denote whether we currently are
     * in such an instruction sequence in which the offset is known due
     * to a preceding wait. The "fixed..Offset" then denotes the offset
     * to use.
     */
    _Bool useFixedBCOffset = 0;
    uint fixedBCOffset = 0;
    _Bool useFixedWCOffset = 0;
    uint fixedWCOffset = 0;

    // TODO: This won't work correctly for segmented schedules
    const uint tdma_interval = getCoreSchedule( ncore, 0 )->interval;

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {

      instr * const inst = bb->instrlist[i];
      assert( inst && "Missing instruction!" );

      // TODO: Assert, that the fixed offset is on the current offset range
      assert( !isOffsetDataEmpty( &result.offsets ) && "Invalid offset information!" );

      /* Backup BCET/ WCET results at the beginning of each new instruction. */
      const ull old_bcet = result.bcet;
      const ull old_wcet = result.wcet;
      const _Bool usedFixedBCOffset = useFixedBCOffset;
      const _Bool usedFixedWCOffset = useFixedWCOffset;

      /* Some temporaries. */
      _Bool waitedForNextTDMASlot;

      /* This will hold the offset results in case we are analyzing
       * with the set representation. */
      offset_data offsetSetResults = createOffsetDataSet();

      /* TODO: The access latencies and the instruction BCET/WCET
       *       are currently just given as a single value. For the
       *       offset set data type we need more information,
       *       i.e. all possible execution times of the access / the
       *       instruction and not just the worst / best one.
       */

      /* Compute instruction cache access duration. */
      const acc_type best_acc  = check_hit_miss( bb, inst, loop_context,
                                                 ACCESS_SCENARIO_BCET );
      const acc_type worst_acc = check_hit_miss( bb, inst, loop_context,
                                                 ACCESS_SCENARIO_WCET );
      if ( useFixedBCOffset ) {
        const uint latency = determine_latency( bb, fixedBCOffset, best_acc, NULL );
        result.bcet += latency;

        // Add the resulting offset if we analyze with offset sets
        if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_SET ) {
          updateOffsetData( &offsetSetResults, &result.offsets,
                            latency, latency, TRUE );
        }
      } else {
        /* Iterate over the current offset range and determine minimum latency. */
        uint min_latency = UINT_MAX;

        ITERATE_OFFSETS( result.offsets, j,
          const uint latency = determine_latency( bb, j, best_acc,
                                                  &waitedForNextTDMASlot );
          if ( latency < min_latency ) {
            min_latency = latency;

            /* Check whether we have reached a block-internal fixed alignment. */
            if ( waitedForNextTDMASlot ) {
              useFixedBCOffset = 1;
              fixedBCOffset    = j;
            }
          }
        );
        assert( min_latency != UINT_MAX );
        result.bcet += min_latency;
      }

      /* Compute instruction cache access duration for worst case. */
      if ( useFixedWCOffset ) {
        const uint latency = determine_latency( bb, fixedWCOffset, worst_acc, NULL );
        result.wcet += latency;

        // Add the resulting offset if we analyze with offset sets
        if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_SET ) {
          updateOffsetData( &offsetSetResults, &result.offsets,
                            latency, latency, TRUE );
        }
      } else {
        /* Iterate over the current offset range and determine maximum latency. */
        uint max_latency = 0;
        ITERATE_OFFSETS( result.offsets, j,
          const uint latency = determine_latency( bb, j, worst_acc,
                                                  &waitedForNextTDMASlot );
          if ( latency > max_latency ) {
            max_latency = latency;

            /* Check whether we have reached a block-internal fixed alignment. */
            if ( waitedForNextTDMASlot ) {
              useFixedWCOffset = 1;
              fixedWCOffset    = j;
            }
          }
        );
        result.wcet += max_latency;
      }

      /* Compute the offset results if in set analysis mode. */
      if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_SET &&
           ( !usedFixedBCOffset || !usedFixedWCOffset ) ) {
        ITERATE_OFFSETS( result.offsets, j,
          const uint best_latency  = determine_latency( bb, j, best_acc,  NULL );
          const uint worst_latency = determine_latency( bb, j, worst_acc, NULL );
          updateOffsetData( &offsetSetResults, &result.offsets, best_latency,
              best_latency, TRUE );
          updateOffsetData( &offsetSetResults, &result.offsets, worst_latency,
              worst_latency, TRUE );
        );
      }

      /* Then add cost for executing the instruction. */
      const uint execution_bcet = getInstructionBCET( inst );
      const uint execution_wcet = getInstructionWCET( inst );
      result.bcet += execution_bcet;
      result.wcet += execution_wcet;

      /* Update the offset results if in set analysis mode. */
      if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_SET ) {
        assert( !isOffsetDataEmpty( &offsetSetResults ) && "Internal error!" );
        updateOffsetData( &offsetSetResults, &offsetSetResults, execution_bcet,
            execution_wcet, FALSE );
      }

      /* Update the offset information. */
      const uint bcTimePassed = result.bcet - old_bcet;
      const uint wcTimePassed = result.wcet - old_wcet;
      if ( useFixedBCOffset ) {
        fixedBCOffset = ( fixedBCOffset + bcTimePassed ) % tdma_interval;
      }
      if ( useFixedWCOffset ) {
        fixedWCOffset = ( fixedWCOffset + wcTimePassed ) % tdma_interval;
      }

      /* For the set representation the offsets are computed during
       * the timing computation above. */
      if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_SET ) {
        result.offsets = offsetSetResults;
      } else {
        const uint bcOffset = getOffsetDataMinimumOffset( &result.offsets ) + bcTimePassed;
        const uint wcOffset = getOffsetDataMaximumOffset( &result.offsets ) + wcTimePassed;
        if ( bcTimePassed > wcTimePassed ) {
          assert( ( useFixedBCOffset || useFixedWCOffset ) &&
              "In normal offset mode the local bcet may never exceed the local wcet" );
          if ( bcOffset > wcOffset ) {
            setOffsetDataMaximal( &result.offsets );
          } else {
            updateOffsetData( &result.offsets, &result.offsets,
                              wcTimePassed, bcTimePassed, FALSE );
          }
        } else {
          updateOffsetData( &result.offsets, &result.offsets,
                            bcTimePassed, wcTimePassed, FALSE );
        }
      }

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure * const callee = getCallee( inst, proc );

        /* Deactivate block-internal fixed offset mode, because the call will
         * return its own offset information which overrides ours. */
        useFixedBCOffset = 0;
        useFixedWCOffset = 0;

        /* For ignoring library calls */
        if ( callee ) {
          DOUT( "Block calls internal function %u\n", callee->pid );
          /* Compute the WCET of the callee procedure here.
           * We don't handle recursive procedure call chain */
          combined_result call_cost = analyze_proc( callee, result.offsets );
          result.bcet    += call_cost.bcet;
          result.wcet    += call_cost.wcet;
          result.offsets  = call_cost.offsets;
        }
      }

      DACTION(
          char offsetString[100];
          char *stringPtr = offsetString;
          if ( useFixedBCOffset || useFixedWCOffset ) {
            stringPtr += sprintf( stringPtr, " (fixed offsets: " );
            if ( useFixedBCOffset )
              stringPtr += sprintf( stringPtr, "bc %u ", fixedBCOffset );
            if ( useFixedWCOffset )
              stringPtr += sprintf( stringPtr, "wc %u ", fixedWCOffset );
            stringPtr += sprintf( stringPtr, ") " );
          }
          stringPtr += sprintf( stringPtr, " %s",
              getOffsetDataString( &result.offsets ) );

          DOUT( "  Instruction 0x%s: BCET %llu, WCET %llu %s\n", inst->addr,
            result.bcet - old_bcet, result.wcet - old_wcet, offsetString );
      );
    }
  }

  /* Insert result into result buffer. */
  if ( bufferLocation != NULL ) {
    MALLOC( *bufferLocation, combined_result*,
        sizeof( combined_result ), "*bufferLocation" );
    **bufferLocation = result;
  }

  DACTION(
    char block_name[10];
    if ( bb->loopid >= 0 ) {
      sprintf( block_name, "%u.%u.%u", bb->pid, bb->loopid, bb->bbid );
    } else {
      sprintf( block_name, "%u.%u", bb->pid, bb->bbid );
    }
    DOUT( "Accounting BCET %llu, WCET %llu for block %s 0x%s - 0x%s\n",
      result.bcet, result.wcet, block_name,
        bb->num_instr > 0 ? bb->instrlist[0]->addr                 : "0x0",
        bb->num_instr > 0 ? bb->instrlist[bb->num_instr - 1]->addr : "0x0" );
  );

  assert( checkResult( &result ) && "Invalid result!" );
  DRETURN( result );
}

/* Computes the BCET, WCET and offset bounds for a single iteration of the given loop
 * when starting from the given offset range. */
static combined_result analyze_single_loop_iteration( const loop * const lp,
    const procedure * const proc, const uint loop_context, const offset_data start_offsets )
{
  DSTART( "analyze_single_loop_iteration" );
  assert( lp && proc && isOffsetDataValid( &start_offsets ) &&
          "Invalid arguments!" );

  /* Buffering currently only works for the "RANGE" data type. */
  combined_result **bufferLocation = NULL;
  if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_RANGE ) {
    /* Check whether we have already computed the requested result. */
    bufferLocation = &loop_results[proc->pid][lp->lpid]
      [loop_context][getOffsetDataMinimumOffset( &start_offsets )]
                    [getOffsetDataMaximumOffset( &start_offsets )];
    if ( *bufferLocation != NULL ) {
      DRETURN( **bufferLocation );
    }
  }

  DOUT( "Loop iteration analysis starts with offsets %s\n",
      getOffsetDataString( &start_offsets ) );

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
    const offset_data block_start_bounds = ( i == lp->num_topo - 1
        ? start_offsets
        : getStartOffsets( bb, proc, lp->topo, lp->num_topo, block_results ) );
    block_results[i] = analyze_block( bb, proc, lp, loop_context, block_start_bounds );
  }

  /* Now all BCETS, WCETs and offset bounds for individual blocks are finished */

  /* Compute final procedure BCET and WCET by propagating the values through the DAG. */
  combined_result result = summarizeDAGResults( lp->num_topo, lp->topo,
                                                block_results, proc );

  free( block_results );

  /* Insert result into result buffer. */
  if ( bufferLocation != NULL ) {
    MALLOC( *bufferLocation, combined_result*,
        sizeof( combined_result ), "*bufferLocation" );
    **bufferLocation = result;
  }

  DOUT( "Loop iteration analysis BCET / WCET result is %llu / %llu"
      " with offsets %s\n", result.bcet, result.wcet,
      getOffsetDataString( &result.offsets ) );

  assert( checkResult( &result ) && "Invalid result!" );
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
static combined_result analyze_loop_global_convergence( const loop * const lp,
    const procedure * const proc, const uint loop_context, const offset_data start_offsets )
{
  DSTART( "analyze_loop_global_convergence" );
  assert( lp && proc && isOffsetDataValid( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Loop %d.%d [lb %d] starts analysis with offsets %s\n",
      lp->pid, lp->lpid, lp->loopbound, getOffsetDataString( &start_offsets ) );

  // This will store the offsets during convergence
  offset_data current_offsets = start_offsets;
  // Specifies the index of the currently analyzed iteration
  unsigned int current_iteration;
  // Holds the result from the iterations (if they were done)
  combined_result **results;
  CALLOC( results, combined_result**, lp->loopbound, sizeof( combined_result* ), "results" );
  // Whether the last iteration used a zero-alignment
  _Bool lastIterationWasAligned = 0;

  // Perform single-iteration-analyses until the offset bound converges
  _Bool brokeOut = 0;
  for( current_iteration = 0; current_iteration < lp->loopbound; current_iteration++ ) {

    // Allocate new result slot
    MALLOC( results[current_iteration], combined_result*, sizeof( combined_result ), 
        "results[current_iteration]" );
    combined_result * const current_result = results[current_iteration];
    combined_result * const last_result = results[current_iteration - 1];

    // Analyze the iteration
    const uint inner_context = getInnerLoopContext( lp, loop_context, 
                                   current_iteration == 0 );
    *current_result = analyze_single_loop_iteration( lp, proc, 
                       inner_context, current_offsets );
    DOUT( "Analyzed new iteration: BCET %llu, WCET %llu, offsets %s (context %u)\n",
        current_result->bcet, current_result->wcet,
        getOffsetDataString( &current_result->offsets ), inner_context );

    /* If the user selected this, then try to analyze the same iteration using
     * a fixed alignment and use the better one of the two results. */
    if ( tryPenalizedAlignment ) {
      const offset_data zero_offsets = createOffsetDataFromOffsetBounds(
                                         currentOffsetRepresentation, 0, 0 );
      combined_result aligned_result = analyze_single_loop_iteration( lp, proc,
                                         inner_context, zero_offsets );

      if ( lastIterationWasAligned ) {
        aligned_result.wcet += endAlign( last_result->wcet );
      } else {
        aligned_result.wcet += startAlign( 0 );
      }

      if ( aligned_result.wcet < current_result->wcet ) {
        *current_result = aligned_result;
        lastIterationWasAligned = 1;
        DOUT( "  Took over zero-aligned result: BCET %llu, WCET %llu, offsets %s "
            "(context %u)\n", current_result->bcet, current_result->wcet,
            getOffsetDataString( &current_result->offsets ), inner_context );
      } else {
        lastIterationWasAligned = 0;
      }
    } else {
      lastIterationWasAligned = 0;
    }

    // Compute new offsets if they changed
    if ( isOffsetDataSubsetOrEqual( &current_result->offsets, &current_offsets ) < 0 ) {
      current_offsets = mergeOffsetData( &current_offsets, &current_result->offsets );
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
                                    ? lp->loopbound - analyzed_iterations + 1  : 1;
    
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

  DOUT( "Loop %d.%d BCET / WCET result is %llu / %llu with offsets %s\n",
      lp->pid, lp->lpid, result.bcet, result.wcet, getOffsetDataString(
          &result.offsets ) );

  assert( checkResult( &result ) && "Invalid result!" );
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
static combined_result analyze_loop_graph_tracking( const loop * const lp,
    const procedure * const proc, const uint loop_context, const offset_data start_offsets )
{
  DSTART( "analyze_loop_graph_tracking" );
  assert( lp && proc && isOffsetDataValid( &start_offsets ) &&
          "Invalid arguments!" );

  DOUT( "Starting graph-tracking analysis for loop %u.%u (loopbound %u) "
      "with offsets %s\n", proc->pid, lp->lpid, lp->loopbound,
      getOffsetDataString( &start_offsets ) );

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
    DRETURN( result );
  } else {
    const uint first_context = getInnerLoopContext( lp, loop_context, 1 );
    result = analyze_single_loop_iteration( lp, proc, first_context, start_offsets );
    DOUT( "Analyzed first iteration: BCET %llu, WCET %llu, offsets %s "
        "(context %u)\n", result.bcet, result.wcet,
        getOffsetDataString( &result.offsets ), first_context );

    if ( lp->loopbound == 1 ) {
      DRETURN( result );
    }
  }

  // Create the offset graph with edges from the supersource to all initial offsets
  offset_graph *graph = createOffsetGraph( tdma_interval );
  uint i;
  if ( isOffsetDataMaximal( &result.offsets ) ) {
    addOffsetGraphEdge( graph, &graph->supersource, &graph->unknown_offset_node, 0, 0 );
  } else {
    ITERATE_OFFSETS( result.offsets, i,
      addOffsetGraphEdge( graph, &graph->supersource, getOffsetGraphNode( graph, i ), 0, 0 );
    );
  }
  DOUT( "Created initial offset graph with %u nodes\n", tdma_interval );

  _Bool graphChanged = 0;             // Whether we changed any graph part in the current iteration
  offset_data current_offsets;        // This will store the offsets
  combined_result iteration_result;   // Result from the last iteration analysis
  uint iteration_number = 1;          // The number of the currently analyzed iteration
                                      // (iteration 0 was peeled off above)
  _Bool allIterationsAreEqual = TRUE; // Whether the BCET/WCET results are the same for all iterations

  do {
    // Get the current offsets
    if ( iteration_number == 1 ) {
      current_offsets = result.offsets;
    } else {
      current_offsets = iteration_result.offsets;
    }

    // Analyze the iteration
    const uint inner_context = getInnerLoopContext( lp, loop_context, 0 );
    const combined_result previous_result = iteration_result;
    iteration_result = analyze_single_loop_iteration( lp, proc, inner_context,
                                                      current_offsets );
    DOUT( "Analyzed new iteration: BCET %llu, WCET %llu, offsets %s "
        "(context %u)\n", iteration_result.bcet, iteration_result.wcet,
        getOffsetDataString( &iteration_result.offsets ), inner_context );
    if ( iteration_number > 1 &&
         ( previous_result.bcet != iteration_result.bcet ||
           previous_result.wcet != iteration_result.wcet ) ) {
      allIterationsAreEqual = FALSE;
    }

    /* If the user selected this, then try to analyze the same iteration using
     * a fixed alignment and use the better one of the two results. */
    /* For graph-tracking we also try this to limit the complexity of the
     * resulting ILP, if the offset results are very bad currently. */
    const _Bool stdOffsetsUnbounded = isOffsetDataMaximal( &iteration_result.offsets );
    if ( tryPenalizedAlignment || stdOffsetsUnbounded ) {
      const offset_data zero_offsets = createOffsetDataFromOffsetBounds(
                                         currentOffsetRepresentation, 0, 0 );
      combined_result aligned_result = analyze_single_loop_iteration( lp, proc,
                                         inner_context, zero_offsets );
      aligned_result.wcet += startAlign( 0 );
      if ( aligned_result.wcet < iteration_result.wcet || stdOffsetsUnbounded ) {
        iteration_result = aligned_result;

        /* Update the offset results with endAlign penalty if needed. */
        if ( isOffsetDataMaximal( &aligned_result.offsets ) ) {
          iteration_result.wcet += endAlign( iteration_result.wcet );
          iteration_result.offsets = createOffsetDataFromOffsetBounds(
                                       currentOffsetRepresentation, 0, 0 );
        }

        DOUT( "  Took over zero-aligned result: BCET %llu, WCET %llu, offsets %s\n",
            iteration_result.bcet, iteration_result.wcet,
            getOffsetDataString( &iteration_result.offsets ) );
      }
    }

    // Apply the needed changes to the graph
    graphChanged = 0;

    // 'j' is the starting offset
    const _Bool source_is_imprecise = isOffsetDataMaximal( &current_offsets );
    const _Bool target_is_imprecise = isOffsetDataMaximal( &iteration_result.offsets );
    ITERATE_OFFSETS( current_offsets, j,
      offset_graph_node * const start = ( source_is_imprecise ?
                                          &graph->unknown_offset_node :
                                          getOffsetGraphNode( graph, j ) );

      // Add the missing edges
      // 'k' indexes the target offset
      ITERATE_OFFSETS( iteration_result.offsets, k,
        offset_graph_node * const end = ( target_is_imprecise ?
                                          &graph->unknown_offset_node :
                                          getOffsetGraphNode( graph, k ) );
        offset_graph_edge * const edge = getOffsetGraphEdge( graph, start, end );

        if ( edge == NULL ) {
          addOffsetGraphEdge( graph, start, end,
              iteration_result.bcet, iteration_result.wcet );
          graphChanged = 1;

          assert( getOffsetGraphEdge( graph, start, end ) &&
                  "Internal error!" );
        } else {
          if ( edge->bcet > iteration_result.bcet ) {
            edge->bcet = iteration_result.bcet;
            graphChanged = 1;
          }
          if ( edge->wcet < iteration_result.wcet ) {
            edge->wcet = iteration_result.wcet;
            graphChanged = 1;
          }
        }

        if ( target_is_imprecise ) {
          break;
        }
      );

      if ( source_is_imprecise ) {
        break;
      }
    );

    // Update iteration number
    iteration_number++;

  } while( // Iterate until the graph converges
           ( graphChanged &&
             // But don't iterate again when the offsets have already converged,
             // because there can be no more changes to the graph then in the
             // next iteration.
             !isOffsetDataEqual( &current_offsets, &iteration_result.offsets ) ) &&
           // In any case, stop when the loopbound is reached
           ( iteration_number < lp->loopbound ) );

  DOUT( "Analyzed %u iterations\n", iteration_number );

  // Add the edges from all offsets to the supersink
  for ( i = 0; i < tdma_interval; i++ ) {
    addOffsetGraphEdge( graph, getOffsetGraphNode( graph, i ), &graph->supersink, 0, 0 );
  }
  addOffsetGraphEdge( graph, &graph->unknown_offset_node, &graph->supersink, 0, 0 );

  // TODO: Improve the heuristic to determine when not to use the
  //       graph-tracking because of its high complexity
  if ( allIterationsAreEqual ) {
    result = analyze_loop_global_convergence( lp, proc, loop_context, start_offsets );
    DOUT( "Took over convergence result: BCET %llu, WCET %llu, offsets %s\n", result.bcet,
        result.wcet, getOffsetDataString( &result.offsets ) );
  } else {
    // Solve flow problems
    // (Mind the peeled-off iteration of the loop, see beginning o this function)
    result.bcet += computeOffsetGraphLoopBCET( graph, lp->loopbound - 1 );
    result.wcet += computeOffsetGraphLoopWCET( graph, lp->loopbound - 1 );
    result.offsets = computeOffsetGraphLoopOffsets( graph, lp->loopbound - 1,
                       currentOffsetRepresentation );
    DOUT( "Loop results: BCET %llu, WCET %llu, offsets %s\n", result.bcet,
            result.wcet, getOffsetDataString( &result.offsets ) );
  }

  freeOffsetGraph( graph );

  assert( checkResult( &result ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given loop when starting from 
 * the given offset range.
 * 
 * This function should not be called directly, only through its wrapper 'analyze_loop'. */
static combined_result analyze_loop_alignment_aware( const loop * const lp,
    const procedure * const proc, const uint loop_context, const offset_data start_offsets )
{
  DSTART( "analyze_loop_alignment_aware" );
  assert( lp && proc && isOffsetDataValid( &start_offsets ) &&
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
static combined_result analyze_loop( const loop * const lp,
    const procedure * const proc, const uint loop_context,
    const offset_data start_offsets )
{
  DSTART( "analyze_loop" );
  assert( lp && proc && isOffsetDataValid( &start_offsets ) &&
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
    const offset_data zero_offsets = createOffsetDataFromOffsetBounds(
                                       currentOffsetRepresentation, 0, 0 );
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
static combined_result analyze_proc_alignment_aware( const procedure * const proc,
                                                     const offset_data start_offsets )
{
  DSTART( "analyze_proc_alignment_aware" );
  assert( proc && isOffsetDataValid( &start_offsets ) &&
          "Invalid arguments!" );

  /* Buffering currently only works for the "RANGE" data type. */
  combined_result **bufferLocation = NULL;
  if ( currentOffsetRepresentation == OFFSET_DATA_TYPE_RANGE ) {
    /* Check whether we have already computed the requested result. */
    bufferLocation = &proc_results[proc->pid]
      [getOffsetDataMinimumOffset( &start_offsets )]
      [getOffsetDataMaximumOffset( &start_offsets )];
    if ( *bufferLocation != NULL ) {
      DRETURN( **bufferLocation );
    }
  }

  DOUT( "Analyzing procedure %d with offsets %s\n",
        proc->pid, getOffsetDataString( &start_offsets ) );

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
    const offset_data block_start_offsets = ( i == proc->num_topo - 1
        ? start_offsets
        : getStartOffsets( bb, proc, proc->topo, proc->num_topo, block_results ) );
    block_results[i] = analyze_block( bb, proc, NULL, 0, block_start_offsets );
  }

  /* Now all BCETS, WCETs and offset bounds for individual blocks are finished */

  /* Compute final procedure BCET and WCET by propagating the values through the DAG. */
  combined_result result = summarizeDAGResults( proc->num_topo, proc->topo,
                                                block_results, proc );

  free( block_results );

  /* Insert result into result buffer. */
  if ( bufferLocation != NULL ) {
    MALLOC( *bufferLocation, combined_result*,
        sizeof( combined_result ), "*bufferLocation" );
    **bufferLocation = result;
  }

  DOUT( "Procedure %d WCET / BCET result is %llu / %llu"
      " with offsets %s\n", proc->pid, result.bcet, result.wcet,
      getOffsetDataString( &result.offsets ) );

  assert( checkResult( &result ) && "Invalid result!" );
  DRETURN( result );
}


/* Computes the BCET, WCET and offset bounds for the given procedure when starting from
 * the given offset range.
 *
 * This method is just an intelligent wrapper for the respective sub-methods. */
static combined_result analyze_proc( const procedure * const proc,
                                     const offset_data start_offsets )
{
  DSTART( "analyze_proc" );

  // Get the result using our alignment-aware procedure analysis
  DOUT( "Performing alignment-sensitive procedure analysis\n" );
  combined_result result = analyze_proc_alignment_aware( proc, start_offsets );

  /* If we are supposed to try the penalized alignment too, then we also compute the result
   * using the zero-alignment and add the appropriate alignment penalties. If this yields a
   * superior solution, then we pick that one instead of the previously computed result. */
  if ( tryPenalizedAlignment ) {
    // TODO: The alignments may be computed for a wrong segment in case of multi-segment
    //       schedules (see definitions of startAlign/endAlign)
    const offset_data zero_offsets = createOffsetDataFromOffsetBounds(
                                       currentOffsetRepresentation, 0, 0 );
    combined_result pal_result = analyze_proc_alignment_aware( proc, zero_offsets );
    pal_result.wcet += startAlign( 0 );

    // If the result is better, then take this one
    if ( pal_result.wcet < result.wcet ) {
      DOUT( "Took over penalized alignment result!\n" );
      result = pal_result;
    }
  }

  DOUT( "Procedure %d WCET / BCET result is %llu / %llu"
      " with offsets %s\n", proc->pid, result.bcet, result.wcet,
      getOffsetDataString( &result.offsets ) );

  DRETURN( result );
}


// #########################################
// #### Definitions of public functions ####
// #########################################


/* Analyze worst case execution time of all the tasks inside a MSC.
 *
 * 'msc' is the MSC to analyze.
 * 'tdma_bus_schedule_file' holds the filename of the TDMA bus specification to use
 * 'analysis_type' should be the type of loop analysis that shall be used
 * 'try_penalized_alignment' if this is true, then the alignment analysis will try to
 *                           work like the purely structural analysis by assuming an
 *                           offset range of [0,0] and adding the appropriate
 *                           alignment penalties.
 * 'offset_representation' should denote the data type which is to be used for the
 *                         offset representation. */
void compute_bus_ET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file,
   enum LoopAnalysisType analysis_type, _Bool try_penalized_alignment,
   enum OffsetDataType offset_representation )
{
  DSTART( "compute_bus_ET_MSC_alignment" );
  assert( msc && tdma_bus_schedule_file && "Invalid arguments!" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );
  // TODO: This won't work for segmented schedules
  setOffsetDataMaxOffset( getCoreSchedule( 0, 0 )->interval - 1 );

  /* Set the analysis options to use. */
  currentLoopAnalysisType = analysis_type;
  tryPenalizedAlignment = try_penalized_alignment;
  currentOffsetRepresentation = offset_representation;

  /* Reset the earliest/latest time of all cores */
  memset( earliest_core_time, 0, num_core * sizeof(ull) );
  memset( latest_core_time, 0, num_core * sizeof(ull) );

  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    DOUT( "Analyzing Task WCET %s (alignment-aware) ......\n", msc->taskList[k].task_name );
    const milliseconds analysis_start = getmsecs();

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* Initialize the result buffers. */
    initResultBuffers( cur_task );

    /* First get the earliest and latest start time of the current task. */
    const ull earliest_start = get_earliest_task_start_time( cur_task, ncore );
    const ull latest_start   = get_latest_task_start_time( cur_task, ncore );
    const offset_data initial_offsets = createOffsetDataFromTimeBounds(
        currentOffsetRepresentation, earliest_start, latest_start );
    DOUT( "Initial offset bounds: %s\n", getOffsetDataString( &initial_offsets ) );

    /* Then compute and set the best and worst case cost of this task */
    combined_result main_result = analyze_proc( task_main, initial_offsets );
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

    /* Free the result buffers. */
    freeResultBuffers( cur_task );

    /* Measure time needed for single-task analysis. */
    const milliseconds analysis_end = getmsecs();
    cur_task->bcet_analysis_time = analysis_end - analysis_start;
    cur_task->wcet_analysis_time = analysis_end - analysis_start;

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
