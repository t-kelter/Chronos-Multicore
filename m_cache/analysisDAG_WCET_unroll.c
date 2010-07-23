#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_WCET_unroll.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"


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
  DEBUG_PRINTF( "Visiting loop = (%d.%lx)\n", lp->lpid, (uintptr_t) lp );

  /* FIXME: correcting loop bound */
  const int lpbound = lp->loopexit ? lp->loopbound : ( lp->loopbound + 1 );
  const uint outer_loops_context = context;

  /* For computing wcet of the loop it must be visited 
   * multiple times equal to the loop bound */
  int i;
  for ( i = 0; i < lpbound; i++ ) {
    /* See header.h:num_chmc for further details about CHMC contexts. */
    const uint current_level_bitmask = ( 1 << lp->level );
    assert( ( outer_loops_context & current_level_bitmask ) == 0 &&
      "Outer context has bits set which belong to inner loop!" );
    const uint inner_context = ( i == 0
        ? outer_loops_context
        : outer_loops_context + current_level_bitmask );

    /* Go through the blocks in topological order */
    int j;
    for ( j = lp->num_topo - 1; j >= 0; j-- ) {
      block * const bb = lp->topo[j];
      assert(bb);

      /* Set the start time of this block in the loop */
      /* If this is the first iteration and loop header
       * set the start time to be the latest finish time
       * of predecessor otherwise latest finish time of
       * loop sink */
      if ( bb->bbid == lp->loophead->bbid && i == 0 ) {
        set_start_time_WCET( bb, proc );
      } else if ( bb->bbid == lp->loophead->bbid ) {
        assert(lp->loopsink);
        bb->start_time = MAX( lp->loopsink->finish_time, bb->start_time );
        DEBUG_PRINTF( "Setting loop %d finish time = %Lu\n", lp->lpid, lp->loopsink->finish_time );
      } else {
        set_start_time_WCET( bb, proc );
      }

      computeWCET_block( bb, proc, lp, inner_context );
    }
  }
}

/* Compute worst case finish time and cost of a block */
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp, uint context )
{
  DEBUG_PRINTF( "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t) bb );

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
      const acc_type acc_t = check_hit_miss( bb, inst, context );
      bb_cost += determine_latency( bb, bb->start_time + bb_cost, acc_t );

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
    }
    /* The accumulated cost is computed. Now set the latest finish
     * time of this block */
    bb->finish_time = bb->start_time + bb_cost;
  }
  DEBUG_PRINTF( "Setting block %d finish time = %Lu\n", bb->bbid, bb->finish_time );
}

static void computeWCET_proc( procedure* proc, ull start_time )
{
  /* Preprocess CHMC classification for each instruction inside
   * the procedure */
  preprocess_chmc_WCET( proc );

  /* Preprocess CHMC classification for L2 cache for each 
   * instruction inside the procedure */
  preprocess_chmc_L2_WCET( proc );

  /* Reset all timing information */
  reset_timestamps( proc, start_time );

#ifdef _DEBUG
  dump_pre_proc_chmc(proc);
#endif

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

#ifdef _DEBUG
  dump_prog_info(proc);
#endif

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
  DEBUG_PRINTF( "Set worst case cost of the procedure %d = %Lu\n", proc->pid, proc->running_cost );
}

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC_unroll( MSC *msc, const char *tdma_bus_schedule_file )
{
  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the latest time of all cores */
  memset( latest_core_time, 0, num_core * sizeof(ull) );
  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    PRINT_PRINTF( "Analyzing Task WCET %s......\n", msc->taskList[k].task_name );

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = msc->taskList[k].main_copy;

    /* First get the latest start time of the current task. */
    ull start_time = get_latest_task_start_time( cur_task, ncore );

    /* Then compute and set the worst case cost of this task */
    computeWCET_proc( task_main, start_time );
    msc->taskList[k].wcet = task_main->running_cost;

    /* Now update the latest starting time in this core */
    latest_core_time[ncore] = start_time + msc->taskList[k].wcet;

    /* Since the interference file for a MSC was dumped in topological 
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_task_latest_start_time( msc, cur_task );

    PRINT_PRINTF( "\n**************************************************************\n" );
    PRINT_PRINTF( "Latest start time of the task = %Lu cycles\n", start_time );
    PRINT_PRINTF( "Latest finish time of the task = %Lu cycles\n", task_main->running_finish_time );
    PRINT_PRINTF( "WCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    PRINT_PRINTF( "**************************************************************\n\n" );
  }
}
