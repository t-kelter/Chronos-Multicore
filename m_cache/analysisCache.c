#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "analysisCache.h"

static int
logBase2(int n)
{
  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
    {
    printf("log2() only works for positive power of two values\n");
    return -1;
    }
  while (n >>= 1)
    power++;

  return power;
}


//read basic cache configuration from configFile and then
//set other cinfiguration
static void
set_cache_basic(char * configFile)
{

  FILE *fptr;
  int ns, na, ls, cmp, i;

  fptr = fopen(configFile, "r" );
  if( !fptr ) {
    fprintf(stderr, "Failed to open file: %s (analysisCache.c:123)\n", configFile);
    exit(1);
  }
  
  fscanf( fptr, "%d %d %d %d", &ns, &na, &ls, &cmp);
  
    cache.ns = ns;
    cache.na = na;
    cache.ls = ls;
    cache.cmp = cmp;

   //set other configuration through the basic: ns, na, ls, cmp
    cache.nsb = logBase2(cache.ns);
    cache.lsb = logBase2(cache.ls);
    // tag bits, #tags mapping to each set
    cache.ntb = MAX_TAG_BITS;
    cache.nt = 1 << cache.ntb;
    // tag + set bits, set + line bits, # of tag + set
    cache.t_sb = cache.ntb + cache.nsb;
    cache.s_lb = cache.nsb + cache.lsb;
    cache.nt_s = 1 << cache.t_sb;
    // cache line mask
    cache.l_msk =  (1 << cache.lsb) - 1;
    // set mask
    cache.s_msk = 0;
    for (i = 1; i < cache.ns; i <<= 1)
    cache.s_msk = cache.s_msk  | i;
    cache.s_msk = cache.s_msk << cache.lsb;
    // tag mask
    cache.t_msk = 0;
    for (i = 1; i < cache.nt; i <<= 1)
    cache.t_msk |= i;
    cache.t_msk = cache.t_msk << (cache.lsb + cache.nsb);
    // set+tag mask
    cache.t_s_msk = cache.t_msk | cache.s_msk;
}

static void
dumpCacheConfig()
{
    printf("Cache Configuration as follow:\n");
    printf("nubmer of set:  %d\n", cache.ns);	
    printf("nubmer of associativity:    %d\n", cache.na);	
    printf("cache line size:    %d\n", cache.ls);	
    printf("cache miss penalty: %d\n", cache.cmp);	
    printf("nubmer of set bit:  %d\n", cache.nsb);	
    printf("nubmer of linesize bit: %d\n", cache.lsb);	
    printf("set mask:   %u\n", cache.s_msk);
    printf("tag mask:   %u\n", cache.t_msk);	
    printf("tag + set mask: %u\n", cache.t_s_msk);	
}

/*
static loop* 
constructLoop(int pid, int lp_id)
{
	int i; 
	procedure *p = procs[ pid ];
	loop * lp = p->loops[lp_id];
	int  num_blk = lp->num_topo;	
	block *bb;

	//lp_ptr = (loop *) CALLOC(lp_ptr, 1, sizeof(loop), "for loop");

	//copy loop[lp_id] in proc[pid] to this procedure; necessary???
	
	for( i = 0; i < num_blk; i++)
	{
		bb = lp->topo[i];
		if(bb->is_loophead)
		{
			bb->lp_ptr = constructLoop(pid, bb->loopid);
		}
		else if(bb->callpid !=-1)
			bb->proc_ptr = constructFunctionCall(bb->callpid);
	}
	return lp;
}
*/

static block* 
copyBlock(block *bb)
{
	block * src, *copy;
	src = bb;
	int i, j;

	copy = (block*) CALLOC(copy, 1, sizeof(block), "block");
	
	copy->bbid = src->bbid;
	//printf("\nnow copying bb %d\n", copy->bbid);
	
	copy->pid= src->pid;	
	copy->startaddr= src->startaddr;
	
	copy->num_outgoing = src->num_outgoing;
	copy->num_outgoing_copy = src->num_outgoing_copy;
	
	if(copy->num_outgoing)
	{
		copy->outgoing = (int *)CALLOC(copy->outgoing, copy->num_outgoing, sizeof(int), "outgoing");
		for(i = 0; i < copy->num_outgoing; i++)
			copy->outgoing[i] = src->outgoing[i];
	}
	copy->num_incoming = src->num_incoming;
	
	if(copy->num_incoming)	
	{
		copy->incoming = (int *)CALLOC(copy->incoming, copy->num_incoming, sizeof(int), "incoming");
		for(i = 0; i < copy->num_incoming; i++)
			copy->incoming[i] = src->incoming[i];
	}
	copy->callpid = src->callpid;	
	copy->size = src->size;	
	//copy->cost = src->cost;	

	//copy->reset = src->reset;
	//copy->bcost = src->bcost;	
	//copy->wcost = src->wcost;	
	//copy->numMblks = src->numMblks;
	//copy->regid = src->regid;
	copy->loopid = src->loopid;
	copy->is_loophead = src->is_loophead;
	copy->num_cache_fetch = src->num_cache_fetch;
	//copy->num_cache_fetch_L2= src->num_cache_fetch_L2;
	
	
	copy->num_instr = src->num_instr;
	
	copy->instrlist = (instr **)CALLOC(copy->instrlist, copy->num_instr, sizeof(instr *), "instruction list");
	for(i = 0; i < copy->num_instr; i++)
	{
		copy->instrlist[i] = (instr*)CALLOC(copy->instrlist[i], 1, sizeof(instr), "instruction");
		
		for(j = 0; j < OP_LEN; j++)
		{
		copy->instrlist[i]->addr[j] = src->instrlist[i]->addr[j];
		copy->instrlist[i]->op[j] = src->instrlist[i]->op[j];
		copy->instrlist[i]->r1[j] = src->instrlist[i]->r1[j];
		copy->instrlist[i]->r2[j] = src->instrlist[i]->r2[j];
		copy->instrlist[i]->r3[j] = src->instrlist[i]->r3[j];
		}
	}
	//copy->proc_ptr= src->proc_ptr;

	//printf("\ncopying bb %d over!!!\n", copy->bbid);

	return copy;
}


static loop* 
copyLoop(procedure *proc, procedure* copy_proc, loop *lp)
{
	loop *src, *copy;
	src = lp;
	int i;

	copy = (loop *)CALLOC(copy, 1, sizeof(loop), "loop");
	copy->lpid = src->lpid;
	//printf("\nnow copying loop %d\n", copy->lpid);
	
	copy->pid = src->pid;
	copy->loopbound = src->loopbound;

	//the  loophead in loop copy
	i = src->loophead->bbid;
	//copy->loophead = proc->bblist[i];
	copy->loophead = copy_proc->bblist[i];

	i = src->loopsink->bbid;
	//copy->loopsink= proc->bblist[i];
	copy->loopsink= copy_proc->bblist[i];

	i = src->loopexit->bbid;
	//copy->loopexit = proc->bblist[i];
	copy->loopexit = copy_proc->bblist[i];
	
	copy->level = src->level;
	copy->nest = src->nest;
	copy->is_dowhile= src->is_dowhile;
	
	copy->num_topo = src->num_topo;
	if(copy->num_topo)
	{
		copy->topo = (block**)CALLOC(copy->topo, copy->num_topo, sizeof(block*), "topo");
		for(i = 0; i < copy->num_topo; i++)
			//copy->topo[i] = copyBlock(procs[src->pid]->bblist[src->topo[i]->bbid]);
			copy->topo[i] = copy_proc->bblist[src->topo[i]->bbid];
	}
	copy->num_cost = src->num_cost;
//	copy->wpath = src->wpath; 		//???

	//printf("\ncopying loop %d over!!!\n", copy->lpid);

	return copy;
}


static procedure* 
copyFunction(procedure* proc)
{
	procedure *src, *copy;
	src = proc;
	int i;

	copy = (procedure*) CALLOC(copy, 1, sizeof(procedure), "procedure");
	copy->pid = src->pid;

	//printf("\nnow copying procdure %d\n", copy->pid);
	
	copy->num_bb = src->num_bb;	
	if(copy->num_bb)
	{
		copy->bblist = (block**) CALLOC(copy->bblist, copy->num_bb, sizeof(block*), "block*");
		for(i = 0; i < copy->num_bb; i++)
			copy->bblist[i] = copyBlock(src->bblist[i]);
	}
	copy->num_loops = src->num_loops;
	if(copy->num_loops)
	{
		copy->loops = (loop**)CALLOC(copy->loops, copy->num_loops, sizeof(loop*), "loops");
		for(i = 0; i < copy->num_loops; i++)
			copy->loops[i] = copyLoop(proc, copy, src->loops[i]);
	}
	
	copy->num_calls = src->num_calls;
	if(copy->num_calls)
	{
		copy->calls = (int *)CALLOC(copy->calls, copy->num_calls, sizeof(int), "calls");
		for(i = 0; i < copy->num_calls; i++)
			copy->calls[i] = src->calls[i];
	}
	
	copy->num_topo = src->num_topo;
	if(copy->num_topo)
	{
		copy->topo = (block**)CALLOC(copy->topo, copy->num_topo, sizeof(block*), "topo");
		for(i = 0; i < copy->num_topo; i++)
			//copy->topo[i] = copyBlock(src->bblist[src->topo[i]->bbid]);
			copy->topo[i] = copy->bblist[src->topo[i]->bbid];
	}
	copy->wcet = src->wcet;
	
//	copy->wpath = src->wpath;  	//???
	//printf("\ncopying procdure %d over!!!\n", copy->pid);
	return copy;
}


static procedure*
constructFunctionCall(procedure *pro, task_t *task)
{
    	procedure *p = copyFunction(pro);
	//loop *lp;
	block * bb;
	int num_blk = p->num_bb;
	int i, j;
	//printf("\nin constructAll\n");
	//printf("\nnumber of function call %d\n", p->num_calls);

	j = indexOfProc(p->pid);
	if(j >= 0)
	{
		if(task->proc_cg_ptr[j].num_proc == 0)
		{
			task->proc_cg_ptr[j].num_proc = 1;
			task->proc_cg_ptr[j].proc = (procedure**)CALLOC(task->proc_cg_ptr[j].proc, 1, sizeof(procedure*), "procedure*");
			task->proc_cg_ptr[j].proc[0] = p;
		}
		else	
		{
			task->proc_cg_ptr[j].num_proc ++;
			task->proc_cg_ptr[j].proc = (procedure**) REALLOC(task->proc_cg_ptr[j].proc, task->proc_cg_ptr[j].num_proc * sizeof(procedure*), "procedure*");
			task->proc_cg_ptr[j].proc[task->proc_cg_ptr[j].num_proc -1] = p;
		}

	}

	else
	{
		printf("\nprocedure does not exsit!exit\n");
		exit(1);
	}
	
	for(i = 0 ; i < num_blk; i++)
	{
		bb = p ->bblist[i];
			
		if(bb->callpid != -1)
		{
	
			bb->proc_ptr	= constructFunctionCall(procs[bb->callpid], task);
			if(print) 
				printf("\nbb->proc_ptr = f%lx for bb[%d] calling proc[%d]\n", (uintptr_t)bb->proc_ptr, bb->bbid, bb->callpid);
/*
			j = indexOfProc(bb->callpid);
			if(j >= 0)
			{
				if(proc_cg_ptr[j].num_proc == 0)
				{
					proc_cg_ptr[j].num_proc = 1;
					proc_cg_ptr[j].proc = (procedure**) CALLOC(proc_cg_ptr[j].proc, 1, sizeof(procedure*), "procedure*");
					proc_cg_ptr[j].proc[0] = bb->proc_ptr;
				}
				else	
				{
					proc_cg_ptr[j].num_proc ++;
					proc_cg_ptr[j].proc = (procedure**) REALLOC(proc_cg_ptr[j].proc, proc_cg_ptr[j].num_proc * sizeof(procedure*), "procedure*");
					proc_cg_ptr[j].proc[proc_cg_ptr[j].num_proc -1] = bb->proc_ptr;
				}
			}
*/
			if(print) printf("\ncopy over\n");
		}	

	}
	return p;		
}





static void
constructAll(task_t *task)
{
    	procedure *p = procs[ main_id ];
/*		
	int i;
	int num_blk = p->num_bb;
	block *bb;
	for(i = 0 ; i < num_blk; i++)
	{
		bb = p ->bblist[i];
			
		if(bb->callpid != -1)
		{
		
			printf("\nbb->proc_ptr = %d for bb[%d] in proc[%d]\n", bb->proc_ptr, bb->bbid, main_id);
			bb->proc_ptr = constructFunctionCall(p);
			//bb->proc_self = p;
			printf("\ncopy over\n");
			
		}	

	}
	return p;		
*/
	main_copy = constructFunctionCall( p, task);
	//exit(1);
}


static void
calculateMust(cache_line_way_t **must, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET(instr_addr);
	
	for(i = 0; i < cache.na; i++)
	{
		if(isInWay(instr_addr, must[i]->entry, must[i]->num_entry))
		{
			if(i != 0)
			{
				must[0]->num_entry++;
				if(must[0]->num_entry == 1) must[0]->entry = (int*)CALLOC(must[0]->entry, must[0]->num_entry, sizeof(int), "enties");
				else	must[0]->entry = (int*)REALLOC(must[0]->entry, (must[0]->num_entry) * sizeof(int), "enties");

				must[0]->entry[(must[0]->num_entry) -1] = instr_addr;

				
				for(j = 0; j < must[i]->num_entry; j++)
				{
					if(must[i]->entry[j] == instr_addr)
					{
						if(j != must[i]->num_entry -1)
							must[i]->entry[j] = must[i]->entry[must[i]->num_entry -1];
						must[i]->num_entry --;
						if(must[i]->num_entry == 0) free(must[i]->entry);
						else must[i]->entry = (int*)REALLOC(must[i]->entry, must[i]->num_entry * sizeof(int), "entry");
	 				}
				} // end for(j)
			}	// 	end if(i!=0)
			//hit in must[0]
			return;
		}
	}
	
	cache_line_way_t * tmp = must[cache.na -1];
	cache_line_way_t *head = NULL;
	
	for(i = cache.na - 1; i > 0 ; i--)
		must[i] = must[i -1];

	free(tmp);

	//printf("\nalready free tail of the old cs\n");
	
	head = (cache_line_way_t *)CALLOC(head, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	head->entry = (int*)CALLOC(head->entry, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	must[0] = head;
}


static void
calculateMay(cache_line_way_t **may, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET(instr_addr);

	for(i = 0; i < cache.na; i++)
	{
		if(isInWay(instr_addr, may[i]->entry, may[i]->num_entry))
		{
			if(i != 0)
			{
				may[0]->num_entry++;
				if(may[0]->num_entry == 1)
				{
					may[0]->entry = (int*)CALLOC(may[0]->entry, may[0]->num_entry, sizeof(int), "enties");
				}
				else	
				{
					may[0]->entry = (int*)REALLOC(may[0]->entry, (may[0]->num_entry) * sizeof(int), "enties");
				}
				may[0]->entry[(may[0]->num_entry) -1] = instr_addr;

				
				for(j = 0; j < may[i]->num_entry; j++)
				{
					if(may[i]->entry[j] == instr_addr)
					{
						if(j != may[i]->num_entry -1) may[i]->entry[j] = may[i]->entry[may[i]->num_entry -1];
						may[i]->num_entry --;
						if(may[i]->num_entry == 0)
						{
							free(may[i]->entry);
						}
						else
						{
							may[i]->entry = (int*)REALLOC(may[i]->entry, may[i]->num_entry * sizeof(int), "entry");
						}
					}
				} // end for(j)
			}	// 	end if(i!=0)
			return;
		}
	}
	
	cache_line_way_t * tmp = may[cache.na -1];
	cache_line_way_t *head = NULL;
	
	for(i = cache.na - 1; i > 0 ; i--)
		may[i]  = may [i -1];		
	free(tmp);


	head = (cache_line_way_t *)CALLOC(head, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	head->entry = (int*)CALLOC(head->entry, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	may[0] = head;
}




static void
calculatePersist(cache_line_way_t **persist, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET(instr_addr);

	for(i = 0; i < cache.na; i++)
	{
		if(isInWay(instr_addr, persist[i]->entry, persist[i]->num_entry))
		{
			if(i != 0)
			{
				persist[0]->num_entry++;
				if(persist[0]->num_entry == 1)
				{
					persist[0]->entry = (int*)CALLOC(persist[0]->entry, persist[0]->num_entry, sizeof(int), "enties");
				}
				else	
				{
					persist[0]->entry = (int*)REALLOC(persist[0]->entry, (persist[0]->num_entry) * sizeof(int), "enties");
				}
				persist[0]->entry[(persist[0]->num_entry) -1] = instr_addr;

				
				for(j = 0; j < persist[i]->num_entry; j++)
				{
					if(persist[i]->entry[j] == instr_addr)
					{
						if(j != persist[i]->num_entry -1) persist[i]->entry[j] = persist[i]->entry[persist[i]->num_entry -1];
						persist[i]->num_entry --;
						if(persist[i]->num_entry == 0)
						{
							free(persist[i]->entry);
						}
						else
						{
							persist[i]->entry = (int*)REALLOC(persist[i]->entry, persist[i]->num_entry * sizeof(int), "entry");
						}
					}
				} // end for(j)
			}	// 	end if(i!=0)
			return;
		}
	}
	
	cache_line_way_t * tmp = persist[cache.na ];
	cache_line_way_t *head = NULL;
	
	for(i = cache.na; i > 0 ; i--)
		persist[i]  = persist [i -1];		
	free(tmp);


	head = (cache_line_way_t *)CALLOC(head, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	head->entry = (int*)CALLOC(head->entry, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	persist[0] = head;
}



static void
calculateCacheState(cache_line_way_t **must, cache_line_way_t **may, cache_line_way_t **persist, int instr_addr)
{
	//printf("\nalready in calculate cs\n");

	calculateMust(must, instr_addr);
	calculateMay(may, instr_addr);
	calculatePersist(persist, instr_addr);
	
	
}


//allocate the memory for cache_state
static cache_state *
allocCacheState()
{
	int j, k;
	cache_state *result = NULL;
	/*
	if(loop_level == -1)
		copies = 1;
	else
		copies = (2<<loop_level);
	*/
	//printf("\nalloc CS memory copies = %d \n", copies);

	result = (cache_state*) CALLOC(result, 1, sizeof(cache_state), "cache_state");
	//result->loop_level = loop_level;

	//result->cs = (cache_state_t*)CALLOC(result->cs, copies, sizeof(cache_state_t), "cache_state_t");
	
	//printf("\nalloc CS memory result->loop_level = %d \n", result->loop_level );

  //int i;
	//for( i = 0; i < copies; i ++)
	//{
		//printf("\nalloc CS memory for i = %d \n", i );

		result->must = NULL;
		result->must = (cache_line_way_t***)CALLOC(result->must, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->may = NULL;
		result->may = (cache_line_way_t***)CALLOC(result->may, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->persist = NULL;
		result->persist = (cache_line_way_t***)CALLOC(result->persist, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	
		for(j = 0; j < cache.ns; j++)
		{
			//printf("\nalloc CS memory for j = %d \n", j );
			result->must[j]= (cache_line_way_t**)	CALLOC(result->must[j], cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			result->may[j]= (cache_line_way_t**)CALLOC(result->may[j], cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			result->persist[j]= (cache_line_way_t**)CALLOC(result->persist[j], cache.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			for( k = 0; k < cache.na; k++)
			{
				//printf("\nalloc CS memory for k = %d \n", k );
				result->must[j][k] = (cache_line_way_t*)CALLOC(result->must[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->must[j][k]->num_entry = 0;
				result->must[j][k]->entry = NULL;

				result->may[j][k] = (cache_line_way_t*)CALLOC(result->may[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->may[j][k]->num_entry = 0;
				result->may[j][k]->entry = NULL;

				result->persist[j][k] = (cache_line_way_t*)CALLOC(result->persist[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->persist[j][k]->num_entry = 0;
				result->persist[j][k]->entry = NULL;
			}
			
			result->persist[j][cache.na] = (cache_line_way_t*)CALLOC(result->persist[j][cache.na], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
			result->persist[j][cache.na]->num_entry = 0;
			result->persist[j][cache.na]->entry = NULL;

		}
	//}

	return result;
}



/*
static void
divideBBintoMBlks(){

}

static int
allocLoop(int pid, int bbid)
{
	//??? get loop id through bb id
	int i, lp_level, copies; 
	procedure *p = procs[ pid ];
	block *bb = p->bblist[bbid];
	loop * lp = p->loops[bb->loopid];

	int  num_blk = lp->num_topo;	

	for(i = 0 ; i < num_blk; i++)
	{
		bb = p ->topo[i];
		lp_level = bb->loopid;
		
		if( lp_level == -1)
			copies = 1;
		else
			copies = 2<< (bb->bb_cache_state->loop_level);

		
		bb->bb_cache_state->cs = 
			(cache_state_t*)CALLOC(bb->bb_cache_state->cs, copies, sizeof(cache_state_t), "cache_state_t");


		if(bb->callpid !=-1)
		{
			allocFunctionCall(bb->callpid);
		}	
		else if(bb->is_loophead)
		{
			allocLoop(pid, bb->bbid);
		}
	}
	return 0;
}

static int
allocFunctionCall(int pid)
{
	int i, lp_level, copies; 
	procedure *p = procs[ pid];
	//loop * lp;
	int  num_blk = p->num_topo;	
	block *bb;
	for(i = 0 ; i < num_blk; i++)
	{
		bb = p ->topo[i];
		lp_level = bb->loopid;
		
		if( lp_level == -1)
			copies = 1;
		else
			copies = 2<< (bb->bb_cache_state->loop_level);
		
		bb->bb_cache_state->cs = 
			(cache_state_t*)CALLOC(bb->bb_cache_state->cs, copies, sizeof(cache_state_t), "cache_state_t");
		if(bb->callpid !=-1)
		{
			allocFunctionCall(bb->callpid);
		}	
		else if(bb->is_loophead)
		{
			allocLoop(pid, bb->bbid);
		}
		
	}
	return 0;
}
//!!! loop level is needed and yet have not get
static void
 allocCacheStates()
{
	cache_line_len = cache.ls * INSN_SIZE;
	cache_line_bits = log_base2(cache_line_len);
	allocFunctionCall(main_id);

}
*/

/* For checking when updating the cache state during 
 * persistence analysis */
static char
isInSet_persist(int addr, cache_line_way_t **set)
{
	int i;
	for(i = 0; i < cache.na + 1; i++)
	{
		if(isInWay(addr, set[i]->entry, set[i]->num_entry) == 1)
			return i;
	}
	return -1;
}

static char
isInSet(int addr, cache_line_way_t **set)
{
	int i;
	for(i = 0; i < cache.na; i++)
	{
		if(isInWay(addr, set[i]->entry, set[i]->num_entry) == 1)
			return i;
	}
	return -1;
}


static char
isInCache(int addr, cache_line_way_t**must)
{
	//printf("\nIn isInCache\n");
	int i;
	addr = TAGSET(addr);
	
	for(i = 0; i < cache.na; i++)
		if(isInWay(addr, must[i]->entry, must[i]->num_entry))
			return 1;
	return 0;	
}

static char
isNeverInCache(int addr, cache_line_way_t**may)
{
	//printf("\nIn isNeverInCache\n");
	int i;
	addr = TAGSET(addr);
	for(i = 0; i < cache.na; i++)
		if(isInWay(addr, may[i]->entry, may[i]->num_entry))
			return 0;
	return 1;
	
}


static cache_state *
copyCacheState(cache_state *cs)
{
	int j, k, num_entry;
	cache_state *copy = NULL;

	//printf("\nIn copy Cache State now\n");

	copy = (cache_state*)CALLOC(copy, 1, sizeof(cache_state), "cache_state");
	copy->must = NULL;
	copy->may = NULL;
	copy->persist = NULL;

	//copy->chmc = NULL;
		
	//lp_level = cs->loop_level;
	
	//printf("\nIn copyCacheState: i is %d\n", i);
	copy->must =(cache_line_way_t***)CALLOC(copy->must, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	copy->may =(cache_line_way_t***)CALLOC(copy->may, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	copy->persist =(cache_line_way_t***)CALLOC(copy->persist, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

	for(j = 0; j < cache.ns; j++)
	{
		copy->must[j] = (cache_line_way_t**)	CALLOC(copy->must[j], cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");
		copy->may[j] = (cache_line_way_t**) CALLOC(copy->may[j], cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");
		copy->persist[j] = (cache_line_way_t**) CALLOC(copy->persist[j], cache.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");


		for( k = 0; k < cache.na; k++)
		{
			copy->must[j][k] = (cache_line_way_t*)CALLOC(copy->must[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t must");
			copy->may[j][k] = (cache_line_way_t*)CALLOC(copy->may[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t may");
			copy->persist[j][k] = (cache_line_way_t*)CALLOC(copy->persist[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t may");

			copy->must[j][k]->num_entry = cs->must[j][k]->num_entry;
			if(copy->must[j][k]->num_entry)
			{
				copy->must[j][k]->entry = (int*)CALLOC(copy->must[j][k]->entry, copy->must[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->must[j][k]->num_entry; num_entry++)
					copy->must[j][k]->entry[num_entry] =  cs->must[j][k]->entry[num_entry];
			}

			copy->may[j][k]->num_entry = cs->may[j][k]->num_entry;
			if(copy->may[j][k]->num_entry)
			{
				copy->may[j][k]->entry = (int*)CALLOC(copy->may[j][k]->entry, copy->may[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->may[j][k]->num_entry; num_entry++)
					copy->may[j][k]->entry[num_entry] =  cs->may[j][k]->entry[num_entry];
			}


			copy->persist[j][k]->num_entry = cs->persist[j][k]->num_entry;
			if(copy->persist[j][k]->num_entry)
			{
				copy->persist[j][k]->entry = (int*)CALLOC(copy->persist[j][k]->entry, copy->persist[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->persist[j][k]->num_entry; num_entry++)
					copy->persist[j][k]->entry[num_entry] =  cs->persist[j][k]->entry[num_entry];
			}
			
		}


		copy->persist[j][cache.na] = (cache_line_way_t*)CALLOC(copy->persist[j][cache.na], 1, sizeof(cache_line_way_t), "one cache_line_way_t may");
		copy->persist[j][cache.na]->num_entry = cs->persist[j][cache.na]->num_entry;
		if(copy->persist[j][cache.na]->num_entry)
		{
			copy->persist[j][cache.na]->entry = (int*)CALLOC(copy->persist[j][cache.na]->entry, copy->persist[j][cache.na]->num_entry, sizeof(int), "entries");

			for(num_entry = 0; num_entry < copy->persist[j][cache.na]->num_entry; num_entry++)
				copy->persist[j][cache.na]->entry[num_entry] =  cs->persist[j][cache.na]->entry[num_entry];
		}

	}
	return copy;
}



static cache_state *
mapLoop(procedure *proc, loop *lp)
{
	int i, j, k, n, set_no, cnt, tmp, addr, addr_next, copies, tag, tag_next; 
	int lp_level;

	procedure *p = proc;
	block *bb, *incoming_bb;
	cache_state *cs_ptr;
	cache_line_way_t **clw;
	//printf("\nIn mapLoop loopid[%d]\n", lp->lpid);

	//printBlock(bb);
	//printProc(p);
	//printLoop(lp);

	int  num_blk = lp->num_topo;
	//printLoop(lp);
	//cs_ptr = copyCacheState(cs);
	//cs_ptr->source_bb = NULL;
	
	CHMC *current_chmc;

	//exit(1);
	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}
		
	//identify which cache state should be used
	cnt = 0;
	for(k = 0; k <= lp_level; k++)
		if(loop_level_arr [k] == NEXT_ITERATION)
			cnt += (1<<(lp_level - k));

	for(i = num_blk -1 ; i >= 0 ; i--)
	{

		//bb in topo
		bb = lp ->topo[i];
		//bb in bblist
		bb = p->bblist[bb->bbid];

		bb->num_instr = bb->size / INSN_SIZE;

		if(print) printf("deal with bb[ %d ] in proc[%d]\n", bb->bbid, bb->pid);	
		
		if(bb->is_loophead && i != num_blk -1)
		{
			//printf("\nbb is a loop head in proc[%d]\n", pid);	

			if(print) printf("\nthe first time go into loop %d", bb->loopid);		
			loop_level_arr[lp_level + 1] = FIRST_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);

			//cs_ptr = bb->bb_cache_state;
		 
			if(print) printf("\nthe second time go into loop %d", bb->loopid);				
			loop_level_arr[lp_level + 1] = NEXT_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level + 1] = INVALID;
			
			if(print) dumpCacheState(cs_ptr);
			//exit(1);

			//freeCacheStateLoop(p,  p->loops[bb->loopid]);
			continue;
		}

		//printf("\nbb->num_incoming = %d\n", bb->num_incoming);		
		//if more than one incoming do operation of cache state  
		//as the input of this bb

		if(bb->is_loophead == 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];

#ifdef _DEBUG
		   fprintf(stdout, "Calculating bb cache state of bbid = %d of proc = %d at X1\n",
					 bb->bbid, bb->pid);
#endif
		   
			//if(incoming_bb->bb_cache_state == NULL) continue;

			cs_ptr = copyCacheState(incoming_bb->bb_cache_state);

			if(loop_level_arr[lp_level] == NEXT_ITERATION && incoming_bb->is_loophead == 0)
			{
				incoming_bb->num_outgoing--;
				if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state == 1)
				{
					incoming_bb->num_cache_state = 0;
					//freeCacheState(incoming_bb->bb_cache_state);
					//incoming_bb->bb_cache_state = NULL;
				}
			}
			

			if(bb->num_incoming > 1)
			{
				//printf("\ndo operations if more than one incoming edge\n");

				//dumpCacheState(cs_ptr);
				
				for(j = 1; j < bb->num_incoming; j++)
				{
					incoming_bb = p->bblist[bb->incoming[j]];
					
					if(incoming_bb->bb_cache_state == NULL) continue;
					
					for(k = 0; k < cache.ns; k++)
					{
						clw = cs_ptr->must[k];							
						cs_ptr->must[k] = intersectCacheState(cs_ptr->must[k], incoming_bb->bb_cache_state->must[k]);

						freeCacheSet(clw);

						clw = cs_ptr->may[k];							
						cs_ptr->may[k] = unionCacheState(cs_ptr->may[k], incoming_bb->bb_cache_state->may[k]);

						freeCacheSet(clw);

						clw = cs_ptr->persist[k];							
						cs_ptr->persist[k] = unionMaxCacheState(cs_ptr->persist[k], incoming_bb->bb_cache_state->persist[k]);

						freeCacheSet(clw);

					}
					if(loop_level_arr[lp_level] == NEXT_ITERATION && incoming_bb->is_loophead == 0)
					{
						incoming_bb->num_outgoing--;
						if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state == 1)
						{
							incoming_bb->num_cache_state = 0;
							//freeCacheState(incoming_bb->bb_cache_state);
							//incoming_bb->bb_cache_state = NULL;
						}
					}
							
				} //end for(all incoming)

			}

			
		}


		//bb is loophead of this loop
		else if(bb->is_loophead && i == num_blk - 1)
		{
			if(loop_level_arr[lp_level] == FIRST_ITERATION)
			{
#ifdef _DEBUG
		   fprintf(stdout, "Calculating bb cache state of bbid = %d of proc %d at X2\n",
					 bb->bbid, bb->pid);
#endif
				incoming_bb = p->bblist[bb->incoming[0]];

				//free(cs_ptr);
			
				cs_ptr = copyCacheState(incoming_bb->bb_cache_state);

				if(bb->num_incoming > 1)
				{
					//printf("\ndo operations if more than one incoming edge\n");

					//dumpCacheState(cs_ptr);
					
					for(j = 1; j < bb->num_incoming; j++)
					{
						incoming_bb = p->bblist[bb->incoming[j]];

						//if(incoming_bb->bb_cache_state == NULL) continue;
					
						for(k = 0; k < cache.ns; k++)
						{
							clw = cs_ptr->must[k];							
							cs_ptr->must[k] = intersectCacheState(cs_ptr->must[k], incoming_bb->bb_cache_state->must[k]);

							freeCacheSet(clw);

							clw = cs_ptr->may[k];							
							cs_ptr->may[k] = unionCacheState(cs_ptr->may[k], incoming_bb->bb_cache_state->may[k]);

							freeCacheSet(clw);

							clw = cs_ptr->persist[k];							
							cs_ptr->persist[k] = unionMaxCacheState(cs_ptr->persist[k], incoming_bb->bb_cache_state->persist[k]);

							freeCacheSet(clw);

						}
					} //end for(all incoming)

				}

			}
			else if(loop_level_arr[lp_level] == NEXT_ITERATION)
			{
			
			//	bb->bb_cache_state_result = bb->bb_cache_state;
			
				cs_ptr = copyCacheState(p->bblist[lp->topo[0]->bbid]->bb_cache_state);
				//cache_state *cs_tmp = cs_ptr;
				//freeCacheState(cs_tmp);
			/*
				for(k = 0; k < cache.ns; k++)
					for(n = 0; n < cache.na; n++)
					{
						clw = bb->bb_cache_state_result->must[k][n];
						bb->bb_cache_state_result->must[k][n] = intersectCacheState(clw, cs_ptr->must[k][n]);
						free(clw);

						clw = bb->bb_cache_state_result->may[k][n];
						bb->bb_cache_state_result->may[k][n] = unionCacheState(clw,  cs_ptr->may[k][n]);
						free(clw);
					}

				p->bblist[lp->topo[0]->bbid]->num_cache_state = 0;
				free(p->bblist[lp->topo[0]->bbid]->bb_cache_state);
				p->bblist[lp->topo[0]->bbid]->bb_cache_state = NULL;
			*/
			}
			else
			{
				printf("\nCFG error!\n");
				exit(1);
			}
		}


		//printf("\ncnt = %d\n", cnt);
		//dumpCacheState(cs_ptr);
		//initialize the output cache state of this bb
		//using the input cache state
	

		if(bb->num_cache_state == 0)
		{
			//bb->bb_cache_state = (cache_state *)CALLOC(bb->bb_cache_state, 1, sizeof(cache_state), "cache_state");

			bb->num_cache_state = 1;
		}

		if(bb->num_chmc == 0)
		{
			//printf("\nallocation for bb in lp[%d]\n", bb->loopid);
			copies = 2<<(lp_level);

			bb->num_chmc = copies;

			bb->chmc = (CHMC**)CALLOC(bb->chmc, copies, sizeof(CHMC*), "CHMC");
			for(tmp = 0; tmp < copies; tmp++)
			{
				bb->chmc[tmp] = (CHMC*)CALLOC(bb->chmc[tmp], 1, sizeof(CHMC), "CHMC");
			}

		}


		bb->bb_cache_state = cs_ptr;

#ifdef _DEBUG
		   fprintf(stdout, "Calculating bb cache state of bbid = %d of proc %d at X3\n",
					 bb->bbid, bb->pid);
#endif
		
		current_chmc = bb->chmc[cnt];
		
		//if(loop_level_arr[lp_level] == NEXT_ITERATION)
			//current_chmc = cs->chmc[cnt];

		current_chmc->hit = 0;
		current_chmc->miss = 0;
		current_chmc->unknow = 0;
		
		current_chmc->wcost= 0;
		current_chmc->bcost= 0;

		//current_chmc->hit_addr = NULL;
		//current_chmc->miss_addr = NULL;
		//current_chmc->unknow_addr = NULL;
		//current_chmc->hitmiss_addr = NULL;
	
		//exit(1);
		//compute categorization of each instruction line  		
		//int start_addr = bb->startaddr;
		//bb->num_cache_fetch = bb->size / cache.ls;

		
		
		//int start_addr_fetch = (start_addr >> cache.lsb) << cache.lsb;
		//tmp = start_addr - start_addr_fetch;

		
		//tmp is not 0 if start address is not multiple of sizeof(cache line) in bytes
		//if(tmp) 	
			//bb->num_cache_fetch++;

		//printf("num_cache_fetch =  %d\n", bb->num_cache_fetch);

		current_chmc->hitmiss = bb->num_instr;
		//if(current_chmc->hitmiss > 0)
			current_chmc->hitmiss_addr = (char*)CALLOC(current_chmc->hitmiss_addr, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

		//dumpCacheState(cs_ptr);
		//exit(1);
		addr = bb->startaddr;
		for(n = 0; n < bb->num_instr ; n++)
		{
			//if(bb->is_loophead!= 0)
				//break;
			set_no = SET(addr);
			
			if(isInCache(addr, bb->bb_cache_state->must[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_HIT;
				//current_chmc->hitmiss++;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}


				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT;
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}
			else if(isNeverInCache(addr, bb->bb_cache_state->may[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_MISS;
				//current_chmc->hitmiss++;
					
				if(current_chmc->miss == 0)
				{
					current_chmc->miss++;
					current_chmc->miss_addr = (int*)CALLOC(current_chmc->miss_addr, 1, sizeof(int), "miss_addr");
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}
				else
				{
					current_chmc->miss++;				
					current_chmc->miss_addr = (int*)REALLOC(current_chmc->miss_addr, current_chmc->miss * sizeof(int), "miss_addr");				
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}

				
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[ n ] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}

			}

			else if(isInCache(addr, bb->bb_cache_state->persist[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = FIRST_MISS;
				lp->num_fm ++;

				//current_chmc->hitmiss++;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}


				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}

			else
			{
				current_chmc->hitmiss_addr[ n ] = UNKNOW;
				//current_chmc->hitmiss++;
				
				if(current_chmc->unknow == 0)
				{
					current_chmc->unknow++;
					current_chmc->unknow_addr = (int*)CALLOC(current_chmc->unknow_addr, 1, sizeof(int), "unknow_addr");
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
				else
				{
					current_chmc->unknow++;				
					current_chmc->unknow_addr = (int*)REALLOC(current_chmc->unknow_addr, current_chmc->unknow * sizeof(int), "unknow_addr");				
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
				
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[ n ] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}
			addr = addr + INSN_SIZE;
		}
//if(print)

		//current_chmc->wcost = current_chmc->hit * IC_HIT + (current_chmc->hitmiss - current_chmc->hit) * IC_MISS;
		//current_chmc->bcost  = current_chmc->miss * IC_MISS + (current_chmc->hitmiss - current_chmc->miss) * IC_HIT;

	if(print)
	{
		
		//for(k = 0; k < current_chmc->hitmiss; k++)
			//printf("L1: %d ", current_chmc->hitmiss_addr[k]);
		printf("\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
		printf("hit = %d, miss= %d, unknow = %d\n", current_chmc->hit, current_chmc->miss, current_chmc->unknow);
		printf("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);	
	}
	//scanf("%c", &tmp);

	//exit(1);


		//start_addr = bb->startaddr;
		//bb->num_cache_fetch = bb->size / cache.ls;

		
		//start_addr_fetch = (start_addr >> cache.lsb) << cache.lsb;
		//tmp = start_addr - start_addr_fetch;

		//tmp is not 0 if start address is not multiple of sizeof(cache line) in bytes
		//if(tmp) 	
			//bb->num_cache_fetch++;
	
		//check the bb if it is a function call
	
		if(bb->callpid != -1)
		{
			//printf("\nbb is a function call\n");	
			//addr = start_addr_fetch;
			addr = bb->startaddr;
			
			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);

				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);

				//dumpCacheState(bb->bb_cache_state);
				//exit(1);
				addr = addr + INSN_SIZE;
			}
			cs_ptr = bb->bb_cache_state;
			bb->bb_cache_state = copyCacheState(mapFunctionCall(bb->proc_ptr, cs_ptr));
			freeCacheState(cs_ptr);
			cs_ptr = NULL;
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;

			//freeCacheStateFunction(bb->proc_ptr);
			
			if(print) dumpCacheState(cs_ptr);
		}	
		else
		{
			//printf("\nbb is a simple bb \n");	
			addr = bb->startaddr;
			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);

				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);

				//dumpCacheState(bb->bb_cache_state);
				//exit(1);
				addr = addr + INSN_SIZE;
			}
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;
			if(print) dumpCacheState(cs_ptr);
		}
		
	}

/*
	if(loop_level_arr[lp_level] == NEXT_ITERATION)
	{
		cs_ptr = p->bblist[lp->topo[0]->bbid]->bb_cache_state;
		bb = p->bblist[lp->topo[num_blk -1]->bbid];
		
		for(k = 0; k < cache.ns; k++)
			for(n = 0; n < cache.na; n++)
			{
				clw = bb->bb_cache_state_result->must[k][n];
				bb->bb_cache_state_result->must[k][n] = intersectCacheState(clw, cs_ptr->must[k][n]);
				free(clw);

				clw = bb->bb_cache_state_result->may[k][n];
				bb->bb_cache_state_result->may[k][n] = unionCacheState(clw,  cs_ptr->may[k][n]);
				free(clw);
			}
			
		free(cs_ptr);
		cs_ptr = bb->bb_cache_state;
		bb->bb_cache_state = bb->bb_cache_state_result;
		free(cs_ptr);
	}
*/	
	return NULL;
}


static cache_state *
mapFunctionCall(procedure *proc, cache_state *cs)
{
	int i, j, k, n, set_no, cnt, addr, addr_next, copies, tmp; 
	int tag, tag_next, lp_level;

	//dumpCacheState(cs);
	//exit(1);
	procedure *p = proc;
	block *bb, *incoming_bb;
	cache_state *cs_ptr;
	cache_line_way_t **clw;
	CHMC *current_chmc;
	
	//printf("\nIn mapFunctionCall, p[%d]\n", p->pid);
	
	cs_ptr = copyCacheState(cs);
	//freeCacheState(cs);
	//cs = NULL;
	
	int  num_blk = p->num_topo;	


	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}

	//if loopid==-1, only one cache state is needed, so cnt=0;
	cnt = 0;		

	//else because of unrolling each loop
	//have to identify which cache state should be used
	//cnt is reverse binary bits of loop_level_arr[]
	//if loop_level_arr[1] = NEXT_ITERATION (for loop1)
	//and loop_level_arr[0] = FIRST_ITERATION(for loop0)
	//then it is the "next iteration" of loop 1
	//and "first iteration" of loop 0 here
	//cnt = 01(reverse loop_level_arr[])
		
	for(k = 0; k <= lp_level; k++)
		if(loop_level_arr [k] == NEXT_ITERATION)
			cnt += (1<<(lp_level - k));
		
	//following the topological order, compute categorization of cache line
	//wcet, bcet for each bb and then derive all the output cache state for each bb
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		//bb is in topo list, to get the bbid of this bb
		bb = p ->topo[i];

		//bb is in bblist
		bb = p->bblist[ bb->bbid ];
		
		bb->num_instr = bb->size / INSN_SIZE;
		
		if(print) printf("\nThis is  bb %d in proc %d\n", bb->bbid, proc->pid);
		if(bb->callpid!= -1)
			if(print) printBlock(bb);
		

		//printf("\ncnt =  %d\n", cnt);
		if(print) printf("\nbb->num_incoming =  %d\n", bb->num_incoming);
		//exit(1);
		
		//if more than one incoming do operation of cache state  
		//as the input of this bb

		if(bb->is_loophead)
		{

			//printf("\nbb is a  loop head in loop[%d]\n", bb->loopid);			
			//printf("\nloop level = %d\n", lp_level);	
			//printBlock(bb);
			//printLoop(p->loops[bb->loopid]);
			//traverse this loop the first time
			loop_level_arr[lp_level +1] = FIRST_ITERATION;
			
			p->loops[bb->loopid]->num_fm = 0;
			
			if(print) printf("\nThe first time go into loop\n");
			mapLoop(p, p->loops[bb->loopid]);
			//free(cs_ptr);
			//cs_ptr = bb->bb_cache_state;

			//cs_temp = bb->bb_cache_state;

			if(print) printf("\nThe second time go into loop\n");
			//traverse this loop the second time
			//get the cache state of the first iteration as the input of the second
			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);
			//free(cs_temp);
			//free(cs_ptr);
			//cs_ptr = copyCacheState(bb->bb_cache_state);

			loop_level_arr[lp_level +1] = INVALID;

			//freeCacheStateLoop(p, p->loops[bb->loopid]);

			continue;
		}


		if(bb->num_incoming > 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];

#ifdef _DEBUG
		   fprintf(stdout, "Calculating bb cache state of bbid = %d of proc %d at X4\n",
					 bb->bbid, bb->pid);
#endif
			
			//printBlock(incoming_bb);
		   //if(incoming_bb->bb_cache_state == NULL) continue;
			
			cs_ptr = copyCacheState(incoming_bb->bb_cache_state);


			incoming_bb->num_outgoing--;
			if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state == 1)
			{
				incoming_bb->num_cache_state = 0;
				//freeCacheState(incoming_bb->bb_cache_state);
				//incoming_bb->bb_cache_state = NULL;
			}
			
			
			if(bb->num_incoming > 1)
			{
				if(print) printf("\ndo operations if more than one incoming edge\n");

				//dumpCacheState(cs_ptr);
				//printBlock(incoming_bb);

				for(j = 1; j < bb->num_incoming; j++)
				{
					incoming_bb = p->bblist[bb->incoming[j]];

					if(incoming_bb->bb_cache_state == NULL) continue;
					
					for(k = 0; k < cache.ns; k++)
					{
						clw = cs_ptr->must[k];							
						cs_ptr->must[k] = intersectCacheState(cs_ptr->must[k], incoming_bb->bb_cache_state->must[k]);

						freeCacheSet(clw);

						clw = cs_ptr->may[k];							
						cs_ptr->may[k] = unionCacheState(cs_ptr->may[k], incoming_bb->bb_cache_state->may[k]);

						freeCacheSet(clw);

						clw = cs_ptr->persist[k];							
						cs_ptr->persist[k] = unionMaxCacheState(cs_ptr->persist[k], incoming_bb->bb_cache_state->persist[k]);

						freeCacheSet(clw);

					}

					incoming_bb->num_outgoing--;
					if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state == 1)
					{
						incoming_bb->num_cache_state = 0;
						//freeCacheState(incoming_bb->bb_cache_state);
						//incoming_bb->bb_cache_state = NULL;
					}
	
				}	//end for all incoming
				if(print) dumpCacheState(cs_ptr);
				//cs_ptr->source_bb = NULL;
				//exit(1);
			}
		}
		


		if(bb->num_cache_state == 0)
		{
			bb->num_cache_state = 1;
		}

		if(bb->num_chmc == 0)
		{
			if(lp_level == -1)
				copies = 1;
			else
				copies = 2<<(lp_level);

			bb->num_chmc = copies;

			bb->chmc = (CHMC**)CALLOC(bb->chmc, copies, sizeof(CHMC*), "CHMC");
			for(tmp = 0; tmp < copies; tmp++)
			{
				bb->chmc[tmp] = (CHMC*)CALLOC(bb->chmc[tmp], 1, sizeof(CHMC), "CHMC");
			}

		}

#ifdef _DEBUG
		   fprintf(stdout, "Calculating bb cache state of bbid = %d of proc %d at X5\n",
					 bb->bbid, bb->pid);
#endif

		bb->bb_cache_state = cs_ptr;
		
		//tmp = cs_ptr->may[0][0]->num_entry;

		if(print)
		{
			if(cs_ptr->may[0][0]->num_entry)
			{
				tmp = cs_ptr->may[0][0]->entry[0];
				printf("\ntmp = %d\n", tmp);
			}
		}

		current_chmc = bb->chmc[cnt];

		current_chmc->hit = 0;
		current_chmc->miss = 0;
		current_chmc->unknow = 0;
		
		current_chmc->wcost= 0;
		current_chmc->bcost= 0;

		//current_chmc->hit_addr = NULL;
		//current_chmc->miss_addr = NULL;
		//current_chmc->unknow_addr = NULL;
		//current_chmc->hitmiss_addr = NULL;
		
		//compute categorization of each instruction line

		
		current_chmc->hitmiss = bb->num_instr;
		//if(current_chmc->hitmiss > 0) 
			current_chmc->hitmiss_addr = (char*)CALLOC(current_chmc->hitmiss_addr, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

		//dumpCacheState(cs_ptr);
		//exit(1);
		addr = bb->startaddr;
		for(n = 0; n < bb->num_instr ; n++)
		{
			//if(bb->is_loophead!= 0)
				//break;
			set_no = SET(addr);
			
			if(isInCache(addr, bb->bb_cache_state->must[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_HIT;
				//current_chmc->hitmiss++;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
                                
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT;
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}
			else if(isNeverInCache(addr, bb->bb_cache_state->may[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_MISS;
				//current_chmc->hitmiss++;
					
				if(current_chmc->miss == 0)
				{
					current_chmc->miss++;
					current_chmc->miss_addr = (int*)CALLOC(current_chmc->miss_addr, 1, sizeof(int), "miss_addr");
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}
				else
				{
					current_chmc->miss++;				
					current_chmc->miss_addr = (int*)REALLOC(current_chmc->miss_addr, current_chmc->miss * sizeof(int), "miss_addr");				
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}
                               
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}

			}

			else if(isInCache(addr, bb->bb_cache_state->persist[ set_no ]))
			{
				current_chmc->hitmiss_addr[ n ] = FIRST_MISS;
				//current_chmc->hitmiss++;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
                                
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}


			else
			{
				current_chmc->hitmiss_addr[ n ] = UNKNOW;
				//current_chmc->hitmiss++;
				
				if(current_chmc->unknow == 0)
				{
					current_chmc->unknow++;
					current_chmc->unknow_addr = (int*)CALLOC(current_chmc->unknow_addr, 1, sizeof(int), "unknow_addr");
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
				else
				{
					current_chmc->unknow++;				
					current_chmc->unknow_addr = (int*)REALLOC(current_chmc->unknow_addr, current_chmc->unknow * sizeof(int), "unknow_addr");				
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
                               
				addr_next = addr + INSN_SIZE;
				tag = SET(addr);
				tag_next = SET(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;

					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(current_chmc->hit == 0)
					{
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}

					addr = addr_next;
					addr_next = addr + INSN_SIZE;

					tag = SET(addr);
					tag_next = SET(addr_next);
				}
				
			}
			addr = addr + INSN_SIZE;

		}

		//exit(1);
		//compute wcet and bcet of this bb
		//current_chmc->wcost = current_chmc->hit * IC_HIT + (current_chmc->hitmiss - current_chmc->hit) * IC_MISS;
		//current_chmc->bcost  = current_chmc->miss * IC_MISS + (current_chmc->hitmiss - current_chmc->miss) * IC_HIT;

	if(print){
		
		for(k = 0; k < current_chmc->hitmiss; k++)
			printf("L1: %d ", current_chmc->hitmiss_addr[k]);
		printf("\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
		printf("hit = %d, miss= %d, unknow = %d\n", current_chmc->hit, current_chmc->miss, current_chmc->unknow);
		printf("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);	
	}
	//scanf("%c", &tmp);
		
		//dumpCacheState(bb->bb_cache_state );
		//exit(1);
		//dumpCacheState(bb->bb_cache_state);
		
	
		//int start_addr = bb->startaddr;
		//bb->num_cache_fetch = bb->size / cache.ls;

		
		//int start_addr_fetch = (start_addr >> cache.lsb) << cache.lsb;
		//tmp = start_addr - start_addr_fetch;

		//tmp is not 0 if start address is not multiple of sizeof(cache line) in bytes
		//if(tmp) 	
			//bb->num_cache_fetch++;
	
		//check the bb if it is a function call

	
		if(bb->callpid != -1)
		{
			//printf("\nbb is a  function: %d\n", bb->callpid);
			//addr = start_addr_fetch;
			addr = bb->startaddr;

			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);

				//dumpCacheState(bb->bb_cache_state,cnt);
				//if(bb->hitmiss[j] == ALWAYS_MISS)
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
				//cs_ptr =bb->bb_cache_state;
				//dumpCacheState(bb->bb_cache_state);
				//exit(1);
				addr = addr + INSN_SIZE;

			}
			
			//free(cs_ptr);
			cs_ptr = bb->bb_cache_state;			
			//cs_temp = bb->bb_cache_state;

			bb->bb_cache_state = copyCacheState(mapFunctionCall(bb->proc_ptr, cs_ptr));
			//free(cs_temp);
			freeCacheState(cs_ptr);
			cs_ptr = NULL;
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;
			
			//freeCacheStateFunction(bb->proc_ptr);

			
			if(print) dumpCacheState(cs_ptr);	
			//exit(1);
		}	
		else
		{
			//printf("\nbb is a  simple bb\n");			
			addr = bb->startaddr;
			for(j = 0; j < bb->num_instr; j ++)
			{
				set_no = SET(addr);

				//dumpCacheState(bb->bb_cache_state,cnt);
				//if(bb->hitmiss[j] == ALWAYS_MISS)
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
				//cs_ptr = bb->bb_cache_state;
				//dumpCacheState(bb->bb_cache_state, cnt);
				//exit(1);
				addr = addr + INSN_SIZE;
			}
			
			//free(cs_ptr);
			//cs_ptr = bb->bb_cache_state;
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;
			if(print) dumpCacheState(cs_ptr);
		}// end if else

	}

	for(i = 0; i < p->num_bb; i ++)
		p->bblist[i]->num_outgoing = p->bblist[i]->num_outgoing_copy;
	
	return p ->bblist[ p->topo[0]->bbid]->bb_cache_state;
}


//do level one cache analysis
static void
cacheAnalysis(){
	int i;
	//printf("\nIn cacheAnalysis() \n It is in main\n");

//loop_level_arr[] used to indicate what are the context of this bb
	loop_level_arr = NULL;
	loop_level_arr = (int*)CALLOC(loop_level_arr, MAX_NEST_LOOP, sizeof(int), "loop_level_arr");

	instr_per_block = cache.ls / INSN_SIZE;
	//not in loop for main, so all elements are invalid
	for(i = 0; i < MAX_NEST_LOOP; i++)
		loop_level_arr[i] = INVALID;

	//set initial cache state for main precedure
	cache_state *start_CS;
	//start = (cache_state*) CALLOC(start, 1, sizeof(cache_state), "cache_state");
	start_CS = allocCacheState();

	
	start_CS = mapFunctionCall(main_copy, start_CS);

	//printf("\nThis the Cache State for main\n");
	//dumpCacheState(start);
	//exit(1);//!!!here is a break !!! come on
}

/*
static void
computeBBcost(){
}
*/
static char 
isInWay(int entry, int *entries, int num_entry)
{
	int i;
	for(i = 0; i < num_entry; i++)
		if(entry == entries[i]) 
			return 1;

	return 0;
}


// from way 0-> way n, younger->older
static cache_line_way_t **
unionCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_a, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}

	//for each way, calculate the result of cs
	for(i = 0; i < cache.na; i++)
	{
		//for each entry in clw_a, is it in the clw_b? 
		//no, add it into result and history
		for(j = 0 ; j < clw_a[i]->num_entry; j++)
		{
			entry_a = clw_a[i]->entry[j];

			age = isInSet(entry_a, clw_b);
			if(age == -1) index = i;
			else if(age < i) index = age;
			/* sudiptac ::: FIXME: all cases covered right ? */
			else
				index = i;	 

			//add to result
			result[index]->num_entry++;
			if(result[index]->num_entry == 1)
			{
				result[index]->entry = (int *)CALLOC(result[index]->entry, result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				result[index]->entry = (int *)REALLOC(result[index]->entry, result[index]->num_entry * sizeof(int), "cache line way");
			}
			result[index]->entry[result[index]->num_entry - 1] = entry_a;

		} //end for


		//for each entry in clw_b[i], is it in result[i]
		//no, add it into result[i]
		for(j = 0; j < clw_b[i]->num_entry; j++)
		{
			entry_b = clw_b[i]->entry[j];
			if(isInSet(entry_b, clw_a) != -1)
				continue;
			else
			{
				result[i]->num_entry ++;
				if(result[i]->num_entry == 1)
				{
					result[i]->entry = (int *)CALLOC(result[i]->entry, result[i]->num_entry, sizeof(int), "cache line way");
				}
				else
				{
					result[i]->entry = (int *)REALLOC(result[i]->entry, result[i]->num_entry*sizeof(int), "cache line way");
				}
				result[i]->entry[result[i]->num_entry - 1] = entry_b;

			 } //end if
		} //end for

	}
	return result;
	
}




// from way n +1 -> way 0, older->younger
static cache_line_way_t **
unionMaxCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_a, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache.na + 1, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na + 1; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}

	//for each way, calculate the result of cs
	for(i = cache.na; i >= 0; i--)
	{
		//for each entry in clw_a, is it in the clw_b? 
		//no, add it into result 
		for(j = 0 ; j < clw_a[i]->num_entry; j++)
		{
			entry_a = clw_a[i]->entry[j];

		   /* FIXME: For persistence we need to check through the 
			 * associativity of the set plus one more. Thus a different
			 * function is needed to check the presence of a memory block 
			 * in a cache set */
			age = isInSet_persist(entry_a, clw_b);
			//only in clw_a
			if(age == -1) index = i;

			//both in clw_a and clw_b
			else if(age > i) index = age;
			/* sudiptac ::: FIXME: all cases covered right ? */
			else
				index = i;	 

			//add to result
			result[index]->num_entry++;
			if(result[index]->num_entry == 1)
			{
				result[index]->entry = (int *)CALLOC(result[index]->entry, result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				result[index]->entry = (int *)REALLOC(result[index]->entry, result[index]->num_entry * sizeof(int), "cache line way");
			}
			result[index]->entry[result[index]->num_entry - 1] = entry_a;

		} //end for


		//for each entry in clw_b[i], is it in clw_a
		//no, add it into result[i]
		for(j = 0; j < clw_b[i]->num_entry; j++)
		{
			entry_b = clw_b[i]->entry[j];
			if(isInSet_persist(entry_b, clw_a) != -1)
				continue;
			
			//only in clw_b
			else
			{
				result[i]->num_entry ++;
				if(result[i]->num_entry == 1)
				{
					result[i]->entry = (int *)CALLOC(result[i]->entry, result[i]->num_entry, sizeof(int), "cache line way");
				}
				else
				{
					result[i]->entry = (int *)REALLOC(result[i]->entry, result[i]->num_entry*sizeof(int), "cache line way");
				}
				result[i]->entry[result[i]->num_entry - 1] = entry_b;

			 } //end if
		} //end for

	}
	return result;
	
}



static void
freeCacheSet(cache_line_way_t **cache_set)
{
	int i;
	for(i = 0; i < cache.na; i++)
	{
		if(cache_set[i]->num_entry)
			free(cache_set[i]->entry);
		free(cache_set[i]);
	}

	free(cache_set);

}


static void
freeCacheState(cache_state *cs)
{
	int i, j;
	
	for(i = 0; i < cache.ns; i++ )
	{
		for(j = 0; j < cache.na; j++)
		{
			if(cs->must[i][j]->num_entry)
				free(cs->must[i][j]->entry);

			if(cs->may[i][j]->num_entry)
				free(cs->may[i][j]->entry);

			if(cs->persist[i][j]->num_entry)
				free(cs->persist[i][j]->entry);

			free(cs->must[i][j]);
			free(cs->may[i][j]);
			free(cs->persist[i][j]);
	
		}

		if(cs->persist[i][cache.na]->num_entry)
			free(cs->persist[i][cache.na]->entry);

		free(cs->persist[i][cache.na]);
	}
		
	for(i = 0; i < cache.ns; i++ )
	{
		free(cs->must[i]);
		free(cs->may[i]);
		free(cs->persist[i]);
	}
	free(cs->must);
	free(cs->may);
	free(cs->persist);
	
	free(cs);
	cs = NULL;	
}

static void
freeCacheStateFunction(procedure * proc)
{
	procedure *p = proc;
	block *bb;
	int i;

	int  num_blk = p->num_topo;
	


	for(i = num_blk -1 ; i >= 0 ; i--)
	{

		bb = p ->topo[i];

		bb = p->bblist[bb->bbid];

		bb->num_cache_state = 0;
		if(bb->bb_cache_state!= NULL)
		{
			freeCacheState(bb->bb_cache_state);	
			bb->bb_cache_state = NULL;
		}
	}

	
}

static void
freeCacheStateLoop(procedure *proc, loop *lp)
{
	procedure *p = proc;
	loop *loop_ptr = lp;
	block *bb;
	int i;

	int  num_blk = loop_ptr->num_topo;

	for(i = num_blk -1 ; i >= 0 ; i--)
	{
		bb = loop_ptr->topo[i];

		bb = p->bblist[bb->bbid];

		if(bb->is_loophead && i == num_blk -1) continue;
		
		bb->num_cache_state = 0;
		if(bb->bb_cache_state != NULL)
		{
			freeCacheState(bb->bb_cache_state);	
			bb->bb_cache_state = NULL;
		}
	}
}


static void
freeAllFunction(procedure *proc)
{
	procedure *p = proc;
	block *bb;
	int i;
	int  num_blk = p->num_topo;	
		
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		bb = p ->topo[i];
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead)  freeAllLoop(p, p->loops[bb->loopid]);
		else if(bb->callpid != -1)
		{
			bb->num_cache_state = 0;
			if(bb->bb_cache_state != NULL)
			{
				freeCacheState(bb->bb_cache_state);
				bb->bb_cache_state = NULL;
			}
			freeAllFunction(bb->proc_ptr);
		}
		else
		{
			bb->num_cache_state = 0;
			if(bb->bb_cache_state != NULL)
			{
				free(bb->bb_cache_state);
				bb->bb_cache_state = NULL;
			}
		}
	}

}

static void
freeAllLoop(procedure *proc, loop *lp)
{
	procedure *p = proc;
	loop *lp_ptr = lp;
	block *bb;
	int i;
	
	int  num_blk = lp_ptr->num_topo;	
		
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		bb = lp_ptr ->topo[i];
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead && i!= num_blk -1)  freeAllLoop(p, p->loops[bb->loopid]);
		else if(bb->callpid != -1)
		{
			bb->num_cache_state = 0;
			if(bb->bb_cache_state != NULL)
			{
				freeCacheState(bb->bb_cache_state);
				bb->bb_cache_state = NULL;
			}
			freeAllFunction(bb->proc_ptr);
		}
		else
		{
			bb->num_cache_state = 0;
			if(bb->bb_cache_state != NULL)
			{
				freeCacheState(bb->bb_cache_state);
				bb->bb_cache_state = NULL;
			}
		}
	}

}

static void
freeAllCacheState()
{
	freeAllFunction(main_copy);
}


// from way n-> way 0, older->younger
static cache_line_way_t **
intersectCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}


	//for each way, calculate the result of cs
	for(i = cache.na - 1; i >= 0; i--)
	{
		for(j = 0; j < clw_b[i]->num_entry; j++)
		{
			entry_b = clw_b[i]->entry[j];
			age = isInSet(entry_b, clw_a);
			
			//kick out entries in clw_b not in clw_a
			if(age == -1)
				continue;

			//in both, get the older age
			if(age > i) index = age;
			else index = i;
			//add  clw_a[i].entry[j] into result[i].entry
			result[index]->num_entry ++;
			if(result[index]->num_entry == 1)
			{
				result[index]->entry = (int *) CALLOC(result[index]->entry , result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				result[index]->entry = (int *) REALLOC(result[index]->entry , result[index]->num_entry*sizeof(int), "cache line way");
			}
			result[index]->entry[result[index]->num_entry - 1] = entry_b;

		} // end for

	}
	return result;
}

static void
dump_cache_line(cache_line_way_t *clw_a)
{
	int j;
	//printf("No of ways %d\n", cache.na);
	if(clw_a->num_entry == 0)
	{
		printf("NULL\n");
		return;
	}
	for(j = 0; j < clw_a->num_entry; j++)
		printf(" %d ", clw_a->entry[j]);

	printf("\n");
	
}

static char test_cs_op()
{
  cache_line_way_t **clw_a, **clw_b;
  cache_line_way_t** temp;

  //initialization for clw_a
  clw_a = (cache_line_way_t**) CALLOC(clw_a , 1, sizeof(cache_line_way_t), "a");
  clw_a[0] = (cache_line_way_t*) CALLOC(clw_a , 2, sizeof(cache_line_way_t), "a");

  clw_a[0][0].num_entry = 3;
  clw_a[0][1].num_entry = 3;
  clw_a[0][0].entry = (int*) CALLOC(clw_a[0][0].entry, clw_a[0][0].num_entry, sizeof(int), "entries for a");
  clw_a[0][1].entry = (int*) CALLOC(clw_a[0][1].entry, clw_a[0][1].num_entry, sizeof(int), "entries for a");

  clw_a[0][0] .entry[0] = 1;
  clw_a[0][0] .entry[1] = 2;
  clw_a[0][0] .entry[2] = 5;
  clw_a[0][1] .entry[0] = 2;
  clw_a[0][1] .entry[1] = 3;
  clw_a[0][1] .entry[2] = 4;
  //initialization for clw_b
  clw_b = (cache_line_way_t**) CALLOC(clw_b , 1, sizeof(cache_line_way_t), "b");
  clw_b[0] = (cache_line_way_t*) CALLOC(clw_b , 3, sizeof(cache_line_way_t), "b");

  clw_b[0][0].num_entry = 3;
  clw_b[0][1].num_entry = 3;
  clw_b[0][0].entry = (int*) CALLOC(clw_b[0][0].entry, clw_b[0][0].num_entry, sizeof(int), "entries for b");
  clw_b[0][1].entry = (int*) CALLOC(clw_b[0][1].entry, clw_b[0][1].num_entry, sizeof(int), "entries for b");

  clw_b[0][0] .entry[0] = 3;
  clw_b[0][0] .entry[1] = 2;
  clw_b[0][0] .entry[1] = 5;
  clw_b[0][1] .entry[0] = 4;
  clw_b[0][1] .entry[1] = 3;
  clw_b[0][1] .entry[2] = 2;
  dump_cache_line(clw_a[0]);
  printf("\n");
  dump_cache_line(clw_b[0]);
  printf("\n");
  printf("Result for union: \n");
  temp = unionCacheState(clw_a, clw_b);
  dump_cache_line(temp[0]);
  printf("\n");
  printf("Result for intersect: \n");
  temp = intersectCacheState(clw_a, clw_b);
  dump_cache_line(temp[0]);
  printf("\n");
  return 0;
}
