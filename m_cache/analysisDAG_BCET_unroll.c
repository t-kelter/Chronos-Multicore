#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "analysisDAG_BCET_unroll.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"
#include "dump.h"

#include <debugmacros/debugmacros.h>


// Forward declarations of static functions
static void computeBCET_loop( loop* lp, procedure* proc, uint context );
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp, uint context );
static void computeBCET_proc( procedure* proc, ull start_time );


/***********************************************************************/
/* sudiptac:: This part of the code is only used for the BCET analysis
 * in presence shared data bus. All procedures in the
 * following is used only for this purpose and therefore can safely 
 * be ignored for analysis which does not include shared data bus 
 */
/***********************************************************************/

/* sudiptac:: Determines BCET of a procedure in presence of shared data
 * bus. We assume the shared cache analysis at this point and CHMC 
 * classification for every instruction has already been computed. We 
 * also assume a statically generated TDMA bus schedule and the 
 * best case starting time of the procedure since in presence of 
 * shared data bus best case execution time of a procedure/loop 
 * depends on its starting time */


/* Computes the earliest finish time and best case cost of a loop.
 * This procedure fully unrolls the loop virtually during computation.
 */
static void computeBCET_loop( loop* lp, procedure* proc, uint context )
{
  DSTART( "computeBCET_loop" );

  DOUT( "Visiting loop = (%d.%lx)\n", lp->lpid, (uintptr_t)lp);

  const int lpbound = lp->loopbound;

  /* For computing BCET of the loop it must be visited 
   * multiple times equal to the loop bound */
  int i;
  for ( i = 0; i < lpbound; i++ ) {
    /* See header.h:num_chmc for further details about CHMC contexts. */
    const uint inner_context = getInnerLoopContext( lp, context, i == 0 );

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
        set_start_time_BCET( bb, proc );
      } else if ( bb->bbid == lp->loophead->bbid ) {
        assert(lp->loopsink);
        bb->start_time = MAX( lp->loopsink->finish_time, bb->start_time );
        DOUT( "Setting loop %d finish time = %Lu\n", lp->lpid, lp->loopsink->finish_time );
      } else {
        set_start_time_BCET( bb, proc );
      }

      computeBCET_block( bb, proc, lp, inner_context );
    }
  }

  DEND();
}

/* Compute worst case finish time and cost of a block */
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp, uint context )
{
  DSTART( "computeBCET_block" );
  DOUT( "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t)bb );

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop * const inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    computeBCET_loop( inlp, proc, context );

  /* Its not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {
    uint bb_cost = 0;

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {
      instr* inst = bb->instrlist[i];
      assert(inst);

      /* First handle instruction cache access time */
      const acc_type acc_t = check_hit_miss( bb, inst, context, 
                                             ACCESS_SCENARIO_BCET );
      bb_cost += determine_latency( bb, bb->start_time + bb_cost, acc_t );

      /* Then add cost for executing the instruction. */
      bb_cost += getInstructionBCET( inst );

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure * const callee = getCallee( inst, proc );

        /* For ignoring library calls */
        if ( callee ) {
          /* Compute the WCET of the callee procedure here.
           * We dont handle recursive procedure call chain */
          computeBCET_proc( callee, bb->start_time + bb_cost );
          bb_cost += callee->running_cost;
        }
      }
    }
    /* The accumulated cost is computed. Now set the latest finish
     * time of this block */
    bb->finish_time = bb->start_time + bb_cost;
  }

  DOUT( "Setting block %d finish time = %Lu\n", bb->bbid, bb->finish_time);
  DEND();
}

static void computeBCET_proc( procedure* proc, ull start_time )
{
  DSTART( "computeBCET_proc" );

  /* Reset all timing information */
  reset_timestamps( proc, start_time );

  DACTION(
      dump_pre_proc_chmc(proc, ACCESS_SCENARIO_BCET);
      dump_pre_proc_chmc(proc, ACCESS_SCENARIO_WCET);
  );

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

    computeBCET_block( bb, proc, NULL, 0 );
  }

#ifdef _DEBUG
  dump_prog_info(proc);
#endif

  /* Now calculate the final BCET */
  ull min_f_time;
  for ( i = 0; i < proc->num_topo; i++ ) {
    assert(proc->topo[i]);

    if ( proc->topo[i]->num_outgoing > 0 )
      break;

    if ( i == 0 )
      min_f_time = proc->topo[i]->finish_time;
    else if ( min_f_time > proc->topo[i]->finish_time )
      min_f_time = proc->topo[i]->finish_time;
  }

  proc->running_finish_time = min_f_time;
  proc->running_cost = min_f_time - start_time;

  DOUT( "Set best case cost of the procedure %d = %Lu\n",
      proc->pid, proc->running_cost);
  DEND();
}

/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC_unroll( MSC *msc, const char *tdma_bus_schedule_file )
{
  DSTART( "compute_bus_BCET_MSC_unroll" );

  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the earliest time of all cores */
  memset( earliest_core_time, 0, num_core * sizeof(ull) );

  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    DOUT( "Analyzing Task BCET %s......\n", msc->taskList[k].task_name);

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = cur_task->main_copy;

    /* First get the earliest start time of the current task. */
    ull start_time = get_earliest_task_start_time( cur_task, ncore );

    /* Then compute and set the worst case cost of this task */
    computeBCET_proc( task_main, start_time );
    cur_task->bcet = task_main->running_cost;

    /* Now update the latest starting time in this core */
    earliest_core_time[ncore] = start_time + cur_task->bcet;

    /* Since the interference file for a MSC was dumped in topological 
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_task_earliest_start_time( msc, cur_task );

    DOUT("\n**************************************************************\n");
    DOUT("Earliest start time of the task = %Lu cycles\n", start_time);
    DOUT("Earliest finish time of the task = %Lu cycles\n",
        task_main->running_finish_time);
    DOUT( "BCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    DOUT("**************************************************************\n\n");
  }

  DEND();
}
