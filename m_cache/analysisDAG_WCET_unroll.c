// Include standard library headers
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "analysisDAG_WCET_unroll.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"
#include "dump.h"
#include "wcrt/cycle_time.h"


// Forward declarations of static functions
static void computeWCET_loop( loop* lp, procedure* proc, uint context );
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp, uint context );
static void computeWCET_proc( procedure* proc, ull start_time );


/***********************************************************************/
/* sudiptac:: This part of the code is only used for the WCET and
 * BCET analysis in presence shared data bus. All procedures in the
 * following is used only for this purpose and therefore can safely 
 * be ignored for analysis which does not include shared data bus 
 */
/***********************************************************************/

/* sudiptac:: Determines WCET of a procedure in presence of shared data
 * bus. We assume the shared cache analysis at this point and CHMC 
 * classification for every instruction has already been computed. We 
 * also assume a statically generated TDMA bus schedule and the 
 * worst/best case starting time of the procedure since in presence of 
 * shared data bus worst/best case execution time of a procedure/loop 
 * depends on its starting time */


/* Computes the latest finish time and worst case cost of a loop.
 * This procedure fully unrolls the loop virtually during computation.
 */
static void computeWCET_loop( loop* lp, procedure* proc, uint context )
{
  DSTART( "computeWCET_loop" );
  DOUT( "Analyzing loop %u.%u in context %u with start time %llu\n",
      lp->pid, lp->lpid, context, lp->loophead->start_time );
  assert( lp && proc && lp->loopsink );

  /* For computing wcet of the loop it must be visited 
   * multiple times equal to the loop bound */
  int i, j;
  for ( i = 0; i < lp->loopbound; i++ ) {
    /* See header.h:num_chmc for further details about CHMC contexts. */
    const uint inner_context = getInnerLoopContext( lp, context, i == 0 );

    /* Go through the blocks in topological order */
    for ( j = lp->num_topo - 1; j >= 0; j-- ) {
      block * const bb = lp->topo[j];
      assert(bb);

      /* Set start time for loop header at first iteration. */
      if ( bb->bbid == lp->loophead->bbid && i == 0 ) {
        set_start_time_WCET( bb, proc );

      /* Set start time for loop header at successive iterations. */
      } else if ( bb->bbid == lp->loophead->bbid ) {
        bb->start_time = MAX( lp->loopsink->finish_time, bb->start_time );

      /* Set start time for a block inside the loop. */
      } else {
        set_start_time_WCET( bb, proc );
      }

      computeWCET_block( bb, proc, lp, inner_context );
    }

    assert( lp->loopsink->finish_time >= lp->loophead->start_time &&
        "Computed negative loop runtime!" );
    DOUT( "Setting loop %u.%u iteration %d finish time = %llu "
        "(iteration wcet %llu, context %u)\n", lp->pid, lp->lpid, i,
        lp->loopsink->finish_time,
        lp->loopsink->finish_time - lp->loophead->start_time, inner_context );
  }

  /* TODO: The execution time of the loop header, after the 'loopbound' iterations,
   *       is not accounted for in any of the DAG-based analyses. If this is fixed,
   *       then fix it in all analyses. */

  /* Carry over the loop WCET to the loop header. */
  lp->loophead->finish_time = ( lp->loopbound <= 0 ? lp->loophead->start_time
                                                   : lp->loopsink->finish_time );

  /* If the loop was not executed at all, set all loop blocks to have the same
   * finish time as the loop header. */
  if ( lp->loopbound <= 0 ) {
    /* Set finish time for all other loop blocks. */
    for ( j = lp->num_topo - 1; j >= 0; j-- ) {
      block * const bb = lp->topo[j];
      assert(bb);

      if ( bb->bbid != lp->loophead->bbid ) {
        bb->finish_time = lp->loophead->finish_time;
      }
    }
    DOUT( "Loop %u.%u is not executed at all, finish time = %Lu\n",
        lp->pid, lp->lpid, lp->loophead->finish_time );
  }

  DEND();
}

/* Compute worst case finish time and cost of a block */
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp, uint context )
{
  DSTART( "computeWCET_block" );
  DOUT( "Visiting block %u.%u at time %llu\n", bb->pid, bb->bbid, bb->start_time );

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop* inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    computeWCET_loop( inlp, proc, context );

  /* It's not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {
    uint bb_cost = 0;

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {
      instr* inst = bb->instrlist[i];
      assert(inst);

      /* First handle instruction cache access time */
      const acc_type acc_t = check_hit_miss( bb, inst, context,
                                             ACCESS_SCENARIO_WCET );
      bb_cost += determine_latency( bb, bb->start_time + bb_cost,
                                    acc_t, NULL, ACCESS_SCENARIO_WCET );

      /* Then add cost for executing the instruction. */
      bb_cost += getInstructionWCET( inst );

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure * const callee = getCallee( inst, proc );

        /* For ignoring library calls */
        if ( callee ) {
          /* Compute the WCET of the callee procedure here.
           * We dont handle recursive procedure call chain */
          computeWCET_proc( callee, bb->start_time + bb_cost );
          bb_cost += callee->running_cost;
        }
      }

      //DOUT( "  Instruction 0x%s: BCET %u\n", inst->addr, bb_cost );
    }
    /* The accumulated cost is computed. Now set the latest finish
     * time of this block */
    bb->finish_time = bb->start_time + bb_cost;
  }

  DOUT( "Setting block %d finish time = %Lu\n", bb->bbid, bb->finish_time );
  DEND();
}

static void computeWCET_proc( procedure* proc, ull start_time )
{
  DSTART( "computeWCET_proc" );

  /* Reset all timing information */
  reset_timestamps( proc, start_time );

  /* Recursively compute the finish time and WCET of each 
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
      set_start_time_WCET( bb, proc );
    computeWCET_block( bb, proc, NULL, 0 );
  }

  /* Now calculate the final WCET */
  ull max_f_time = 0;
  for ( i = 0; i < proc->num_topo; i++ ) {
    assert(proc->topo[i]);
    if ( proc->topo[i]->num_outgoing > 0 )
      break;
    if ( max_f_time < proc->topo[i]->finish_time )
      max_f_time = proc->topo[i]->finish_time;
  }

  proc->running_finish_time = max_f_time;
  proc->running_cost = max_f_time - start_time;

  assert( max_f_time >= start_time && "Computed negative procedure runtime!" );
  DOUT( "Set worst case cost of the procedure %d = %Lu\n", proc->pid, proc->running_cost );
  DEND();
}

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC_unroll( MSC *msc, const char *tdma_bus_schedule_file )
{
  DSTART( "compute_bus_WCET_MSC_unroll" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the latest time of all cores */
  memset( latest_core_time, 0, num_core * sizeof(ull) );
  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    DOUT( "Analyzing Task WCET %s......\n", msc->taskList[k].task_name );
    const milliseconds analysis_start = getmsecs();

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* First get the latest start time of the current task. */
    ull start_time = get_latest_task_start_time( cur_task, ncore );

    /* Then compute and set the worst case cost of this task */
    computeWCET_proc( task_main, start_time );
    cur_task->wcet = task_main->running_cost;

    /* Now update the latest starting time in this core */
    latest_core_time[ncore] = start_time + cur_task->wcet;

    /* Since the interference file for a MSC was dumped in topological 
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_task_latest_start_time( msc, cur_task );

    /* Measure time needed for single-task analysis. */
    const milliseconds analysis_end = getmsecs();
    cur_task->wcet_analysis_time = analysis_end - analysis_start;

    DOUT( "**************************************************************\n" );
    DOUT( "Latest start time of the task = %Lu cycles\n", start_time );
    DOUT( "Latest finish time of the task = %Lu cycles\n", task_main->running_finish_time );
    DOUT( "WCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    DOUT( "**************************************************************\n\n" );
  }

  DEND();
}
