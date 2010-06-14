/*
 * DAG-based WCET analysis functions.
 */

#include <stdio.h>


/*
 * Goes through DAG in reverse topological order (given in topo), collecting weight.
 * Can be used for both loop (disregarding back-edge) and procedure call (disregarding loops).
 * Reverse topological order ensures that by the time we reach a block,
 * the cost of its successors are already computed.
 *
 * The cost of a basic block (final value of wcet_arr) includes:
 * - its own cost, i.e. #instruction cycles + variable access cost
 * - cost of procedure called within it
 * - cost of its immediate successor
 * (and then, minus variable access cost due to allocated variables).
 *
 * When writing ILP constraints, cost of procedure call and cost of immediate successor
 * are given as variables (with their own related constraints), e.g.
 *   W1 = W2 + cost1 - GainAlloc + WProc
 * where cost1 is the cost of the basic block (variable access cost included).
 * Thus we should first obtain this constant, then write ILP constraints,
 * and then only update the cost to include successor and procedure call.
 *
 * Note: the above is written in CPLEX as
 *   W1 - W2 + GainAlloc - WProc = cost1
 * i.e. LHS are variables only and RHS is a single constant.
 *
 */
 
int analyseDAGFunction(procedure *proc, int index)
{

  ull *wcet_arr = NULL;
  // Stores the weight of each node including the weight of its successors.
  // The index corresponds to basic block id.

  //char *tmp_wpath;
  // Stores the heaviest branch taken at the node.

  int i, id;
  char fn[100];
  char max;
  block *bb;
  int freq;

  procedure *p;
  loop *lp, *lpn;

  char *wpath;
  // int len;

  block **topo;
  int num_topo;

  // print_cfg();

    p        =  proc;
    topo     = p->topo;
    num_topo = p->num_topo;

  // store temporary calculations, index corresponding to bblist for ease of locating


    // ****** THE REGION TRANSITION IS NOT CORRECTLY & COMPLETELY ADAPTED HERE YET **********

    wcet_arr  = (ull*)  CALLOC( wcet_arr,  p->num_bb, sizeof(ull), "wcet_arr" );
   // tmp_wpath = (char*) CALLOC( tmp_wpath, p->num_bb, sizeof(char), "tmp_wpath" );

    for( i = 0; i < num_topo; i++ ) {
      bb = topo[i];
      bb = p->bblist[bb->bbid];
      // printf( "bb %d-%d: ", bb->pid, bb->bbid );
      // printBlock( bb );

      // if dummy block, ignore
      if( bb->startaddr == -1 )
	continue;

      // if nested loophead, directly use the wcet of loop (should have been calculated)
      if( bb->is_loophead) 
      {
		lpn = p->loops[ bb->loopid ];
		
		wcet_arr[ bb->bbid ] = lpn->wcet[index * 2] + lpn->wcet[index * 2 + 1];

	// add wcet at loopexit ???
		wcet_arr[ bb->bbid ] += wcet_arr[ lpn->loopexit->bbid ];
	//if(print)
		{
			printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);
		}

      } // end if nested loophead

      else {
	  //cost of this bb
		wcet_arr[ bb->bbid ] = bb->chmc_L2[index]->wcost;
	  
	//if(print)
		{
			printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);
		}

	//---- successors ----
	switch( bb->num_outgoing ) {

	case 0:
	  break;

	case 1:	 // just add the weight of successor
	  wcet_arr[ bb->bbid ] += wcet_arr[ bb->outgoing[0] ];
	//if(print)
		{
			printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);
		}
	  break;

	case 2:  // choose heaviest
	  if(bb->loopid != p->bblist[bb->outgoing[0]]->loopid)
	  	max = 1;
	  else if(bb->loopid != p->bblist[bb->outgoing[1]]->loopid)
	  	max = 0;
	  
	 else
	 {
	 	if( wcet_arr[ bb->outgoing[0] ] >= wcet_arr[ bb->outgoing[1] ] )
	    		max = 0;
	  	else
	    		max = 1;
	 }
	  wcet_arr[ bb->bbid ] += wcet_arr[ bb->outgoing[max] ];
	//if(print)
		{
			printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);
		}

	  
	 // tmp_wpath[ bb->bbid ] = max;
	  break;

	default:
	  printf( "Invalid number of outgoing edges at %d-%d\n", bb->pid, bb->bbid );
	  exit(1);

	} // end switch

	// called procedure
	if( bb->callpid != -1 ) {
	//add cost of the procedure
  	 	wcet_arr[ bb->bbid ] += bb->proc_ptr->wcet[index];

	  printf( "bb %d-%d procedure call %d wcet: %Lu\n", bb->pid, bb->bbid,
		  bb->callpid, p->wcet[index] ); fflush( stdout );

	//if(print)
		{
			printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);
		}


	/*
	  // region transition
	  if( regionmode && bb->regid != -1 ) {
	    id = procs[bb->callpid]->bblist[0]->regid;
	    if( id != -1 && bb->regid != id ) {
	      printf( "region transition %d-%d(%d) procedure call %d(%d) cost: %u\n",
		      bb->pid, bb->bbid, bb->regid, bb->callpid, id, regioncost[id] ); fflush( stdout );
	      wcet_arr[ bb->bbid ] += regioncost[id];
	    }
	    // procedure call typically ends the bb
	    id = procs[bb->callpid]->bblist[ procs[bb->callpid]->num_bb - 1 ]->regid;
	    if( id != -1 && bb->regid != id ) {
	      printf( "region transition %d-%d(%d) procedure return %d(%d) cost: %u\n",
		      bb->pid, bb->bbid, bb->regid, bb->callpid, id, regioncost[bb->regid] ); fflush( stdout );
	      wcet_arr[ bb->bbid ] += regioncost[bb->regid];
	    }
	  }
	  */
	}
      } // end else loophead
    } // end for


    // once more traverse cfg following the wcet path, collect wpvar and wpath
/* 
    wpath = (char*) MALLOC( wpath, sizeof(char), "wpath" );
    wpath[0] = '\0';

    i = num_topo - 1;
 
    while( 1 ) {
      bb = topo[i];
      bb = p->bblist[bb->bbid];
      // printf( "bb %d-%d: ", bb->pid, bb->bbid );
      // printBlock( bb );
      // printf( "cost: %d\n", wcet_arr[bb->bbid] );

      // wpath
      if( bb->bbid != topo[num_topo-1]->bbid )
	sprintf( fn, "-%d", bb->bbid );
      else
	sprintf( fn, "%d", bb->bbid );
      wpath = (char*) REALLOC( wpath, (strlen(wpath) + strlen(fn) + 1) * sizeof(char), "wpath" );
      strcat( wpath, fn );

      // for loading frequency calculation in dynamic locking
      if( bb->startaddr != -1 ) {

	// ignore nested loophead
	if( bb->is_loophead && ( objtype == PROC || bb->loopid != lp->lpid ));
	else {
	  freq = 1;
	  if( objtype == LOOP ) {
	    freq *= lp->loopbound;
	    if( !lp->is_dowhile && bb->bbid == lp->loophead->bbid )
	      freq++;
	  }
	  fprintf( stderr, "%d %d %d\n", p->pid, bb->bbid, freq );

	  if( bb->callpid != -1 )
	    fprintf( stderr, "P%d\n", bb->callpid );
	}
      }

      if( !bb->num_outgoing )
	break;

      i = getblock( bb->outgoing[ tmp_wpath[ bb->bbid ] ], topo, 0, i-1 );
    }
*/


    // record wcet, wpath, wpvar
       p->wcet[index] = wcet_arr[ topo[num_topo-1]->bbid ];
	
	//if(print)
		{
			printf("p->wcet[%d] = %Lu\n",index, p->wcet[index]);
		}

      //if( p->wpath == NULL || strlen( p->wpath ) < strlen( wpath ))
	//p->wpath = (char*) REALLOC( p->wpath, (strlen(wpath) + 1) * sizeof(char), "proc wpath" );
      //strcpy( p->wpath, wpath );
      //free( wpath );

	/*
      // cost of reload if the back edge is a region transition
      if( regionmode && lp->loophead->regid != lp->loopsink->regid ) {
	//
	//printf( "add reload cost for back-edge %d(%d)-->%d(%d): %d * %u\n",
	 //       lp->loophead->bbid, lp->loophead->regid, lp->loopsink->bbid, lp->loopsink->regid,
	 //       lp->loopbound-1, regioncost[lp->loophead->regid] ); fflush( stdout );
        //
	lp->wcet += (lp->loopbound - 1) * regioncost[lp->loophead->regid];
      }

      if( lp->wpath == NULL || strlen( lp->wpath ) < strlen( wpath ))
	lp->wpath = (char*) REALLOC( lp->wpath, (strlen(wpath) + 1) * sizeof(char), "loop wpath" );
      strcpy( lp->wpath, wpath );
      free( wpath );
	*/  
    free( wcet_arr );
   //free( tmp_wpath );

  return 0;
}



int analyseDAGLoop(procedure *proc, loop *lop, int index ) 
{
  ull *wcet_arr_1 = NULL;
  ull *wcet_arr_2 = NULL;
  // Stores the weight of each node including the weight of its successors.
  // The index corresponds to basic block id.

  //char *tmp_wpath;
  // Stores the heaviest branch taken at the node.

  int i, id;
  //char fn[100];
  char max1, max2;
  block *bb;
  //int freq;

  procedure *p;
  loop *lp, *lpn;

  //char *wpath;
  // int len;

  block **topo;
  int num_topo;

  // print_cfg();

    lp       =  lop;
    p        = proc;
    topo     = lp->topo;
    num_topo = lp->num_topo;

  // store temporary calculations, index corresponding to bblist for ease of locating


    // ****** THE REGION TRANSITION IS NOT CORRECTLY & COMPLETELY ADAPTED HERE YET **********

    wcet_arr_1  = (ull*)  CALLOC( wcet_arr_1,  p->num_bb, sizeof(ull), "wcet_arr_1");
    wcet_arr_2  = (ull*)  CALLOC( wcet_arr_2,  p->num_bb, sizeof(ull), "wcet_arr_2");
  
   // tmp_wpath = (char*) CALLOC( tmp_wpath, p->num_bb, sizeof(char), "tmp_wpath" );

    for( i = 0; i < num_topo; i++ ) 
   {
      bb = topo[i];
      bb = p->bblist[bb->bbid];
      // printf( "bb %d-%d: ", bb->pid, bb->bbid );
      // printBlock( bb );

      // if dummy block, ignore
      if( bb->startaddr == -1 )
	continue;

      // if nested loophead, directly use the wcet of loop (should have been calculated)
      if( bb->is_loophead && (bb->loopid != lp->lpid)) 
      {
		lpn = p->loops[ bb->loopid ];
		
		wcet_arr_1[ bb->bbid ] = lpn->wcet[index * 2];
		wcet_arr_2[ bb->bbid ] = lpn->wcet[index * 2 + 1] * (lp->loopbound -1);

	// add wcet at loopexit ?????????????????????
		//wcet_arr_1[ bb->bbid ] += wcet_arr_1[ lpn->loopexit->bbid ];
		wcet_arr_2[ bb->bbid ] += wcet_arr_2[ lpn->loopexit->bbid ];

	//if(print)
		{
			printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
			printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);
		}


      } // end if nested loophead

      else {
	  //cost of this bb
	  
	//cost of bb excluding loophead of other loops
		wcet_arr_1[ bb->bbid ] += bb->chmc_L2[index * 2]->wcost;
		wcet_arr_2[ bb->bbid ] += bb->chmc_L2[index * 2 + 1]->wcost * (lp->loopbound -1);
		//if(bb->is_loophead)
			//wcet_arr[ bb->bbid ] += bb->chmc_L2[index]->wcost;
	//if(print)
		{
			printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
			printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);
		}

	//---- successors ----
	switch( bb->num_outgoing ) {

	case 0:
	  break;

	case 1:	 // just add the weight of successor
	  wcet_arr_1[ bb->bbid ] += wcet_arr_1[ bb->outgoing[0] ];
	  wcet_arr_2[ bb->bbid ] += wcet_arr_2[ bb->outgoing[0] ];
	//if(print)
		{
			printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
			printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);
		}
	  
	  break;

	case 2:  // choose heaviest

	//what is exit? real exit?
	  if(bb->loopid > p->bblist[bb->outgoing[0]]->loopid)
	  {
	  	max1 = 1;
		max2 = 1;
	  }
	  else if(bb->loopid > p->bblist[bb->outgoing[1]]->loopid)
	  {
	  	max1 = 0;
		max2 = 0;
	  }
	  
	 else
	 {
	 	if( wcet_arr_1[ bb->outgoing[0] ] >= wcet_arr_1[ bb->outgoing[1] ] )
	    		max1 = 0;
	  	else
	    		max1 = 1;
		
	 	if( wcet_arr_2[ bb->outgoing[0] ] >= wcet_arr_2[ bb->outgoing[1] ] )
	    		max2 = 0;
	  	else
	    		max2 = 1;
	 }
	  wcet_arr_1[ bb->bbid ] += wcet_arr_1[ bb->outgoing[max1] ];
	  wcet_arr_2[ bb->bbid ] += wcet_arr_2[ bb->outgoing[max2] ];
	//if(print)
		{
			printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
			printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);
		}
	 // tmp_wpath[ bb->bbid ] = max;
	  break;

	default:
	  printf( "Invalid number of outgoing edges at %d-%d\n", bb->pid, bb->bbid );
	  exit(1);

	} // end switch

	// called procedure
	if( bb->callpid != -1 ) {
	//add cost of the procedure
	  	wcet_arr_1[ bb->bbid ] += bb->proc_ptr->wcet[index * 2];
	  	wcet_arr_2[ bb->bbid ] += bb->proc_ptr->wcet[index * 2 + 1] * (lp->loopbound -1);
	
	  printf( "bb %d-%d procedure call %d wcet: 1 %Lu	2 %Lu\n", bb->pid, bb->bbid,
		  bb->callpid, p->wcet[index * 2], p->wcet[index * 2 + 1]); fflush( stdout );
	//if(print)
		{
			printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
			printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);
		}

	/*
	  // region transition
	  if( regionmode && bb->regid != -1 ) {
	    id = procs[bb->callpid]->bblist[0]->regid;
	    if( id != -1 && bb->regid != id ) {
	      printf( "region transition %d-%d(%d) procedure call %d(%d) cost: %u\n",
		      bb->pid, bb->bbid, bb->regid, bb->callpid, id, regioncost[id] ); fflush( stdout );
	      wcet_arr[ bb->bbid ] += regioncost[id];
	    }
	    // procedure call typically ends the bb
	    id = procs[bb->callpid]->bblist[ procs[bb->callpid]->num_bb - 1 ]->regid;
	    if( id != -1 && bb->regid != id ) {
	      printf( "region transition %d-%d(%d) procedure return %d(%d) cost: %u\n",
		      bb->pid, bb->bbid, bb->regid, bb->callpid, id, regioncost[bb->regid] ); fflush( stdout );
	      wcet_arr[ bb->bbid ] += regioncost[bb->regid];
	    }
	  }
	  */
	}
      } // end else loophead
    } // end for


    // once more traverse cfg following the wcet path, collect wpvar and wpath
/* 
    wpath = (char*) MALLOC( wpath, sizeof(char), "wpath" );
    wpath[0] = '\0';

    i = num_topo - 1;
 
    while( 1 ) {
      bb = topo[i];
      bb = p->bblist[bb->bbid];
      // printf( "bb %d-%d: ", bb->pid, bb->bbid );
      // printBlock( bb );
      // printf( "cost: %d\n", wcet_arr[bb->bbid] );

      // wpath
      if( bb->bbid != topo[num_topo-1]->bbid )
	sprintf( fn, "-%d", bb->bbid );
      else
	sprintf( fn, "%d", bb->bbid );
      wpath = (char*) REALLOC( wpath, (strlen(wpath) + strlen(fn) + 1) * sizeof(char), "wpath" );
      strcat( wpath, fn );

      // for loading frequency calculation in dynamic locking
      if( bb->startaddr != -1 ) {

	// ignore nested loophead
	if( bb->is_loophead && ( objtype == PROC || bb->loopid != lp->lpid ));
	else {
	  freq = 1;
	  if( objtype == LOOP ) {
	    freq *= lp->loopbound;
	    if( !lp->is_dowhile && bb->bbid == lp->loophead->bbid )
	      freq++;
	  }
	  fprintf( stderr, "%d %d %d\n", p->pid, bb->bbid, freq );

	  if( bb->callpid != -1 )
	    fprintf( stderr, "P%d\n", bb->callpid );
	}
      }

      if( !bb->num_outgoing )
	break;

      i = getblock( bb->outgoing[ tmp_wpath[ bb->bbid ] ], topo, 0, i-1 );
    }
*/


    // record wcet, wpath, wpvar
      lp->wcet[index] = wcet_arr_1[ topo[num_topo-1]->bbid ];
      lp->wcet[index + 1] = wcet_arr_2[ topo[num_topo-1]->bbid ];

      if( !lp->is_dowhile ) 
    {
	// one extra execution of loophead ?????????????????
	lp->wcet[index] += p->bblist[lp->loophead->bbid]->chmc_L2[index * 2 + 1]->wcost;
	if( lp->loophead->callpid != -1 )
	  lp->wcet[index]  += p->bblist[lp->loophead->bbid]->proc_ptr->wcet[index];
      	}

	//if(print)
		{
			printf("lp->wcet[%d] = %Lu\n", index, lp->wcet[index]);
			printf("lp->wcet[%d] = %Lu\n", index+1, lp->wcet[index + 1]);
		}

	/*
      // cost of reload if the back edge is a region transition
      if( regionmode && lp->loophead->regid != lp->loopsink->regid ) {
	//
	//printf( "add reload cost for back-edge %d(%d)-->%d(%d): %d * %u\n",
	 //       lp->loophead->bbid, lp->loophead->regid, lp->loopsink->bbid, lp->loopsink->regid,
	 //       lp->loopbound-1, regioncost[lp->loophead->regid] ); fflush( stdout );
        //
	lp->wcet += (lp->loopbound - 1) * regioncost[lp->loophead->regid];
      }

      if( lp->wpath == NULL || strlen( lp->wpath ) < strlen( wpath ))
	lp->wpath = (char*) REALLOC( lp->wpath, (strlen(wpath) + 1) * sizeof(char), "loop wpath" );
      strcpy( lp->wpath, wpath );
      free( wpath );
	*/  

    free( wcet_arr_1 );
    free( wcet_arr_2 );
   //free( tmp_wpath );
  return 0;
}


/*
 * Determines WCET and WCET path of a procedure.
 * First analyse loops separately, starting from the inmost level.
 * When analysing an outer loop, the inner loop is treated as a black box.
 * Then analyse the procedure by treating the outmost loops as black boxes.
 */
int analyseProc( procedure *p ) {

  int  i, j;
  loop *lp;

  // analyse each loop from the inmost (reverse order from detection)
  for( i = p->num_loops - 1; i >= 0; i-- ) {
    lp = p->loops[i];

    // printf( "Analysis loop %d-%d\n", p->pid, lp->lpid ); fflush( stdout );
    // printLoop( lp ); fflush( stdout );
	for(j = 0; j < (1<<lp->level); j++)
		analyseDAGLoop(p, lp, j*2);
    // printf( "WCET: %d\n\n", lp->wcet );
  }

  // analyse procedure
  // printf( "Analysis proc %d\n", p->pid ); fflush( stdout );
  // printProc( p ); fflush( stdout );

for(j = 0; j < p->num_cost; j++)
  analyseDAGFunction( p, j);
  // printf( "WCET: %d\n\n", p->wcet );
}


/*
 * Determines WCET and WCET path of a program.
 * Analyse procedures separately, starting from deepest call level.
 * proc_topo stores the reverse topological order of procedure calls.
 * When analysing a procedure, a procedure call is treated as a black box.
 * The main procedure is at the top call level,
 * and WCET of program == WCET of main procedure.
 */
int analysis_dag(MSC *msc) {

  int i, j, k;
  double t;

  cycle_time(0);

for(k = 0; k < msc->num_task; k++)
{
  // analyse each procedure in reverse topological order of call graph
  for( i = 0; i < msc->taskList[k].num_proc; i++ )
  	for(j = 0; j < msc->taskList[k].proc_cg_ptr[i].num_proc; j++)
  	{
    		analyseProc(msc->taskList[k].proc_cg_ptr[i].proc[j]);
	}
  // for loading frequency calculation in dynamic locking
 // fprintf( stderr, "P%d\n", main_id );
	msc->taskList[k].wcet = msc->taskList[k].main_copy->wcet[0];
	//if(print)
	{
		printf("msc->taskList[%d].wcet = %Lu\n", k, msc->taskList[k].wcet);
	}


}
  t = cycle_time(1);
  printf( "\nTime taken (analysis): %f ms\n", t/CYCLES_PER_MSEC ); fflush( stdout );

  return 0;
}
