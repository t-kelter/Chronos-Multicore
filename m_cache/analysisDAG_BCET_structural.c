// Include standard library headers
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "analysisDAG_BCET_structural.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"
#include "dump.h"
#include "wcrt/cycle_time.h"


// Forward declarations of static functions
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp );
static void computeBCET_proc( procedure* proc, ull start_time );


/***********************************************************************/
/* sudiptac:: This part of the code is only used for the WCET and
 * BCET analysis in presence shared data bus. All procedures in the
 * following is used only for this purpose and therefore can safely 
 * be ignored for analysis which does not include shared data bus 
 */
/***********************************************************************/

/* sudiptac:: Determines BCET of a procedure in presence of shared data
 * bus. We assume the shared cache analysis at this point and CHMC 
 * classification for every instruction has already been computed. We 
 * also assume a statically generated TDMA bus schedule and the 
 * worst/best case starting time of the procedure since in presence of 
 * shared data bus worst/best case execution time of a procedure/loop 
 * depends on its starting time */


/* This sets the latest starting time of a block during BCET calculation.
 * (Context-aware)
 */
static void set_start_time_BCET_opt( block* bb, procedure* proc, uint context )
{
  DSTART( "set_start_time_BCET_opt" );
  ull min_start;

  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    block * const pred_bb = proc->bblist[bb->incoming[i]];
    assert( pred_bb && "Missing basic block!" );

    /* Determine the predecessors' latest finish time */
    if ( i == 0 ) {
      min_start = pred_bb->fin_opt[context];
    } else {
      min_start = MIN( min_start, pred_bb->fin_opt[context] );
    }
  }

  if ( bb->num_incoming == 0 ) {
    min_start = 0;
  }

  /* Now set the starting time of this block to be the latest
   * finish time of predecessors block */
  bb->start_opt[context] = min_start;

  DOUT( "Setting min start of bb %d (context %u) = %Lu\n",
      bb->bbid, context, min_start );
  DEND();
}


/* Returns the BCET of a loop, after loop BCETs have been computed for
 * all CHMC contexts. This method summarises the contexts' BCETs, adds the
 * bus alignment offsets and returns the final loop BCET.
 *
 * 'enclosing_loop_context' should be the context of the surrounding loop,
 * for which the BCET should be obtained.
 * */
static ull getLoopBCET( const loop *lp, int enclosing_loop_context )
{
  /* Each CHMC index is of the form
   *
   * i = x_{head->num_chmc} x_{head->num_chmc - 1} ... x_1
   *
   * where each x is a binary digit. We are interested in an upper bound
   * on the BCET of the function in its first and in the succeeding
   * iterations. We take the least 'lp->level - 1' bits from the
   * enclosing loop context and add a '0' to get the BCET for the
   * first iteration and a '1' to get the BCET for the successive
   * iterations.
   *
   * For further information about CHMC contexts see header.h:num_chmc
   */

  const int index_first_iteration = getInnerLoopContext( lp, enclosing_loop_context, 1 );
  const int index_next_iterations = getInnerLoopContext( lp, enclosing_loop_context, 0 );

  const ull firstIterationBCET = lp->bcet_opt[index_first_iteration];
  const ull nextIterationsBCET = lp->bcet_opt[index_next_iterations];

  if ( lp->loopbound >= 1 ) {
    // For the BCET analysis we must not consider any alignment penalty
    const ull execution_cost = firstIterationBCET
          + ( nextIterationsBCET * ( lp->loopbound - 1 ) );
    return execution_cost;
  } else {
    return 0;
  }

}

/* Preprocess one loop for optimized bus aware BCET calculation */
/* This takes care of the alignments of loop at the beginning and at the
 * end */
static void preprocess_one_loop( loop* lp, procedure* proc )
{
  DSTART( "preprocess_one_loop" );

  /* We can assume the start time to be always zero */
  const ull start_time = 0;
  uint min_fin[64] = { 0 };

  /* Compute only once */
  if ( lp->bcet_opt[0] )
    DRETURN();

  DOUT( "Visiting loop = %d.%d.0x%x\n", lp->pid, lp->lpid, (uintptr_t)lp );

  /* Traverse all the blocks in topological order. Topological
   * order does not assume internal loops. Thus all internal 
   * loops are considered to be black boxes */
  int i;
  for ( i = lp->num_topo - 1; i >= 0; i-- ) {
    block *bb = lp->topo[i];
    /* bb cannot be empty */
    assert(bb);

    memset( min_fin, 0, 64 );

    /* Traverse over all the CHMC-s of this basic block */
    int j;
    for ( j = 0; j < bb->num_chmc; j++ ) {

      /* First of all, we have to determine the worst-case starting time
       * in the current context. This time is needed in all cases, also when
       * the current block 'bb' is a nested loop head, because there may be
       * other instructions in the loop that precede the nested head. */
      if ( i == lp->num_topo - 1 ) {
        bb->start_opt[j] = start_time;
      } else {
        set_start_time_BCET_opt( bb, proc, j );
      }

      /* Check whether this basic block is the header of some other 
       * loop */
      loop * const inlp = check_loop( bb, proc );
      if ( inlp && i != lp->num_topo - 1 ) {

        /* Backup the value of bb->start_opt[j], because the analysis of the
         * inner loop will overwrite it. */
        const ull original_start_time = bb->start_opt[j];
        assert( inlp->level == lp->level + 1 && "Invalid internal data!" );

        /* As this inner loop header has twice the amount of CHMC contexts compared to
         * the outer loop's blocks, it is sufficient to compute the bb->fin_opt values
         * for the contexts j < bb->num_chmc / 2, because all others are contexts which
         * belong to the inner loop. */
        if ( j < bb->num_chmc / 2 ) {
          preprocess_one_loop( inlp, proc );
          bb->fin_opt[j] = original_start_time + getLoopBCET( inlp, j );
        }

      } else {

        uint bb_cost = 0;

        int k;
        for ( k = 0; k < bb->num_instr; k++ ) {

          instr * const inst = bb->instrlist[k];
          assert(inst);

          /* First handle instruction cache access time */
          const acc_type acc_t = check_hit_miss( bb, inst, j,
                                                 ACCESS_SCENARIO_BCET );
          bb_cost += determine_latency( bb, bb->start_opt[j] + bb_cost,
                                        acc_t, NULL, ACCESS_SCENARIO_BCET);

          /* Then add cost for executing the instruction. */
          bb_cost += getInstructionBCET( inst );

          /* Handle procedure call instruction */
          if ( IS_CALL(inst->op) ) {
            procedure * const callee = getCallee( inst, proc );

            /* For ignoring library calls */
            if ( callee ) {
              /* Compute the BCET of the callee procedure here.
               * We dont handle recursive procedure call chain */
              computeBCET_proc( callee, bb->start_opt[j] + bb_cost );
              bb_cost += callee->running_cost;
            }
          }
        }

        /* Set finish time of the basic block */
        bb->fin_opt[j] = bb->start_opt[j] + bb_cost;
      }

      /* Set max finish time */
      min_fin[j] = MAX( min_fin[j], bb->fin_opt[j] );
    }
  }

  int j;
  for ( j = 0; j < lp->loophead->num_chmc; j++ ) {
    lp->bcet_opt[j] = ( min_fin[j] - 1 );
    DOUT( "BCET of loop (%d.%d.0x%x)[%d] = %Lu\n", lp->pid, lp->lpid,
        (unsigned int)(uintptr_t) lp, j, lp->bcet_opt[j] );
  }

  DEND();
}

/* Preprocess each loop for optimized bus aware BCET calculation */
static void preprocess_all_loops( procedure* proc )
{
  /* Preprocess loops....in reverse topological order i.e. in reverse 
   * order of detection */
  int i;
  for ( i = proc->num_loops - 1; i >= 0; i-- ) {
    /* Preprocess only outermost loop, inner ones will be processed 
     * recursively */
    if ( proc->loops[i]->level == 0 )
      preprocess_one_loop( proc->loops[i], proc );
  }
}

/* Compute worst case finish time and cost of a block */
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp )
{
  DSTART( "computeBCET_block" );

  const uint proc_body_context = 0;

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop* inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    DOUT( "Block represents inner loop!\n" );
    bb->finish_time = bb->start_time + getLoopBCET( inlp, proc_body_context );

  /* It's not a loop. Go through all the instructions and
   * compute the BCET of the block */
  } else {
    uint bb_cost = 0;

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {
      instr* inst = bb->instrlist[i];
      assert(inst);

      const uint old_cost = bb_cost;

      /* First handle instruction cache access. */
      const acc_type acc_t = check_hit_miss( bb, inst, proc_body_context,
                                             ACCESS_SCENARIO_BCET );
      bb_cost += determine_latency( bb, bb->start_time + bb_cost,
                                    acc_t, NULL, ACCESS_SCENARIO_BCET );

      /* Then add cost for executing the instruction. */
      bb_cost += getInstructionBCET( inst );

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure * const callee = getCallee( inst, proc );

        /* For ignoring library calls */
        if ( callee ) {
          DOUT( "Block calls internal function %u\n", callee->pid );
          /* Compute the BCET of the callee procedure here.
           * We dont handle recursive procedure call chain */
          computeBCET_proc( callee, bb->start_time + bb_cost );
          bb_cost += callee->running_cost;
        }
      }

      DOUT( "  Instruction 0x%s: BCET %u\n", inst->addr,
          bb_cost - old_cost );
    }
    /* The accumulated cost is computed. Now set the latest finish
     * time of this block */
    bb->finish_time = bb->start_time + bb_cost;
  }

  DOUT( "Accounting BCET %llu for block 0x%s - 0x%s\n",
      bb->finish_time - bb->start_time, bb->instrlist[0]->addr,
      bb->instrlist[bb->num_instr - 1]->addr );
  DEND();
}

static void computeBCET_proc( procedure* proc, ull start_time )
{
  DSTART( "computeBCET_proc" );

  /* Preprocess all the loops for optimized BCET calculation */
  /********CAUTION*******/
  preprocess_all_loops( proc );

  /* Reset all timing information */
  reset_timestamps( proc, start_time );

  /* Recursively compute the finish time and BCET of each
   * predecessors first */
  int i;
  for ( i = proc->num_topo - 1; i >= 0; i-- ) {
    block* bb = proc->topo[i];
    assert(bb);
    /* If this is the first block of the procedure then
     * set the start time of this block to be the same 
     * with the start time of the procedure itself */
    if ( i == proc->num_topo - 1 )
      bb->start_time = start_time;
    else
      set_start_time_BCET( bb, proc );
    computeBCET_block( bb, proc, NULL );
  }

  /* Now calculate the final BCET */
  ull min_f_time = 0;
  for ( i = 0; i < proc->num_topo; i++ ) {
    assert(proc->topo[i]);

    if ( proc->topo[i]->num_outgoing > 0 )
      break;

    if ( i == 0 ) {
      min_f_time = proc->topo[i]->finish_time;
    } else {
      min_f_time = MIN( min_f_time, proc->topo[i]->finish_time );
    }
  }

  proc->running_finish_time = min_f_time;
  proc->running_cost = min_f_time - start_time;

  DOUT( "Set best case cost of the procedure %d = %Lu\n", proc->pid, proc->running_cost );
  DEND();
}

/* Analyze best case execution time of all the tasks inside
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC_structural( MSC *msc, const char *tdma_bus_schedule_file )
{
  DSTART( "compute_bus_BCET_MSC_structural" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the latest time of all cores */
  memset( latest_core_time, 0, num_core * sizeof(ull) );
  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    DOUT( "Analyzing Task BCET %s......\n", msc->taskList[k].task_name );
    const milliseconds analysis_start = getmsecs();

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* First get the earliest start time of the current task. */
    ull start_time = get_earliest_task_start_time( cur_task, ncore );

    /* Then compute and set the best case cost of this task */
    computeBCET_proc( task_main, start_time );
    cur_task->bcet = task_main->running_cost;

    /* Now update the earliest starting time in this core */
    earliest_core_time[ncore] = start_time + cur_task->bcet;

    /* Since the interference file for a MSC was dumped in topological 
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_task_earliest_start_time( msc, cur_task );

    /* Measure time needed for single-task analysis. */
    const milliseconds analysis_end = getmsecs();
    cur_task->bcet_analysis_time = analysis_end - analysis_start;

    DOUT( "**************************************************************\n" );
    DOUT( "Earliest start time of the program = %Lu cycles\n", start_time );
    DOUT( "Earliest finish time of the task = %Lu cycles\n", task_main->running_finish_time );
    DOUT( "BCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    DOUT( "**************************************************************\n\n" );
  }

  DEND();
}
