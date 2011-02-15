// Include standard library headers
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "analysisCache_L1.h"
#include "analysisCache_common.h"
#include "handler.h"
#include "dump.h"


// Forward declarations of static functions

static block* 
copyBlock(block *bb);

static loop* 
copyLoop(procedure *proc, procedure* copy_proc, loop *lp);

static procedure* 
copyFunction(procedure* proc);

static procedure*
constructFunctionCall(procedure *pro, task_t *task);

static void
calculateMust(cache_line_way_t **must, int instr_addr);

static void
calculateMay(cache_line_way_t **may, int instr_addr);

static void
calculatePersist(cache_line_way_t **persist, int instr_addr);

static void
calculateCacheState(cache_line_way_t **must, cache_line_way_t **may,
    cache_line_way_t **persist, int instr_addr);

static cache_state *
allocCacheState();

static char
isInSet_persist(int addr, cache_line_way_t **set);

static char
isInSet(int addr, cache_line_way_t **set);

static char
isInCache(int addr, cache_line_way_t**must);

static char
isNeverInCache(int addr, cache_line_way_t**may);

static cache_state *
copyCacheState(cache_state *cs);

static cache_state *
mapLoop(procedure *proc, loop *lp);

static cache_state *
mapFunctionCall(procedure *proc, cache_state *cs);

static cache_line_way_t **
unionCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

static cache_line_way_t **
unionMaxCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

static void
freeCacheState(cache_state *cs);

/*static void
freeCacheStateFunction(procedure * proc);

static void
freeCacheStateLoop(procedure *proc, loop *lp);*/

static void
freeAllFunction(procedure *proc);

static void
freeAllLoop(procedure *proc, loop *lp);

static cache_line_way_t **
intersectCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);


//read basic cache configuration from configFile and then
//set other cinfiguration
void
set_cache_basic(char * configFile)
{

  FILE *fptr;
  int ns, na, ls, hit_latency, miss_latency, i;

  fptr = fopen(configFile, "r" );
  if( !fptr ) {
    fprintf(stderr, "Failed to open file: %s (analysisCache.c:123)\n", configFile);
    exit(1);
  }
  
  fscanf( fptr, "%d %d %d %d %d", &ns, &na, &ls, &hit_latency, &miss_latency );
  
    cache.ns = ns;
    cache.na = na;
    cache.ls = ls;
    cache.hit_latency  = hit_latency;
    cache.miss_latency = miss_latency;

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

void
dumpCacheConfig()
{
    printf("Cache Configuration as follow:\n");
    printf("nubmer of set:  %d\n", cache.ns);	
    printf("nubmer of associativity:    %d\n", cache.na);	
    printf("cache line size:    %d\n", cache.ls);	
    printf("cache hit penalty: %d\n", cache.hit_latency);
    printf("cache miss penalty: %d\n", cache.miss_latency);
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

	//CALLOC(lp_ptr, loop*, 1, sizeof(loop), "for loop");

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
  DSTART( "copyBlock" );

	block * src, *copy;
	src = bb;
	int i, j;

	CALLOC(copy, block*, 1, sizeof(block), "block");
	
	copy->bbid = src->bbid;
	DOUT("\nnow copying bb %d\n", copy->bbid);
	
	copy->pid= src->pid;	
	copy->startaddr= src->startaddr;
	
	copy->num_outgoing = src->num_outgoing;
	copy->num_outgoing_copy = src->num_outgoing_copy;
	
	if(copy->num_outgoing)
	{
		CALLOC(copy->outgoing, int *, copy->num_outgoing, sizeof(int), "outgoing");
		for(i = 0; i < copy->num_outgoing; i++)
			copy->outgoing[i] = src->outgoing[i];
	}
	copy->num_incoming = src->num_incoming;
	
	if(copy->num_incoming)	
	{
		CALLOC(copy->incoming, int *, copy->num_incoming, sizeof(int), "incoming");
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
	
	CALLOC(copy->instrlist, instr **, copy->num_instr, sizeof(instr *), "instruction list");
	for(i = 0; i < copy->num_instr; i++)
	{
		CALLOC(copy->instrlist[i], instr*, 1, sizeof(instr), "instruction");
		
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

	DRETURN( copy );
}


static loop* 
copyLoop(procedure *proc, procedure* copy_proc, loop *lp)
{
  DSTART( "copyLoop" );

	loop *src, *copy;
	src = lp;
	int i;

	CALLOC(copy, loop *, 1, sizeof(loop), "loop");
	copy->lpid = src->lpid;
	DOUT("\nnow copying loop %d\n", copy->lpid);
	
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
	
	copy->num_exits = src->num_exits;
	CALLOC( copy->exits, block**, src->num_exits, sizeof( block* ), "exits" );
	for ( i = 0; i < src->num_exits; i++ ) {
	  copy->exits[i] = copy_proc->bblist[src->exits[i]->bbid];
	}

	copy->level = src->level;
	copy->nest = src->nest;
	copy->is_dowhile= src->is_dowhile;
	
	copy->num_topo = src->num_topo;
	if(copy->num_topo)
	{
		CALLOC(copy->topo, block**, copy->num_topo, sizeof(block*), "topo");
		for(i = 0; i < copy->num_topo; i++)
			//copy->topo[i] = copyBlock(procs[src->pid]->bblist[src->topo[i]->bbid]);
			copy->topo[i] = copy_proc->bblist[src->topo[i]->bbid];
	}
	copy->num_cost = src->num_cost;
//	copy->wpath = src->wpath; 		//???

	DRETURN( copy );
}


static procedure* 
copyFunction(procedure* proc)
{
  DSTART( "copyFunction" );

	procedure *src, *copy;
	src = proc;
	int i;

	CALLOC(copy, procedure*, 1, sizeof(procedure), "procedure");
	copy->pid = src->pid;

	DOUT("\nnow copying procdure %d\n", copy->pid);
	
	copy->num_bb = src->num_bb;	
	if(copy->num_bb)
	{
		CALLOC(copy->bblist, block**, copy->num_bb, sizeof(block*), "block*");
		for(i = 0; i < copy->num_bb; i++)
			copy->bblist[i] = copyBlock(src->bblist[i]);
	}
	copy->num_loops = src->num_loops;
	if(copy->num_loops)
	{
		CALLOC(copy->loops, loop**, copy->num_loops, sizeof(loop*), "loops");
		for(i = 0; i < copy->num_loops; i++)
			copy->loops[i] = copyLoop(proc, copy, src->loops[i]);
	}
	
	copy->num_calls = src->num_calls;
	if(copy->num_calls)
	{
		CALLOC(copy->calls, int *, copy->num_calls, sizeof(int), "calls");
		for(i = 0; i < copy->num_calls; i++)
			copy->calls[i] = src->calls[i];
	}
	
	copy->num_topo = src->num_topo;
	if(copy->num_topo)
	{
		CALLOC(copy->topo, block**, copy->num_topo, sizeof(block*), "topo");
		for(i = 0; i < copy->num_topo; i++)
			//copy->topo[i] = copyBlock(src->bblist[src->topo[i]->bbid]);
			copy->topo[i] = copy->bblist[src->topo[i]->bbid];
	}
	copy->wcet = src->wcet;

//	copy->wpath = src->wpath;  	//???

	DRETURN( copy );
}


static procedure*
constructFunctionCall(procedure *pro, task_t *task)
{
  DSTART( "constructFunctionCall" );

  procedure *p = copyFunction(pro);
	//loop *lp;
	block * bb;
	int num_blk = p->num_bb;
	int i, j;

	j = indexOfProc(p->pid);
	if(j >= 0)
	{
		if(task->proc_cg_ptr[j].num_proc == 0)
		{
			task->proc_cg_ptr[j].num_proc = 1;
			CALLOC(task->proc_cg_ptr[j].proc, procedure**, 1, sizeof(procedure*), "procedure*");
			task->proc_cg_ptr[j].proc[0] = p;
		}
		else	
		{
			task->proc_cg_ptr[j].num_proc ++;
			REALLOC(task->proc_cg_ptr[j].proc, procedure**, task->proc_cg_ptr[j].num_proc * sizeof(procedure*), "procedure*");
			task->proc_cg_ptr[j].proc[task->proc_cg_ptr[j].num_proc -1] = p;
		}

	}

	else
	{
		DOUT("\nprocedure does not exsit!exit\n");
		exit(1);
	}
	
	for(i = 0 ; i < num_blk; i++)
	{
		bb = p ->bblist[i];
			
		if(bb->callpid != -1)
		{
	
			bb->proc_ptr	= constructFunctionCall(procs[bb->callpid], task);
			DOUT("\nbb->proc_ptr = f%lx for bb[%d] calling proc[%d]\n", (uintptr_t)bb->proc_ptr, bb->bbid, bb->callpid);
/*
			j = indexOfProc(bb->callpid);
			if(j >= 0)
			{
				if(proc_cg_ptr[j].num_proc == 0)
				{
					proc_cg_ptr[j].num_proc = 1;
					CALLOC(proc_cg_ptr[j].proc, procedure**, 1, sizeof(procedure*), "procedure*");
					proc_cg_ptr[j].proc[0] = bb->proc_ptr;
				}
				else	
				{
					proc_cg_ptr[j].num_proc ++;
					REALLOC(proc_cg_ptr[j].proc, procedure**, proc_cg_ptr[j].num_proc * sizeof(procedure*), "procedure*");
					proc_cg_ptr[j].proc[proc_cg_ptr[j].num_proc -1] = bb->proc_ptr;
				}
			}
*/
			DOUT("\nCopy over\n");
		}	

	}
	DRETURN( p );
}





void
constructAll(task_t *task)
{
  procedure *p = procs[ main_id ];
	main_copy = constructFunctionCall( p, task);
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
				if(must[0]->num_entry == 1) {
          CALLOC(must[0]->entry, int*, must[0]->num_entry, sizeof(int), "enties");
        } else {
          REALLOC(must[0]->entry, int*, (must[0]->num_entry) * sizeof(int), "enties");
        }

				must[0]->entry[(must[0]->num_entry) -1] = instr_addr;

				
				for(j = 0; j < must[i]->num_entry; j++)
				{
					if(must[i]->entry[j] == instr_addr)
					{
						if(j != must[i]->num_entry -1)
							must[i]->entry[j] = must[i]->entry[must[i]->num_entry -1];
						must[i]->num_entry --;
						if(must[i]->num_entry == 0) {
              FREE(must[i]->entry);
						} else {
              REALLOC(must[i]->entry, int*, must[i]->num_entry * sizeof(int), "entry");
						}
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

	FREE(tmp);
	
	CALLOC(head, cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	CALLOC(head->entry, int*, 1, sizeof(int), "in head->entry");
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
					CALLOC(may[0]->entry, int*, may[0]->num_entry, sizeof(int), "enties");
				}
				else	
				{
					REALLOC(may[0]->entry, int*, (may[0]->num_entry) * sizeof(int), "enties");
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
							FREE(may[i]->entry);
						}
						else
						{
							REALLOC(may[i]->entry, int*, may[i]->num_entry * sizeof(int), "entry");
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
	FREE(tmp);


	CALLOC(head, cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	CALLOC(head->entry, int*, 1, sizeof(int), "in head->entry");
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
					CALLOC(persist[0]->entry, int*, persist[0]->num_entry, sizeof(int), "enties");
				}
				else	
				{
					REALLOC(persist[0]->entry, int*, (persist[0]->num_entry) * sizeof(int), "enties");
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
							FREE(persist[i]->entry);
						}
						else
						{
							REALLOC(persist[i]->entry, int*, persist[i]->num_entry * sizeof(int), "entry");
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
	FREE(tmp);


	CALLOC(head, cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	CALLOC(head->entry, int*, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	persist[0] = head;
}



static void
calculateCacheState(cache_line_way_t **must, cache_line_way_t **may, cache_line_way_t **persist, int instr_addr)
{
	calculateMust(must, instr_addr);
	calculateMay(may, instr_addr);
	calculatePersist(persist, instr_addr);
}


//allocate the memory for cache_state
static cache_state *
allocCacheState()
{
  DSTART( "allocCacheState" );

	int j, k;
	cache_state *result = NULL;

	CALLOC(result, cache_state*, 1, sizeof(cache_state), "cache_state");
	
		result->must = NULL;
		CALLOC(result->must, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->may = NULL;
		CALLOC(result->may, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->persist = NULL;
		CALLOC(result->persist, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	
		for(j = 0; j < cache.ns; j++)
		{
			DOUT("\nalloc CS memory for j = %d \n", j );
			CALLOC(result->must[j], cache_line_way_t**, cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			CALLOC(result->may[j], cache_line_way_t**, cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			CALLOC(result->persist[j], cache_line_way_t**, cache.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			for( k = 0; k < cache.na; k++)
			{
				DOUT("\nalloc CS memory for k = %d \n", k );
				CALLOC(result->must[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->must[j][k]->num_entry = 0;
				result->must[j][k]->entry = NULL;

				CALLOC(result->may[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->may[j][k]->num_entry = 0;
				result->may[j][k]->entry = NULL;

				CALLOC(result->persist[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				result->persist[j][k]->num_entry = 0;
				result->persist[j][k]->entry = NULL;
			}
			
			CALLOC(result->persist[j][cache.na], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t");
			result->persist[j][cache.na]->num_entry = 0;
			result->persist[j][cache.na]->entry = NULL;

		}

	DRETURN( result );
}


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

	CALLOC(copy, cache_state*, 1, sizeof(cache_state), "cache_state");
	copy->must = NULL;
	copy->may = NULL;
	copy->persist = NULL;

	//copy->chmc = NULL;
		
	//lp_level = cs->loop_level;
	
	CALLOC(copy->must, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	CALLOC(copy->may, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");
	CALLOC(copy->persist, cache_line_way_t***, cache.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

	for(j = 0; j < cache.ns; j++)
	{
     CALLOC(copy->must[j], cache_line_way_t**, cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");
		 CALLOC(copy->may[j], cache_line_way_t**, cache.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");
		 CALLOC(copy->persist[j], cache_line_way_t**, cache.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");


		for( k = 0; k < cache.na; k++)
		{
			CALLOC(copy->must[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t must");
			CALLOC(copy->may[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t may");
			CALLOC(copy->persist[j][k], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t may");

			copy->must[j][k]->num_entry = cs->must[j][k]->num_entry;
			if(copy->must[j][k]->num_entry)
			{
				CALLOC(copy->must[j][k]->entry, int*, copy->must[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->must[j][k]->num_entry; num_entry++)
					copy->must[j][k]->entry[num_entry] =  cs->must[j][k]->entry[num_entry];
			}

			copy->may[j][k]->num_entry = cs->may[j][k]->num_entry;
			if(copy->may[j][k]->num_entry)
			{
				CALLOC(copy->may[j][k]->entry, int*, copy->may[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->may[j][k]->num_entry; num_entry++)
					copy->may[j][k]->entry[num_entry] =  cs->may[j][k]->entry[num_entry];
			}


			copy->persist[j][k]->num_entry = cs->persist[j][k]->num_entry;
			if(copy->persist[j][k]->num_entry)
			{
				CALLOC(copy->persist[j][k]->entry, int*, copy->persist[j][k]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->persist[j][k]->num_entry; num_entry++)
					copy->persist[j][k]->entry[num_entry] =  cs->persist[j][k]->entry[num_entry];
			}
			
		}


		CALLOC(copy->persist[j][cache.na], cache_line_way_t*, 1, sizeof(cache_line_way_t), "one cache_line_way_t may");
		copy->persist[j][cache.na]->num_entry = cs->persist[j][cache.na]->num_entry;
		if(copy->persist[j][cache.na]->num_entry)
		{
			CALLOC(copy->persist[j][cache.na]->entry, int*, copy->persist[j][cache.na]->num_entry, sizeof(int), "entries");

			for(num_entry = 0; num_entry < copy->persist[j][cache.na]->num_entry; num_entry++)
				copy->persist[j][cache.na]->entry[num_entry] =  cs->persist[j][cache.na]->entry[num_entry];
		}

	}
	return copy;
}



static cache_state *
mapLoop(procedure *proc, loop *lp)
{
  DSTART( "mapLoop" );

	int i, j, k, n, set_no, cnt, tmp, addr, addr_next, copies, tag, tag_next; 
	int lp_level;

	procedure *p = proc;
	block *bb, *incoming_bb;
	cache_state *cs_ptr;
	cache_line_way_t **clw;
	DOUT("\nIn mapLoop loopid[%d]\n", lp->lpid);

	DACTION(
      printProc(p);
      printLoop(lp);
  );

	int  num_blk = lp->num_topo;

	//cs_ptr = copyCacheState(cs);
	//cs_ptr->source_bb = NULL;
	
	CHMC *current_chmc;

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

		DOUT("deal with bb[ %d ] in proc[%d]\n", bb->bbid, bb->pid);
		
		if(bb->is_loophead && i != num_blk -1)
		{
			DOUT("\nbb is a loop head in proc[%d]\n", bb->pid);

		  DOUT("\nthe first time go into loop %d\n", bb->loopid);
			loop_level_arr[lp_level + 1] = FIRST_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);

			//cs_ptr = bb->bb_cache_state;
		 
			DOUT("\nthe second time go into loop %d\n", bb->loopid);
			loop_level_arr[lp_level + 1] = NEXT_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level + 1] = INVALID;
			
			DACTION(
			    dumpCacheState(cs_ptr);
			);

			continue;
		}

		DOUT("\nbb->num_incoming = %d\n", bb->num_incoming);
		//if more than one incoming do operation of cache state  
		//as the input of this bb

		if(bb->is_loophead == 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];

		   DOUT( "Calculating bb cache state of bbid = %d of proc = %d at X1\n",
					 bb->bbid, bb->pid);
		   
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
				DOUT("\ndo operations if more than one incoming edge\n");
				DACTION( dumpCacheState(cs_ptr); );
				
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
		   DOUT( "Calculating bb cache state of bbid = %d of proc %d at X2\n",
					 bb->bbid, bb->pid);
				incoming_bb = p->bblist[bb->incoming[0]];

				//free(cs_ptr);
			
				cs_ptr = copyCacheState(incoming_bb->bb_cache_state);

				if(bb->num_incoming > 1)
				{
					DOUT( "\ndo operations if more than one incoming edge\n");
					DACTION( dumpCacheState(cs_ptr); );
					
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
						FREE(clw);

						clw = bb->bb_cache_state_result->may[k][n];
						bb->bb_cache_state_result->may[k][n] = unionCacheState(clw,  cs_ptr->may[k][n]);
						FREE(clw);
					}

				p->bblist[lp->topo[0]->bbid]->num_cache_state = 0;
				FREE(p->bblist[lp->topo[0]->bbid]->bb_cache_state);
				p->bblist[lp->topo[0]->bbid]->bb_cache_state = NULL;
			*/
			}
			else
			{
				DOUT("\nCFG error!\n");
				exit(1);
			}
		}


		DOUT("\ncnt = %d\n", cnt);
		DACTION( dumpCacheState(cs_ptr); );
		//initialize the output cache state of this bb
		//using the input cache state
	

		if(bb->num_cache_state == 0)
		{
			//CALLOC(bb->bb_cache_state, cache_state *, 1, sizeof(cache_state), "cache_state");

			bb->num_cache_state = 1;
		}

		if(bb->num_chmc == 0)
		{
			DOUT("\nallocation for bb in lp[%d]\n", bb->loopid);
			copies = 2<<(lp_level);

			bb->num_chmc = copies;

			CALLOC(bb->chmc, CHMC**, copies, sizeof(CHMC*), "CHMC");
			for(tmp = 0; tmp < copies; tmp++)
			{
				CALLOC(bb->chmc[tmp], CHMC*, 1, sizeof(CHMC), "CHMC");
			}

		}


		bb->bb_cache_state = cs_ptr;

		DOUT( "Calculating bb cache state of bbid = %d of proc %d at X3\n", bb->bbid, bb->pid);
		
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

		DOUT("num_cache_fetch =  %d\n", bb->num_cache_fetch);

		current_chmc->hitmiss = bb->num_instr;
		//if(current_chmc->hitmiss > 0)
			CALLOC(current_chmc->hitmiss_addr, char*, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

		DACTION( dumpCacheState(cs_ptr) );

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
					CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->miss_addr, int*, 1, sizeof(int), "miss_addr");
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}
				else
				{
					current_chmc->miss++;				
					REALLOC(current_chmc->miss_addr, int*, current_chmc->miss * sizeof(int), "miss_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->unknow_addr, int*, 1, sizeof(int), "unknow_addr");
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
				else
				{
					current_chmc->unknow++;				
					REALLOC(current_chmc->unknow_addr, int*, current_chmc->unknow * sizeof(int), "unknow_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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

		//current_chmc->wcost = current_chmc->hit * IC_HIT + (current_chmc->hitmiss - current_chmc->hit) * IC_MISS;
		//current_chmc->bcost  = current_chmc->miss * IC_MISS + (current_chmc->hitmiss - current_chmc->miss) * IC_HIT;

	DOUT("\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
	DOUT("hit = %d, miss= %d, unknow = %d\n", current_chmc->hit, current_chmc->miss, current_chmc->unknow);
  DOUT("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);

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
			DOUT("\nbb is a function call\n");
			//addr = start_addr_fetch;
			addr = bb->startaddr;
			
			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
				addr = addr + INSN_SIZE;
			}
			cs_ptr = bb->bb_cache_state;
			bb->bb_cache_state = copyCacheState(mapFunctionCall(bb->proc_ptr, cs_ptr));
			freeCacheState(cs_ptr);
			cs_ptr = NULL;
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;

			//freeCacheStateFunction(bb->proc_ptr);
		}	
		else
		{
			DOUT("\nbb is a simple bb \n");
			addr = bb->startaddr;
			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
				addr = addr + INSN_SIZE;
			}
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;
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
				FREE(clw);

				clw = bb->bb_cache_state_result->may[k][n];
				bb->bb_cache_state_result->may[k][n] = unionCacheState(clw,  cs_ptr->may[k][n]);
				FREE(clw);
			}
			
		FREE(cs_ptr);
		cs_ptr = bb->bb_cache_state;
		bb->bb_cache_state = bb->bb_cache_state_result;
		FREE(cs_ptr);
	}
*/	
	DRETURN( NULL );
}


static cache_state *
mapFunctionCall(procedure *proc, cache_state *cs)
{
  DSTART( "mapFunctionCall" );

	int i, j, k, n, set_no, cnt, addr, addr_next, copies, tmp; 
	int tag, tag_next, lp_level;

	procedure *p = proc;
	block *bb, *incoming_bb;
	cache_line_way_t **clw;
	CHMC *current_chmc;

	cache_state *cs_ptr = copyCacheState(cs);
	
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

    DOUT("\nThis is  bb %d in proc %d\n", bb->bbid, proc->pid);
    DACTION(
        if(bb->callpid!= -1)
          printBlock(bb);
    );
		
		DOUT("\nbb->num_incoming =  %d\n", bb->num_incoming);
		
		//if more than one incoming do operation of cache state  
		//as the input of this bb

		if(bb->is_loophead)
		{

			DOUT("\nbb is a  loop head in loop[%d]\n", bb->loopid);
			DOUT("\nloop level = %d\n", lp_level);
			DACTION(
			    printBlock(bb);
          printLoop(p->loops[bb->loopid]);
      );
			//traverse this loop the first time
			loop_level_arr[lp_level +1] = FIRST_ITERATION;
			
			p->loops[bb->loopid]->num_fm = 0;
			
      DOUT("\nThe first time go into loop\n");
			mapLoop(p, p->loops[bb->loopid]);
			//FREE(cs_ptr);
			//cs_ptr = bb->bb_cache_state;

			//cs_temp = bb->bb_cache_state;

			DOUT("\nThe second time go into loop\n");
			//traverse this loop the second time
			//get the cache state of the first iteration as the input of the second
			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			mapLoop(p, p->loops[bb->loopid]);
			//FREE(cs_temp);
			//FREE(cs_ptr);
			//cs_ptr = copyCacheState(bb->bb_cache_state);

			loop_level_arr[lp_level +1] = INVALID;

			//freeCacheStateLoop(p, p->loops[bb->loopid]);

			continue;
		}


		if(bb->num_incoming > 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];

		   DOUT( "Calculating bb cache state of bbid = %d of proc %d at X4\n",
					 bb->bbid, bb->pid);
			
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
        DOUT("\ndo operations if more than one incoming edge\n");

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
				DACTION( dumpCacheState(cs_ptr); );
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

			CALLOC(bb->chmc, CHMC**, copies, sizeof(CHMC*), "CHMC");
			for(tmp = 0; tmp < copies; tmp++)
			{
				CALLOC(bb->chmc[tmp], CHMC*, 1, sizeof(CHMC), "CHMC");
			}

		}

		DOUT( "Calculating bb cache state of bbid = %d of proc %d at X5\n",
		      bb->bbid, bb->pid);

		bb->bb_cache_state = cs_ptr;
		
		//tmp = cs_ptr->may[0][0]->num_entry;

		if(cs_ptr->may[0][0]->num_entry) {
		  DOUT("\ntmp = %d\n", cs_ptr->may[0][0]->entry[0]);
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
			CALLOC(current_chmc->hitmiss_addr, char*, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

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
					CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->miss_addr, int*, 1, sizeof(int), "miss_addr");
					current_chmc->miss_addr[current_chmc->miss-1] = addr;
				}
				else
				{
					current_chmc->miss++;				
					REALLOC(current_chmc->miss_addr, int*, current_chmc->miss * sizeof(int), "miss_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
					current_chmc->hit_addr[current_chmc->hit-1] = addr;
				}
				else
				{
					current_chmc->hit++;				
					REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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
					CALLOC(current_chmc->unknow_addr, int*, 1, sizeof(int), "unknow_addr");
					current_chmc->unknow_addr[current_chmc->unknow-1] = addr;
				}
				else
				{
					current_chmc->unknow++;				
					REALLOC(current_chmc->unknow_addr, int*, current_chmc->unknow * sizeof(int), "unknow_addr");				
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
						CALLOC(current_chmc->hit_addr, int*, 1, sizeof(int), "hit_addr");
						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
					}
					else
					{
						current_chmc->hit++;				
						REALLOC(current_chmc->hit_addr, int*, current_chmc->hit * sizeof(int), "hit_addr");				
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

	for(k = 0; k < current_chmc->hitmiss; k++)
		DOUT("L1: %d ", current_chmc->hitmiss_addr[k]);
	DOUT("\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
	DOUT("hit = %d, miss= %d, unknow = %d\n", current_chmc->hit, current_chmc->miss, current_chmc->unknow);
	DOUT("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);

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
			DOUT("\nbb is a  function: %d\n", bb->callpid);
			//addr = start_addr_fetch;
			addr = bb->startaddr;

			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET(addr);
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
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

			//exit(1);
		}	
		else
		{
			DOUT("\nbb is a  simple bb\n");
			addr = bb->startaddr;
			for(j = 0; j < bb->num_instr; j ++)
			{
				set_no = SET(addr);
				calculateCacheState(bb->bb_cache_state->must[set_no], bb->bb_cache_state->may[set_no], bb->bb_cache_state->persist[set_no], addr);
				addr = addr + INSN_SIZE;
			}
			
			//FREE(cs_ptr);
			//cs_ptr = bb->bb_cache_state;
			//cs_ptr = copyCacheState(bb->bb_cache_state);
			//cs_ptr->source_bb = bb;

		}// end if else

	}

	for(i = 0; i < p->num_bb; i ++)
		p->bblist[i]->num_outgoing = p->bblist[i]->num_outgoing_copy;
	
	DRETURN( p->bblist[p->topo[0]->bbid]->bb_cache_state );
}


//do level one cache analysis
void
cacheAnalysis()
{
  DSTART( "cacheAnalysis" );

	int i;

	//loop_level_arr[] used to indicate what are the context of this bb
	loop_level_arr = NULL;
	CALLOC(loop_level_arr, int*, MAX_NEST_LOOP, sizeof(int), "loop_level_arr");

	//not in loop for main, so all elements are invalid
	for(i = 0; i < MAX_NEST_LOOP; i++)
		loop_level_arr[i] = INVALID;

	//set initial cache state for main precedure
	cache_state *start_CS = allocCacheState();
	cache_state *final_CS = mapFunctionCall(main_copy, start_CS);
	freeCacheState( start_CS );

	DOUT("\nThis the Cache State for main\n");
	DACTION( dumpCacheState( final_CS ); );

	DEND();
}


// from way 0-> way n, younger->older
static cache_line_way_t **
unionCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_a, entry_b;
	//int flag = 1;
  cache_line_way_t **result;
  CALLOC(result, cache_line_way_t **, cache.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na; i++)
	{
		CALLOC(result[i], cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t *");
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
				CALLOC(result[index]->entry, int *, result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				REALLOC(result[index]->entry, int *, result[index]->num_entry * sizeof(int), "cache line way");
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
					CALLOC(result[i]->entry, int *, result[i]->num_entry, sizeof(int), "cache line way");
				}
				else
				{
					REALLOC(result[i]->entry, int *, result[i]->num_entry*sizeof(int), "cache line way");
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
  cache_line_way_t **result;
	CALLOC(result, cache_line_way_t **, cache.na + 1, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na + 1; i++)
	{
		CALLOC(result[i], cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t *");
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
				CALLOC(result[index]->entry, int *, result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				REALLOC(result[index]->entry, int *, result[index]->num_entry * sizeof(int), "cache line way");
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
					CALLOC(result[i]->entry, int *, result[i]->num_entry, sizeof(int), "cache line way");
				}
				else
				{
					REALLOC(result[i]->entry, int *, result[i]->num_entry*sizeof(int), "cache line way");
				}
				result[i]->entry[result[i]->num_entry - 1] = entry_b;

			 } //end if
		} //end for

	}
	return result;
	
}



void
freeCacheSet(cache_line_way_t **cache_set)
{
	int i;
	for(i = 0; i < cache.na; i++)
	{
		if(cache_set[i]->entry != NULL) {
			FREE(cache_set[i]->entry);
		}
		FREE(cache_set[i]);
	}

	FREE(cache_set);
}


static void
freeCacheState(cache_state *cs)
{
	int i, j;
	
	for(i = 0; i < cache.ns; i++ ) {
		for(j = 0; j < cache.na; j++) {
			if(cs->must[i][j]->entry != NULL)
				FREE(cs->must[i][j]->entry);

			if(cs->may[i][j]->entry != NULL)
				FREE(cs->may[i][j]->entry);

			if(cs->persist[i][j]->entry != NULL)
				FREE(cs->persist[i][j]->entry);

			FREE(cs->must[i][j]);
			FREE(cs->may[i][j]);
			FREE(cs->persist[i][j]);
		}

		if(cs->persist[i][cache.na]->entry != NULL)
			FREE(cs->persist[i][cache.na]->entry);

		FREE(cs->persist[i][cache.na]);
		
		FREE(cs->must[i]);
		FREE(cs->may[i]);
		FREE(cs->persist[i]);
	}

	FREE(cs->must);
	FREE(cs->may);
	FREE(cs->persist);
	
	FREE(cs);
}

/*static void
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
}*/


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

		if(bb->is_loophead)
		  freeAllLoop(p, p->loops[bb->loopid]);
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
				FREE(bb->bb_cache_state);
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

		if(bb->is_loophead && i!= num_blk -1)
		  freeAllLoop(p, p->loops[bb->loopid]);
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

void
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
  cache_line_way_t **result;
	CALLOC(result, cache_line_way_t **, cache.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache.na; i++)
	{
		CALLOC(result[i], cache_line_way_t *, 1, sizeof(cache_line_way_t), "cache_line_way_t *");
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
				 CALLOC(result[index]->entry , int *, result[index]->num_entry, sizeof(int), "cache line way");
			}
			else
			{
				 REALLOC(result[index]->entry , int *, result[index]->num_entry*sizeof(int), "cache line way");
			}
			result[index]->entry[result[index]->num_entry - 1] = entry_b;

		} // end for

	}
	return result;
}
