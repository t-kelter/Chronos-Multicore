#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "analysisDAG_BCET_unroll.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"


// Forward declarations of static functions
static void computeBCET_loop( loop* lp, procedure* proc );
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp );
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
static void computeBCET_loop( loop* lp, procedure* proc )
{
  DEBUG_PRINTF( "Visiting loop = (%d.%lx)\n", lp->lpid, (uintptr_t)lp);

  const int lpbound = lp->loopbound;

  /* For computing BCET of the loop it must be visited 
   * multiple times equal to the loop bound */
  int i;
  for ( i = 0; i < lpbound; i++ ) {
    /* CAUTION: Update the current context */
    if ( i == 0 )
      cur_context *= 2;
    else if ( i == 1 )
      cur_context = cur_context + 1;

    /* Go through the blocks in topological order */
    int j;
    for ( j = lp->num_topo - 1; j >= 0; j-- ) {
      block* bb = lp->topo[j];
      assert(bb);
      /* Set the start time of this block in the loop */
      /* If this is the first iteration and loop header
       * set the start time to be the earliest finish time
       * of predecessor otherwise latest finish time of
       * loop sink */
      if ( bb->bbid == lp->loophead->bbid && i == 0 )
        set_start_time_BCET( bb, proc );
      else if ( bb->bbid == lp->loophead->bbid ) {
        assert(lp->loopsink);
        bb->start_time = ( bb->start_time < lp->loopsink->finish_time ) ? lp->loopsink->finish_time : bb->start_time;
        DEBUG_PRINTF( "Setting loop %d finish time = %Lu\n", lp->lpid,
            lp->loopsink->finish_time);
      } else
        set_start_time_BCET( bb, proc );
      computeBCET_block( bb, proc, lp );
    }
  }
  /* CAUTION: Update the current context */
  cur_context /= 2;
}

/* Compute worst case finish time and cost of a block */
static void computeBCET_block( block* bb, procedure* proc, loop* cur_lp )
{
  uint acc_cost = 0;

  DEBUG_PRINTF( "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t)bb);

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop * const inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    computeBCET_loop( inlp, proc );

  /* Its not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {
    int i;
    for ( i = 0; i < bb->num_instr; i++ ) {
      instr* inst = bb->instrlist[i];
      assert(inst);

      /* Handle procedure call instruction */
      if ( IS_CALL(inst->op) ) {
        procedure* callee = getCallee( inst, proc );

        /* For ignoring library calls */
        if ( callee ) {
          /* Compute the WCET of the callee procedure here.
           * We dont handle recursive procedure call chain
           */
          computeBCET_proc( callee, bb->start_time + acc_cost );
          acc_cost += callee->running_cost;
        }
      }
      /* No procedure call ---- normal instruction */
      else {
        /* If its a L1 hit add only L1 cache latency */
        acc_type acc_t = check_hit_miss( bb, inst, cur_context );
        if ( acc_t == L1_HIT )
          acc_cost += L1_HIT_LATENCY;
        /* If its a L1 miss and L2 hit add only L2 cache
         * latency */
        else if ( acc_t == L2_HIT ) {
          if ( g_shared_bus )
            acc_cost += compute_bus_delay( bb->start_time + acc_cost, ncore, L2_HIT );
          else
            acc_cost += ( L2_HIT_LATENCY + 1 );
        } else {
          /* Otherwise the instruction must be fetched from memory.
           * Since the request will go through a shared bus, we have
           * the amount of delay is not constant and depends on the
           * start time of the request. This is computed by the
           * compute_bus_delay function (bus delay + memory latency)
           *---ncore representing the core in which the program is
           * being executed */
          if ( g_shared_bus )
            acc_cost += compute_bus_delay( bb->start_time + acc_cost, ncore, L2_MISS );
          else
            acc_cost += ( MISS_PENALTY + 1 );
        }
      }
    }
    /* The accumulated cost is computed. Now set the latest finish
     * time of this block */
    bb->finish_time = bb->start_time + acc_cost;
  }
  DEBUG_PRINTF( "Setting block %d finish time = %Lu\n", bb->bbid,
      bb->finish_time);
}

static void computeBCET_proc( procedure* proc, ull start_time )
{
  /* Initialize current context. Set to zero before the start 
   * of each new procedure */
  cur_context = 0;

  /* Preprocess CHMC classification for each instruction inside
   * the procedure */
  preprocess_chmc_BCET( proc );

  /* Preprocess CHMC classification for L2 cache for each 
   * instruction inside the procedure */
  preprocess_chmc_L2_BCET( proc );

  /* Reset all timing information */
  reset_timestamps( proc, start_time );

#ifdef _DEBUG
  dump_pre_proc_chmc(proc);
#endif

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

  DEBUG_PRINTF( "Set best case cost of the procedure %d = %Lu\n",
      proc->pid, proc->running_cost);
}


/* This is the entry point for the non-MSC-aware version of the DAG-based analysis. The function
 * does not consider the mscs, it just searches the list of known functions for the 'main' function
 * and starts the analysis there.
 */
void computeBCET_unroll( ull start_time )
{
  acc_bus_delay = 0;
  cur_task = NULL;

  /* Send the pointer to the "main" to compute the WCET of the
   * whole program */
  assert(proc_cg);
  int id = num_procs - 1;

  int top_func = -1;
  while ( id >= 0 ) {
    top_func = proc_cg[id];
    /* Ignore all un-intended library calls like "printf" */
    if ( top_func >= 0 && top_func <= num_procs - 1 )
      break;
    id--;
  }
  computeBCET_proc( procs[top_func], start_time );

  PRINT_PRINTF( "\n\n**************************************************************\n" );
  PRINT_PRINTF( "Earliest start time of the program = %Lu start_time\n", start_time );
  PRINT_PRINTF( "Earliest finish time of the program = %Lu cycles\n",
      procs[top_func]->running_finish_time );
  PRINT_PRINTF( "BCET of the program %s shared bus = %Lu cycles\n",
      g_shared_bus ? "with" : "without", procs[top_func]->running_cost );
  PRINT_PRINTF( "**************************************************************\n\n" );
}

/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC_unroll( MSC *msc, const char *tdma_bus_schedule_file )
{
  /* Set the global TDMA bus schedule */
  setSchedule( tdma_bus_schedule_file );

  /* Reset the latest time of all cores */
  memset( latest_core_time, 0, num_core * sizeof(ull) );
  /* reset latest time of all tasks */
  reset_all_task( msc );

  int k;
  for ( k = 0; k < msc->num_task; k++ ) {
    acc_bus_delay = 0;

    NPRINT_PRINTF( "Analyzing Task BCET %s......\n", msc->taskList[k].task_name);

    /* Get needed inputs. */
    cur_task = &( msc->taskList[k] );
    ncore = get_core( cur_task );
    procedure * const task_main = msc->taskList[k].main_copy;

    /* First get the earliest start time of the current task. */
    ull start_time = get_earliest_start_time( cur_task, ncore );

    /* Then compute and set the worst case cost of this task */
    computeBCET_proc( task_main, start_time );
    msc->taskList[k].bcet = msc->taskList[k].main_copy->running_cost;

    /* Now update the latest starting time in this core */
    latest_core_time[ncore] = start_time + msc->taskList[k].bcet;

    /* Since the interference file for a MSC was dumped in topological 
     * order and read back in the same order we are assured of the fact
     * that we analyze the tasks inside a MSC only after all of its
     * predecessors have been analyzed. Thus After analyzing one task
     * update all its successor tasks' latest time */
    update_succ_earliest_time( msc, cur_task );

    PRINT_PRINTF("\n\n**************************************************************\n");
    PRINT_PRINTF("Earliest start time of the task = %Lu start_time\n", start_time);
    PRINT_PRINTF("Earliest finish time of the task = %Lu cycles\n",
        task_main->running_finish_time);
    PRINT_PRINTF( "BCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    PRINT_PRINTF("**************************************************************\n\n");
  }
  /* DONE :: BCET computation of the MSC */
}
