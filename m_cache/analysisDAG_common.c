#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_common.h"
#include "busSchedule.h"


/* Compute waiting time for a memory request and a given 
 * deterministic TDMA schedule */
static uint compute_waiting_time(core_sched_p head_core, ull start_time, acc_type type)
{
  long long delta, check_len, check_len_p;
  uint delay_elem;
  uint latency;

  if(type == L2_HIT)
    latency = L2_HIT_LATENCY;
  else if(type == L2_MISS)
    latency = MISS_PENALTY;  

  assert(head_core);

  delta = start_time - head_core->start_time;

  /* sudiptac :::: It's a bit mathematical. Feel free to consult the draft 
   * instead of breaking your head here */
  
  if(delta < 0)
    delay_elem = abs(delta);
  else
  {
    check_len_p = (delta - ((delta/head_core->interval) * head_core->interval));   
    if(check_len_p < head_core->slot_len)  
      delay_elem = 0;
    else
    {
      check_len = (((delta / head_core->interval) + 1) * head_core->interval);
      delay_elem = check_len - delta; 
    } 
  }
  
  return delay_elem;
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

#ifdef _NDEBUG
   fprintf(stdout, "hex string = %s\n", hex_string);
#endif

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

#ifdef _NDEBUG
   fprintf(stdout, "hex value = %x\n", value);
#endif

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
#ifdef _DEBUG
     fprintf(stdout, "jump address = %x, callee address = %x\n", 
        get_hex(GET_CALLEE(inst)), f_block->startaddr); 
#endif
     
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
 * This is computed from the shared cache analysis */
acc_type check_hit_miss(block* bb, instr* inst)
{
  /* If the flow is under testing mode....dont 
   * bother about CHMC. Just return all miss */
  if(g_testing_mode)
    return L2_MISS;

  /* FIXME: If no chmc return miss */
  if(!inst->acc_t)    
    return L2_MISS;

  if(inst->acc_t[cur_context] == L1_HIT)
    return L1_HIT;
  
  if(!inst->acc_t_l2)   
    return L2_MISS;
  if(inst->acc_t_l2[cur_context] == L2_HIT)
  {
#ifdef _DEBUG
    fprintf(stdout, "Returning L2 HIT\n"); 
#endif
    return L2_HIT;  
  } 

  return L2_MISS;   
}


/* Given a starting time and a particular core, this function 
 * calculates the variable memory latency for the request */
int compute_bus_delay(ull start_time, uint ncore, acc_type type)
{
  sched_p glob_sched = NULL;
  segment_p cur_seg = NULL; 
  core_sched_p cur_core_sched;
  uint add_delay = 0;

  /* The global TDMA schedule must have been set here */         
  glob_sched = getSchedule();
  assert(glob_sched);
  assert(ncore < glob_sched->n_cores);

  /* Find the proper segment for start time in case there are 
   * multiple segments present in the full bus schedule */  
  if(glob_sched->type != SCHED_TYPE_1)
  {
     cur_seg = find_segment(glob_sched->seg_list, glob_sched->n_segments,
        start_time); 
  }
  else
     cur_seg = glob_sched->seg_list[0]; 

  assert(cur_seg->per_core_sched);    
  assert(cur_seg->per_core_sched[ncore]);

  cur_core_sched = cur_seg->per_core_sched[ncore];

  add_delay = compute_waiting_time(cur_core_sched, start_time, type);

  /* Print not-desired bus related delay */   
#ifdef _NPRINT
   if(add_delay < 0 || add_delay > 50)
        fprintf(stdout, "Request time = %Lu and Waiting time = %d\n", 
           start_time, add_delay);    
#endif 
  acc_bus_delay += add_delay;

  if(!g_no_bus_modeling)    
  {
    if(type == L2_HIT)  
      return (uint)(add_delay + L2_HIT_LATENCY);
    else if(type == L2_MISS)
      return (uint)(add_delay + MISS_PENALTY);
  }

  return (uint)(MISS_PENALTY + MAX_BUS_DELAY);  
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
