#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_common.h"
#include "busSchedule.h"


/* Attach best-case hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2_BCET( instr* inst, CHMC** chmc_l2, int n_chmc_l2, int inst_id )
{
  if ( !n_chmc_l2 )
    return;

  /* Allocate memory here */
  if ( !inst->acc_t_l2 )
    inst->acc_t_l2 = (acc_type *) malloc( n_chmc_l2 * sizeof(acc_type) );
  if ( !inst->acc_t_l2 )
    prerr( "Error: Out of memory" );

  memset( inst->acc_t_l2, 0, n_chmc_l2 * sizeof(acc_type) );

  int i;
  for ( i = 0; i < n_chmc_l2; i++ ) {
    assert(chmc_l2[i]);

    if ( chmc_l2[i]->hitmiss_addr[inst_id] != ALWAYS_MISS &&
         inst->acc_t[i] != L1_HIT ) {
      inst->acc_t_l2[i] = L2_HIT;
    }
  }
}

/* Attach best-case hit/miss classification to the instruction */
static void classify_inst_BCET( instr* inst, CHMC** chmc, int n_chmc, int inst_id )
{
  if ( !n_chmc )
    return;

  /* Allocate memory here */
  if ( !inst->acc_t )
    inst->acc_t = (acc_type *) malloc( n_chmc * sizeof(acc_type) );
  if ( !inst->acc_t )
    prerr( "Error: Out of memory" );

  memset( inst->acc_t, 0, n_chmc * sizeof(acc_type) );

  int i;
  for ( i = 0; i < n_chmc; i++ ) {
    assert(chmc[i]);

    if ( chmc[i]->hitmiss_addr[inst_id] != ALWAYS_MISS ) {
      inst->acc_t[i] = L1_HIT;
    }
  }
}

/* Attach best-case chmc classification for L2 cache to the instruction
 * data structure */
void preprocess_chmc_L2_BCET( procedure* proc )
{
  int i;
  for ( i = 0; i < proc->num_bb; i++ ) {
    block* bb = proc->bblist[i];

    int j;
    for ( j = 0; j < bb->num_instr; j++ ) {
      instr* inst = bb->instrlist[j];
      classify_inst_L2_BCET( inst, bb->chmc_L2, bb->num_chmc_L2, j );
    }
  }
}

/* Attach best-case chmc classification to the instruction data structure */
void preprocess_chmc_BCET( procedure* proc )
{
  int i;
  for ( i = 0; i < proc->num_bb; i++ ) {
    block* bb = proc->bblist[i];

    int j;
    for ( j = 0; j < bb->num_instr; j++ ) {
      instr* inst = bb->instrlist[j];
      classify_inst_BCET( inst, bb->chmc, bb->num_chmc, j );
    }
  }
}


/* Attach worst-case hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2_WCET( instr* inst, CHMC** chmc_l2, int n_chmc_l2, int inst_id )
{
  if ( !n_chmc_l2 )
    return;

  /* Allocate memory here */
  if ( !inst->acc_t_l2 )
    inst->acc_t_l2 = (acc_type *) malloc( n_chmc_l2 * sizeof(acc_type) );
  if ( !inst->acc_t_l2 )
    prerr( "Error: Out of memory" );

  /* FIXME: Default value is L2 miss */
  memset( inst->acc_t_l2, 0, n_chmc_l2 * sizeof(acc_type) );

  int i;
  for ( i = 0; i < n_chmc_l2; i++ ) {
    assert(chmc_l2[i]);

    if ( !chmc_l2[i]->hitmiss_addr )
      continue;

    if ( ( chmc_l2[i]->hitmiss_addr[inst_id] == ALWAYS_HIT ) && inst->acc_t[i] != L1_HIT ) {
      inst->acc_t_l2[i] = L2_HIT;
    }
  }
}

/* Attach worst-case hit/miss classification to the instruction */
static void classify_inst_WCET( instr* inst, CHMC** chmc, int n_chmc, int inst_id )
{
  if ( !n_chmc )
    return;

  /* Allocate memory here */
  if ( !inst->acc_t )
    inst->acc_t = (acc_type *) malloc( n_chmc * sizeof(acc_type) );
  if ( !inst->acc_t )
    prerr( "Error: Out of memory" );

  /* FIXME: Default value is L2 miss */
  memset( inst->acc_t, 0, n_chmc * sizeof(acc_type) );

  int i;
  for ( i = 0; i < n_chmc; i++ ) {
    assert(chmc[i]);

    /* FIXME: I think this is possible for a buggy implementation
     * in cache analysis */
    if ( !chmc[i]->hitmiss_addr ) {
      continue;
    }

    if ( chmc[i]->hitmiss_addr[inst_id] == ALWAYS_HIT ) {
      inst->acc_t[i] = L1_HIT;
    }
  }
}

/* Attach worst-case chmc classification for L2 cache to the instruction
 * data structure */
void preprocess_chmc_L2_WCET( procedure* proc )
{
  int i;
  for ( i = 0; i < proc->num_bb; i++ ) {
    block* bb = proc->bblist[i];

    int j;
    for ( j = 0; j < bb->num_instr; j++ ) {
      instr* inst = bb->instrlist[j];
      classify_inst_L2_WCET( inst, bb->chmc_L2, bb->num_chmc_L2, j );
    }
  }
}

/* Attach worst-case chmc classification to the instruction data structure */
void preprocess_chmc_WCET( procedure* proc )
{
  int i;
  for ( i = 0; i < proc->num_bb; i++ ) {
    block *bb = proc->bblist[i];

    int j;
    for ( j = 0; j < bb->num_instr; j++ ) {
      instr* inst = bb->instrlist[j];
      classify_inst_WCET( inst, bb->chmc, bb->num_chmc, j );
    }
  }
}


/* This sets the latest starting time of a block during WCET calculation.
 * (Not context-aware) */
void set_start_time_WCET( block* bb, procedure* proc )
{
  ull max_start = bb->start_time;

  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {
    int in_index = bb->incoming[i];
    assert(proc->bblist[in_index]);
    /* Determine the predecessors' latest finish time */
    if ( max_start < proc->bblist[in_index]->finish_time )
      max_start = proc->bblist[in_index]->finish_time;
  }

  /* Now set the starting time of this block to be the latest
   * finish time of predecessors block */
  bb->start_time = max_start;

  DEBUG_PRINTF( "Setting max start of bb %d = %Lu\n", bb->bbid, max_start );
}

/* This sets the earliest starting time of a block during BCET calculation
 * (Not context-aware) */
void set_start_time_BCET( block* bb, procedure* proc )
{
  ull min_start = 0;

  assert(bb);

  int i;
  for ( i = 0; i < bb->num_incoming; i++ ) {
    int in_index = bb->incoming[i];
    assert(proc->bblist[in_index]);
    if ( i == 0 )
      min_start = proc->bblist[in_index]->finish_time;
    /* Determine the predecessors' latest finish time */
    else if ( min_start > proc->bblist[in_index]->finish_time )
      min_start = proc->bblist[in_index]->finish_time;
  }

  /* Now set the starting time of this block to be the earliest
   * finish time of predecessors block */
  bb->start_time = min_start;

  DEBUG_PRINTF( "Setting min start of bb %d = %Lu\n", bb->bbid, min_start);
}


/* Given a MSC and a task inside it, this function computes
 * the earliest start time of the argument task. Finding out
 * the earliest start time is important as the bus aware BCET
 * analysis depends on the same */
void update_succ_task_earliest_start_time( MSC* msc, task_t* task )
{
  DEBUG_PRINTF( "Number of Successors = %d\n", task->numSuccs);

  int i;
  for ( i = 0; i < task->numSuccs; i++ ) {

    DEBUG_PRINTF( "Successor id with %d found\n", task->succList[i]);

    uint sid = task->succList[i];
    msc->taskList[sid].l_start = task->l_start + task->bcet;

    DEBUG_PRINTF( "Updating earliest start time of successor = %Lu\n",
        msc->taskList[sid].l_start);
  }
}

/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks
 * imposed by the partial order of the MSC */
ull get_earliest_task_start_time( task_t* cur_task, uint core )
{
  /* A task in the MSC can be delayed because of two reasons. Either
   * the tasks it is dependent upon has not finished executing or
   * since we consider a non-preemptive scheduling policy the task can
   * also be delayed because of some other processe's execution in the
   * same core. Thus we need to consider the minimum of two
   * possibilities */
  ull start = MIN( cur_task->l_start, latest_core_time[core] );

  DEBUG_PRINTF( "Assigning the latest starting time of the task = %Lu\n", start);

  return start;
}


/* Given a MSC and a task inside it, this function computes
 * the latest start time of the argument task. Finding out
 * the latest start time is important as the bus aware WCET
 * analysis depends on the same */
void update_succ_task_latest_start_time( MSC* msc, task_t* task )
{
  DEBUG_PRINTF( "Number of Successors = %d\n", task->numSuccs );

  int i;
  for ( i = 0; i < task->numSuccs; i++ ) {
    DEBUG_PRINTF( "Successor id with %d found\n", task->succList[i] );
    uint sid = task->succList[i];
    if ( msc->taskList[sid].l_start < ( task->l_start + task->wcet ) )
      msc->taskList[sid].l_start = task->l_start + task->wcet;
    DEBUG_PRINTF( "Updating latest start time of successor = %Lu\n", msc->taskList[sid].l_start );
  }
}

/* Returns the latest starting of a task in the MSC */
ull get_latest_task_start_time( task_t* cur_task, uint core )
{
  /* If independent task mode return 0 */
  if ( g_independent_task )
    return 0;

  /* A task in the MSC can be delayed because of two reasons. Either
   * the tasks it is dependent upon has not finished executing or
   * since we consider a non-preemptive scheduling policy the task can
   * also be delayed because of some other processe's execution in the
   * same core. Thus we need to consider the maximum of two
   * possibilities */
  ull start = MAX( cur_task->l_start, latest_core_time[core] );
  DEBUG_PRINTF( "Assigning the latest starting time of the task = %Lu\n", start );

  return start;
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

/* Determine latency of a memory access in the presence of a shared bus
 *
 * 'bb' is the block after which the access takes place
 * 'access_time' is the precise time when the access takes place
 * 'type' specifies whether the bus access is a L2 cache hit or miss
 *        ( L2_HIT / L2_MISS )
 */
uint determine_latency( block* bb, ull access_time, acc_type type )
{
  // If the access is a L1 hit, then the access time is constant
  if ( type == L1_HIT ) {
    return L1_HIT_LATENCY;

  // All other cases may suffer a variable delay
  } else {

    /* Get schedule data */
    const core_sched_p core_schedule = getCoreSchedule( ncore, access_time );
    const uint slot_start = core_schedule->start_time;
    const uint slot_len = core_schedule->slot_len;
    const ull interval = core_schedule->interval;
    assert( num_core * slot_len == interval && "Inconsistent model!" );

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
      return ( num_core - 1 ) * slot_len +
        2 * ( type == L2_HIT ? L2_HIT_LATENCY : MISS_PENALTY );
    }

    /* Get the time needed to perform the access itself. */
    const ull simple_access_duration = ( type == L2_HIT ? L2_HIT_LATENCY :
                                                          MISS_PENALTY );

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
    }

    /* Then sum up the waiting and the access time to form the final delay. */
    const ull delay = waiting_time + simple_access_duration;

    /* Assert that the delay does not exceed the maximum possible delay */
    assert( delay <= ( num_core - 1 ) * slot_len + 2 * MISS_PENALTY &&
        "Bus delay exceeded maximum limit" );

    return delay;
  }
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


uint get_hex(char* hex_string)
{
   int len,i;
   char dig;
   uint value = 0;

    len = strlen(hex_string);

   NDEBUG_PRINTF( "hex string = %s\n", hex_string);

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

   NDEBUG_PRINTF( "hex value = %x\n", value);

   return value;
}

/* Returns the callee procedure for a procedure call instruction */
procedure* getCallee(instr* inst, procedure* proc)
{
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
     return NULL;

  addr = get_hex(GET_CALLEE(inst));

  /* For MSC based analysis return the procedure pointer in the 
   * task */
  if(cur_task)
    return get_task_callee(addr);
  
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
     DEBUG_PRINTF( "jump address = %x, callee address = %x\n", 
        get_hex(GET_CALLEE(inst)), f_block->startaddr); 
     
     /* Check the starting address of the procedure */
     if(f_block->startaddr == addr)
     { 
       /* FIXME: If no MSC-based calculation return callee 
        * directly otherwise return the modified pointer in
        * the task */
          return callee;
     }   
  }

  /* Must not come here */
  /* ERROR ::::: CALLED PROCEDURE NOT FOUND */
  /* FIXME: Am I right doing here ? */    
  /*  prerr("Error: Called procedure not found"); */

  /* to make compiler happy */    
  return NULL;
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


/* Return the type of the instruction access MISS/L1_HIT/L2_HIT.
 * This is computed from the shared cache analysis. The context
 * is given as a context index, see header.h:num_chmc for further
 * details. */
acc_type check_hit_miss(block* bb, instr* inst,uint context)
{
  /* If the flow is under testing mode....dont 
   * bother about CHMC. Just return all miss */
  if(g_testing_mode)
    return L2_MISS;

  /* FIXME: If no chmc return miss */
  if(!inst->acc_t)    
    return L2_MISS;

  if(inst->acc_t[context] == L1_HIT)
    return L1_HIT;
  
  if(!inst->acc_t_l2)   
    return L2_MISS;
  if(inst->acc_t_l2[context] == L2_HIT)
  {
    DEBUG_PRINTF( "Returning L2 HIT\n"); 
    return L2_HIT;  
  } 

  return L2_MISS;   
}


/* Check whether the block specified in the header "bb"
 * is header of some loop in the procedure "proc" */
loop* check_loop(block* bb, procedure* proc)
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
    msc->taskList[k].l_start = 0;  
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
