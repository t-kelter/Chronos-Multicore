#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_WCET_structural.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"


// Forward declarations of static functions
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp );
static void computeWCET_proc( procedure* proc, ull start_time );


// The total cycles used for aligning the loops to TDMA slots during the WCET analysis
static ull totalAlignCost = 0;


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


/* This sets the latest starting time of a block during WCET calculation.
 * (Context-aware)
 */
static void set_start_time_WCET_opt( block* bb, procedure* proc, uint context )
{
  ull max_start = bb->start_opt[context];

  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    block * const pred_bb = proc->bblist[bb->incoming[i]];
    assert( pred_bb && "Missing basic block!" );

    /* Determine the predecessors' latest finish time */
    max_start = MAX( max_start, pred_bb->fin_opt[context] );
  }

  /* Now set the starting time of this block to be the latest
   * finish time of predecessors block */
  bb->start_opt[context] = max_start;

  DEBUG_PRINTF( "Setting max start of bb %d (context %u) = %Lu\n",
      bb->bbid, context, max_start );
}

/* Computes end alignment cost of the loop */
static ull endAlign( ull fin_time )
{
  const ull interval = global_sched_data->seg_list[0]->per_core_sched[ncore]->interval;

  if ( fin_time % interval == 0 ) {
    DEBUG_PRINTF( "End align = 0\n" );
    return 0;
  } else {
    DEBUG_PRINTF( "End align = %Lu\n", ( fin_time / interval + 1 ) * interval - fin_time );
    return ( ( fin_time / interval + 1 ) * interval - fin_time );
  }
}

/* computes start alignment cost */
static ull startAlign()
{
  const ull interval = global_sched_data->seg_list[0]->per_core_sched[ncore]->interval;

  DEBUG_PRINTF( "Start align = %u\n", interval );
  return interval;
}

/* Returns the WCET of a loop, after loop WCETs have been computed for
 * all CHMC contexts. This method summarises the contexts' WCETs, adds the
 * bus alignment offsets and returns the final loop WCET.
 *
 * 'enclosing_loop_context' should be the context of the surrounding loop,
 * for which the WCET should be obtained.
 * */
static ull getLoopWCET( const loop *lp, int enclosing_loop_context )
{
  /* Each CHMC index is of the form
   *
   * i = x_{head->num_chmc} x_{head->num_chmc - 1} ... x_1
   *
   * where each x is a binary digit. We are interested in an upper bound
   * on the WCET of the function in its first and in the succeeding
   * iterations. We take the least 'lp->level - 1' bits from the
   * enclosing loop context and add a '0' to get the WCET for the
   * first iteration and a '1' to get the WCET for the successive
   * iterations.
   *
   * For further information about CHMC contexts see header.h:num_chmc
   */

  const int index_first_iteration = getInnerLoopContext( lp, enclosing_loop_context, 1 );
  const int index_next_iterations = getInnerLoopContext( lp, enclosing_loop_context, 0 );

  const ull firstIterationWCET = lp->wcet_opt[index_first_iteration];
  const ull nextIterationsWCET = lp->wcet_opt[index_next_iterations];

  /*
  PRINT_PRINTF( "Using effective WCET of loop (%d.%d)[surrounding context: %d] (first iteration) = %Lu\n",
      lp->pid, lp->lpid, enclosing_context_bits, firstIterationWCET );
  PRINT_PRINTF( "Using effective WCET of loop (%d.%d)[surrounding context: %d] (other iterations) = %Lu\n",
        lp->pid, lp->lpid, enclosing_context_bits, nextIterationsWCET );
  */

  const ull execution_cost = firstIterationWCET +
        + ( nextIterationsWCET * ( lp->loopbound - 1 ) );
  const ull alignment_cost = startAlign() + endAlign( firstIterationWCET )
      + ( endAlign( nextIterationsWCET ) * ( lp->loopbound - 1 ) );

  totalAlignCost += alignment_cost;

  return execution_cost + alignment_cost;
}

/* Preprocess one loop for optimized bus aware WCET calculation */
/* This takes care of the alignments of loop at the beginning and at the
 * end */
static void preprocess_one_loop( loop* lp, procedure* proc )
{
  /* We can assume the start time to be always zero */
  const ull start_time = 0;
  uint max_fin[64] = { 0 };

  /* Compute only once */
  if ( lp->wcet_opt[0] )
    return;

  NPRINT_PRINTF( "Visiting loop = %d.%d.0x%x\n", lp->pid, lp->lpid, (unsigned) lp );

  /* Traverse all the blocks in topological order. Topological
   * order does not assume internal loops. Thus all internal 
   * loops are considered to be black boxes */
  int i;
  for ( i = lp->num_topo - 1; i >= 0; i-- ) {
    block *bb = lp->topo[i];
    /* bb cannot be empty */
    assert(bb);

    memset( max_fin, 0, 64 );

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
        set_start_time_WCET_opt( bb, proc, j );
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
          bb->fin_opt[j] = original_start_time + getLoopWCET( inlp, j );
        }

      } else {

        uint bb_cost = 0;

        int k;
        for ( k = 0; k < bb->num_instr; k++ ) {

          instr * const inst = bb->instrlist[k];
          assert(inst);

          /* First handle instruction cache access time */
          const acc_type acc_t = check_hit_miss( bb, inst, j );
          bb_cost += determine_latency( bb, bb->start_opt[j] + bb_cost, acc_t );

          /* Then add cost for executing the instruction. */
          bb_cost += getInstructionWCET( inst );

          /* Handle procedure call instruction */
          if ( IS_CALL(inst->op) ) {
            procedure * const callee = getCallee( inst, proc );

            /* For ignoring library calls */
            if ( callee ) {
              /* Compute the WCET of the callee procedure here.
               * We dont handle recursive procedure call chain */
              computeWCET_proc( callee, bb->start_opt[j] + bb_cost );
              bb_cost += callee->running_cost;
            }
          }
        }

        /* Set finish time of the basic block */
        bb->fin_opt[j] = bb->start_opt[j] + bb_cost;
      }

      /* Set max finish time */
      max_fin[j] = MAX( max_fin[j], bb->fin_opt[j] );
    }
  }

  int j;
  for ( j = 0; j < lp->loophead->num_chmc; j++ ) {
    lp->wcet_opt[j] = ( max_fin[j] - 1 );
    PRINT_PRINTF( "WCET of loop (%d.%d.0x%lx)[%d] = %Lu\n", lp->pid, lp->lpid, (uintptr_t) lp, j, lp->wcet_opt[j] );
  }
}

/* Preprocess each loop for optimized bus aware WCET calculation */
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
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp )
{
  DEBUG_PRINTF( "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t) bb );

  const uint proc_body_context = 0;

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop* inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    bb->finish_time = bb->start_time + getLoopWCET( inlp, proc_body_context );

  /* It's not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {
    uint bb_cost = 0;

    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {
      instr* inst = bb->instrlist[i];
      assert(inst);

      /* First handle instruction cache access. */
      const acc_type acc_t = check_hit_miss( bb, inst, proc_body_context );
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

  /* Preprocess all the loops for optimized WCET calculation */
  /********CAUTION*******/
  preprocess_all_loops( proc );

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
    computeWCET_block( bb, proc, NULL );
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
void compute_bus_WCET_MSC_structural( MSC *msc, const char *tdma_bus_schedule_file )
{
  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the latest time of all cores */
  memset( latest_core_time, 0, num_core * sizeof(ull) );
  /* reset latest time of all tasks */
  reset_all_task( msc );

  totalAlignCost = 0;

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {

    PRINT_PRINTF( "Analyzing Task WCET %s......\n", msc->taskList[k].task_name );

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

    PRINT_PRINTF( "\n**************************************************************\n" );
    PRINT_PRINTF( "Latest start time of the program = %Lu cycles\n", start_time );
    PRINT_PRINTF( "Latest finish time of the task = %Lu cycles\n", task_main->running_finish_time );
    PRINT_PRINTF( "WCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    PRINT_PRINTF( "Final alignment cost in analysis = %llu (%llu%%)\n", totalAlignCost,
        totalAlignCost * 100 / task_main->running_cost );
    PRINT_PRINTF( "**************************************************************\n\n" );
  }
}
