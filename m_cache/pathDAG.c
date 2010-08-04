#include <stdlib.h>

#include "pathDAG.h"

// Forward declarations of static functions

static void
pathFunction(procedure * proc);

static void 
pathLoop(procedure *proc, loop *lp);



static void
pathFunction(procedure *proc)
{
	int i, cnt, lp_level, num_blk, copies;
	procedure *p = proc;
	block *bb;

	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}

	cnt = 0;		
	for(i = 0; i <= lp_level; i++)
		if(loop_level_arr [i] == NEXT_ITERATION)
			cnt += (1<<(lp_level - i));

	if(lp_level == -1)
	{
		copies = 1;		
	}
	else
	{
		copies = (2<<lp_level);
	}

	
	if(proc->num_cost == 0)
	{
		proc->num_cost = copies;
		
	      	CALLOC(proc->wcet, ull*, copies, sizeof(ull), "ull");     
	      	CALLOC(proc->bcet, ull*, copies, sizeof(ull), "ull");

	}
	proc->wcet[cnt] = 0;
	proc->bcet[cnt] = 0;


	num_blk = p->num_topo;	
	
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		//bb is in topo list, to get the bbid of this bb
		bb = p ->topo[i];

		//bb is in bblist
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead)
		{

			loop_level_arr[lp_level +1] = FIRST_ITERATION;
			pathLoop(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			pathLoop(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = INVALID;

			continue;
		}

		//check the bb if it is a function call
		if(bb->callpid != -1) {
			pathFunction(bb->proc_ptr);
		}

	}
			
}// end if else

static void
pathLoop(procedure *proc, loop *lp)
{
	int i, cnt, lp_level, num_blk, copies;

	procedure *p = proc;
	block *bb;
	loop *lp_ptr = lp;

	num_blk = lp_ptr->num_topo;

	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}

	cnt = 0;		
	for(i = 0; i <= lp_level; i++)
		if(loop_level_arr [i] == NEXT_ITERATION)
			cnt += (1<<(lp_level - i));


	if(lp_level == -1)
	{
		copies = 1;		
	}
	else
	{
		copies = (2<<lp_level);
	}

	if(lp_ptr->num_cost == 0) {
		lp_ptr->num_cost = copies;
		
	 	CALLOC(lp_ptr->wcet, ull*, copies, sizeof(ull), "ull");
	 	CALLOC(lp_ptr->bcet, ull*, copies, sizeof(ull), "ull");
	}
	
  lp_ptr->wcet[cnt] = 0;
  lp_ptr->bcet[cnt] = 0;

	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		//bb is in topo list, to get the bbid of this bb
		bb = lp_ptr->topo[i];

		//bb is in bblist
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead && i!= num_blk -1)
		{
			loop_level_arr[lp_level +1] = FIRST_ITERATION;

			pathLoop(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			pathLoop(p, p->loops[bb->loopid]);
			
			loop_level_arr[lp_level +1] = INVALID;

			continue;
		}

		//check the bb if it is a function call
		
		if(bb->callpid != -1)
		{
			pathFunction(bb->proc_ptr);
		}
	}
}

void 
pathDAG(MSC *msc)
{
	int i, j;
	
	for(i = 0; i < msc->num_task; i ++) {
		for(j = 0; j < MAX_NEST_LOOP; j++)
			loop_level_arr[j] = INVALID;

		pathFunction(msc->taskList[i].main_copy);
	}
}
