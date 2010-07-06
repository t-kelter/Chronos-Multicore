#include "pathDAG.h"

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
		
	      	proc->wcet = (ull*)CALLOC(proc->wcet, copies, sizeof(ull), "ull");     
	      	proc->bcet = (ull*)CALLOC(proc->bcet, copies, sizeof(ull), "ull");           
	  
	   	proc->hit_wcet = (ull*)CALLOC(proc->hit_wcet, copies, sizeof(ull), "ull");
	   	proc->miss_wcet = (ull*)CALLOC(proc->miss_wcet, copies, sizeof(ull), "ull");
		proc->unknow_wcet = (ull*)CALLOC(proc->unknow_wcet, copies, sizeof(ull), "ull");

		proc->hit_wcet_L2 = (ull*)CALLOC(proc->hit_wcet_L2, copies, sizeof(ull), "ull");
		proc->miss_wcet_L2 = (ull*)CALLOC(proc->miss_wcet_L2, copies, sizeof(ull), "ull");
		proc->unknow_wcet_L2 = (ull*)CALLOC(proc->unknow_wcet_L2, copies, sizeof(ull), "ull");



	   	proc->hit_bcet = (ull*)CALLOC(proc->hit_bcet, copies, sizeof(ull), "ull");
	   	proc->miss_bcet = (ull*)CALLOC(proc->miss_bcet, copies, sizeof(ull), "ull");
		proc->unknow_bcet = (ull*)CALLOC(proc->unknow_bcet, copies, sizeof(ull), "ull");

		proc->hit_bcet_L2 = (ull*)CALLOC(proc->hit_bcet_L2, copies, sizeof(ull), "ull");
		proc->miss_bcet_L2 = (ull*)CALLOC(proc->miss_bcet_L2, copies, sizeof(ull), "ull");
		proc->unknow_bcet_L2 = (ull*)CALLOC(proc->unknow_bcet_L2, copies, sizeof(ull), "ull");

	}
	proc->wcet[cnt] = 0;
	proc->bcet[cnt] = 0;

   	proc->hit_wcet[cnt] = 0;
   	proc->miss_wcet[cnt] = 0;
   	proc->unknow_wcet[cnt] = 0;


	proc->hit_wcet_L2[cnt] = 0;
	proc->miss_wcet_L2[cnt] = 0;
	proc->unknow_wcet_L2[cnt] = 0;


   	proc->hit_bcet[cnt] = 0;
   	proc->miss_bcet[cnt] = 0;
   	proc->unknow_bcet[cnt] = 0;


	proc->hit_bcet_L2[cnt] = 0;
	proc->miss_bcet_L2[cnt] = 0;
	proc->unknow_bcet_L2[cnt] = 0;


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
		
		if(bb->callpid != -1)
		{
			pathFunction(bb->proc_ptr);
/*
			//wcet and bcet for each called procedure and basic blcok
			proc->wcet[cnt] += bb->chmc_L2[cnt]->wcost;
			proc->wcet[cnt] += bb->proc_ptr->wcet[cnt];

			proc->bcet[cnt] += bb->chmc_L2[cnt]->bcost;
			proc->bcet[cnt] += bb->proc_ptr->bcet[cnt];
			if(print)
				printf("proc->wcet[cnt] = %Lu, proc->bcet[cnt] = %Lu\n", proc->wcet[cnt], proc->bcet[cnt]);
			if(print)
				scanf("%c", &tmp);

			//hit and miss
			
			//hit in L1, wcet
			
			proc->hit_wc[cnt] += bb->proc_ptr->hit_wc[cnt];
			proc->hit_wc[cnt] += bb->chmc[cnt]->hit;
			
			//miss in L1, wcet

			proc->miss_wc[cnt] += bb->proc_ptr->miss_wc[cnt];
			proc->miss_wc[cnt] += bb->chmc[cnt]->miss;
			proc->miss_wc[cnt] += bb->chmc[cnt]->unknow;
			
			//hit in L1, bcet
			
			proc->hit_bc[cnt] += bb->proc_ptr->hit_bc[cnt];
			proc->hit_bc[cnt] += bb->chmc[cnt]->hit;
			proc->hit_bc[cnt] += bb->chmc[cnt]->unknow;
			
			//miss in L1, bcet

			proc->miss_bc[cnt] += bb->proc_ptr->miss_bc[cnt];
			proc->miss_bc[cnt] += bb->chmc[cnt]->miss;

			

			//hit in L2, wcet
			
			proc->hit_wc_L2[cnt] += bb->proc_ptr->hit_wc_L2[cnt];
			proc->hit_wc_L2[cnt] += bb->chmc_L2[cnt]->hit;
			
			//miss in L2, wcet

			proc->miss_wc_L2[cnt] += bb->proc_ptr->miss_wc_L2[cnt];
			proc->miss_wc_L2[cnt] += bb->chmc_L2[cnt]->miss;
			proc->miss_wc_L2[cnt] += bb->chmc_L2[cnt]->unknow;
			
			//hit in L2, bcet
			
			proc->hit_bc_L2[cnt] += bb->proc_ptr->hit_bc_L2[cnt];
			proc->hit_bc_L2[cnt] += bb->chmc_L2[cnt]->hit;
			proc->hit_bc_L2[cnt] += bb->chmc_L2[cnt]->unknow;
			
			//miss in L2, bcet

			proc->miss_bc_L2[cnt] += bb->proc_ptr->miss_bc_L2[cnt];
			proc->miss_bc_L2[cnt] += bb->chmc_L2[cnt]->miss;

*/		
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

	if(lp_ptr->num_cost == 0)
	{
		lp_ptr->num_cost = copies;
		
	      	lp_ptr->wcet = (ull*)CALLOC(lp_ptr->wcet, copies, sizeof(ull), "ull");           
	      	lp_ptr->bcet = (ull*)CALLOC(lp_ptr->bcet, copies, sizeof(ull), "ull");           
	  
	   	lp_ptr->hit_wcet = (ull*)CALLOC(lp_ptr->hit_wcet, copies, sizeof(ull), "ull");
	   	lp_ptr->miss_wcet = (ull*)CALLOC(lp_ptr->miss_wcet, copies, sizeof(ull), "ull");
	   	lp_ptr->unknow_wcet = (ull*)CALLOC(lp_ptr->unknow_wcet, copies, sizeof(ull), "ull");

		lp_ptr->hit_wcet_L2 = (ull*)CALLOC(lp_ptr->hit_wcet_L2, copies, sizeof(ull), "ull");
		lp_ptr->miss_wcet_L2 = (ull*)CALLOC(lp_ptr->miss_wcet_L2, copies, sizeof(ull), "ull");
		lp_ptr->unknow_wcet_L2 = (ull*)CALLOC(lp_ptr->unknow_wcet_L2, copies, sizeof(ull), "ull");



	   	lp_ptr->hit_bcet = (ull*)CALLOC(lp_ptr->hit_bcet, copies, sizeof(ull), "ull");
	   	lp_ptr->miss_bcet = (ull*)CALLOC(lp_ptr->miss_bcet, copies, sizeof(ull), "ull");
	   	lp_ptr->unknow_bcet = (ull*)CALLOC(lp_ptr->unknow_bcet, copies, sizeof(ull), "ull");

		lp_ptr->hit_bcet_L2 = (ull*)CALLOC(lp_ptr->hit_bcet_L2, copies, sizeof(ull), "ull");
		lp_ptr->miss_bcet_L2 = (ull*)CALLOC(lp_ptr->miss_bcet_L2, copies, sizeof(ull), "ull");
		lp_ptr->unknow_bcet_L2 = (ull*)CALLOC(lp_ptr->unknow_bcet_L2, copies, sizeof(ull), "ull");

	}
	
      	lp_ptr->wcet[cnt] = 0;           
      	lp_ptr->bcet[cnt] = 0;           
  
   	lp_ptr->hit_wcet[cnt] = 0;
   	lp_ptr->miss_wcet[cnt] = 0;
	lp_ptr->unknow_wcet[cnt] = 0;
	
	lp_ptr->hit_wcet_L2[cnt] = 0;
	lp_ptr->miss_wcet_L2[cnt] = 0;
	lp_ptr->unknow_wcet_L2[cnt] = 0;



   	lp_ptr->hit_bcet[cnt] = 0;
   	lp_ptr->miss_bcet[cnt] = 0;
	lp_ptr->unknow_bcet[cnt] = 0;
	
	lp_ptr->hit_bcet_L2[cnt] = 0;
	lp_ptr->miss_bcet_L2[cnt] = 0;
	lp_ptr->unknow_bcet_L2[cnt] = 0;

		
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
/*
			lp_ptr->wcet[cnt] += p->loops[bb->loopid]->wcet[cnt * 2];
			lp_ptr->wcet[cnt] += p->loops[bb->loopid]->wcet[cnt * 2 + 1];
			
			lp_ptr->bcet[cnt] += p->loops[bb->loopid]->bcet[cnt * 2];
			lp_ptr->bcet[cnt] += p->loops[bb->loopid]->bcet[cnt * 2 + 1];

			if(print)
				printf("lp_ptr->wcet[cnt] = %Lu, lp_ptr->bcet[cnt] = %Lu\n", lp_ptr->wcet[cnt], lp_ptr->bcet[cnt]);
			if(print)
				scanf("%c", &tmp);


			lp_ptr->hit_wc[cnt] += p->loops[bb->loopid]->hit_wc[cnt * 2];
			lp_ptr->hit_wc[cnt] += (p->loops[bb->loopid]->hit_wc[cnt * 2 + 1]);
			
			lp_ptr->hit_bc[cnt] += p->loops[bb->loopid]->hit_bc[cnt * 2];
			lp_ptr->hit_bc[cnt] += p->loops[bb->loopid]->hit_bc[cnt * 2 + 1];

			lp_ptr->miss_wc[cnt] += p->loops[bb->loopid]->miss_wc[cnt * 2];
			lp_ptr->miss_wc[cnt] += (p->loops[bb->loopid]->miss_wc[cnt * 2 + 1]);
			
			lp_ptr->miss_bc[cnt] += p->loops[bb->loopid]->miss_bc[cnt * 2];
			lp_ptr->miss_bc[cnt] += p->loops[bb->loopid]->miss_bc[cnt * 2 + 1];



			lp_ptr->hit_wc_L2[cnt] += p->loops[bb->loopid]->hit_wc_L2[cnt * 2];
			lp_ptr->hit_wc_L2[cnt] += (p->loops[bb->loopid]->hit_wc_L2[cnt * 2 + 1]);
			
			lp_ptr->hit_bc_L2[cnt] += p->loops[bb->loopid]->hit_bc_L2[cnt * 2];
			lp_ptr->hit_bc_L2[cnt] += p->loops[bb->loopid]->hit_bc_L2[cnt * 2 + 1];

			lp_ptr->miss_wc_L2[cnt] += p->loops[bb->loopid]->miss_wc_L2[cnt * 2];
			lp_ptr->miss_wc_L2[cnt] += (p->loops[bb->loopid]->miss_wc_L2[cnt * 2 + 1]);
			
			lp_ptr->miss_bc_L2[cnt] += p->loops[bb->loopid]->miss_bc_L2[cnt * 2];
			lp_ptr->miss_bc_L2[cnt] += p->loops[bb->loopid]->miss_bc_L2[cnt * 2 + 1];
*/
			continue;
		}

		//check the bb if it is a function call
		
		if(bb->callpid != -1)
		{
			pathFunction(bb->proc_ptr);
		}
	}
}


static void 
pathDAG(MSC *msc)
{
	int i, j;
	//printf("pathDAG\n");
	
	for(i = 0; i < msc->num_task; i ++)
	{
		for(j = 0; j < MAX_NEST_LOOP; j++)
			loop_level_arr[j] = INVALID;

		pathFunction(msc->taskList[i].main_copy);
		/*
		msc->taskList[i].wcet = msc->taskList[i].main_copy->wcet[0];
		msc->taskList[i].bcet = msc->taskList[i].main_copy->bcet[0];

		msc->taskList[i].hit_wc = msc->taskList[i].main_copy->hit_wc[0];
		msc->taskList[i].miss_wc = msc->taskList[i].main_copy->miss_wc[0];
		msc->taskList[i].hit_bc = msc->taskList[i].main_copy->hit_bc[0];
		msc->taskList[i].miss_bc = msc->taskList[i].main_copy->miss_bc[0];

		msc->taskList[i].hit_wc_L2 = msc->taskList[i].main_copy->hit_wc_L2[0];
		msc->taskList[i].miss_wc_L2 = msc->taskList[i].main_copy->miss_wc_L2[0];
		msc->taskList[i].hit_bc_L2 = msc->taskList[i].main_copy->hit_bc_L2[0];
		msc->taskList[i].miss_bc_L2 = msc->taskList[i].main_copy->miss_bc_L2[0];

		printf("msc->taskList[%d].wcet = %Lu, msc->taskList[%d].bcet = %Lu\n", i, msc->taskList[i].wcet, i, msc->taskList[i].bcet);
		*/
		
	}


}



