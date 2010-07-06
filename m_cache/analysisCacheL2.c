/*
  do Lever two Cache Analysis using Abstract Interpretation
*/

#include <stdio.h>
#include <assert.h>


static cache_state *
mapLoop_L2(procedure *pro, loop *lp);

static cache_state *
mapFunctionCall_L2(procedure *proc, cache_state *cs);

static void
resetHitMiss_L2(MSC *msc);

static void
resetFunction_L2(procedure *proc);

static void
resetLoop_L2(procedure *proc, loop* lp);

static void
calculateCacheState_L2(cache_line_way_t **must, cache_line_way_t **may, cache_line_way_t **persist, int instr_addr);

static cache_state *
copyCacheState_L2(cache_state *cs);


//static void dump_cache_line(cache_line_way_t *clw_a);
/* return log of integer n to the base 2 
* -1  if n<0 or n!=power(2,m);
*/
/*
static char 
isInWay(int entry, int *entries, int num_entry);
*/

static void
freeAll_L2();

static void
freeAllFunction_L2(procedure *proc);

static void
freeAllLoop_L2(procedure *proc, loop *lp);

static char
isInCache_L2(int addr, cache_line_way_t**must);

static cache_line_way_t **
unionCacheState_L2(cache_line_way_t ** clw_a, cache_line_way_t **clw_b);

/*
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
*/

//read basic cache configuration from configFile and then
//set other cinfiguration
static void
set_cache_basic_L2(char * configFile)
{

  FILE *fptr;
  int ns, na, ls, cmp, i;

  fptr = fopen(configFile, "r" );
  if (fptr == NULL) {
    fprintf(stderr, "Failed to open file: %s (analysisCacheL2.c:91)\n", configFile);
    exit (1);
  }
  
  fscanf( fptr, "%d %d %d %d", &ns, &na, &ls, &cmp);
  
    cache_L2.ns = ns;
    cache_L2.na = na;
    cache_L2.ls = ls;
    cache_L2.cmp = cmp;

   //set other configuration through the basic: ns, na, ls, cmp
    cache_L2.nsb = logBase2(cache_L2.ns);
    cache_L2.lsb = logBase2(cache_L2.ls);
    // tag bits, #tags mapping to each set
    cache_L2.ntb = MAX_TAG_BITS;
    cache_L2.nt = 1 << cache_L2.ntb;
    // tag + set bits, set + line bits, # of tag + set
    cache_L2.t_sb = cache_L2.ntb + cache_L2.nsb;
    cache_L2.s_lb = cache_L2.nsb + cache_L2.lsb;
    cache_L2.nt_s = 1 << cache_L2.t_sb;
    // cache line mask
    cache_L2.l_msk =  (1 << cache_L2.lsb) - 1;
    // set mask
    cache_L2.s_msk = 0;
    for (i = 1; i < cache_L2.ns; i <<= 1)
    cache_L2.s_msk = cache_L2.s_msk  | i;
    cache_L2.s_msk = cache_L2.s_msk << cache_L2.lsb;
    // tag mask
    cache_L2.t_msk = 0;
    for (i = 1; i < cache_L2.nt; i <<= 1)
    cache_L2.t_msk |= i;
    cache_L2.t_msk = cache_L2.t_msk << (cache_L2.lsb + cache_L2.nsb);
    // set+tag mask
    cache_L2.t_s_msk = cache_L2.t_msk | cache_L2.s_msk;
}

static void
dumpCacheConfig_L2()
{
    printf("Cache Configuration as follow:\n");
    printf("nubmer of set:  %d\n", cache_L2.ns);	
    printf("nubmer of associativity:    %d\n", cache_L2.na);	
    printf("cache line size:    %d\n", cache_L2.ls);	
    printf("cache miss penalty: %d\n", cache_L2.cmp);	
    printf("nubmer of set bit:  %d\n", cache_L2.nsb);	
    printf("nubmer of linesize bit: %d\n", cache_L2.lsb);	
    printf("set mask:   %u\n", cache_L2.s_msk);
    printf("tag mask:   %u\n", cache_L2.t_msk);	
    printf("tag + set mask: %u\n", cache_L2.t_s_msk);	
}


static void
calculateMust_L2(cache_line_way_t **must, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET_L2(instr_addr);
	
	for(i = 0; i < cache_L2.na; i++)
	{
		if(isInWay(instr_addr, must[i]->entry, must[i]->num_entry))
		{
			if(i != 0)
			{
				must[0]->num_entry++;
				if(must[0]->num_entry == 1)
				{
					must[0]->entry = (int*)CALLOC(must[0]->entry, 1, sizeof(int), "entry");
				}
				else
				{
					must[0]->entry = (int*)REALLOC(must[0]->entry, (must[0]->num_entry) * sizeof(int), "enties");
				}
				must[0]->entry[(must[0]->num_entry) -1] = instr_addr;
				
				
				for(j = 0; j < must[i]->num_entry; j++)
				{
					if(must[i]->entry[j] == instr_addr)
					{
						if(j != must[i]->num_entry -1)
							must[i]->entry[j] = must[i]->entry[must[i]->num_entry -1];
						must[i]->num_entry --;
						if(must[i]->num_entry == 0)
						{
							free(must[i]->entry);
						}
						else
						{
							must[i]->entry = (int*)REALLOC(must[i]->entry, must[i]->num_entry * sizeof(int), "entry");
						}
					}
				} // end for(j)
			}	// 	end if(i!=0)
			return;
		}
	}
	
	cache_line_way_t * tmp = must[cache_L2.na -1];
	cache_line_way_t *head = NULL;
	
	for(i = cache_L2.na - 1; i > 0 ; i--)
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
calculateMay_L2(cache_line_way_t **may, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET_L2(instr_addr);
	
	for(i = 0; i < cache_L2.na; i++)
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
						if(j != may[i]->num_entry -1)
							may[i]->entry[j] = may[i]->entry[may[i]->num_entry -1];
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
	
	cache_line_way_t * tmp = may[cache_L2.na -1];
	cache_line_way_t *head = NULL;
	
	for(i = cache_L2.na - 1; i > 0 ; i--)
		may[i]  = may [i -1];		
	free(tmp);

	head = (cache_line_way_t *)CALLOC(head, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	head->entry = (int*)CALLOC(head->entry, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	may[0] = head;
}




static void
calculatePersist_L2(cache_line_way_t **persist, int instr_addr)
{
	int i, j;
	instr_addr = TAGSET_L2(instr_addr);

	/* FIXME:::: Foe persistence analysis associativity is one more than 
	 * the actual to contain all victim cache blocks */
	for(i = 0; i < cache_L2.na + 1; i++)
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
						if(j != persist[i]->num_entry -1)
							persist[i]->entry[j] = persist[i]->entry[persist[i]->num_entry -1];
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
	
	//cache_line_way_t * tmp = persist[cache_L2.na -1];
	/* FIXME :::: Last block right ? */
	cache_line_way_t * tmp = persist[cache_L2.na];
	cache_line_way_t *head = NULL;
	
	for(i = cache_L2.na; i > 0 ; i--)
		persist[i]  = persist [i -1];		
	free(tmp);

	head = (cache_line_way_t *)CALLOC(head, 1, sizeof(cache_line_way_t), "cache_line_way_t");
	head->entry = (int*)CALLOC(head->entry, 1, sizeof(int), "in head->entry");
	head->entry[0] = instr_addr;
	head->num_entry = 1;
	persist[0] = head;
}


static void
calculateCacheState_L2(cache_line_way_t **must, cache_line_way_t **may, cache_line_way_t **persist, int instr_addr)
{
	//printf("\nalready in calculate cs\n");

	calculateMust_L2(must, instr_addr);
	calculateMay_L2(may, instr_addr);
	calculatePersist_L2(persist, instr_addr);
	
}

/* Needed for checking membership when updating cache
 * state of persistence cache analysis */
static char
isInSet_L2_persist(int addr, cache_line_way_t **set)
{
	int i;
	for(i = 0; i < cache_L2.na + 1; i++)
	{
		if(isInWay(addr, set[i]->entry, set[i]->num_entry) == 1)
			return i;
	}
	return -1;
}

static char
isInSet_L2(int addr, cache_line_way_t **set)
{
	int i;
	for(i = 0; i < cache_L2.na; i++)
	{
		if(isInWay(addr, set[i]->entry, set[i]->num_entry) == 1)
			return i;
	}
	return -1;
}

// from way n-> way 0, older->younger
static cache_line_way_t **
intersectCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache_L2.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache_L2.na; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}


	//for each way, calculate the result of cs
	for(i = cache_L2.na - 1; i >= 0; i--)
	{
		for(j = 0; j < clw_b[i]->num_entry; j++)
		{
			entry_b = clw_b[i]->entry[j];
			age = isInSet_L2(entry_b, clw_a);
			
			//kick out entries in clw_b not in clw_a
			if(age == -1)
				continue;

			//get the older age
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


// from way 0-> way n, younger->older
static cache_line_way_t **
unionCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_a, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache_L2.na, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache_L2.na; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}

	//for each way, calculate the result of cs
	for(i = 0; i < cache_L2.na; i++)
	{
		//for each entry in clw_a, is it in the clw_b? 
		//no, add it into result and history
		for(j = 0 ; j < clw_a[i]->num_entry; j++)
		{
			entry_a = clw_a[i]->entry[j];

			age = isInSet_L2(entry_a, clw_b);
			if(age == -1) index = i;
			else if(age < i) index = age;
			/* sudiptac ::: FIXME: Covered all cases right ? */
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
			if(isInSet_L2(entry_b, clw_a) != -1)
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
unionMaxCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b)
{
	int i, j, age, index, entry_a, entry_b;
	//int flag = 1;
	cache_line_way_t **result = (cache_line_way_t **) CALLOC(result, cache_L2.na + 1, sizeof(cache_line_way_t*), "cache_line_way_t **");
	
	for(i = 0; i < cache_L2.na + 1; i++)
	{
		result[i] = (cache_line_way_t *)CALLOC(result[i], 1, sizeof(cache_line_way_t), "cache_line_way_t *");
		result[i]->num_entry = 0;
		result[i]->entry = NULL;
	}

	//for each way, calculate the result of cs
	for(i = cache_L2.na; i >= 0; i--)
	{
		//for each entry in clw_a, is it in the clw_b? 
		//no, add it into result 
		for(j = 0 ; j < clw_a[i]->num_entry; j++)
		{
			entry_a = clw_a[i]->entry[j];
		  
			/* FIXME:: For persistence analysis a different membership 
			 * function is needed */
			age = isInSet_L2_persist(entry_a, clw_b);
			//only in clw_a
			if(age == -1) index = i;

			//both in clw_a and clw_b
			else if(age > i) index = age;
			
			/* sudiptac ::: FIXME: Covered all cases right ? */
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
			if(isInSet_L2_persist(entry_b, clw_a) != -1)
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


//allocate the memory for cache_state
static cache_state *
allocCacheState_L2()
{
	int j, k;
	cache_state *result = NULL;
	//printf("\nalloc CS memory copies = %d \n", copies);

	result = (cache_state *)CALLOC(result, 1, sizeof(cache_state), "cache_state_t");
	
	//printf("\nalloc CS memory result->loop_level = %d \n", result->loop_level );

		result->must = NULL;
		result->must = (cache_line_way_t***)CALLOC(result->must, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->may = NULL;
		result->may = (cache_line_way_t***)CALLOC(result->may, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		result->persist= NULL;
		result->persist = (cache_line_way_t***)CALLOC(result->persist, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		for(j = 0; j < cache_L2.ns; j++)
		{
			//printf("\nalloc CS memory for j = %d \n", j );
			result->must[j]= (cache_line_way_t**)	CALLOC(result->must[j], cache_L2.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			result->may[j]= (cache_line_way_t**)CALLOC(result->may[j], cache_L2.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			result->persist[j]= (cache_line_way_t**)CALLOC(result->persist[j], cache_L2.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			for( k = 0; k < cache_L2.na; k++)
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
			result->persist[j][cache_L2.na] = (cache_line_way_t*)CALLOC(result->persist[j][cache_L2.na], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
			result->persist[j][cache_L2.na]->num_entry = 0;
			result->persist[j][cache_L2.na]->entry = NULL;

			
		}
	//}

	return result;
}






static void
freeCacheState_L2(cache_state *cs)
{
	int i, j;
	
	for(i = 0; i < cache_L2.ns; i++ )
	{
		for(j = 0; j < cache_L2.na; j++)
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
		if(cs->persist[i][cache_L2.na]->num_entry)
			free(cs->persist[i][cache_L2.na]->entry);
		free(cs->persist[i][cache_L2.na]);
		
	}
		
	for(i = 0; i < cache_L2.ns; i++ )
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
freeCacheStateFunction_L2(procedure * proc)
{
	procedure *p = proc;
	block *bb;
	int i;

	int  num_blk = p->num_topo;
	


	for(i = num_blk -1 ; i >= 0 ; i--)
	{

		bb = p ->topo[i];

		bb = p->bblist[bb->bbid];

		bb->num_cache_state_L2= 0;
		if(bb->bb_cache_state_L2!= NULL)
		{
			freeCacheState_L2(bb->bb_cache_state_L2);	
			bb->bb_cache_state_L2 = NULL;
		}
	}

	
}



static void
freeCacheStateLoop_L2(procedure *proc, loop *lp)
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

		bb->num_cache_state_L2 = 0;
		if(bb->bb_cache_state_L2!= NULL)
		{
			freeCacheState_L2(bb->bb_cache_state_L2);
			bb->bb_cache_state_L2 = NULL;
		}
	}
}



static void
freeAllFunction_L2(procedure *proc)
{
	procedure *p = proc;
	block *bb;
	int i;
	int  num_blk = p->num_topo;	
		
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		bb = p ->topo[i];
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead)  freeAllLoop_L2(p, p->loops[bb->loopid]);
		else if(bb->callpid != -1)
		{
			bb->num_cache_state_L2 = 0;
			if(bb->bb_cache_state_L2 != NULL)
			{
				free(bb->bb_cache_state_L2);
				bb->bb_cache_state_L2 = NULL;
			}
			freeAllFunction_L2(bb->proc_ptr);
		}
		else
		{
			bb->num_cache_state_L2 = 0;
			if(bb->bb_cache_state_L2 != NULL)
			{
				free(bb->bb_cache_state_L2);
				bb->bb_cache_state_L2 = NULL;
			}
		}
	}

}

static void
freeAllLoop_L2(procedure *proc, loop *lp)
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

		if(bb->is_loophead && i!= num_blk -1)  freeAllLoop_L2(p, p->loops[bb->loopid]);
		else if(bb->callpid != -1)
		{
			bb->num_cache_state_L2 = 0;
			if(bb->bb_cache_state_L2 != NULL)
			{
				free(bb->bb_cache_state_L2);
				bb->bb_cache_state_L2 = NULL;
			}
			freeAllFunction_L2(bb->proc_ptr);
		}
		else
		{
			bb->num_cache_state_L2 = 0;
			if(bb->bb_cache_state_L2 != NULL)
			{
				free(bb->bb_cache_state_L2);
				bb->bb_cache_state_L2 = NULL;
			}
		}
	}

}

static void
freeAll_L2()
{
	freeAllFunction_L2(main_copy);
}


static char
isInCache_L2(int addr, cache_line_way_t**must)
{
	//printf("\nIn isInCache\n");
	int i;
	addr = TAGSET_L2(addr);
	
	for(i = 0; i < cache_L2.na; i++)
		if(isInWay(addr, must[i]->entry, must[i]->num_entry))
			return i;
	return -1;	
}

/* For persistence analysis the function would be different */

static cache_line_way_t **
copyCacheSet_persist(cache_line_way_t **cache_set)
{
	int i, j;
	cache_line_way_t **src = cache_set, **copy;
	copy = (cache_line_way_t**) CALLOC(copy, cache_L2.na + 1, sizeof(cache_line_way_t*), "cache_line_way_t*");
	for(i = 0; i < cache_L2.na + 1; i++)
	{
		copy[i] = (cache_line_way_t*) CALLOC(copy, 1, sizeof(cache_line_way_t), "cache_line_way_t");

		copy[i]->num_entry = src[i]->num_entry;
		if(copy[i]->num_entry)
		{
			copy[i]->entry = (int*) CALLOC(copy[i]->entry, copy[i]->num_entry, sizeof(int), "cache_line_way_t");
			for(j = 0; j < copy[i]->num_entry; j++)
				copy[i]->entry[j] = src[i]->entry[j];
		}
	}
	return copy;
}

static cache_line_way_t **
copyCacheSet(cache_line_way_t **cache_set)
{
	int i, j;
	cache_line_way_t **src = cache_set, **copy;
	copy = (cache_line_way_t**) CALLOC(copy, cache_L2.na, sizeof(cache_line_way_t*), "cache_line_way_t*");
	for(i = 0; i < cache_L2.na; i++)
	{
		copy[i] = (cache_line_way_t*) CALLOC(copy, 1, sizeof(cache_line_way_t), "cache_line_way_t");

		copy[i]->num_entry = src[i]->num_entry;
		if(copy[i]->num_entry)
		{
			copy[i]->entry = (int*) CALLOC(copy[i]->entry, copy[i]->num_entry, sizeof(int), "cache_line_way_t");
			for(j = 0; j < copy[i]->num_entry; j++)
				copy[i]->entry[j] = src[i]->entry[j];
		}
	}
	return copy;
}


static void
freeCacheSet_L2(cache_line_way_t **cache_set)
{
	int i;
	for(i = 0; i < cache_L2.na; i++)
	{
		if(cache_set[i]->num_entry)
			free(cache_set[i]->entry);
		free(cache_set[i]);
	}

	free(cache_set);

}

static char
isNeverInCache_L2(int addr, cache_line_way_t**may)
{
	//printf("\nIn isNeverInCache\n");
	int i;
	addr = TAGSET_L2(addr);
	
	for(i = 0; i < cache_L2.na; i++)
		if(isInWay(addr, may[i]->entry, may[i]->num_entry))
			return 0;
	return 1;
	
}



static cache_state *
mapLoop_L2(procedure *pro, loop *lp)
{
	int i, j, k, n, set_no, cnt, tmp, addr, addr_next, copies, age; 
	int lp_level, tag, tag_next;
	
	procedure *p = pro;

	block *bb, *incoming_bb;
	cache_state *cs_ptr;
	cache_line_way_t **cache_set_must, **cache_set_may, **cache_set_persist, **clw;

	int  num_blk = lp->num_topo;

	//cs_ptr = copyCacheState_L2(cs);
	
	CHMC *current_chmc;

	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}
		
	cnt = 0;
	for(k = 0; k <= lp_level; k++)
		if(loop_level_arr [k] == NEXT_ITERATION)
			cnt += (1<<(lp_level - k));


	for(i = num_blk -1 ; i >= 0 ; i--)
	{

		bb = lp ->topo[i];
		
		bb = p->bblist[bb->bbid];

		bb->num_instr = bb->size / INSN_SIZE;

		if(bb->is_loophead && i != num_blk -1)
		{

			loop_level_arr[lp_level + 1] = FIRST_ITERATION;
			p->loops[bb->loopid]->num_fm_L2 = 0;

			mapLoop_L2(p, p->loops[bb->loopid]);

			//cs_ptr = bb->bb_cache_state_L2;
			 
			loop_level_arr[lp_level + 1] = NEXT_ITERATION;

			mapLoop_L2(p, p->loops[bb->loopid]);

			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);

			loop_level_arr[lp_level + 1] = INVALID;

			//freeCacheStateLoop_L2(p, p->loops[bb->loopid]);
			continue;
		}



		if(bb->is_loophead == 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];

			if(!incoming_bb->bb_cache_state_L2) 
				continue;	 

			cs_ptr = copyCacheState_L2(incoming_bb->bb_cache_state_L2);

			if(bb->num_incoming > 1)
			{
				//printf("\ndo operations if more than one incoming edge\n");

				//dumpCacheState(cs_ptr);
				
				for(j = 1; j < bb->num_incoming; j++)
				{
					incoming_bb = p->bblist[bb->incoming[j]];
					
					if(incoming_bb->bb_cache_state_L2 == NULL) continue;
					
					for(k = 0; k < cache_L2.ns; k++)
					{
						clw = cs_ptr->must[k];							
						cs_ptr->must[k] = intersectCacheState_L2(cs_ptr->must[k], incoming_bb->bb_cache_state_L2->must[k]);

						freeCacheSet_L2(clw);

						clw = cs_ptr->may[k];							
						cs_ptr->may[k] = unionCacheState_L2(cs_ptr->may[k], incoming_bb->bb_cache_state_L2->may[k]);

						freeCacheSet_L2(clw);

						clw = cs_ptr->persist[k];							
						cs_ptr->persist[k] = unionMaxCacheState_L2(cs_ptr->persist[k], incoming_bb->bb_cache_state_L2->persist[k]);

						freeCacheSet_L2(clw);

					}
				} //end for(all incoming)

			}

			
		}

		else
		{
			if(loop_level_arr[lp_level] == FIRST_ITERATION)
			{
				incoming_bb = p->bblist[bb->incoming[0]];

				//free(cs_ptr);
				if(!incoming_bb->bb_cache_state_L2)
					 continue;
			
				cs_ptr = copyCacheState_L2(incoming_bb->bb_cache_state_L2);

				if(bb->num_incoming > 1)
				{
					//printf("\ndo operations if more than one incoming edge\n");

					//dumpCacheState(cs_ptr);
					
					for(j = 1; j < bb->num_incoming; j++)
					{
						incoming_bb = p->bblist[bb->incoming[j]];

						if(incoming_bb->bb_cache_state_L2 == NULL) continue;
	
						
						for(k = 0; k < cache_L2.ns; k++)
						{
							clw = cs_ptr->must[k];							
							cs_ptr->must[k] = intersectCacheState_L2(cs_ptr->must[k], incoming_bb->bb_cache_state_L2->must[k]);

							freeCacheSet_L2(clw);

							clw = cs_ptr->may[k];							
							cs_ptr->may[k] = unionCacheState_L2(cs_ptr->may[k], incoming_bb->bb_cache_state_L2->may[k]);

							freeCacheSet_L2(clw);

							clw = cs_ptr->persist[k];							
							cs_ptr->persist[k] = unionMaxCacheState_L2(cs_ptr->persist[k], incoming_bb->bb_cache_state_L2->persist[k]);

							freeCacheSet_L2(clw);

						}
					} //end for(all incoming)

				}

			}
			else if(loop_level_arr[lp_level] == NEXT_ITERATION)
			{
				if(! (p->bblist[lp->topo[0]->bbid]->bb_cache_state_L2))
					 continue;

				cs_ptr = copyCacheState_L2(p->bblist[lp->topo[0]->bbid]->bb_cache_state_L2);
				//cache_state *cs_tmp = cs_ptr;
				//freeCacheState(cs_tmp);


			}
			else
			{
				printf("\nCFG error!\n");
				exit(1);
			}
		}

		if(bb->num_cache_state_L2 == 0)
		{
			//bb->bb_cache_state_L2 = (cache_state *)CALLOC(bb->bb_cache_state_L2, 1, sizeof(cache_state), "cache_state");

			bb->num_cache_state_L2 = 1;
		}

		if(bb->num_chmc_L2 == 0)
		{
			copies = 2<<(lp_level);

			bb->num_chmc_L2 = copies;

			bb->chmc_L2 = (CHMC**)CALLOC(bb->chmc_L2, copies, sizeof(CHMC*), "CHMC");

			for(tmp = 0; tmp < copies; tmp++)
			{
				bb->chmc_L2[tmp] = (CHMC*)CALLOC(bb->chmc_L2[tmp], 1, sizeof(CHMC), "CHMC");
			}

		}

		bb->bb_cache_state_L2 = cs_ptr;
		
		current_chmc = bb->chmc_L2[cnt];

		current_chmc->hit = 0;
		current_chmc->hit_copy = 0;

		current_chmc->miss = 0;
		
		current_chmc->unknow = 0;
		current_chmc->unknow_copy = 0;

		current_chmc->wcost = 0;
		current_chmc->bcost = 0;
		current_chmc->wcost_copy = 0;
		current_chmc->bcost_copy = 0;
	
		
		//current_chmc->hit_addr = NULL;
		//current_chmc->miss_addr = NULL;
		//current_chmc->unknow_addr = NULL;
		//current_chmc->hitmiss_addr= NULL;
		//current_chmc->hit_change_miss= NULL;

		//int start_addr = bb->startaddr;
		//bb->num_cache_fetch_L2 = bb->size / cache_L2.ls;
		
		//int start_addr_fetch = (start_addr >> cache_L2.lsb) << cache_L2.lsb;
		//tmp = start_addr - start_addr_fetch;

		
		//if(tmp) bb->num_cache_fetch_L2++;

		current_chmc->hitmiss = bb->num_instr;
		//if(current_chmc->hitmiss > 0) 
			current_chmc->hitmiss_addr = (char*)CALLOC(current_chmc->hitmiss_addr, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

		addr = bb->startaddr;
		
		for(n = 0; n < bb->num_instr ; n++)
		{
			set_no = SET_L2(addr);

			if(isInWay(addr, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
			{
				current_chmc->hitmiss_addr[n] = HIT_UPPER;
				addr = addr + INSN_SIZE;
				continue;
			}

			if(main_copy->hit_cache_set_L2[set_no] != USED)
			{	
				numConflictTask[set_no]++;
				numConflictMSC[set_no]++;
				//not hit in L1?
				main_copy->hit_cache_set_L2[set_no] = USED;
			}

			main_copy->hit_addr[set_no].num_entry++;
			if(main_copy->hit_addr[set_no].num_entry == 1)
			{
				main_copy->hit_addr[set_no].entry = (int*)CALLOC(main_copy->hit_addr[set_no].entry, 1, sizeof(int), "entry");
				main_copy->hit_addr[set_no].entry[0] = TAGSET_L2(addr); 
			}
			else
			{
				main_copy->hit_addr[set_no].entry = (int*)REALLOC(main_copy->hit_addr[set_no].entry, main_copy->hit_addr[set_no].num_entry * sizeof(int), "entry");
				main_copy->hit_addr[set_no].entry[main_copy->hit_addr[set_no].num_entry -1] = TAGSET_L2(addr); 
			}


			age = isInCache_L2(addr, bb->bb_cache_state_L2->must[set_no]);
			if(age != -1)
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_HIT;

				main_copy->hit_cache_set_L2[set_no] = USED;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

					current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
					current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;
					
				}
				else
				{
					current_chmc->hit++;	

					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
					current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;

				}
				
				addr_next = addr + INSN_SIZE;
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT;
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = age;
					
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = age;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}


				
			}
			else if(isNeverInCache_L2(addr, bb->bb_cache_state_L2->may[set_no]))
			{
				current_chmc->hitmiss_addr[ n ] = ALWAYS_MISS;

				main_copy->hit_cache_set_L2[set_no] = USED;
					
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
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na - 1;
						
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na - 1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}


			}

			else if(isInCache_L2(addr, bb->bb_cache_state_L2->persist[set_no]) != -1)
			{
				current_chmc->hitmiss_addr[ n ] = FIRST_MISS;
				lp->num_fm_L2 ++;

				main_copy->hit_cache_set_L2[set_no] = USED;
				
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

					current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
					current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;
					
				}
				else
				{
					current_chmc->hit++;	

					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
					current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;

				}
				
				addr_next = addr + INSN_SIZE;
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT;
					
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = age;
					
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = age;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}
			}

			else
			{
				current_chmc->hitmiss_addr[ n ] = UNKNOW;

				main_copy->hit_cache_set_L2[set_no] = USED;
				
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
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na - 1;
				
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age= (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na - 1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}

				
			}
			addr = addr_next;

		}

		for(n = 0; n < bb->num_instr; n++)
		{
			//L1 hit
			if(bb->chmc[cnt]->hitmiss_addr[ n ] == ALWAYS_HIT)
			{
				current_chmc->wcost += IC_HIT;
				current_chmc->bcost += IC_HIT;
			}

			//L1 fm
			else if(bb->chmc[cnt]->hitmiss_addr[ n ] == FIRST_MISS)
			{
				if(loop_level_arr[lp_level] == FIRST_ITERATION)
				{
					if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
					{
						current_chmc->wcost += IC_HIT_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
					
					else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS ||current_chmc->hitmiss_addr[ n ] == FIRST_MISS )
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_MISS_L2;
					}
					
					else
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
				}
				
				else if(loop_level_arr[lp_level] == NEXT_ITERATION)
				{
					current_chmc->wcost += IC_HIT;
					current_chmc->bcost += IC_HIT;
				}
					
			}
			
			//L1 miss
			else if(bb->chmc[cnt]->hitmiss_addr[ n ] == ALWAYS_MISS)
			{
				if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
				{
					current_chmc->wcost += IC_HIT_L2;
					current_chmc->bcost += IC_HIT_L2;
				}

				else if(current_chmc->hitmiss_addr[ n ] == FIRST_MISS)
				{
					if(loop_level_arr[lp_level] == FIRST_ITERATION)
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_MISS_L2;
					}
					else if(loop_level_arr[lp_level] == NEXT_ITERATION)
					{
						current_chmc->wcost += IC_HIT_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
				}
				
				else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS)
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_MISS_L2;

				}
				else
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT_L2;
				}
			}
			//L1 unknow
			else
			{
				if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
				{
					current_chmc->wcost += IC_HIT_L2;
					current_chmc->bcost += IC_HIT;
				}

				else if(current_chmc->hitmiss_addr[ n ] == FIRST_MISS)
				{
					if(loop_level_arr[lp_level] == FIRST_ITERATION)
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_HIT;
					}
					else if(loop_level_arr[lp_level] == NEXT_ITERATION)
					{
						current_chmc->wcost += IC_HIT;
						current_chmc->bcost += IC_HIT_L2;
					}
				}

				else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS)
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT;

				}
				else
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT;
				}	//end L2

			}	//end L1

		}


		current_chmc->hit_copy = current_chmc->hit;
		current_chmc->unknow_copy = current_chmc->unknow;

		current_chmc->wcost_copy = current_chmc->wcost;
		current_chmc->bcost_copy = current_chmc->bcost;


/*
	{
		current_chmc->wcost = bb->chmc[cnt]->hit * IC_HIT;
		current_chmc->wcost += current_chmc->hit * IC_HIT_L2 + (current_chmc->miss + current_chmc->unknow) * IC_MISS_L2;


		current_chmc->bcost  = (bb->chmc[cnt]->hit + bb->chmc[cnt]->unknow) * IC_HIT;

		if(bb->chmc[cnt]->miss > (current_chmc->hit + current_chmc->unknow))
		{
			current_chmc->bcost  +=  (current_chmc->hit + current_chmc->unknow)* IC_HIT_L2; 
			current_chmc->bcost  +=  (bb->chmc[cnt]->miss - (current_chmc->hit + current_chmc->unknow)) * IC_MISS_L2;
		}
		else
		{
			current_chmc->bcost  +=  bb->chmc[cnt]->miss * IC_HIT_L2;
		}
	}
*/
	if(print)
	{
		
		//for(k = 0; k < current_chmc->hitmiss; k++)
			//printf("L2: %d ", current_chmc->hitmiss_addr[k]);
		printf("cnt = %d, bb->size = %d, bb->startaddr = %d\n", cnt, bb->size, bb->startaddr);

		printf("L1:\nnum of fetch = %d, hit = %d, miss= %d, unknow = %d\n", bb->chmc[cnt]->hitmiss, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);
		printf("L2:\nnum of fetch = %d, hit = %d, miss= %d, unknow = %d\n", current_chmc->hitmiss, current_chmc->hit, current_chmc->miss, current_chmc->unknow);

		printf("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);	
	}

	if(print)
		printf("%c", tmp);

		//compute output cache state of this bb
		//check the bb if it is a function call
		if(bb->callpid != -1)
		{
			addr = bb->startaddr;
			
			for(j = 0; j < bb->num_instr; j++)
			{
				set_no = SET_L2(addr);

				if(isInWay(addr, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					continue;
				else 	if(isInWay(addr, current_chmc->unknow_addr, current_chmc->unknow))
				{
					cache_set_must = copyCacheSet(bb->bb_cache_state_L2->must[set_no]);
					cache_set_may = copyCacheSet(bb->bb_cache_state_L2->may[set_no]);
					cache_set_persist = copyCacheSet_persist(bb->bb_cache_state_L2->persist[set_no]);
					
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);

					/* for(tmp = 0; tmp < cache_L2.na; tmp ++)
					{*/

						bb->bb_cache_state_L2->must[set_no] = intersectCacheState_L2(cache_set_must, bb->bb_cache_state_L2->must[set_no]);
						bb->bb_cache_state_L2->may[set_no] = unionCacheState_L2(cache_set_may, bb->bb_cache_state_L2->may[set_no]);
						bb->bb_cache_state_L2->persist[set_no] = unionMaxCacheState_L2(cache_set_persist, bb->bb_cache_state_L2->persist[set_no]);

					/* } */

					freeCacheSet(cache_set_must);
					freeCacheSet(cache_set_may);
					freeCacheSet(cache_set_persist);
				}
				else
				{
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);
				}
				addr = addr + INSN_SIZE;

			}

			cs_ptr = bb->bb_cache_state_L2;

			bb->bb_cache_state_L2 = copyCacheState_L2(mapFunctionCall_L2(bb->proc_ptr, cs_ptr));

			freeCacheState_L2(cs_ptr);
			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);

			//freeCacheStateFunction_L2(bb->proc_ptr);

		}	
		else
		{
			addr = bb->startaddr;
			
			for(j = 0; j < bb->num_instr; j ++)
			{
				set_no = SET_L2(addr);

				if(isInWay(addr, current_chmc->hit_addr, current_chmc->hit))
					continue;
				else 	if(isInWay(addr, current_chmc->unknow_addr, current_chmc->unknow))
				{
					cache_set_must = copyCacheSet(bb->bb_cache_state_L2->must[set_no]);
					cache_set_may = copyCacheSet(bb->bb_cache_state_L2->may[set_no]);
					cache_set_persist = copyCacheSet_persist(bb->bb_cache_state_L2->persist[set_no]);
					
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);

				/*	for(tmp = 0; tmp < cache_L2.na; tmp ++)
					{ */

						bb->bb_cache_state_L2->must[set_no] = intersectCacheState_L2(cache_set_must, bb->bb_cache_state_L2->must[set_no]);
						bb->bb_cache_state_L2->may[set_no] = unionCacheState_L2(cache_set_may, bb->bb_cache_state_L2->may[set_no]);
						bb->bb_cache_state_L2->persist[set_no] = unionMaxCacheState_L2(cache_set_persist, bb->bb_cache_state_L2->persist[set_no]);

				/*	} */
					freeCacheSet(cache_set_must);
					freeCacheSet(cache_set_may);
					freeCacheSet(cache_set_persist);
					
				}
				else
				{
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);
				}
				addr = addr + INSN_SIZE;
				
			}

			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);
			
		}// normal bb
		
	}
	
	return p->bblist[lp ->topo[0]->bbid]->bb_cache_state_L2;
}



static cache_state *
copyCacheState_L2(cache_state *cs)
{
	int j, k, num_entry;
	cache_state *copy = NULL;

	//printf("\nIn copy Cache State now\n");

	copy = (cache_state*)CALLOC(copy, 1, sizeof(cache_state), "cache_state");
	copy->must = NULL;
	copy->may = NULL;
	copy->persist = NULL;

		
	//lp_level = cs->loop_level;
  //int i;
//	for( i = 0; i < copies; i ++)
//	{
		//printf("\nIn copyCacheState: i is %d\n", i);

		copy->must = (cache_line_way_t***)CALLOC(copy->must, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		copy->may = (cache_line_way_t***)CALLOC(copy->may, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		copy->persist = (cache_line_way_t***)CALLOC(copy->persist, cache_L2.ns, sizeof(cache_line_way_t**), "NO set cache_line_way_t");

		for(j = 0; j < cache_L2.ns; j++)
		{
			copy->must[j] = (cache_line_way_t**)	CALLOC(copy->must[j], cache_L2.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			copy->may[j] = (cache_line_way_t**)CALLOC(copy->may[j], cache_L2.na, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			copy->persist[j] = (cache_line_way_t**)CALLOC(copy->persist[j], cache_L2.na + 1, sizeof(cache_line_way_t*), "NO assoc cache_line_way_t");

			for( k = 0; k < cache_L2.na; k++)
			{
				copy->must[j][k] = (cache_line_way_t*)CALLOC(copy->must[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				copy->may[j][k] = (cache_line_way_t*)CALLOC(copy->may[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");
				copy->persist[j][k] = (cache_line_way_t*)CALLOC(copy->persist[j][k], 1, sizeof(cache_line_way_t), "one cache_line_way_t");

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


			copy->persist[j][cache_L2.na] = (cache_line_way_t*)CALLOC(copy->persist[j][cache_L2.na], 1, sizeof(cache_line_way_t), "one cache_line_way_t may");
			copy->persist[j][cache_L2.na]->num_entry = cs->persist[j][cache_L2.na]->num_entry;
			if(copy->persist[j][cache_L2.na]->num_entry)
			{
				copy->persist[j][cache_L2.na]->entry = (int*)CALLOC(copy->persist[j][cache_L2.na]->entry, copy->persist[j][cache_L2.na]->num_entry, sizeof(int), "entries");

				for(num_entry = 0; num_entry < copy->persist[j][cache_L2.na]->num_entry; num_entry++)
					copy->persist[j][cache_L2.na]->entry[num_entry] =  cs->persist[j][cache_L2.na]->entry[num_entry];
			}

		}
//	}
	return copy;
}





static cache_state *
mapFunctionCall_L2(procedure *proc, cache_state *cs)
{
	int i, j, k, n, set_no, cnt, addr, addr_next, copies, tmp, tag, tag_next; 
	int lp_level, age;

	//dumpCacheState(cs);
	//exit(1);
	procedure *p = proc;
	block *bb, *incoming_bb;
	cache_state *cs_ptr;
	cache_line_way_t **cache_set_must, **cache_set_may, **clw;
	CHMC *current_chmc;
	
	//printf("\nIn mapFunctionCall, p[%d]\n", p->pid);
	
	cs_ptr = copyCacheState_L2(cs);
	
	int  num_blk = p->num_topo;	


	for(i = 0; i < MAX_NEST_LOOP; i++)
		if(loop_level_arr[i] == INVALID)
		{
			lp_level = i -1;
			break;
		}

	cnt = 0;		
	
	for(k = 0; k <= lp_level; k++)
		if(loop_level_arr [k] == NEXT_ITERATION)
			cnt += (1<<(lp_level - k));
		
	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		bb = p ->topo[i];

		bb = p->bblist[ bb->bbid ];

		bb->num_instr = bb->size / INSN_SIZE;

		if(bb->is_loophead)
		{

			loop_level_arr[lp_level +1] = FIRST_ITERATION;
			p->loops[bb->loopid]->num_fm_L2 = 0;
			
			mapLoop_L2(p, p->loops[bb->loopid]);


			loop_level_arr[lp_level +1] = NEXT_ITERATION;

			mapLoop_L2(p, p->loops[bb->loopid]);

			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);

			loop_level_arr[lp_level +1] = INVALID;

			//freeCacheStateLoop_L2(p, p->loops[bb->loopid]);
			continue;
		}

		
		if(bb->num_incoming > 0)
		{
			incoming_bb = p->bblist[bb->incoming[0]];
			
			//printBlock(incoming_bb);
			if(!incoming_bb->bb_cache_state_L2) continue;
			
			cs_ptr = copyCacheState_L2(incoming_bb->bb_cache_state_L2);


			incoming_bb->num_outgoing--;
			if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state_L2 == 1)
			{
				incoming_bb->num_cache_state_L2 = 0;
				//freeCacheState_L2(incoming_bb->bb_cache_state_L2);
				//incoming_bb->bb_cache_state_L2 = NULL;
			}
			
			
			if(bb->num_incoming > 1)
			{
				if(print) printf("\ndo operations if more than one incoming edge\n");

				//dumpCacheState(cs_ptr);
				//printBlock(incoming_bb);

				for(j = 1; j < bb->num_incoming; j++)
				{
					incoming_bb = p->bblist[bb->incoming[j]];

					if(incoming_bb->bb_cache_state_L2 == NULL) continue;
					
					for(k = 0; k < cache_L2.ns; k++)
					{
						clw = cs_ptr->must[k];							
						cs_ptr->must[k] = intersectCacheState_L2(cs_ptr->must[k], incoming_bb->bb_cache_state_L2->must[k]);

						freeCacheSet_L2(clw);

						clw = cs_ptr->may[k];							
						cs_ptr->may[k] = unionCacheState_L2(cs_ptr->may[k], incoming_bb->bb_cache_state_L2->may[k]);

						freeCacheSet_L2(clw);

						clw = cs_ptr->persist[k];							
						cs_ptr->persist[k] = unionMaxCacheState_L2(cs_ptr->persist[k], incoming_bb->bb_cache_state_L2->persist[k]);

						freeCacheSet_L2(clw);

					}

					incoming_bb->num_outgoing--;
					if(incoming_bb->num_outgoing < 1 && incoming_bb->num_cache_state_L2 == 1)
					{
						incoming_bb->num_cache_state_L2 = 0;
						//freeCacheState_L2(incoming_bb->bb_cache_state_L2);
						//incoming_bb->bb_cache_state_L2 = NULL;
					}
	
				}	//end for all incoming
				if(print) dumpCacheState_L2(cs_ptr);
				//cs_ptr->source_bb = NULL;
				//exit(1);
			}
		}



		if(bb->num_cache_state_L2== 0)
		{
			//bb->bb_cache_state_L2 = (cache_state *)CALLOC(bb->bb_cache_state_L2, 1, sizeof(cache_state), "cache_state");

			bb->num_cache_state_L2 = 1;
		}

		if(bb->num_chmc_L2 == 0)
		{
			if(lp_level == -1)
				copies = 1;
			else
				copies = 2<<(lp_level);

			bb->num_chmc_L2 = copies;

			bb->chmc_L2 = (CHMC**)CALLOC(bb->chmc_L2, copies, sizeof(CHMC*), "CHMC");
	
			for(tmp = 0; tmp < copies; tmp++)
			{
				bb->chmc_L2[tmp] = (CHMC*)CALLOC(bb->chmc_L2[tmp], 1, sizeof(CHMC), "CHMC");
			}

		}


		bb->bb_cache_state_L2 = cs_ptr;
		
		current_chmc = bb->chmc_L2[cnt];

		current_chmc->hit = 0;
		current_chmc->hit_copy = 0;

		current_chmc->miss = 0;
		
		current_chmc->unknow = 0;
		current_chmc->unknow_copy = 0;

		current_chmc->wcost = 0;
		current_chmc->bcost = 0;
		current_chmc->wcost_copy = 0;
		current_chmc->bcost_copy = 0;
	
		
		//current_chmc->hit_addr = NULL;
		//current_chmc->miss_addr = NULL;
		//current_chmc->unknow_addr = NULL;
		//current_chmc->hitmiss_addr= NULL;
		//current_chmc->hit_change_miss= NULL;
		
		//compute categorization of each instruction line
		//int start_addr = bb->startaddr;
		//bb->num_cache_fetch_L2 = bb->size / cache_L2.ls;

		//int start_addr_fetch = (start_addr >> cache_L2.lsb) << cache_L2.lsb;
		//tmp = start_addr - start_addr_fetch;

		
		//tmp is not 0 if start address is not multiple of sizeof(cache line) in bytes
		//if(tmp) bb->num_cache_fetch_L2++;

		current_chmc->hitmiss = bb->num_instr;
		//if(current_chmc->hitmiss > 0) 
			current_chmc->hitmiss_addr = (char*)CALLOC(current_chmc->hitmiss_addr, current_chmc->hitmiss, sizeof(char),"hitmiss_addr");

		addr = bb->startaddr;


		//printf("bbid = %d, num_instr = %d, hit = %d, miss = %d, unknow = %d\n", bb->bbid, bb->num_instr, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);

		for(n = 0; n < bb->num_instr; n++)
		{
			//if(bb->is_loophead!= 0)
				//break;
			set_no = SET_L2(addr);

			//hit in L1?
			if(isInWay(addr, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
			{
                                //printf("hello world %d\n", addr);
				current_chmc->hitmiss_addr[n] = HIT_UPPER;
				addr = addr +  INSN_SIZE;
				continue;
			}

			if(main_copy->hit_cache_set_L2[set_no] != USED)
			{	
				numConflictTask[set_no]++;
				numConflictMSC[set_no]++;
				//not hit in L1?
				main_copy->hit_cache_set_L2[set_no] = USED;
			}
			
			main_copy->hit_addr[set_no].num_entry++;
			if(main_copy->hit_addr[set_no].num_entry == 1)
			{
				main_copy->hit_addr[set_no].entry = (int*)CALLOC(main_copy->hit_addr[set_no].entry, 1, sizeof(int), "entry");
				main_copy->hit_addr[set_no].entry[0] = TAGSET_L2(addr); 
			}
			else
			{
				main_copy->hit_addr[set_no].entry = (int*)REALLOC(main_copy->hit_addr[set_no].entry, main_copy->hit_addr[set_no].num_entry * sizeof(int), "entry");
				main_copy->hit_addr[set_no].entry[main_copy->hit_addr[set_no].num_entry -1] = TAGSET_L2(addr); 
			}

			age = isInCache_L2(addr, bb->bb_cache_state_L2->must[set_no]);
			if(age !=-1)
			{
				current_chmc->hitmiss_addr[n] = ALWAYS_HIT;

				//current_chmc->hitmiss++;
				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

					current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");

					current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;
				}
				else
				{
					current_chmc->hit++;		
					
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
					current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "hit_change_miss");				

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;


				}
				//current_chmc_copy->hit = current_chmc->hit;

				addr_next = addr + INSN_SIZE;
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT;

					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");

						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;
						
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}

			}
			else if(isNeverInCache_L2(addr, bb->bb_cache_state_L2->may[set_no]))
			{
				current_chmc->hitmiss_addr[n] = ALWAYS_MISS;

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
				//current_chmc_copy->miss = current_chmc->miss;

				addr_next = addr + INSN_SIZE;
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");

						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;
						
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}

				
			}

			else if(isInCache_L2(addr, bb->bb_cache_state_L2->persist[set_no])!= -1)
			{
				current_chmc->hitmiss_addr[n] = FIRST_MISS;

				if(current_chmc->hit == 0)
				{
					current_chmc->hit++;
					current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

					current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");

					current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;
				}
				else
				{
					current_chmc->hit++;		
					
					current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
					current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
					current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "hit_change_miss");				

					current_chmc->hit_addr[current_chmc->hit-1] = addr;
					current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
					current_chmc->age[current_chmc->hit-1] = age;


				}
				//current_chmc_copy->hit = current_chmc->hit;

				addr_next = addr + INSN_SIZE;
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 

					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");

						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;
						
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}

			}
			else
			{
				current_chmc->hitmiss_addr[n] = UNKNOW;
			        
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
				tag = SET_L2(addr);
				tag_next = SET_L2(addr_next);
				while(tag == tag_next && n < bb->num_instr - 1)
				{
					n++;
					current_chmc->hitmiss_addr[n] = ALWAYS_HIT; 
					if(!isInWay(addr_next, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
					{
						if(current_chmc->hit == 0)
					       {
						current_chmc->hit++;
						current_chmc->hit_addr = (int*)CALLOC(current_chmc->hit_addr, 1, sizeof(int), "hit_addr");

						current_chmc->hit_change_miss = (char*)CALLOC(current_chmc->hit_change_miss, 1, sizeof(char), "hit_change_miss");
						current_chmc->age = (char*)CALLOC(current_chmc->age, 1, sizeof(char), "age");

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;
						
					     	}
					  	else
					  	{
						current_chmc->hit++;		
						
						current_chmc->hit_addr = (int*)REALLOC(current_chmc->hit_addr, current_chmc->hit * sizeof(int), "hit_addr");				
						current_chmc->hit_change_miss = (char*)REALLOC(current_chmc->hit_change_miss, current_chmc->hit * sizeof(char), "hit_change_miss");				
						current_chmc->age = (char*)REALLOC(current_chmc->age, current_chmc->hit * sizeof(char), "age");				

						current_chmc->hit_addr[current_chmc->hit-1] = addr_next;
						current_chmc->hit_change_miss[current_chmc->hit-1] = HIT;
						current_chmc->age[current_chmc->hit-1] = cache_L2.na -1;

						}
					}	
					
                                        addr_next += INSN_SIZE;
	                                tag_next = SET_L2(addr_next);
             						  

				}

				
			} //end else
			
			addr = addr_next;
		}

	       //printf("bbid = %d, num_instr = %d, hit = %d, miss = %d, unknow = %d\n", bb->bbid, bb->num_instr, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);

		//printf("bbid = %d, num_instr = %d, L2: hit = %d, miss = %d, unknow = %d\n", bb->bbid, bb->num_instr, current_chmc->hit,current_chmc->miss, current_chmc->unknow);
	        assert((bb->chmc[cnt]->miss + bb->chmc[cnt]->unknow) == (current_chmc->hit + current_chmc->miss + current_chmc->unknow));
		
		for(n = 0; n < bb->num_instr; n++)
		{
			//L1 hit
			if(bb->chmc[cnt]->hitmiss_addr[ n ] == ALWAYS_HIT)
			{
				current_chmc->wcost += IC_HIT;
				current_chmc->bcost += IC_HIT;
			}

			//L1 fm
			else if(bb->chmc[cnt]->hitmiss_addr[ n ] == FIRST_MISS)
			{
				if(loop_level_arr[lp_level] == FIRST_ITERATION)
				{
					if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
					{
						current_chmc->wcost += IC_HIT_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
					
					else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS ||current_chmc->hitmiss_addr[ n ] == FIRST_MISS )
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_MISS_L2;
					}
					
					else
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
				}
				
				else if(loop_level_arr[lp_level] == NEXT_ITERATION)
				{
					current_chmc->wcost += IC_HIT;
					current_chmc->bcost += IC_HIT;
				}
					
			}
			
			//L1 miss
			else if(bb->chmc[cnt]->hitmiss_addr[ n ] == ALWAYS_MISS)
			{
				if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
				{
					current_chmc->wcost += IC_HIT_L2;
					current_chmc->bcost += IC_HIT_L2;
				}

				else if(current_chmc->hitmiss_addr[ n ] == FIRST_MISS)
				{
					if(loop_level_arr[lp_level] == FIRST_ITERATION)
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_MISS_L2;
					}
					else if(loop_level_arr[lp_level] == NEXT_ITERATION)
					{
						current_chmc->wcost += IC_HIT_L2;
						current_chmc->bcost += IC_HIT_L2;
					}
				}
				
				else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS)
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_MISS_L2;

				}
				else
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT_L2;
				}
			}
			//L1 unknow
			else
			{
				if(current_chmc->hitmiss_addr[ n ] == ALWAYS_HIT)
				{
					current_chmc->wcost += IC_HIT_L2;
					current_chmc->bcost += IC_HIT;
				}

				else if(current_chmc->hitmiss_addr[ n ] == FIRST_MISS)
				{
					if(loop_level_arr[lp_level] == FIRST_ITERATION)
					{
						current_chmc->wcost += IC_MISS_L2;
						current_chmc->bcost += IC_HIT;
					}
					else if(loop_level_arr[lp_level] == NEXT_ITERATION)
					{
						current_chmc->wcost += IC_HIT;
						current_chmc->bcost += IC_HIT_L2;
					}
				}

				else if(current_chmc->hitmiss_addr[ n ] == ALWAYS_MISS)
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT;

				}
				else
				{
					current_chmc->wcost += IC_MISS_L2;
					current_chmc->bcost += IC_HIT;
				}	//end L2

			}	//end L1

		}

	
		current_chmc->hit_copy = current_chmc->hit;
		current_chmc->unknow_copy = current_chmc->unknow;

		current_chmc->wcost_copy = current_chmc->wcost;
		current_chmc->bcost_copy = current_chmc->bcost;

		/*
		current_chmc->wcost = bb->chmc[cnt]->hit * IC_HIT;
		current_chmc->wcost += current_chmc->hit * IC_HIT_L2 + (current_chmc->miss + current_chmc->unknow) * IC_MISS_L2;


		current_chmc->bcost  = (bb->chmc[cnt]->hit + bb->chmc[cnt]->unknow) * IC_HIT;

		if(bb->chmc[cnt]->miss > (current_chmc->hit + current_chmc->unknow))
		{
			current_chmc->bcost  +=  (current_chmc->hit + current_chmc->unknow)* IC_HIT_L2; 
			current_chmc->bcost  +=  (bb->chmc[cnt]->miss - (current_chmc->hit + current_chmc->unknow)) * IC_MISS_L2;
		}
		else
		{
			current_chmc->bcost  +=  bb->chmc[cnt]->miss * IC_HIT_L2;
		}
		*/

	if(print)
	{
		
		//for(k = 0; k < current_chmc->hitmiss; k++)
			//printf("L2: %d ", current_chmc->hitmiss_addr[k]);
		printf("cnt = %d, bb->size = %d, bb->startaddr = %d\n", cnt, bb->size, bb->startaddr);

		printf("L1:\nnum of fetch = %d, hit = %d, miss= %d, unknow = %d\n", bb->chmc[cnt]->hitmiss, bb->chmc[cnt]->hit, bb->chmc[cnt]->miss, bb->chmc[cnt]->unknow);
		printf("L2:\nnum of fetch = %d, hit = %d, miss= %d, unknow = %d\n", current_chmc->hitmiss, current_chmc->hit, current_chmc->miss, current_chmc->unknow);

		printf("\nwcost = %d, bcost = %d\n", current_chmc->wcost, current_chmc->bcost);	
	}
	if(print)
		printf("%c\n", tmp);
		//check the bb if it is a function call
		
		if(bb->callpid != -1)
		{
			addr = bb->startaddr;
			
			for(j = 0; j < bb->num_instr; j ++)
			{
			
				set_no = SET_L2(addr);

				if(isInWay(addr, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
				{
					continue;
				}
				else if(isInWay(addr, current_chmc->unknow_addr, current_chmc->unknow))
				{
					cache_set_must = copyCacheSet(bb->bb_cache_state_L2->must[set_no]);
					cache_set_may = copyCacheSet(bb->bb_cache_state_L2->may[set_no]);
					
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);

					/*for(tmp = 0; tmp < cache_L2.na; tmp ++)
					{*/

						bb->bb_cache_state_L2->must[set_no] = intersectCacheState_L2(cache_set_must, bb->bb_cache_state_L2->must[set_no]);
						bb->bb_cache_state_L2->may[set_no] = unionCacheState_L2(cache_set_may, bb->bb_cache_state_L2->may[set_no]);

					/*}*/
					freeCacheSet(cache_set_must);
					freeCacheSet(cache_set_may);
				}
				else
				{
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);
				}

				addr = addr + INSN_SIZE;

			}

			cs_ptr = bb->bb_cache_state_L2;
			bb->bb_cache_state_L2 = copyCacheState_L2(mapFunctionCall_L2(bb->proc_ptr, cs_ptr));
			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);
			freeCacheState_L2(cs_ptr);
			cs_ptr = NULL;
			//freeCacheStateFunction_L2(bb->proc_ptr);
		}

		else
		{
			addr = bb->startaddr;

			for(j = 0; j < bb->num_instr; j ++)
			{
				set_no = SET_L2(addr);

				//hit in L1
				if(isInWay(addr, bb->chmc[cnt]->hit_addr, bb->chmc[cnt]->hit))
				{
					continue;
				}
				//unknow in L1, consider access and not access L2, both cases
				else 	if(isInWay(addr, current_chmc->unknow_addr, current_chmc->unknow))
				{
					cache_set_must = copyCacheSet(bb->bb_cache_state_L2->must[set_no]);
					cache_set_may = copyCacheSet(bb->bb_cache_state_L2->may[set_no]);
					
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);

					/*for(tmp = 0; tmp < cache_L2.na; tmp ++)
					{*/

						bb->bb_cache_state_L2->must[set_no] = intersectCacheState_L2(cache_set_must, bb->bb_cache_state_L2->must[set_no]);
						bb->bb_cache_state_L2->may[set_no] = unionCacheState_L2(cache_set_may, bb->bb_cache_state_L2->may[set_no]);

					/*}*/
					freeCacheSet(cache_set_must);
					freeCacheSet(cache_set_may);
				}
				//miss in L1
				else
				{
					calculateCacheState_L2(bb->bb_cache_state_L2->must[set_no], bb->bb_cache_state_L2->may[set_no], bb->bb_cache_state_L2->persist[set_no], addr);
				}
				addr = addr  +INSN_SIZE;

			}
			
			//free(cs_ptr);
			//cs_ptr = copyCacheState_L2(bb->bb_cache_state_L2);
		}// end if else

	}

	for(i = 0; i < p->num_bb; i ++)
		p->bblist[i]->num_outgoing = p->bblist[i]->num_outgoing_copy;

	return p ->bblist[ p->topo[0]->bbid]->bb_cache_state_L2;
}


static void
resetFunction_L2(procedure * proc)
{
	int i, j, cnt, lp_level, num_blk;

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
			resetLoop_L2(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			resetLoop_L2(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = INVALID;

			continue;
		}


		bb->chmc_L2[cnt]->hit =  bb->chmc_L2[cnt]->hit_copy;
		bb->chmc_L2[cnt]->unknow =  bb->chmc_L2[cnt]->unknow_copy;

		bb->chmc_L2[cnt]->wcost = bb->chmc_L2[cnt]->wcost_copy;
		bb->chmc_L2[cnt]->bcost = bb->chmc_L2[cnt]->bcost_copy;
		
		for(j = 0; j < bb->chmc_L2[cnt]->hit; j ++)
			bb->chmc_L2[cnt]->hit_change_miss[j] = HIT;

		//check the bb if it is a function call
		
		if(bb->callpid != -1)
		{
			resetFunction_L2(bb->proc_ptr);
		}
		
	}
			
}

static void
resetLoop_L2(procedure * proc, loop * lp)
{

	int i, j, cnt, lp_level, num_blk;

	procedure *p = proc;
	block *bb;
	loop *lp_ptr = lp;

	num_blk = lp_ptr->num_topo;

	//printf("pathLoop\n");

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

	for(i = num_blk -1 ; i >= 0 ; i--)
	{	
		//bb is in topo list, to get the bbid of this bb
		bb = lp_ptr->topo[i];

		//bb is in bblist
		bb = p->bblist[ bb->bbid ];

		if(bb->is_loophead && i!= num_blk -1)
		{

			loop_level_arr[lp_level +1] = FIRST_ITERATION;

			resetLoop_L2(p, p->loops[bb->loopid]);

			loop_level_arr[lp_level +1] = NEXT_ITERATION;
			resetLoop_L2(p, p->loops[bb->loopid]);
			
			loop_level_arr[lp_level +1] = INVALID;

			continue;
		}

		//check the bb if it is a function call
		
		bb->chmc_L2[cnt]->hit =  bb->chmc_L2[cnt]->hit_copy;
		bb->chmc_L2[cnt]->unknow =  bb->chmc_L2[cnt]->unknow_copy;

		bb->chmc_L2[cnt]->wcost = bb->chmc_L2[cnt]->wcost_copy;
		bb->chmc_L2[cnt]->bcost = bb->chmc_L2[cnt]->bcost_copy;

		for(j = 0; j < bb->chmc_L2[cnt]->hit; j ++)
			bb->chmc_L2[cnt]->hit_change_miss[j] = HIT;
		
		if(bb->callpid != -1)
		{
			resetFunction_L2(bb->proc_ptr);
			
		}
		
	}
			
}// end if else




static void
resetHitMiss_L2(MSC *msc)
{
	int i;
	//printf("\nreset %s\n", msc->msc_name);
	
	for(i = 0; i < MAX_NEST_LOOP; i++)
		loop_level_arr[i] = INVALID;
	for(i = 0; i < msc->num_task; i++)
	{
		resetFunction_L2(msc->taskList[i].main_copy);
	}
}



//do level one cache analysis
static void
cacheAnalysis_L2()
{
	int i;

	instr_per_block_L2 = cache_L2.ls / INSN_SIZE;
	
	for(i = 0; i < MAX_NEST_LOOP; i++)
		loop_level_arr[i] = INVALID;

	//set initial cache state for main precedure
	cache_state *start;

	start = allocCacheState_L2();

	main_copy->hit_cache_set_L2 = (char*)CALLOC(main_copy->hit_cache_set_L2, cache_L2.ns, sizeof(char), "hit_cache_set_L2");
	main_copy->hit_addr = (cache_line_way_t*)CALLOC(main_copy->hit_addr, cache_L2.ns, sizeof(cache_line_way_t), "cache_line_way_t");
	for(i = 0; i < cache_L2.ns; i++)
		main_copy->hit_cache_set_L2[i] = NOT_USED;

	
	start = mapFunctionCall_L2(main_copy, start);

        //exit(1);

	//printf("\nprocedure %d\n\n", main_copy->pid);
	//for(i = 0; i < cache_L2.ns; i++)
		//printf("%d ", main_copy->hit_cache_set_L2[i]);
	//printf("\n");

}



/*

// from way 0-> way n, younger->older
static cache_line_way_t *
unionCacheState_L2(cache_line_way_t * clw_a, cache_line_way_t *clw_b)
{
	int i, j, k, entry_a, entry_b, t, n, num_history = 0;
	int *history_entries = NULL;
	//int flag = 1;
	cache_line_way_t *result = 
	(cache_line_way_t *) CALLOC(result, cache_L2.na, sizeof(cache_line_way_t), "cache line way");

	//printf("test in union, result[0].num_entry is : %d ", result[0].num_entry);
	//dump_cache_line( result );
	
	//for each way, calculate the result of cs
	for(i = 0; i < cache_L2.na; i++)
	{
		result[i].num_entry = 0;
		
		//for each entry in clw_a, is it present in the history? 
		//no, add it into result and history
		for(j = 0 ; j < clw_a[i].num_entry; j++)
		{
			entry_a = clw_a[i].entry[j];
			if(isInWay(entry_a, history_entries, num_history))
				continue;
			else
			{
			//add to result
			result[i].num_entry++;

			result[i].entry = (int *)REALLOC(result[i].entry, result[i].num_entry*sizeof(int), "cache line way");
			result[i].entry[result[i].num_entry - 1] = entry_a;

			//add to history
			num_history++;

			history_entries = (int *)REALLOC(history_entries, num_history*sizeof(int), "cache line way");
			history_entries[num_history - 1] = entry_a;
			} // end if
		} //end for


		//for each entry in clw_b[i], is it in result[i]
		//no, is it present in the history? 
		//no, add it into result[i] and history
		for(j = 0; j < clw_b[i].num_entry; j++)
		{
			entry_b = clw_b[i].entry[j];
			if(isInWay(entry_b, result[i].entry, result[i].num_entry))
				continue;
			else if(!isInWay(entry_b, history_entries, num_history))
			{
				//add  clw_b[i].entry[j] into result[i].entry
				result[i].num_entry ++;
				result[i].entry = (int *) 
				REALLOC(result[i].entry, result[i].num_entry*sizeof(int), "cache line way");
				result[i].entry[result[i].num_entry - 1] = entry_b;

				//add to history
				num_history++;
				history_entries = (int *)REALLOC(history_entries, num_history*sizeof(int), "cache line way");
				history_entries[num_history - 1] = entry_b;
			 } //end if
		} //end for

	}
return result;
	
}
*/

/*
// from way n-> way 0, older->younger
static cache_line_way_t *
intersectCacheState_L2(cache_line_way_t * clw_a, cache_line_way_t *clw_b)
{
	int i, j, k, entry_a, entry_b, num_history = 0;
	int *history_entries = NULL;
	//int flag = 1;
	cache_line_way_t *result = (cache_line_way_t *) CALLOC(result, 1, sizeof(cache_line_way_t), "cache line way");


	//for each way, calculate the result of cs
	//for(i = cache_L2.na - 1; i >= 0; i--)
	{
		result[i].num_entry = 0;
		
		//kick out entries in clw_b not in clw_a
		for(j = 0; j < clw_b[i].num_entry; j++)
		{
			entry_b = clw_b[i].entry[j];
			if(!isInWay(entry_b, clw_a[i].entry, clw_a[i].num_entry))
				continue;
			else if(!isInWay(entry_b, history_entries, num_history))
			{
				//add  clw_a[i].entry[j] into result[i].entry
				result[i].num_entry ++;
				result[i].entry = (int *)  
				REALLOC(result[i].entry , result[i].num_entry*sizeof(int), "cache line way");
				result[i].entry[result[i].num_entry - 1] = entry_b;

						//add to history
				num_history++;
				history_entries = (int *)REALLOC(history_entries, num_history*sizeof(int), "cache line way");
				history_entries[num_history - 1] = entry_b;	
			} //end if
		} // end for

	}
return result;
}
*/


/*
static void
dump_cache_line(cache_line_way_t *clw_a)
{
	int i, j;
	//printf("No of ways %d\n", cache.na);
		//printf("way %d :	\n", i);
	if(clw_a->num_entry == 0)
	{
		printf("NULL\n");
		return;
	}
	for(j = 0; j < clw_a->num_entry; j++)
		printf(" %d ", clw_a->entry[j]);

	printf("\n");
	
}

static char
test_cs_op()
{
cache_line_way_t *clw_a, *clw_b;

//initialization for clw_a
clw_a = (cache_line_way_t*) CALLOC(clw_a , 2, sizeof(cache_line_way_t), "a");

clw_a[0].num_entry = 3;
clw_a[1].num_entry = 3;
clw_a[0].entry = (int*) CALLOC(clw_a[0].entry, clw_a[0].num_entry, sizeof(int), "entries for a");
clw_a[1].entry = (int*) CALLOC(clw_a[1].entry, clw_a[1].num_entry, sizeof(int), "entries for a");

clw_a[0] .entry[0] = 1;
clw_a[0] .entry[1] = 2;
clw_a[0] .entry[2] = 5;
clw_a[1] .entry[0] = 2;
clw_a[1] .entry[1] = 3;
clw_a[1] .entry[2] = 4;
//initialization for clw_b
clw_b = (cache_line_way_t*) CALLOC(clw_b , 3, sizeof(cache_line_way_t), "b");

clw_b[0].num_entry = 3;
clw_b[1].num_entry = 3;
clw_b[0].entry = (int*) CALLOC(clw_b[0].entry, clw_b[0].num_entry, sizeof(int), "entries for b");
clw_b[1].entry = (int*) CALLOC(clw_b[1].entry, clw_b[1].num_entry, sizeof(int), "entries for b");

clw_b[0] .entry[0] = 3;
clw_b[0] .entry[1] = 2;
clw_b[0] .entry[1] = 5;
clw_b[1] .entry[0] = 4;
clw_b[1] .entry[1] = 3;
clw_b[1] .entry[2] = 2;
dump_cache_line(clw_a);
printf("\n");
dump_cache_line(clw_b);
printf("\n");
printf("Result for union: \n");
dump_cache_line(unionCacheState(clw_a, clw_b));
printf("\n");
printf("Result for intersect: \n");
dump_cache_line(intersectCacheState(clw_a, clw_b));	
printf("\n");
return 0;
}
*/












