// Include standard library headers
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "analysisDAG_common.h"
#include "analysisCache_L1.h"
#include "analysisCache_common.h"
#include "block.h"
#include "busSchedule.h"
#include "handler.h"


static uint get_hex( const char * const hex_string )
{
  DSTART( "get_hex" );

   int len,i;
   char dig;
   uint value = 0;

   len = strlen(hex_string);

   DOUT( "hex string = %s\n", hex_string);

   for(i = 0; i < len; i++)
   {
      dig = hex_string[len-i-1];

      switch(dig)
      {
        case 'a':
          value += 10 * pow(16,i);
          break;
        case 'b':
          value += 11 * pow(16,i);
          break;
        case 'c':
          value += 12 * pow(16,i);
          break;
        case 'd':
          value += 13 * pow(16,i);
          break;
        case 'e':
          value += 14 * pow(16,i);
          break;
        case 'f':
          value += 15 * pow(16,i);
          break;
        default:
          value += (dig - 48) * pow(16,i);
      };
   }

   DOUT( "hex value = %x\n", value);

   DRETURN( value );
}


/* This function returns a context id for the iterations an inner loop inside a
 * surrounding loop.
 *
 * 'lp' the loop for which a context is requested.
 * 'surroundingLoopContext' may be the context of the surrounding loop, or '0',
 *                          to denote that there is no surrounding loop.
 * 'firstInnerIteration' if this is true, then a context for the first iteration
 *                       of the inner loop is returned, else a context for the
 *                       other iterations of the inner loop is returned.
 */
uint getInnerLoopContext( const loop *lp, uint surroundingLoopContext, _Bool firstInnerIteration )
{
  const int index_bitlength = lp->level + 1;
  const int enclosing_index_bitlength = index_bitlength - 1;

  const int index_bitmask = ( 1 << ( index_bitlength - 1 ) );
  const int enclosing_index_bitmask = ( 1 << enclosing_index_bitlength ) - 1;

  // Assert that only the bits for the surrounding loop levels may be set
  assert( ( surroundingLoopContext & ( ~enclosing_index_bitmask ) ) == 0 &&
      "Invalid context for enclosing loop!" );

  const int enclosing_context_bits = surroundingLoopContext &
                                     enclosing_index_bitmask;
  return ( firstInnerIteration ? 0 : 1 ) * index_bitmask + enclosing_context_bits;
}


/* Returns the maximum loop context that can be used in this procedure. */
uint getMaximumLoopContext( const procedure * const proc )
{
  DSTART( "getMaximumLoopContext" );

  int maxLevel = -1;
  int i;
  for ( i = 0; i < proc->num_loops; i++ ) {
    maxLevel = MAX( maxLevel, proc->loops[i]->level );
  }

  DOUT( "Determined maximum loop level %d\n", maxLevel );

  if ( maxLevel >= 0 ) {
    /* Returns a number consisting of leading zeros followed by
     * exactly 'maxLevel' ones. */
    const int shift_count = maxLevel + 1;
    const uint result = ( shift_count < ( sizeof( uint ) * 8 ) )
                          ? ( 1U << shift_count ) - 1U
                          : UINT_MAX;

    DOUT( "Determined maximum loop context as %x\n", result );
    DRETURN( result );
  } else {
    /* No loops -> Only one procedure-wide context. */
    DOUT( "Determined maximum loop context as %x\n", 0 );
    DRETURN( 0 );
  }
}


/* The result type of 'isLoopExit'. */
enum LoopExitBlockType {
  LEBT_MAIN_EXIT,
  LEBT_SECONDARY_EXIT,
  LEBT_NO_EXIT
};

/* Determines whether 'bb' is the exit of any loop in 'proc'.
 * Only read stored data, does not perform computations. */
static enum LoopExitBlockType getLoopExitType( block* bb, procedure* proc )
{
  uint i;
  for ( i = 0; i < proc->num_loops; i++ ) {
    const loop * const lp = proc->loops[i];
    if ( lp->loopexit == bb ) {
      return LEBT_MAIN_EXIT;
    } else if ( getblock( bb->bbid, lp->exits, 0, lp->num_exits - 1 ) >= 0 ) {
      return LEBT_SECONDARY_EXIT;
    }
  }
  return LEBT_NO_EXIT;
}

/* Self-explanatory. */
static _Bool allPredecessorsAreLoopExits( block* bb, procedure* proc )
{
  uint i;
  for ( i = 0; i < bb->num_incoming; i++ ) {
    if ( getLoopExitType( proc->bblist[bb->incoming[i]], proc ) == LEBT_NO_EXIT ) {
      return FALSE;
    }
  }
  return TRUE;
}

/* This sets the latest starting time of a block during WCET calculation.
 * (Not context-aware) */
void set_start_time_WCET( block* bb, procedure* proc )
{
  DSTART( "set_start_time_WCET" );

  ull max_start = bb->start_time;
  const _Bool allPredsAreLoopExits = allPredecessorsAreLoopExits( bb, proc );
  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    block * const predecessor = proc->bblist[bb->incoming[i]];
    assert( predecessor && "Missing basic block!" );

    /* If all predecessors are loop exits, use only the main loop exits
     * as predecessors, because the finish time of side exits may not
     * have been updated in some cases. */
    if ( allPredsAreLoopExits && getLoopExitType( predecessor, proc ) != LEBT_MAIN_EXIT ) {
      continue;
    }

    /* Determine the predecessors' latest finish time */
    max_start = MAX( max_start, predecessor->finish_time );
  }

  /* Now set the starting time of this block to be the latest
   * finish time of predecessors block */
  bb->start_time = max_start;

  DOUT( "Setting max start of bb %d = %Lu\n", bb->bbid, max_start );
  DEND();
}

/* This sets the earliest starting time of a block during BCET calculation
 * (Not context-aware) */
void set_start_time_BCET( block* bb, procedure* proc )
{
  DSTART( "set_start_time_BCET" );

  ull min_start = 0;
  const _Bool allPredsAreLoopExits = allPredecessorsAreLoopExits( bb, proc );
  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {

    block * const predecessor = proc->bblist[bb->incoming[i]];
    assert( predecessor && "Missing basic block!" );

    /* If all predecessors are loop exits, use only the main loop exits
     * as predecessors, because the finish time of side exits may not
     * have been updated in some cases. */
    if ( allPredsAreLoopExits && getLoopExitType( predecessor, proc ) != LEBT_MAIN_EXIT ) {
      continue;
    }

    /* Determine the predecessors' earliest finish time */
    if ( i == 0 ) {
      min_start = predecessor->finish_time;
    } else {
      min_start = MIN( min_start, predecessor->finish_time );
    }
  }

  /* Now set the starting time of this block to be the earliest
   * finish time of predecessors block */
  bb->start_time = min_start;

  DOUT( "Setting min start of bb %d = %Lu\n", bb->bbid, min_start);
  DEND();
}


/* Given a MSC and a task inside it, this function computes
 * the earliest start time of the argument task. Finding out
 * the earliest start time is important as the bus aware BCET
 * analysis depends on the same */
void update_succ_task_earliest_start_time( MSC* msc, task_t* task )
{
  DSTART( "update_succ_task_earliest_start_time" );
  DOUT( "Number of Successors = %d\n", task->numSuccs);

  int i;
  for ( i = 0; i < task->numSuccs; i++ ) {

    DOUT( "Successor id with %d found\n", task->succList[i]);

    task_t * const successor = &msc->taskList[task->succList[i]];
    /* The earliest start times are initialized with 0. This will always
     * dominate the following minimum operation, though a task with a
     * predecessor may not start at time 0. Therefore we must filter this
     * case out manually in the following. */
    if ( successor->earliest_start_time != 0 ) {
      successor->earliest_start_time = MIN(
        successor->earliest_start_time,
        task->earliest_start_time + task->bcet );
    } else {
      successor->earliest_start_time = task->earliest_start_time + task->bcet;
    }

    DOUT( "Updating earliest start time of successor = %Lu\n",
        successor->earliest_start_time );
  }
  DEND();
}


/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks
 * imposed by the partial order of the MSC */
ull get_earliest_task_start_time( task_t* cur_task, uint core )
{
  DSTART( "get_earliest_task_start_time" );

  /* If independent task mode return 0 */
  if ( g_independent_task )
    DRETURN( 0 );

  /* A task in the MSC can be delayed because of two reasons. Either
   * the tasks it is dependent upon has not finished executing or
   * since we consider a non-preemptive scheduling policy the task can
   * also be delayed because of some other process' execution in the
   * same core. Thus we need to consider the maximum of the two
   * possibilities. */
  ull start = MAX( cur_task->earliest_start_time, earliest_core_time[core] );

  DOUT( "Assigning the earliest starting time of the task = %Lu\n", start);

  DRETURN( start );
}


/* Given a MSC and a task inside it, this function computes
 * the latest start time of the argument task. Finding out
 * the latest start time is important as the bus aware WCET
 * analysis depends on the same */
void update_succ_task_latest_start_time( MSC* msc, task_t* task )
{
  DSTART( "update_succ_task_latest_start_time" );
  DOUT( "Number of Successors = %d\n", task->numSuccs );

  int i;
  for ( i = 0; i < task->numSuccs; i++ ) {
    DOUT( "Successor id with %d found\n", task->succList[i] );

    task_t * const successor = &msc->taskList[task->succList[i]];
    successor->latest_start_time = MAX(
        successor->latest_start_time,
        task->latest_start_time + task->wcet );

    DOUT( "Updating latest start time of successor = %Lu\n",
        successor->latest_start_time );
  }
  DEND();
}


/* Returns the latest starting of a task in the MSC */
ull get_latest_task_start_time( task_t* cur_task, uint core )
{
  DSTART( "get_latest_task_start_time" );

  /* If independent task mode return 0 */
  if ( g_independent_task )
    DRETURN( 0 );

  /* A task in the MSC can be delayed because of two reasons. Either
   * the tasks it is dependent upon has not finished executing or
   * since we consider a non-preemptive scheduling policy the task can
   * also be delayed because of some other processe's execution in the
   * same core. Thus we need to consider the maximum of two
   * possibilities */
  ull start = MAX( cur_task->latest_start_time, latest_core_time[core] );
  DOUT( "Assigning the latest starting time of the task = %Lu\n", start );

  DRETURN( start );
}


/* Returns the BCET of a single instruction. */
ull getInstructionBCET( const instr *instruction )
{
  /* Normally this value should have been computed by the pipeline analysis
   * (for whole blocks), but we use a constant here, because we are mainly
   * interested in the shared bus analyis. */
  return 1;
}


/* Returns the WCET of a single instruction. */
ull getInstructionWCET( const instr *instruction )
{
  /* Normally this value should have been computed by the pipeline analysis
   * (for whole blocks), but we use a constant here, because we are mainly
   * interested in the shared bus analyis. */
  return 1;
}


/* Simply returns the time that is takes for the memory hierarchy to
 * perform an access with the given access classification. This does
 * not include bus delays, it only sums up the cache miss penalties
 * of the individual cache levels.
 */
static uint getAccessLatency( acc_type type )
{
  uint result = 0;
  if ( type == L1_HIT ) {
    result += cache.hit_latency;
  } else {
    result += cache.miss_latency;
    if ( type == L2_HIT ) {
      result += cache_L2.hit_latency;
    } else {
      assert( type == L2_MISS && "Unknown access type!" );
      result += cache_L2.miss_latency;
    }
  }
  return result;
}


/* Determine latency of a memory access in the presence of a shared bus
 *
 * 'bb' is the block after which the access takes place
 * 'access_time' is the precise time when the access takes place
 * 'type' specifies whether the memory access is a L1/L2 cache hit or miss
 * 'has_waited_for_next_tdma_slot' is an output variable which (if not NULL)
 *                                 will contain the information whether the
 *                                 current access had to wait for the next
 *                                 TDMA slot
 * 'accessScenario' should indicate whether we are analyzing WCET or BCET
 */
uint determine_latency( const block * const bb, const ull access_time,
    const acc_type type, _Bool * const has_waited_for_next_tdma_slot,
    enum AccessScenario accessScenario )
{
  DSTART( "determine_latency" );

  uint result = 0;

  /* Get schedule data */
  const core_sched_p core_schedule = getCoreSchedule( ncore, access_time );
  const uint slot_start = core_schedule->start_time;
  const uint slot_len = core_schedule->slot_len;
  const ull interval = core_schedule->interval;
  assert( num_core * slot_len == interval && "Inconsistent model!" );

  /* Init output parameter. */
  if ( has_waited_for_next_tdma_slot != NULL ) {
    *has_waited_for_next_tdma_slot = 0;
  }

  // If the access is a L1 hit, then the access time is constant
  if ( type == L1_HIT ) {

    result = getAccessLatency( type );

  // All other cases may suffer a variable delay
  } else {

    // Determine the offset of the access in the TDMA schedule
    const ull offset = access_time % interval;
    assert( offset < interval && "Internal error: Invalid offset!" );

    /* Return maximum if no bus is modeled. The maximum occurs when a bus request
     * arrives at (slot_end_time - request_duration - 1) which thus cannot be
     * fulfilled in the core's remaining slot time and thus must be delayed. Total
     * delay is then:
     *
     * - request_duration - 1 cycles for the first request which fails
     * - ( num_core - 1 ) * slot_len for waiting for the next free slot
     * - request_duration for issuing the request a second time in the next bus slot
     * */
    if ( g_no_bus_modeling ) {

      result = ( accessScenario == ACCESS_SCENARIO_BCET ? 0 :
                 ( num_core - 1 ) * slot_len + 2 * getAccessLatency( type ) );

    } else {

      /* Get the time needed to perform the access itself. */
      const ull simple_access_duration = getAccessLatency( type );

      /* First compute the waiting time that is needed before the successful
       * bus access can begin. */
      ull waiting_time = 0;

      /* If the access if before the core's slot begins, then wait until the
       * slot begins and do the access then. */
      if ( offset < slot_start ) {
        waiting_time = slot_start - offset;
      /* If the access fits into the current core's slot, then register this. */
      } else if ( offset <= slot_start + slot_len - simple_access_duration ) {
        waiting_time = 0;
      /* Else compute the time until the beginning of the next slot of the core
       * and add the access time itself to get the total delay. */
      } else {
        waiting_time = ( interval - offset + slot_start );
        if ( has_waited_for_next_tdma_slot != NULL ) {
          *has_waited_for_next_tdma_slot = 1;
        }
      }

      /* Then sum up the waiting and the access time to form the final delay. */
      result = waiting_time + simple_access_duration;

      /* Assert that the delay does not exceed the maximum possible delay */
      assert( result <= ( num_core - 1 ) * slot_len + 2 * getAccessLatency( L2_MISS ) &&
          "Bus delay exceeded maximum limit" );
    }
  }

  DACTION(
    char classification[10];
    switch( type ) {
      case L1_HIT:  sprintf( classification, "L1 hit" );  break;
      case L2_HIT:  sprintf( classification, "L2 hit" );  break;
      case L2_MISS: sprintf( classification, "L2 miss" ); break;
      default:      assert( 0 && "Invalid result!" );
    }
    DOUT( "Accounting latency %u for %s at time %llu\n",
        result, classification, access_time );
  );

  DRETURN( result );
}


/* Return the procedure pointer in the task data structure */
static procedure* get_task_callee(uint startaddr)
{
  int i;
  procedure* callee;
  block* f_block;

  assert(cur_task);   
   
  for( i = 0; i < cur_task->num_proc; i++ )
  {  
    callee = cur_task->proc_cg_ptr[i].proc[0]; 
    assert(callee);
    
    /* get the first block */
    f_block = callee->topo[callee->num_topo - 1];

    /* If the start address is the same then return 
     * the callee */
    if(f_block->startaddr == startaddr)
       return callee;
  }
  
  /* To make compiler happy */
  return NULL;    
}


/* Returns the callee procedure for a procedure call instruction */
procedure *getCallee( const instr * const inst, const procedure * const proc)
{
  DSTART( "getCallee" );

  int i;
  procedure* callee;
  block* f_block;
  uint addr;

  /* It cannot be an assert because some library calls
   * are ignored. This if the procedure has no callee 
   * list return then the callee must have been a library
   * function and its cost is ignored */
  /* assert(proc->calls); */
  if(!proc->calls)
     DRETURN( NULL );

  addr = get_hex(GET_CALLEE(inst));

  /* For MSC based analysis return the procedure pointer in the 
   * task */
  if(cur_task)
    DRETURN( get_task_callee(addr) );
  
  for(i = 0; i < proc->num_calls; i++)
  {  
     callee =  procs[proc->calls[i]]; 
     assert(callee);
     /* Get the first block of the callee */
     f_block = callee->topo[callee->num_topo - 1];
     /* If the start address of the first block matches
      * with the jump instruction of the callee procedure
      * then this is the callee --- in that case return 
      * the callee procedure */
     DOUT( "jump address = %x, callee address = %x\n",
        get_hex(GET_CALLEE(inst)), f_block->startaddr); 
     
     /* Check the starting address of the procedure */
     if(f_block->startaddr == addr)
     { 
       /* FIXME: If no MSC-based calculation return callee 
        * directly otherwise return the modified pointer in
        * the task */
          DRETURN( callee );
     }   
  }

  /* Must not come here */
  /* ERROR ::::: CALLED PROCEDURE NOT FOUND */
  /* FIXME: Am I right doing here ? */    
  /*  prerr("Error: Called procedure not found"); */

  /* to make compiler happy */    
  DRETURN( NULL );
}


/* Reset start and finish time of all basic blocks in this 
 * procedure */
void reset_timestamps(procedure* proc, ull start_time)
{
  int i;
  block* bb;
  
  for(i = 0; i < proc->num_bb; i++)
  {
    bb = proc->bblist[i];
    bb->start_time = bb->finish_time = start_time;
  }   
}


/* Return the type of the instruction access L1_HIT/L2_HIT/L2_MISS
 * This is computed from the shared cache analysis. The context
 * is given as a context index, see header.h:num_chmc for further
 * details.
 *
 * 'scenario' determines for which scenario (BCET/WCET) the access
 * type shall be determined. */
acc_type check_hit_miss( const block *bb, const instr *inst, 
                         uint context, enum AccessScenario scenario )
{
  DSTART( "check_hit_miss" );
  assert( bb && inst && 
          context < bb->num_chmc &&
          context < bb->num_chmc_L2 &&
          "Invalid arguments!" );

  /* If the flow is under testing mode....dont
   * bother about CHMC. Just return all miss */
  acc_type result = -1;
  if(g_testing_mode) {
    result = L2_MISS;
  } else {

    // Get the cache analysis results (CHMCs) from the block
    CHMC ** const chmc_l1 = bb->chmc;
    CHMC ** const chmc_l2 = bb->chmc_L2;
    const char *l1_result = chmc_l1[context]->hitmiss_addr;
    const char *l2_result = chmc_l2[context]->hitmiss_addr;
    // Get the index of the instruction inside the block
    const int instr_index = getinstruction( inst,
        (const instr**)bb->instrlist, 0, bb->num_instr - 1 );

    // Compute the result
    switch( scenario ) {

      case ACCESS_SCENARIO_BCET:

        // Check level 1 analysis result
        if ( !l1_result || l1_result[instr_index] != ALWAYS_MISS ) {
          result = L1_HIT;
        } else
        // Check level 2 analysis result
        if ( !l2_result || l2_result[instr_index] != ALWAYS_MISS ) {
          result = L2_HIT;
        }

        break;

      case ACCESS_SCENARIO_WCET:

        // Check level 1 analysis result
        if ( !l1_result || l1_result[instr_index] == ALWAYS_HIT ) {
          result = L1_HIT;
        } else
        // Check level 2 analysis result
        if ( !l2_result || l2_result[instr_index] == ALWAYS_HIT ) {
          result = L2_HIT;
        }

        break;

      default:
        assert( 0 && "Unknown access scenario!" );
    }

    // If no better assumption can be made, assume a L2 miss
    if ( result == -1 ) {
      result = L2_MISS;
    }
  }

  DACTION(
    char classification[10];
    switch( result ) {
      case L1_HIT:  sprintf( classification, "L1 HIT" );  break;
      case L2_HIT:  sprintf( classification, "L2 HIT" );  break;
      case L2_MISS: sprintf( classification, "L2 MISS" ); break;
      default:      assert( 0 && "Invalid result!" );
    }
    char scenario_name[5];
    switch( scenario ) {
      case ACCESS_SCENARIO_BCET: sprintf( scenario_name, "BCET" );  break;
      case ACCESS_SCENARIO_WCET: sprintf( scenario_name, "WCET" );  break;
      default:                   assert( 0 && "Invalid result!" );
    }
    DOUT( "Predicting %s for access to 0x%s (ctxt %u, %s scenario)\n",
        classification, inst->addr, context, scenario_name );
  );
  DRETURN( result );
}


/* Check whether the block specified in the header "bb"
 * is header of some loop in the procedure "proc" */
loop* check_loop( const block * const bb, const procedure * const proc )
{
  int i;

  for(i = 0; i < proc->num_loops; i++)
  {
     assert(proc->loops[i]); 
     assert(proc->loops[i]->loophead); 
     if(proc->loops[i]->loophead->bbid == bb->bbid)
      return proc->loops[i];     
  }

  return NULL;
}


/* Reset latest start time of all tasks in the MSC before 
 * the analysis of the MSC starts */
void reset_all_task(MSC* msc)
{
  int k;
  
  assert(msc);
  for(k = 0; k < msc->num_task; k++)
  {
    msc->taskList[k].earliest_start_time = 0;
    msc->taskList[k].latest_start_time = 0;
  }
}


/* Given a task this function returns the core number in which 
 * the task is assigned to. Assignment to cores to individual 
 * tasks are done statically before any analysis took place */
uint get_core(task_t* cur_task)
{
  /* FIXME: Currently returns 0 */

  return 0;
}


/* Computes the end alignment cost of a loop iteration which ends at 'fin_time'.
 * This is only needed for analyses which use the concept of aligning loops
 * to the TDMA slots to increase the analysis precision. */
ull endAlign( ull fin_time )
{
  DSTART( "endAlign" );

  const core_sched_p core_schedule = getCoreSchedule( ncore, fin_time );
  const ull interval = core_schedule->interval;

  ull align = 0;
  if ( fin_time % interval == 0 ) {
    align = 0;
  } else {
    align = ( fin_time / interval + 1 ) * interval - fin_time;
  }
  DOUT( "End align = %Lu\n", align );
  DRETURN( align );
}


/* Computes the start alignment cost of a loop which starts at 'start_time'.
 * This is only needed for analyses which use the concept of aligning loops
 * to the TDMA slots to increase the analysis precision. */
ull startAlign( ull start_time )
{
  DSTART( "startAlign" );

  const core_sched_p core_schedule = getCoreSchedule( ncore, start_time );
  const ull interval = core_schedule->interval;

  DOUT( "Start align = %u\n", interval );
  DRETURN( interval );
}
