#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_WCET_alignment.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"


// Forward declarations of static functions
static void computeWCET_loop( loop* lp, procedure* proc );
static void computeWCET_block( block* bb, procedure* proc, loop* cur_lp );
static void computeWCET_proc( procedure* proc, ull start_time );


/* This sets the latest starting time of a block during WCET calculation.
 * (Context-aware)
 *
 * Additionally this function carries over the 'latest_bus' and 'latest_latency'
 * fields from the predecessor with maximum finishing time.
 */
static void set_start_time_WCET_opt( block* bb, procedure* proc, uint context )
{
  ull max_start = bb->start_opt[context];

  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {
    int in_index = bb->incoming[i];
    assert(proc->bblist[in_index]);
    /* Determine the predecessors' latest finish time */
    if ( max_start < proc->bblist[in_index]->fin_opt[context] ) {
      /* If a bus-miss has been encountered in predecessor basic block
       * propagate the information to the current block */
      if ( proc->bblist[in_index]->latest_bus[context] )
        bb->latest_bus[context] = proc->bblist[in_index]->latest_bus[context];
      if ( proc->bblist[in_index]->latest_latency[context] )
        bb->latest_latency[context] = proc->bblist[in_index]->latest_latency[context];
      max_start = proc->bblist[in_index]->fin_opt[context];
    }
  }

  /* Now set the starting time of this block to be the latest
   * finish time of predecessors block */
  bb->start_opt[context] = max_start;

  DEBUG_PRINTF( "Setting max start of bb %d (context %u) = %Lu\n",
      bb->bbid, context, max_start );
}

/* Determine approximate memory/bus delay for a L1 cache miss
 *
 * 'bb' is the block after which the access takes place
 * 'context' is the context in which we analyze the latency
 * 'bb_cost' is the WCET of the block
 * 'type' specifies whether the bus access is a L2 cache hit or miss
 *        ( L2_HIT / L2_MISS )
 */
static uint determine_latency( block* bb, uint context, uint bb_cost, acc_type type )
{
  // The result value
  ull delay = 0;

  // The offset of the block end
  const ull offset = bb->start_opt[context] + bb_cost - bb->latest_bus[context];

  /* FIXME: Core number is hard-coded */
  const int ncore = 4;
  uint slot_len = global_sched_data->seg_list[0]->per_core_sched[0]->slot_len;
  ull interval = global_sched_data->seg_list[0]->per_core_sched[0]->interval;

  /* Return maximum if no bus is modeled */
  if ( g_no_bus_modeling ) {
    if ( type == L2_HIT )
      return ( ( ncore - 1 ) * interval + 2 * L2_HIT_LATENCY );
    else if ( type == L2_MISS )
      return ( ( ncore - 1 ) * interval + 2 * MISS_PENALTY );
  }

  /* CAUTION :::: Following code is a bit of mathematical manipulation.     
   * Hard to understand without the draft */
  if ( !bb->latest_bus[context] ) {
    bb->latest_bus[context] = bb->start_opt[context] + bb_cost;
    if ( type == L2_HIT ) {
      bb->latest_bus[context] += L2_HIT_LATENCY;
      bb->latest_latency[context] = L2_HIT_LATENCY;
      delay = ( interval - slot_len ) + L2_HIT_LATENCY;
    } else if ( type == L2_MISS ) {
      bb->latest_bus[context] += MISS_PENALTY;
      bb->latest_latency[context] = MISS_PENALTY;
      delay = ( interval - slot_len ) + MISS_PENALTY;
    }
  } else if ( ( offset < slot_len - L2_HIT_LATENCY ) && type == L2_HIT )
    delay = L2_HIT_LATENCY;
  else if ( ( offset < slot_len - MISS_PENALTY ) && type == L2_MISS )
    delay = MISS_PENALTY;
  /* Mathematics, mathematics and more mathematics */
  else {
    const uint latency = ( type == L2_HIT ) ? L2_HIT_LATENCY : MISS_PENALTY;
    const uint p_latency = ( bb->latest_latency[context] );
    const int n = ( ( offset + p_latency ) / ( ncore * slot_len ) ) + 1;

    /* Check whether can be served in an outstanding bus slot */
    if ( offset <= ( ( n - 1 ) * ncore + 1 ) * slot_len - latency - p_latency )
      delay = latency;
    /* Otherwise create a new bus slot and schedule */
    else {
      delay = ( n * ncore * slot_len ) + latency - p_latency - offset;
      bb->latest_bus[context] = bb->start_opt[context] + bb_cost + delay;
      bb->latest_latency[context] = latency;
    }
  }

  /* Exceeds maximum possible delay......print it out */
  if ( delay > slot_len * ( ncore - 1 ) + 2 * MISS_PENALTY ) {
    PRINT_PRINTF( "Bus delay exceeded maximum limit (%Lu)\n", delay );
  }
  return delay;
}

/* Computes end alignment cost of the loop */
static ull endAlign( ull fin_time )
{
  const ull interval = global_sched_data->seg_list[0]->per_core_sched[0]->interval;

  if ( fin_time % interval == 0 ) {
    DEBUG_PRINTF( "End align = 0\n" );
    return 0;
  } else {
    DEBUG_PRINTF( "End align = %Lu\n", ( fin_time / interval + 1 ) * interval - fin_time );
    return ( ( fin_time / interval + 1 ) * interval - fin_time );
  }
}

/* computes start alignment cost */
static uint startAlign()
{
  DEBUG_PRINTF( "Start align = %u\n", global_sched_data->seg_list[0]->per_core_sched[0]->interval );
  return global_sched_data->seg_list[0]->per_core_sched[0]->interval;
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
    /* initialize basic block cost */
    uint bb_cost = 0;
    memset( max_fin, 0, 64 );

    /* Traverse over all the CHMC-s of this basic block.
     * For each CHMC, a loop WCET is computed. */
    int j;
    for ( j = 0; j < bb->num_chmc; j++ ) {
      const CHMC * const cur_chmc = (CHMC *) bb->chmc[j];
      const CHMC * const cur_chmc_L2 = (CHMC *) bb->chmc_L2[j];
      /* Reset start , finish time and cost */
      bb->start_opt[j] = 0;
      bb->fin_opt[j] = 0;
      bb_cost = 0;

      /* Check whether this basic block is the header of some other 
       * loop */
      loop * const inlp = check_loop( bb, proc );
      if ( inlp && i != lp->num_topo - 1 ) {
        /* FIXME: do I need this ? */
        /* set_start_time_WCET_opt(bb, proc, j); */
        preprocess_one_loop( inlp, proc );
        bb->fin_opt[j] = bb->start_opt[j] + startAlign() + inlp->wcet_opt[2 * j] + startAlign() + ( inlp->wcet_opt[2
            * j + 1] + endAlign( inlp->wcet_opt[2 * j + 1] ) ) * inlp->loopbound;
        continue;
      }

      if ( i == lp->num_topo - 1 )
        bb->start_opt[j] = start_time;
      /* Otherwise, set the maximum of the finish time of predecesssor 
       * basic blocks */
      else
        set_start_time_WCET_opt( bb, proc, j );

      NPRINT_PRINTF( "Current CHMC = 0x%x\n", (unsigned) cur_chmc );
      NPRINT_PRINTF( "Current CHMC L2 = 0x%x\n", (unsigned) cur_chmc_L2 );

      int k;
      for ( k = 0; k < bb->num_instr; k++ ) {
        instr * const inst = bb->instrlist[k];
        /* Instruction cannot be empty */
        assert(inst);

        /* Check for a L1 miss */
        if ( cur_chmc->hitmiss_addr[k] != ALWAYS_HIT )
          NPRINT_PRINTF( "L1 miss at 0x%x\n", (unsigned) bb->startaddr );
        /* first check whether the instruction is an L1 hit or not */
        /* In that easy case no bus access is required */
        if ( cur_chmc->hitmiss_addr[k] == ALWAYS_HIT ) {
          bb_cost += L1_HIT_LATENCY;
        }
        /* Otherwise if it is an L2 hit */
        else if ( cur_chmc_L2->hitmiss_addr[k] == ALWAYS_HIT ) {
          /* access shared bus */
          uint latency = determine_latency( bb, j, bb_cost, L2_HIT );
          NPRINT_PRINTF( "Latency = %u\n", latency );
          bb_cost += latency;
        }
        /* Else it is an L2 miss */
        else {
          /* access shared bus */
          uint latency = determine_latency( bb, j, bb_cost, L2_MISS );
          NPRINT_PRINTF( "Latency = %u\n", latency );
          bb_cost += latency;
        }
        /* Handle procedure call instruction */
        if ( IS_CALL(inst->op) ) {
          procedure* callee = getCallee( inst, proc );

          /* For ignoring library calls */
          if ( callee ) {
            /* Compute the WCET of the callee procedure here.
             * We dont handle recursive procedure call chain
             */
            computeWCET_proc( callee, bb->start_opt[j] + bb_cost );
            /* Single cost for call instruction */
            bb_cost += callee->running_cost;
          }
        }
      }

      /* Set finish time of the basic block */
      bb->fin_opt[j] = bb->start_opt[j] + bb_cost;
      /* Set max finish time */
      if ( max_fin[j] < bb->fin_opt[j] )
        max_fin[j] = bb->fin_opt[j];
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

  /* Check whether the block is some header of a loop structure.
   * In that case do separate analysis of the loop */
  /* Exception is when we are currently in the process of analyzing
   * the same loop */
  loop* inlp = check_loop( bb, proc );
  if ( inlp && ( !cur_lp || ( inlp->lpid != cur_lp->lpid ) ) ) {

    bb->finish_time = bb->start_time + startAlign() + inlp->wcet_opt[0] + startAlign() + ( inlp->wcet_opt[1]
        + endAlign( inlp->wcet_opt[1] ) ) * inlp->loopbound;

  /* It's not a loop. Go through all the instructions and
   * compute the WCET of the block */
  } else {
    uint acc_cost = 0;

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
          computeWCET_proc( callee, bb->start_time + acc_cost );
          /* Single cost for call instruction */
          acc_cost += ( callee->running_cost + 1 );
        }
      }
      /* No procedure call ---- normal instruction */
      else {
        all_inst++;
        /* If its a L1 hit add only L1 cache latency */
        acc_type acc_t;
        if ( ( acc_t = check_hit_miss( bb, inst ) ) == L1_HIT )
          acc_cost += ( L1_HIT_LATENCY );
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
  DEBUG_PRINTF( "Setting block %d finish time = %Lu\n", bb->bbid, bb->finish_time );
}

static void computeWCET_proc( procedure* proc, ull start_time )
{
  /* Initialize current context. Set to zero before the start 
   * of each new procedure */
  cur_context = 0;

  /* Preprocess CHMC classification for each instruction inside
   * the procedure */
  preprocess_chmc_WCET( proc );

  /* Preprocess CHMC classification for L2 cache for each 
   * instruction inside the procedure */
  preprocess_chmc_L2_WCET( proc );

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

/* This is the entry point for the non-MSC-aware version of the DAG-based analysis. The function
 * does not consider the mscs, it just searches the list of known functions for the 'main' function
 * and starts the analysis there.
 */
void computeWCET_alignment( ull start_time )
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
  computeWCET_proc( procs[top_func], start_time );

  PRINT_PRINTF( "\n\n**************************************************************\n" );
  PRINT_PRINTF( "Latest start time of the program = %Lu start_time\n", start_time );
  PRINT_PRINTF( "Latest finish time of the program = %Lu cycles\n", procs[top_func]->running_finish_time );
  PRINT_PRINTF( "WCET of the program %s shared bus = %Lu cycles\n",
      g_shared_bus ? "with" : "without", procs[top_func]->running_cost );
  PRINT_PRINTF( "**************************************************************\n\n" );
}

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC_alignment( MSC *msc, const char *tdma_bus_schedule_file )
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

    PRINT_PRINTF( "Analyzing Task WCET %s......\n", msc->taskList[k].task_name );

    /* Get needed inputs. */
    all_inst = 0;
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

    PRINT_PRINTF( "\n\n**************************************************************\n" );
    PRINT_PRINTF( "Latest start time of the program = %Lu start_time\n", start_time );
    PRINT_PRINTF( "Latest finish time of the task = %Lu cycles\n", task_main->running_finish_time );
    PRINT_PRINTF( "WCET of the task %s shared bus = %Lu cycles\n",
        g_shared_bus ? "with" : "without", task_main->running_cost );
    PRINT_PRINTF( "**************************************************************\n\n" );
  }
}
