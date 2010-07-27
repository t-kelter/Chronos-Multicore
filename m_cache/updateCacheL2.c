// Include standard library headers
#include <stdlib.h>
#include <stdio.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "updateCacheL2.h"
#include "analysisCache.h"


// Forward declarations of static functions
static void
updateLoop(MSC *msc, int index, procedure *proc, loop *lp);
static void
updateFunctionCall(MSC *msc, int index, procedure* proc);


static int
conflictStatistics(MSC *msc, int index, int set_no, int current_addr)
{
  int i, j, entry, conflict = 0;
	int *history = NULL, cnt = 0;
	history = (int *)CALLOC(history, MAX_LEN * 2, sizeof(int), "history int[]");
	
    for(i = 0; i < msc->num_task; i++)
    {
        if(i != index)
        {
            for(j = 0; j < msc->taskList[i].main_copy->hit_addr[set_no].num_entry; j++)
            {
            	entry = msc->taskList[i].main_copy->hit_addr[set_no].entry[j];
                if(entry == current_addr) continue;
		if(isInWay(entry, history, cnt)) continue;
		conflict++;
		history[cnt] = entry;
		cnt++;
		if(cnt >= MAX_LEN * 2) printf("entry overflow!\n"), exit(1);
	    }
        }
    }
	free(history);
    return conflict;
}


static void
updateLoop(MSC *msc, int index, procedure *proc, loop *lp)
{
  DSTART( "updateLoop" );

    int i, j, k, n, set_no, cnt, addr, lp_level, num_conflict;
    procedure *p = proc;
    block *bb;
    CHMC * current_chmc;
	 int offset;
    
    int  num_blk = lp->num_topo;

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
    n = 0;
    
    for(i = num_blk -1 ; i >= 0 ; i--)
    {   
        //bb is in topo list, to get the bbid of this bb
        bb = lp ->topo[i];

        //bb is in bblist
        bb = p->bblist[ bb->bbid ];
        
        DOUT("\nThis is  bb %d in proc %d\n", bb->bbid, proc->pid);
        
        if(bb->is_loophead && i!= num_blk -1)
        {

            loop_level_arr[lp_level +1] = FIRST_ITERATION;

            DOUT("\nThe first time go into loop\n");
            updateLoop(msc, index, p, p->loops[bb->loopid]);
            DOUT("\nThe second time go into loop\n");

            loop_level_arr[lp_level +1] = NEXT_ITERATION;
            updateLoop(msc, index, p, p->loops[bb->loopid]);
            
            loop_level_arr[lp_level +1] = INVALID;

            continue;
        }

        current_chmc = bb->chmc_L2[cnt];



        //change hit into unknow here
        for(k = 0; k < msc->num_task; k++)
        {
            if(k == index||(msc->interferInfo[index][k] == 0)) continue;
            else
            {
                for(j = 0; j < current_chmc->hit; j++)
                {
                    if(current_chmc->hit_change_miss[j] == HIT)
                    {
                        addr = current_chmc->hit_addr[j];
                        set_no = SET_L2(addr);
								offset = (addr - bb->startaddr) / INSN_SIZE;
                        if(msc->taskList[k].main_copy->hit_cache_set_L2[set_no] == USED)
                        {
                            num_conflict = conflictStatistics(msc, index, set_no, addr);
                            DOUT("num_conflict = %d\n", num_conflict);
   
                            if(current_chmc->age[j] + num_conflict >= cache_L2.na)
                            {
									 /* sudiptac :::: for bus-aware WCET aanalysis */
									 if(offset > 0)
										 current_chmc->hitmiss_addr[offset] = UNKNOW;
                            current_chmc->hit_change_miss[j] = MISS;
                            current_chmc->hit--;
                            current_chmc->unknow++;

                            n++;
                            current_chmc->wcost += (IC_MISS_L2 - IC_HIT_L2);

                            }
                        }
                    }

                }
            }
        }
        

        DACTION(
            for(k = 0; k < current_chmc->hitmiss; k++)
              DOUT("L2: %d ", current_chmc->hitmiss_addr[k]);
        );
        DOUT("\nL2:\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
        DOUT("num of fetch = %d, hit = %d, miss= %d, unknow = %d\n", current_chmc->hitmiss, current_chmc->hit, current_chmc->miss, current_chmc->unknow);

        DOUT("num of fetch = %d, hit = %d, miss= %d, unknow = %d\n", bb->chmc[cnt]->hitmiss, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);

        DOUT("\ncnt = %d, wcost = %d, bcost = %d\n", cnt, current_chmc->wcost, current_chmc->bcost);
    
        DOUT("\nL2 Loop:  bb->wcost = %d\n", current_chmc->wcost);
        DOUT("\nL2 Loop:  bb->bcost = %d\n", current_chmc->bcost);

    
        //check the bb if it is a function call
        
        if(bb->callpid != -1)
          updateFunctionCall(msc, index, bb->proc_ptr);
    }

    if(n)
      DOUT("\n%d cache line changed into unknow in proc[%d]->loop[%d]\n", n, p->pid, lp->lpid);

    DEND();
}


static void
updateFunctionCall(MSC *msc, int index, procedure* proc)
{
  DSTART( "updateFunctionCall" );

    int i, j, k, n, set_no, cnt, addr, lp_level, num_conflict;
    procedure *p = proc;
    block *bb;
    CHMC * current_chmc;
    int offset; 
    int  num_blk = p->num_topo; 

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

    n = 0;
        
    for(i = num_blk -1 ; i >= 0 ; i--)
    {   
        //bb is in topo list, to get the bbid of this bb
        bb = p ->topo[i];

        //bb is in bblist
        bb = p->bblist[ bb->bbid ];
        
        DOUT("\nThis is  bb %d in proc %d\n", bb->bbid, proc->pid);
        
        if(bb->is_loophead)
        {

            loop_level_arr[lp_level +1] = FIRST_ITERATION;

            DOUT("\nThe first time go into loop\n");
            updateLoop(msc, index, p, p->loops[bb->loopid]);
            DOUT("\nThe second time go into loop\n");

            loop_level_arr[lp_level +1] = NEXT_ITERATION;
            updateLoop(msc, index, p, p->loops[bb->loopid]);
            
            loop_level_arr[lp_level +1] = INVALID;

            continue;
        }

        current_chmc = bb->chmc_L2[cnt];


        for(k = 0; k < msc->num_task; k++)
        {
            if(k == index||(msc->interferInfo[index][k] == 0)) continue;
            else
            {
                for(j = 0; j < current_chmc->hit; j++)
                {
                    if(current_chmc->hit_change_miss[j] == HIT)
                    {
                        addr = current_chmc->hit_addr[j];
                        set_no = SET_L2(addr);
								offset = (addr - bb->startaddr) / INSN_SIZE;
                        if(msc->taskList[k].main_copy->hit_cache_set_L2[set_no] == USED)
                        {
                            num_conflict = conflictStatistics(msc, index, set_no, addr);
                            DOUT("num_conflict = %d\n", num_conflict);
                            
                           if(current_chmc->age[j] + num_conflict >= cache_L2.na)
                            {
										  if(offset > 0)
											 current_chmc->hitmiss_addr[offset] = UNKNOW;
                                current_chmc->hit_change_miss[j] = MISS;
                                current_chmc->hit--;
                                current_chmc->unknow++;

                                n++;
                                current_chmc->wcost += (IC_MISS_L2 - IC_HIT_L2);
                            }
                            //current_chmc->bcost += IC_MISS_L2 - IC_HIT_L2;
                            
                        }
                    }

                }
            }
        }


        DACTION(
            for(k = 0; k < current_chmc->hitmiss; k++)
              DOUT("L2: %d ", current_chmc->hitmiss_addr[k]);
        );
        DOUT("\nL2:\nbb->size = %d, bb->startaddr = %d\n", bb->size, bb->startaddr);
        DOUT("num of fetch = %d, hit = %d, miss= %d, unknow = %d\n", current_chmc->hitmiss, current_chmc->hit, current_chmc->miss, current_chmc->unknow);

        DOUT("num of fetch = %d, hit = %d, miss= %d, unknow = %d\n", bb->chmc[cnt]->hitmiss, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);

        DOUT("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);

        DOUT("\nL2 function:  bb->wcost = %d\n", current_chmc->wcost);
        DOUT("\nL2 function:  bb->bcost = %d\n", current_chmc->bcost);

        //check the bb if it is a function call
        
        if(bb->callpid != -1)
            updateFunctionCall(msc, index, bb->proc_ptr);
    }

    if(n)
      DOUT("\n%d cache line changed into unknow in proc[%d]\n", n, p->pid);

    DEND();
}


void
updateCacheState(MSC *msc)
{
    int i, j;
    
    for(i = 0; i < msc->num_task; i ++) {
        for(j = 0; j < MAX_NEST_LOOP; j++)
            loop_level_arr[i] = INVALID;

        updateFunctionCall(msc, i, msc->taskList[i].main_copy);
    }
}
