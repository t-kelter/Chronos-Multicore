#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "analysisDAG_BCET.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"

// Forward declarations of static functions

static void computeBCET_loop(loop* lp, procedure* proc);

static void computeBCET_block(block* bb, procedure* proc, loop* cur_lp);

static void computeBCET_proc(procedure* proc, ull start_time);

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

/* Attach hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2_BCET(instr* inst, CHMC** chmc_l2, int n_chmc_l2,
      int inst_id)
{
  int i;

  if(!n_chmc_l2)  
    return;

  /* Allocate memory here */
  if(!inst->acc_t_l2)
    inst->acc_t_l2 = (acc_type *)malloc(n_chmc_l2 * sizeof(acc_type));
  if(!inst->acc_t_l2)
    prerr("Error: Out of memory");  
  /* FIXME: Default value is L2 miss */ 
  memset(inst->acc_t_l2, 0, n_chmc_l2 * sizeof(acc_type));  
  
  for(i = 0; i < n_chmc_l2; i++)
  {
    assert(chmc_l2[i]);
    //int j;
    //for(j = 0; j < chmc_l2[i]->hit; j++)
    //{
      /* If it is not L1 hit then change it to L2 hit */
      if((chmc_l2[i]->hitmiss_addr[inst_id] != ALWAYS_MISS) 
           && inst->acc_t[i] != L1_HIT)
      {
        inst->acc_t_l2[i] = L2_HIT;  
      }
    //}
    /* For BCET we need to change the unknown also to L2 hit */
    //uint addr = get_hex(inst->addr);
    //for(j = 0; j < chmc_l2[i]->unknow; j++)
    //{
      //if(!chmc_l2[i]->unknow_addr)
      //  break;   
      /* If it is not L1 hit then change it to L2 hit */
      //if((chmc_l2[i]->unknow_addr[j] == addr) && inst->acc_t[i] != L1_HIT)
      //{
       // inst->acc_t_l2[i] = L2_HIT;  
      //}
    //}
  }
}

/* Attach hit/miss classification to the instruction */
static void classify_inst_BCET(instr* inst, CHMC** chmc, int n_chmc, int inst_id)
{
  int i;

  if(!n_chmc) 
    return;

  /* Allocate memory here */
  if(!inst->acc_t)
    inst->acc_t = (acc_type *)malloc(n_chmc * sizeof(acc_type));
  if(!inst->acc_t)
    prerr("Error: Out of memory");  
  /* FIXME: Default value is L2 miss */ 
  memset(inst->acc_t, 0, n_chmc * sizeof(acc_type));  
  
  for(i = 0; i < n_chmc; i++)
  {
    assert(chmc[i]);
    //int j;
    //for(j = 0; j < chmc[i]->hit; j++)
    //{
      if(chmc[i]->hitmiss_addr[inst_id] != ALWAYS_MISS)
      {
        inst->acc_t[i] = L1_HIT;   
      }
    //}  
    /* For BCET we need to change the unknown also to L1 hit */
    //uint addr = get_hex(inst->addr);
    //for(j = 0; j < chmc[i]->unknow; j++)
    //{
      /* If it is not L1 hit then change it to L2 hit */
      //if(chmc[i]->unknow_addr[j] == addr)
      //{
       // inst->acc_t[i] = L1_HIT;   
      //}
    //}
  }
}

/* Attach chmc classification for L2 cache to the instruction 
 * data structure */
static void preprocess_chmc_L2_BCET(procedure* proc)
{
  int i,j;
  block* bb;
  instr* inst;

  for(i = 0; i < proc->num_bb; i++)
  {
    bb = proc->bblist[i];     
    
    for(j = 0; j < bb->num_instr; j++)
    {
      inst = bb->instrlist[j];
      classify_inst_L2_BCET(inst, bb->chmc_L2, bb->num_chmc_L2, j);    
    }
  }
}

/* Attach chmc classification to the instruction data structure */
static void preprocess_chmc_BCET(procedure* proc)
{
  int i,j;
  block* bb;
  instr* inst;

  for(i = 0; i < proc->num_bb; i++)
  {
    bb = proc->bblist[i];     
    
    for(j = 0; j < bb->num_instr; j++)
    {
      inst = bb->instrlist[j];
      classify_inst_BCET(inst, bb->chmc, bb->num_chmc, j);     
    }
  }
}

/* This sets the latest starting time of a block
 * during BCET calculation */
static void set_start_time_BCET(block* bb, procedure* proc)
{
  int i, in_index;
  ull min_start;
  /* ull max_start = 0; */

  assert(bb);

  for(i = 0; i < bb->num_incoming; i++)
  {
     in_index = bb->incoming[i];
     assert(proc->bblist[in_index]);
     if(i == 0)
      min_start = proc->bblist[in_index]->finish_time;     
     /* Determine the predecessors' latest finish time */
     else if(min_start > proc->bblist[in_index]->finish_time)
      min_start = proc->bblist[in_index]->finish_time;     
  }

  /* Now set the starting time of this block to be the earliest 
   * finish time of predecessors block */
  bb->start_time = min_start;

  DEBUG_PRINTF( "Setting min start of bb %d = %Lu\n", bb->bbid, min_start);
}

/* Computes the latest finish time and worst case cost
 * of a loop */
static void computeBCET_loop(loop* lp, procedure* proc)
{
  int i, lpbound;
  int j;
  block* bb;

  DEBUG_PRINTF( "Visiting loop = (%d.%lx)\n", lp->lpid, (uintptr_t)lp);

  lpbound = lp->loopbound;
  
  /* For computing BCET of the loop it must be visited 
   * multiple times equal to the loop bound */
  for(i = 0; i < lpbound; i++)
  {
     /* CAUTION: Update the current context */ 
     if(i == 0)
       cur_context *= 2;
     else if (i == 1)  
       cur_context = cur_context + 1;
       
     /* Go through the blocks in topological order */
     for(j = lp->num_topo - 1; j >= 0; j--)
     {
       bb = lp->topo[j];
       assert(bb);
       /* Set the start time of this block in the loop */
       /* If this is the first iteration and loop header
        * set the start time to be the earliest finish time 
        * of predecessor otherwise latest finish time of 
        * loop sink */
       if(bb->bbid == lp->loophead->bbid && i == 0)
        set_start_time_BCET(bb, proc);     
       else if(bb->bbid == lp->loophead->bbid)
       {
        assert(lp->loopsink);  
        bb->start_time = (bb->start_time < lp->loopsink->finish_time) 
                    ? lp->loopsink->finish_time
                    : bb->start_time;
        DEBUG_PRINTF( "Setting loop %d finish time = %Lu\n", lp->lpid,
           lp->loopsink->finish_time);
       }  
       else
        set_start_time_BCET(bb, proc);     
       computeBCET_block(bb, proc, lp);    
     }
  }
   /* CAUTION: Update the current context */ 
  cur_context /= 2;
}

/* Compute worst case finish time and cost of a block */
static void computeBCET_block(block* bb, procedure* proc, loop* cur_lp)
{
  int i;    
  loop* inlp;
  instr* inst;
  uint acc_cost = 0;
  acc_type acc_t;
  procedure* callee;

  DEBUG_PRINTF( "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t)bb);

   /* Check whether the block is some header of a loop structure. 
    * In that case do separate analysis of the loop */
   /* Exception is when we are currently in the process of analyzing 
    * the same loop */
   if((inlp = check_loop(bb,proc)) && (!cur_lp || (inlp->lpid != cur_lp->lpid)))
    computeBCET_loop(inlp, proc);

   /* Its not a loop. Go through all the instructions and
    * compute the WCET of the block */  
   else
   {
     for(i = 0; i < bb->num_instr; i++)
     {
       inst = bb->instrlist[i];
       assert(inst);

       /* Handle procedure call instruction */
       if(IS_CALL(inst->op))
       {  
         callee = getCallee(inst, proc);  

         /* For ignoring library calls */
         if(callee)
         {
           /* Compute the WCET of the callee procedure here.
            * We dont handle recursive procedure call chain 
            */
           computeBCET_proc(callee, bb->start_time + acc_cost);
           acc_cost += callee->running_cost;   
         }
       }
       /* No procedure call ---- normal instruction */
       else
       {
         /* If its a L1 hit add only L1 cache latency */
         if((acc_t = check_hit_miss(bb, inst)) == L1_HIT)
          acc_cost += L1_HIT_LATENCY;
         /* If its a L1 miss and L2 hit add only L2 cache 
          * latency */
         else if (acc_t == L2_HIT) {
           if(g_shared_bus)
            acc_cost += compute_bus_delay(bb->start_time + acc_cost, ncore, L2_HIT);   
           else
            acc_cost += (L2_HIT_LATENCY + 1);     
         }  
         else {
         /* Otherwise the instruction must be fetched from memory. 
          * Since the request will go through a shared bus, we have 
          * the amount of delay is not constant and depends on the
          * start time of the request. This is computed by the 
          * compute_bus_delay function (bus delay + memory latency)
          *---ncore representing the core in which the program is 
          * being executed */ 
          if(g_shared_bus)
           acc_cost += compute_bus_delay(bb->start_time + acc_cost, ncore, L2_MISS);   
          else
           acc_cost += (MISS_PENALTY + 1);    
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

static void computeBCET_proc(procedure* proc, ull start_time)
{
  int i;    
  block* bb;
  ull min_f_time;

  /* Initialize current context. Set to zero before the start 
   * of each new procedure */   
  cur_context = 0;

  /* Preprocess CHMC classification for each instruction inside
   * the procedure */
  preprocess_chmc_BCET(proc);
  
  /* Preprocess CHMC classification for L2 cache for each 
   * instruction inside the procedure */
  preprocess_chmc_L2_BCET(proc);

  /* Reset all timing information */
  reset_timestamps(proc, start_time);

#ifdef _DEBUG
  dump_pre_proc_chmc(proc);   
#endif

  /* Recursively compute the finish time and WCET of each 
   * predecessors first */
  for(i = proc->num_topo - 1; i >= 0; i--)
  {
    bb = proc->topo[i];
    assert(bb);
    /* If this is the first block of the procedure then
     * set the start time of this block to be the same 
     * with the start time of the procedure itself */
    if(i == proc->num_topo - 1)
      bb->start_time = start_time;
    else
      set_start_time_BCET(bb, proc);
    computeBCET_block(bb, proc, NULL);
  }

#ifdef _DEBUG
  dump_prog_info(proc);   
#endif

  /* Now calculate the final WCET */
  for(i = 0; i < proc->num_topo; i++)
  {
     assert(proc->topo[i]);
     if(proc->topo[i]->num_outgoing > 0)
      break;     
     if(i == 0)
      min_f_time = proc->topo[i]->finish_time;     
     else if(min_f_time > proc->topo[i]->finish_time)
      min_f_time = proc->topo[i]->finish_time;     
  }  

  proc->running_finish_time = min_f_time;
  proc->running_cost = min_f_time - start_time;

  DEBUG_PRINTF( "Set best case cost of the procedure %d = %Lu\n", 
        proc->pid, proc->running_cost);     
}

/* Given a MSC and a task inside it, this function computes 
 * the earliest start time of the argument task. Finding out 
 * the earliest start time is important as the bus aware BCET
 * analysis depends on the same */
static void update_succ_earliest_time(MSC* msc, task_t* task)
{
  int i;    
  uint sid;

  DEBUG_PRINTF( "Number of Successors = %d\n", task->numSuccs);
  for(i = 0; i < task->numSuccs; i++)
  {
    DEBUG_PRINTF( "Successor id with %d found\n", task->succList[i]);
    sid = task->succList[i];
    msc->taskList[sid].l_start = task->l_start + task->bcet;
    DEBUG_PRINTF( "Updating earliest start time of successor = %Lu\n", 
        msc->taskList[sid].l_start);       
  }
}

/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks 
 * imposed by the partial order of the MSC */
static ull get_earliest_start_time(task_t* cur_task, uint core)
{   
  ull start;

  /* A task in the MSC can be delayed because of two reasons. Either
   * the tasks it is dependent upon has not finished executing or 
   * since we consider a non-preemptive scheduling policy the task can 
   * also be delayed because of some other processe's execution in the 
   * same core. Thus we need to consider the maximum of two 
   * possibilities */
  if(cur_task->l_start > latest[core])          
     start = cur_task->l_start;
  else
    start = latest[core];
  DEBUG_PRINTF( "Assigning the latest starting time of the task = %Lu\n", start);   

  return start;   
}

/* This is a top level call and always start computing the WCET from 
 * "main" function */
void computeBCET(ull start_time)
{
   int top_func;
   int id;

   acc_bus_delay = 0;
   cur_task = NULL;

   /* Send the pointer to the "main" to compute the WCET of the 
    * whole program */
   assert(proc_cg);
   id = num_procs - 1;

   while(id >= 0)
   {
     top_func = proc_cg[id];
     /* Ignore all un-intended library calls like "printf" */
     if(top_func >= 0 && top_func <= num_procs - 1)
      break;     
     id--;  
   }    
   computeBCET_proc(procs[top_func], start_time); 

  PRINT_PRINTF("\n\n**************************************************************\n");    
  PRINT_PRINTF("Earliest start time of the program = %Lu start_time\n", start_time); 
  PRINT_PRINTF("Earliest finish time of the program = %Lu cycles\n", 
      procs[top_func]->running_finish_time);
  if(g_shared_bus)    
      PRINT_PRINTF("BCET of the program with shared bus = %Lu cycles\n", 
        procs[top_func]->running_cost);
  else      
      PRINT_PRINTF("BCET of the program without shared bus = %Lu cycles\n", 
        procs[top_func]->running_cost);
  PRINT_PRINTF("**************************************************************\n\n");    
}

/* Analyze best case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_BCET_MSC(MSC *msc) {
  int k;
  ull start_time = 0;
  procedure* proc;
        
  /* Set the global TDMA bus schedule */      
  setSchedule("TDMA_bus_sched.db");

  /* Reset the latest time of all cores */
  memset(latest, 0, num_core * sizeof(ull)); 
  /* reset latest time of all tasks */
  reset_all_task(msc);

  for(k = 0; k < msc->num_task; k++)
  {
    acc_bus_delay = 0;
    /* Get the main procedure */ 
    /* For printing not desired bus delay */
    NPRINT_PRINTF( "Analyzing Task BCET %s......\n", msc->taskList[k].task_name);
    fflush(stdout);
    cur_task = &(msc->taskList[k]);   
    proc = msc->taskList[k].main_copy;
    /* Return the core number in which the task is assigned */
    ncore = get_core(cur_task);
    /* First get the latest start time of task id "k" in this msc 
    * because the bus aware BCET analysis depends on the latest 
    * starting time of the corresponding task */
    start_time = get_earliest_start_time(cur_task, ncore);
    computeBCET_proc(proc, start_time);
    /* Set the worst case cost of this task */
    msc->taskList[k].bcet = msc->taskList[k].main_copy->running_cost;
    /* Now update the latest starting time in this core */
    latest[ncore] = start_time + msc->taskList[k].bcet;
    /* Since the interference file for a MSC was dumped in topological 
    * order and read back in the same order we are assured of the fact 
    * that we analyze the tasks inside a MSC only after all of its 
    * predecessors have been analyzed. Thus After analyzing one task 
    * update all its successor tasks' latest time */
    update_succ_earliest_time(msc, cur_task);
  PRINT_PRINTF("\n\n**************************************************************\n");    
  PRINT_PRINTF("Earliest start time of the program = %Lu start_time\n", start_time); 
  PRINT_PRINTF("Earliest finish time of the task = %Lu cycles\n", 
      proc->running_finish_time);
  if(g_shared_bus)    
      PRINT_PRINTF("BCET of the task with shared bus = %Lu cycles\n", 
        proc->running_cost);
  else      
      PRINT_PRINTF("BCET of the task without shared bus = %Lu cycles\n", 
        proc->running_cost);
  PRINT_PRINTF("**************************************************************\n\n");
  fflush(stdout);  
   }
  /* DONE :: BCET computation of the MSC */
}
