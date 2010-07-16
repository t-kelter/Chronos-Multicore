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
int analyseDAGFunction_BCET(procedure *proc, int index)
{

  ull *bcet_arr = NULL;

  ull *hit_arr = NULL;
  ull *miss_arr = NULL;
  ull *unknow_arr = NULL;
  
  ull *hit_arr_L2 = NULL;
  ull *miss_arr_L2 = NULL;
  ull *unknow_arr_L2 = NULL;

  // Stores the weight of each node including the weight of its successors.
  // The index corresponds to basic block id.

  //char *tmp_wpath;
  // Stores the heaviest branch taken at the node.

  int i;
  //char fn[100];
  char min;
  block *bb;
 //int freq;

  procedure *p;
  loop *lpn;

  //char *wpath;
  // int len;

  block **topo;
  int num_topo;

  // print_cfg();

    p        =  proc;
    topo     = p->topo;
    num_topo = p->num_topo;

  // store temporary calculations, index corresponding to bblist for ease of locating


    // ****** THE REGION TRANSITION IS NOT CORRECTLY & COMPLETELY ADAPTED HERE YET **********

    bcet_arr  = (ull*)  CALLOC( bcet_arr,  p->num_bb, sizeof(ull), "bcet_arr" );

  hit_arr = (ull*)  CALLOC( hit_arr,  p->num_bb, sizeof(ull), "hit_arr" );
  miss_arr = (ull*)  CALLOC( miss_arr,  p->num_bb, sizeof(ull), "miss_arr" );
  unknow_arr = (ull*)  CALLOC( unknow_arr,  p->num_bb, sizeof(ull), "unknow_arr" );
  
  hit_arr_L2 = (ull*)  CALLOC( hit_arr_L2,  p->num_bb, sizeof(ull), "hit_arr_L2" );
  miss_arr_L2 = (ull*)  CALLOC( miss_arr_L2,  p->num_bb, sizeof(ull), "miss_arr_L2" );
  unknow_arr_L2 = (ull*)  CALLOC( unknow_arr_L2,  p->num_bb, sizeof(ull), "unknow_arr_L2" );


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
		
 		/*     
		bcet_arr[ bb->bbid ] = lpn->bcet[index * 2] + lpn->bcet[index * 2 + 1];

		hit_arr[ bb->bbid ] = lpn->hit_bcet[index * 2] + lpn->hit_bcet[index * 2 + 1];
		miss_arr[ bb->bbid ] = lpn->miss_bcet[index * 2] + lpn->miss_bcet[index * 2 + 1];
		unknow_arr[ bb->bbid ] = lpn->unknow_bcet[index * 2] + lpn->unknow_bcet[index * 2 + 1];
		
		hit_arr_L2[ bb->bbid ] = lpn->hit_bcet_L2[index * 2] + lpn->hit_bcet_L2[index * 2 + 1];
		miss_arr_L2[ bb->bbid ] = lpn->miss_bcet_L2[index * 2] + lpn->miss_bcet_L2[index * 2 + 1];
		unknow_arr_L2[ bb->bbid ] = lpn->unknow_bcet_L2[index * 2] + lpn->unknow_bcet_L2[index * 2 + 1];
		*/	



		// add wcet at loopexit ???
		bcet_arr[ bb->bbid ] += bcet_arr[ lpn->loopexit->bbid ] + bb->chmc_L2[ index ]->bcost;

		hit_arr[ bb->bbid ] += hit_arr[ lpn->loopexit->bbid ] + bb->chmc[ index ]->hit;
		miss_arr[ bb->bbid ] += miss_arr[ lpn->loopexit->bbid ] + bb->chmc[ index ]->miss;
		unknow_arr[ bb->bbid ] += unknow_arr[ lpn->loopexit->bbid ] + bb->chmc[ index ]->unknow;
		
		hit_arr_L2[ bb->bbid ] += hit_arr_L2[ lpn->loopexit->bbid ] + bb->chmc_L2[ index ]->hit;
		miss_arr_L2[ bb->bbid ] += miss_arr_L2[ lpn->loopexit->bbid ] + bb->chmc_L2[ index ]->miss;
		unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ lpn->loopexit->bbid ] + bb->chmc_L2[ index ]->unknow;



	
	if(print)
		{
			printf("bcet_arr[%d] = %Lu\n", bb->bbid, bcet_arr[bb->bbid]);

			printf("loop[%d]:\nlp->wcet = %Lu, %Lu\n", lpn->lpid, lpn->wcet[ index * 2 ], lpn->wcet[ index * 2 +1 ]);

			printf("hit_wcet = %Lu, %Lu\n",  lpn->hit_wcet[ index * 2 ], lpn->hit_wcet[ index * 2 + 1 ]);
			printf("miss_wcet = %Lu, %Lu\n",  lpn->miss_wcet[ index * 2 ], lpn->miss_wcet[ index * 2 + 1 ]);
			printf("unknow_wcet = %Lu, %Lu\n",  lpn->unknow_wcet[ index * 2 ], lpn->unknow_wcet[ index * 2 + 1 ]);

			printf("hit_wcet_L2 = %Lu, %Lu\n",  lpn->hit_wcet_L2[ index * 2 ], lpn->hit_wcet_L2[ index * 2 + 1 ]);
			printf("miss_wcet_L2 = %Lu, %Lu\n",  lpn->miss_wcet_L2[ index * 2 ], lpn->miss_wcet_L2[ index * 2 + 1 ]);
			printf("unknow_wcet_L2 = %Lu, %Lu\n",  lpn->unknow_wcet_L2[ index * 2 ], lpn->unknow_wcet_L2[ index * 2 + 1 ]);

		}

      } // end if nested loophead

      else {
	  //cost of this bb
		bcet_arr[ bb->bbid ] = bb->chmc_L2[index]->bcost;

		hit_arr[ bb->bbid ] = bb->chmc[ index ]->hit;
		miss_arr[ bb->bbid ] = bb->chmc[ index ]->miss;
		unknow_arr[ bb->bbid ] = bb->chmc[ index ]->unknow;
		
		hit_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->hit;
		miss_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->miss;
		unknow_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->unknow;


	  
	if(print)
		{
			printf("bcet_arr[%d] = %Lu\n", bb->bbid, bcet_arr[bb->bbid]);


		}

	//---- successors ----
	switch( bb->num_outgoing ) {

	case 0:
	  break;

	case 1:	 // just add the weight of successor
	  bcet_arr[ bb->bbid ] += bcet_arr[ bb->outgoing[0] ];

	  hit_arr[ bb->bbid ] += hit_arr[ bb->outgoing[0] ];
	  miss_arr[ bb->bbid ] += miss_arr[ bb->outgoing[0] ];
	  unknow_arr[ bb->bbid ] += unknow_arr[ bb->outgoing[0] ];
	
	  hit_arr_L2[ bb->bbid ] += hit_arr_L2[ bb->outgoing[0] ];
	  miss_arr_L2[ bb->bbid ] += miss_arr_L2[ bb->outgoing[0] ];
	  unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ bb->outgoing[0] ];

	if(print)
	{
		printf("bcet_arr[%d] = %Lu\n", bb->bbid, bcet_arr[bb->bbid]);

		printf("hit_arr[%d]->hit = %Lu\n", bb->bbid, hit_arr[ bb->bbid ]);
		printf("miss_arr[%d]->miss = %Lu\n", bb->bbid, miss_arr[ bb->bbid ]);
		printf("unknow_arr[%d]->unknow = %Lu\n", bb->bbid, unknow_arr[ bb->bbid ]);
		printf("hit_arr_L2[%d]->hit = %Lu\n", bb->bbid, hit_arr_L2[ bb->bbid ]);
		printf("miss_arr_L2[%d]->miss = %Lu\n", bb->bbid, miss_arr_L2[ bb->bbid ]);
		printf("unknow_arr_L2[%d]->unknow = %Lu\n", bb->bbid, unknow_arr_L2[ bb->bbid ]);

	}
	  break;

	case 2:  // choose heaviest
	/*
	  if(bb->loopid != p->bblist[bb->outgoing[0]]->loopid)
	  	min = 1;
	  else if(bb->loopid != p->bblist[bb->outgoing[1]]->loopid)
	  	min = 0;
	  
	 else
	 */
	 {
	 	if( bcet_arr[ bb->outgoing[0] ] >= bcet_arr[ bb->outgoing[1] ] )
	    		min = 1;
	  	else
	    		min = 0;
	 }
	  bcet_arr[ bb->bbid ] += bcet_arr[ bb->outgoing[(int)min] ];

	  hit_arr[ bb->bbid ] += hit_arr[ bb->outgoing[(int)min] ];
	  miss_arr[ bb->bbid ] += miss_arr[ bb->outgoing[(int)min] ];
	  unknow_arr[ bb->bbid ] +=  unknow_arr[ bb->outgoing[(int)min] ];
	
	  hit_arr_L2[ bb->bbid ] += hit_arr_L2[ bb->outgoing[(int)min] ];
	  miss_arr_L2[ bb->bbid ] += miss_arr_L2[ bb->outgoing[(int)min] ];
	  unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ bb->outgoing[(int)min] ];


	if(print)
		{
			printf("bcet_arr[%d] = %Lu\n", bb->bbid, bcet_arr[bb->bbid]);

			printf("hit_arr[%d]->hit = %Lu\n", bb->bbid, hit_arr[ bb->bbid ]);
			printf("miss_arr[%d]->miss = %Lu\n", bb->bbid, miss_arr[ bb->bbid ]);
			printf("unknow_arr[%d]->unknow = %Lu\n", bb->bbid, unknow_arr[ bb->bbid ]);
			printf("hit_arr_L2[%d]->hit = %Lu\n", bb->bbid, hit_arr_L2[ bb->bbid ]);
			printf("miss_arr_L2[%d]->miss = %Lu\n", bb->bbid, miss_arr_L2[ bb->bbid ]);
			printf("unknow_arr_L2[%d]->unknow = %Lu\n", bb->bbid, unknow_arr_L2[ bb->bbid ]);
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
  	 	bcet_arr[ bb->bbid ] += bb->proc_ptr->bcet[index];

		hit_arr[ bb->bbid ] += bb->proc_ptr->hit_bcet[ index ];
		miss_arr[ bb->bbid ] += bb->proc_ptr->miss_bcet[ index ];
		unknow_arr[ bb->bbid ] += bb->proc_ptr->unknow_bcet[ index ];

		hit_arr_L2[ bb->bbid ] += bb->proc_ptr->hit_bcet_L2[ index ];
		miss_arr_L2[ bb->bbid ] += bb->proc_ptr->miss_bcet_L2[ index ];
		unknow_arr_L2[ bb->bbid ] += bb->proc_ptr->unknow_bcet_L2[ index ];


	if(print)
		printf( "bb %d-%d procedure call %d bcet: %Lu\n", bb->pid, bb->bbid, bb->callpid, bb->proc_ptr->bcet[index] ); fflush( stdout );

	if(print)
		{
			printf("bcet_arr[%d] = %Lu\n", bb->bbid, bcet_arr[bb->bbid]);

			printf("hit_arr[%d]->hit = %Lu\n", bb->bbid, hit_arr[ bb->bbid ]);
			printf("miss_arr[%d]->miss = %Lu\n", bb->bbid, miss_arr[ bb->bbid ]);
			printf("unknow_arr[%d]->unknow = %Lu\n", bb->bbid, unknow_arr[ bb->bbid ]);
			printf("hit_arr_L2[%d]->hit = %Lu\n", bb->bbid, hit_arr_L2[ bb->bbid ]);
			printf("miss_arr_L2[%d]->miss = %Lu\n", bb->bbid, miss_arr_L2[ bb->bbid ]);
			printf("unknow_arr_L2[%d]->unknow = %Lu\n", bb->bbid, unknow_arr_L2[ bb->bbid ]);

		}


	/*
	  // region transition
	  if( regionmode && bb->regid != -1 ) {
	    int id = procs[bb->callpid]->bblist[0]->regid;
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
    loop *lp;
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
       p->bcet[index] = bcet_arr[ topo[num_topo-1]->bbid ];


	p->hit_bcet[index] = hit_arr[ topo[num_topo-1]->bbid ];
	p->miss_bcet[index] = miss_arr[ topo[num_topo-1]->bbid ];
	p->unknow_bcet[index] = unknow_arr[ topo[num_topo-1]->bbid ];

	p->hit_bcet_L2[index] = hit_arr_L2[ topo[num_topo-1]->bbid ];
	p->miss_bcet_L2[index] = miss_arr_L2[ topo[num_topo-1]->bbid ];
	p->unknow_bcet_L2[index] = unknow_arr_L2[ topo[num_topo-1]->bbid ];


	
	if(print)
		{
			printf("p->bcet[%d] = %Lu\n",index, p->bcet[index]);

			printf( "p-> hit[ %d ]%Lu\n", index, p->hit_bcet[ index ]); 
	 		printf( "p-> miss[ %d ]%Lu\n", index, p->miss_bcet[ index ]); 
	 		printf( "p-> unknow[ %d ]%Lu\n", index, p->unknow_bcet[ index ]); 

			printf( "p-> hit_L2[ %d ]%Lu\n", index, p->hit_bcet_L2[ index ]); 
	 		printf( "p-> miss_L2[ %d ]%Lu\n", index, p->miss_bcet_L2[ index ]); 
	 		printf( "p-> unknow_L2[ %d ]%Lu\n", index, p->unknow_bcet_L2[ index ]); 

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
    free( bcet_arr );
   //free( tmp_wpath );

  return 0;
}



int analyseDAGLoop_BCET(procedure *proc, loop *lop, int index ) 
{
  ull *bcet_arr_1 = NULL;
  ull *bcet_arr_2 = NULL;

  ull *hit_arr_1 = NULL;
  ull *miss_arr_1 = NULL;
  ull *unknow_arr_1 = NULL;
  
  ull *hit_arr_L2_1 = NULL;
  ull *miss_arr_L2_1 = NULL;
  ull *unknow_arr_L2_1 = NULL;

  ull *hit_arr_2 = NULL;
  ull *miss_arr_2 = NULL;
  ull *unknow_arr_2 = NULL;
  
  ull *hit_arr_L2_2 = NULL;
  ull *miss_arr_L2_2 = NULL;
  ull *unknow_arr_L2_2 = NULL;



// Stores the weight of each node including the weight of its successors.
  // The index corresponds to basic block id.

  //char *tmp_wpath;
  // Stores the heaviest branch taken at the node.

  int i;
  //char fn[100];
  char min1, min2;
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

    bcet_arr_1  = (ull*)  CALLOC( bcet_arr_1,  p->num_bb, sizeof(ull), "bcet_arr_1");
    bcet_arr_2  = (ull*)  CALLOC( bcet_arr_2,  p->num_bb, sizeof(ull), "bcet_arr_2");

    hit_arr_1 = (ull*)  CALLOC( hit_arr_1,  p->num_bb, sizeof(ull), "hit_arr_1");
    miss_arr_1 = (ull*)  CALLOC( miss_arr_1,  p->num_bb, sizeof(ull), "miss_arr_1");
    unknow_arr_1 = (ull*)  CALLOC(unknow_arr_1,  p->num_bb, sizeof(ull), "unknow_arr_1");

    hit_arr_L2_1 = (ull*)  CALLOC( hit_arr_L2_1,  p->num_bb, sizeof(ull), "hit_arr_L2_1");
    miss_arr_L2_1 = (ull*)  CALLOC( miss_arr_L2_1,  p->num_bb, sizeof(ull), "miss_arr_L2_1");
    unknow_arr_L2_1 = (ull*)  CALLOC(unknow_arr_L2_1,  p->num_bb, sizeof(ull), "unknow_arr_L2_1");


    hit_arr_2 = (ull*)  CALLOC( hit_arr_2,  p->num_bb, sizeof(ull), "hit_arr_2");
    miss_arr_2 = (ull*)  CALLOC( miss_arr_2,  p->num_bb, sizeof(ull), "miss_arr_2");
    unknow_arr_2 = (ull*)  CALLOC(unknow_arr_2,  p->num_bb, sizeof(ull), "unknow_arr_2");

    hit_arr_L2_2 = (ull*)  CALLOC( hit_arr_L2_2,  p->num_bb, sizeof(ull), "hit_arr_L2_2");
    miss_arr_L2_2 = (ull*)  CALLOC( miss_arr_L2_2,  p->num_bb, sizeof(ull), "miss_arr_L2_2");
    unknow_arr_L2_2 = (ull*)  CALLOC(unknow_arr_L2_2,  p->num_bb, sizeof(ull), "unknow_arr_L2_2");



  
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
		
		bcet_arr_1[ bb->bbid ] = lpn->bcet[index];
		bcet_arr_2[ bb->bbid ] = lpn->bcet[index + 1] * (lp->loopbound -1);

		hit_arr_1[ bb->bbid ] = lpn->hit_bcet[ index ];
		miss_arr_1[ bb->bbid ] = lpn->miss_bcet[ index ];
		unknow_arr_1[ bb->bbid ] = lpn->unknow_bcet[ index  ];

		hit_arr_L2_1[ bb->bbid ] = lpn->hit_bcet_L2[ index ];
		miss_arr_L2_1[ bb->bbid ] = lpn->miss_bcet_L2[ index ];
		unknow_arr_L2_1[ bb->bbid ] = lpn->unknow_bcet_L2[ index ];



		hit_arr_2[ bb->bbid ] = lpn->hit_bcet[ index + 1 ] * (lp->loopbound -1);
		miss_arr_2[ bb->bbid ] = lpn->miss_bcet[ index + 1  ] * (lp->loopbound -1);
		unknow_arr_2[ bb->bbid ] = lpn->unknow_bcet[ index + 1  ] * (lp->loopbound -1);

		hit_arr_L2_2[ bb->bbid ] = lpn->hit_bcet_L2[ index + 1 ] * (lp->loopbound -1);
		miss_arr_L2_2[ bb->bbid ] = lpn->miss_bcet_L2[ index + 1  ] * (lp->loopbound -1);
		unknow_arr_L2_2[ bb->bbid ] = lpn->unknow_bcet_L2[ index + 1  ] * (lp->loopbound -1);




	// add wcet at loopexit ?????????????????????
		//wcet_arr_1[ bb->bbid ] += wcet_arr_1[ lpn->loopexit->bbid ];
		bcet_arr_2[ bb->bbid ] += bcet_arr_2[ lpn->loopexit->bbid ];

		hit_arr_2[ bb->bbid ] += hit_arr_2[ lpn->loopexit->bbid ];
		miss_arr_2[ bb->bbid ] += miss_arr_2[ lpn->loopexit->bbid ];
		unknow_arr_2[ bb->bbid ] += unknow_arr_2[ lpn->loopexit->bbid ];
		
		hit_arr_L2_2[ bb->bbid ] += hit_arr_L2_2[ lpn->loopexit->bbid ];
		miss_arr_L2_2[ bb->bbid ] += miss_arr_L2_2[ lpn->loopexit->bbid ];
		unknow_arr_L2_2[ bb->bbid ] += unknow_arr_L2_2[ lpn->loopexit->bbid ];


	if(print)
		{
			printf("bcet_arr_1[%d] = %Lu\n", bb->bbid, bcet_arr_1[bb->bbid]);
			printf("bcet_arr_2[%d] = %Lu\n", bb->bbid, bcet_arr_2[bb->bbid]);

			printf("hit_arr_1[%d] = %Lu\n", bb->bbid, hit_arr_1[bb->bbid]);
			printf("miss_arr_1[%d] = %Lu\n", bb->bbid, miss_arr_1[bb->bbid]);
			printf("unknow_arr_1[%d] = %Lu\n", bb->bbid, unknow_arr_1[bb->bbid]);

			printf("hit_arr_L2_1[%d] = %Lu\n", bb->bbid, hit_arr_L2_1[bb->bbid]);
			printf("miss_arr_L2_1[%d] = %Lu\n", bb->bbid, miss_arr_L2_1[bb->bbid]);
			printf("unknow_arr_L2_1[%d] = %Lu\n", bb->bbid, unknow_arr_L2_1[bb->bbid]);


			printf("hit_arr_2[%d] = %Lu\n", bb->bbid, hit_arr_2[bb->bbid]);
			printf("miss_arr_2[%d] = %Lu\n", bb->bbid, miss_arr_2[bb->bbid]);
			printf("unknow_arr_2[%d] = %Lu\n", bb->bbid, unknow_arr_2[bb->bbid]);

			printf("hit_arr_L2_2[%d] = %Lu\n", bb->bbid, hit_arr_L2_2[bb->bbid]);
			printf("miss_arr_L2_2[%d] = %Lu\n", bb->bbid, miss_arr_L2_2[bb->bbid]);
			printf("unknow_arr_L2_2[%d] = %Lu\n", bb->bbid, unknow_arr_L2_2[bb->bbid]);


		}


      } // end if nested loophead

      else {
	  //cost of this bb
	  
	//cost of bb excluding loophead of other loops
		bcet_arr_1[ bb->bbid ] = bb->chmc_L2[index]->bcost;
		bcet_arr_2[ bb->bbid ] = bb->chmc_L2[index + 1]->bcost * (lp->loopbound -1);

		//first iteration, L1
		hit_arr_1[ bb->bbid ] = bb->chmc[ index ]->hit;
		miss_arr_1[ bb->bbid ] = bb->chmc[ index ]->miss;
		unknow_arr_1[ bb->bbid ] = bb->chmc[ index ]->unknow;

		//first iteration, L2
		hit_arr_L2_1[ bb->bbid ] = bb->chmc_L2[ index ]->hit;
		miss_arr_L2_1[ bb->bbid ] = bb->chmc_L2[ index ]->miss;
		unknow_arr_L2_1[ bb->bbid ] = bb->chmc_L2[ index ]->unknow;


		//next iteration, L1
		hit_arr_2[ bb->bbid ] = ( bb->chmc[index + 1]->hit ) * (lp->loopbound -1);
		miss_arr_2[ bb->bbid ] = ( bb->chmc[index + 1]->miss ) * (lp->loopbound -1);
		unknow_arr_2[ bb->bbid ] = ( bb->chmc[index + 1]->unknow ) * (lp->loopbound -1);
		
		//next iteration, L2
		hit_arr_L2_2[ bb->bbid ] = ( bb->chmc_L2[index + 1]->hit ) * (lp->loopbound -1);
		miss_arr_L2_2[ bb->bbid ] = ( bb->chmc_L2[index + 1]->miss ) * (lp->loopbound -1);
		unknow_arr_L2_2[ bb->bbid ] = ( bb->chmc_L2[index + 1]->unknow ) * (lp->loopbound -1);


		//if(bb->is_loophead)
			//wcet_arr[ bb->bbid ] += bb->chmc_L2[index]->wcost;
		if(print)
		{
			printf("bcet_arr_1[%d] = %Lu\n", bb->bbid, bcet_arr_1[bb->bbid]);
			printf("bcet_arr_2[%d] = %Lu\n", bb->bbid, bcet_arr_2[bb->bbid]);

			printf("hit_arr_1[%d] = %Lu\n", bb->bbid, hit_arr_1[bb->bbid]);
			printf("miss_arr_1[%d] = %Lu\n", bb->bbid, miss_arr_1[bb->bbid]);
			printf("unknow_arr_1[%d] = %Lu\n", bb->bbid, unknow_arr_1[bb->bbid]);

			printf("hit_arr_L2_1[%d] = %Lu\n", bb->bbid, hit_arr_L2_1[bb->bbid]);
			printf("miss_arr_L2_1[%d] = %Lu\n", bb->bbid, miss_arr_L2_1[bb->bbid]);
			printf("unknow_arr_L2_1[%d] = %Lu\n", bb->bbid, unknow_arr_L2_1[bb->bbid]);


			printf("hit_arr_2[%d] = %Lu\n", bb->bbid, hit_arr_2[bb->bbid]);
			printf("miss_arr_2[%d] = %Lu\n", bb->bbid, miss_arr_2[bb->bbid]);
			printf("unknow_arr_2[%d] = %Lu\n", bb->bbid, unknow_arr_2[bb->bbid]);

			printf("hit_arr_L2_2[%d] = %Lu\n", bb->bbid, hit_arr_L2_2[bb->bbid]);
			printf("miss_arr_L2_2[%d] = %Lu\n", bb->bbid, miss_arr_L2_2[bb->bbid]);
			printf("unknow_arr_L2_2[%d] = %Lu\n", bb->bbid, unknow_arr_L2_2[bb->bbid]);
}

	//---- successors ----
	switch( bb->num_outgoing ) {

	case 0:
	  break;

	case 1:	 // just add the weight of successor
	  bcet_arr_1[ bb->bbid ] += bcet_arr_1[ bb->outgoing[0] ];
	  bcet_arr_2[ bb->bbid ] += bcet_arr_2[ bb->outgoing[0] ];

	  hit_arr_1[ bb->bbid ] += hit_arr_1[ bb->outgoing[0] ];
	  miss_arr_1[ bb->bbid ] += miss_arr_1[ bb->outgoing[0] ];
	  unknow_arr_1[ bb->bbid ] +=  unknow_arr_1[ bb->outgoing[0] ];
	
	  hit_arr_L2_1[ bb->bbid ] += hit_arr_L2_1[ bb->outgoing[0] ];
	  miss_arr_L2_1[ bb->bbid ] += miss_arr_L2_1[ bb->outgoing[0] ];
	  unknow_arr_L2_1[ bb->bbid ] += unknow_arr_L2_1[ bb->outgoing[0] ];


	  hit_arr_2[ bb->bbid ] += hit_arr_2[ bb->outgoing[0] ];
	  miss_arr_2[ bb->bbid ] += miss_arr_2[ bb->outgoing[0] ];
	  unknow_arr_2[ bb->bbid ] +=  unknow_arr_2[ bb->outgoing[0] ];
	
	  hit_arr_L2_2[ bb->bbid ] += hit_arr_L2_2[ bb->outgoing[0] ];
	  miss_arr_L2_2[ bb->bbid ] += miss_arr_L2_2[ bb->outgoing[0] ];
	  unknow_arr_L2_2[ bb->bbid ] += unknow_arr_L2_2[ bb->outgoing[0] ];
	  
		if(print)
		{
			printf("bcet_arr_1[%d] = %Lu\n", bb->bbid, bcet_arr_1[bb->bbid]);
			printf("bcet_arr_2[%d] = %Lu\n", bb->bbid, bcet_arr_2[bb->bbid]);

			printf("hit_arr_1[%d] = %Lu\n", bb->bbid, hit_arr_1[bb->bbid]);
			printf("miss_arr_1[%d] = %Lu\n", bb->bbid, miss_arr_1[bb->bbid]);
			printf("unknow_arr_1[%d] = %Lu\n", bb->bbid, unknow_arr_1[bb->bbid]);

			printf("hit_arr_L2_1[%d] = %Lu\n", bb->bbid, hit_arr_L2_1[bb->bbid]);
			printf("miss_arr_L2_1[%d] = %Lu\n", bb->bbid, miss_arr_L2_1[bb->bbid]);
			printf("unknow_arr_L2_1[%d] = %Lu\n", bb->bbid, unknow_arr_L2_1[bb->bbid]);


			printf("hit_arr_2[%d] = %Lu\n", bb->bbid, hit_arr_2[bb->bbid]);
			printf("miss_arr_2[%d] = %Lu\n", bb->bbid, miss_arr_2[bb->bbid]);
			printf("unknow_arr_2[%d] = %Lu\n", bb->bbid, unknow_arr_2[bb->bbid]);

			printf("hit_arr_L2_2[%d] = %Lu\n", bb->bbid, hit_arr_L2_2[bb->bbid]);
			printf("miss_arr_L2_2[%d] = %Lu\n", bb->bbid, miss_arr_L2_2[bb->bbid]);
			printf("unknow_arr_L2_2[%d] = %Lu\n", bb->bbid, unknow_arr_L2_2[bb->bbid]);

		}
	  
	  break;

	case 2:  // choose heaviest

	//what is exit? real exit?
	  if(bb->loopid > p->bblist[bb->outgoing[0]]->loopid)
	  {
	  	min1 = 1;
		min2 = 1;
	  }
	  else if(bb->loopid > p->bblist[bb->outgoing[1]]->loopid)
	  {
	  	min1 = 0;
		min2 = 0;
	  }
	  
	 else
	 {
	 	if( bcet_arr_1[ bb->outgoing[0] ] >= bcet_arr_1[ bb->outgoing[1] ] )
	    		min1 = 1;
	  	else
	    		min1 = 0;
		
	 	if( bcet_arr_2[ bb->outgoing[0] ] >= bcet_arr_2[ bb->outgoing[1] ] )
	    		min2 = 1;
	  	else
	    		min2 = 0;
	 }
	 
	  bcet_arr_1[ bb->bbid ] += bcet_arr_1[ bb->outgoing[(int)min1] ];
	  bcet_arr_2[ bb->bbid ] += bcet_arr_2[ bb->outgoing[(int)min2] ];


	  hit_arr_1[ bb->bbid ] +=  hit_arr_1[ bb->outgoing[(int)min1] ];
	  miss_arr_1[ bb->bbid ] += miss_arr_1[ bb->outgoing[(int)min1] ];
	  unknow_arr_1[ bb->bbid ] +=  unknow_arr_1[ bb->outgoing[(int)min1] ];
	
	  hit_arr_L2_1[ bb->bbid ] +=  hit_arr_L2_1[ bb->outgoing[(int)min1] ];
	  miss_arr_L2_1[ bb->bbid ] += miss_arr_L2_1[ bb->outgoing[(int)min1] ];
	  unknow_arr_L2_1[ bb->bbid ] += unknow_arr_L2_1[ bb->outgoing[(int)min1] ];


	  hit_arr_2[ bb->bbid ] +=  hit_arr_2[ bb->outgoing[(int)min2] ];
	  miss_arr_2[ bb->bbid ] += miss_arr_2[ bb->outgoing[(int)min2] ];
	  unknow_arr_2[ bb->bbid ] +=  unknow_arr_2[ bb->outgoing[(int)min2] ];
	
	  hit_arr_L2_2[ bb->bbid ] +=  hit_arr_L2_2[ bb->outgoing[(int)min2] ];
	  miss_arr_L2_2[ bb->bbid ] += miss_arr_L2_2[ bb->outgoing[(int)min2] ];
	  unknow_arr_L2_2[ bb->bbid ] += unknow_arr_L2_2[ bb->outgoing[(int)min2] ];


		if(print)
		{
			printf("bcet_arr_1[%d] = %Lu\n", bb->bbid, bcet_arr_1[bb->bbid]);
			printf("bcet_arr_2[%d] = %Lu\n", bb->bbid, bcet_arr_2[bb->bbid]);

			printf("hit_arr_1[%d] = %Lu\n", bb->bbid, hit_arr_1[bb->bbid]);
			printf("miss_arr_1[%d] = %Lu\n", bb->bbid, miss_arr_1[bb->bbid]);
			printf("unknow_arr_1[%d] = %Lu\n", bb->bbid, unknow_arr_1[bb->bbid]);

			printf("hit_arr_L2_1[%d] = %Lu\n", bb->bbid, hit_arr_L2_1[bb->bbid]);
			printf("miss_arr_L2_1[%d] = %Lu\n", bb->bbid, miss_arr_L2_1[bb->bbid]);
			printf("unknow_arr_L2_1[%d] = %Lu\n", bb->bbid, unknow_arr_L2_1[bb->bbid]);


			printf("hit_arr_2[%d] = %Lu\n", bb->bbid, hit_arr_2[bb->bbid]);
			printf("miss_arr_2[%d] = %Lu\n", bb->bbid, miss_arr_2[bb->bbid]);
			printf("unknow_arr_2[%d] = %Lu\n", bb->bbid, unknow_arr_2[bb->bbid]);

			printf("hit_arr_L2_2[%d] = %Lu\n", bb->bbid, hit_arr_L2_2[bb->bbid]);
			printf("miss_arr_L2_2[%d] = %Lu\n", bb->bbid, miss_arr_L2_2[bb->bbid]);
			printf("unknow_arr_L2_2[%d] = %Lu\n", bb->bbid, unknow_arr_L2_2[bb->bbid]);

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
	  	bcet_arr_1[ bb->bbid ] += bb->proc_ptr->bcet[index ];
	  	bcet_arr_2[ bb->bbid ] += bb->proc_ptr->bcet[index + 1] * (lp->loopbound -1);


		hit_arr_1[ bb->bbid ] += bb->proc_ptr->hit_bcet[ index ];
		miss_arr_1[ bb->bbid ] += bb->proc_ptr->miss_bcet[ index ];
		unknow_arr_1[ bb->bbid ] += bb->proc_ptr->unknow_bcet[ index ];

		hit_arr_L2_1[ bb->bbid ] += bb->proc_ptr->hit_bcet_L2[ index ];
		miss_arr_L2_1[ bb->bbid ] += bb->proc_ptr->miss_bcet_L2[ index ];
		unknow_arr_L2_1[ bb->bbid ] += bb->proc_ptr->unknow_bcet_L2[ index ];


		hit_arr_2[ bb->bbid ] += bb->proc_ptr->hit_bcet[ index + 1 ] * (lp->loopbound -1);
		miss_arr_2[ bb->bbid ] += bb->proc_ptr->miss_bcet[ index + 1 ] * (lp->loopbound -1);
		unknow_arr_2[ bb->bbid ] += bb->proc_ptr->unknow_bcet[ index + 1 ] * (lp->loopbound -1);

		hit_arr_L2_2[ bb->bbid ] += bb->proc_ptr->hit_bcet_L2[ index + 1 ] * (lp->loopbound -1);
		miss_arr_L2_2[ bb->bbid ] += bb->proc_ptr->miss_bcet_L2[ index + 1 ] * (lp->loopbound -1);
		unknow_arr_L2_2[ bb->bbid ] += bb->proc_ptr->unknow_bcet_L2[ index + 1 ] * (lp->loopbound -1);

		if(print)  
		printf( "bb %d-%d procedure call %d bcet: 1 %Lu	2 %Lu\n", bb->pid, bb->bbid, bb->callpid, bb->proc_ptr->bcet[index * 2], bb->proc_ptr->bcet[index * 2 + 1]); fflush( stdout );
		if(print)
		{
			printf("bcet_arr_1[%d] = %Lu\n", bb->bbid, bcet_arr_1[bb->bbid]);
			printf("bcet_arr_2[%d] = %Lu\n", bb->bbid, bcet_arr_2[bb->bbid]);

			printf("hit_arr_1[%d] = %Lu\n", bb->bbid, hit_arr_1[bb->bbid]);
			printf("miss_arr_1[%d] = %Lu\n", bb->bbid, miss_arr_1[bb->bbid]);
			printf("unknow_arr_1[%d] = %Lu\n", bb->bbid, unknow_arr_1[bb->bbid]);

			printf("hit_arr_L2_1[%d] = %Lu\n", bb->bbid, hit_arr_L2_1[bb->bbid]);
			printf("miss_arr_L2_1[%d] = %Lu\n", bb->bbid, miss_arr_L2_1[bb->bbid]);
			printf("unknow_arr_L2_1[%d] = %Lu\n", bb->bbid, unknow_arr_L2_1[bb->bbid]);


			printf("hit_arr_2[%d] = %Lu\n", bb->bbid, hit_arr_2[bb->bbid]);
			printf("miss_arr_2[%d] = %Lu\n", bb->bbid, miss_arr_2[bb->bbid]);
			printf("unknow_arr_2[%d] = %Lu\n", bb->bbid, unknow_arr_2[bb->bbid]);

			printf("hit_arr_L2_2[%d] = %Lu\n", bb->bbid, hit_arr_L2_2[bb->bbid]);
			printf("miss_arr_L2_2[%d] = %Lu\n", bb->bbid, miss_arr_L2_2[bb->bbid]);
			printf("unknow_arr_L2_2[%d] = %Lu\n", bb->bbid, unknow_arr_L2_2[bb->bbid]);

		}

	/*
	  // region transition
	  if( regionmode && bb->regid != -1 ) {
	    int id = procs[bb->callpid]->bblist[0]->regid;
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
      lp->bcet[index] = bcet_arr_1[ topo[num_topo-1]->bbid ];
      lp->bcet[index + 1] = bcet_arr_2[ topo[num_topo-1]->bbid ];

	//first iteration, L1
	lp->hit_bcet[ index ] = hit_arr_1[ topo[num_topo-1]->bbid ];
	lp->miss_bcet[ index ] = miss_arr_1[ topo[num_topo-1]->bbid ];
	lp->unknow_bcet[ index ] = unknow_arr_1[ topo[num_topo-1]->bbid ];

	//first iteration, L2
	lp->hit_bcet_L2[ index ] = hit_arr_L2_1[ topo[num_topo-1]->bbid ];
	lp->miss_bcet_L2[ index ] = miss_arr_L2_1[ topo[num_topo-1]->bbid ];
	lp->unknow_bcet_L2[ index ] = unknow_arr_L2_1[ topo[num_topo-1]->bbid ];


	//next iteration, L1
	lp->hit_bcet[ index + 1 ] = hit_arr_2[ topo[num_topo-1]->bbid ];
	lp->miss_bcet[ index + 1 ] = miss_arr_2[ topo[num_topo-1]->bbid ];
	lp->unknow_bcet[ index + 1 ] = unknow_arr_2[ topo[num_topo-1]->bbid ];

	//next iteration, L2
	lp->hit_bcet_L2[ index + 1 ] = hit_arr_L2_2[ topo[num_topo-1]->bbid ];
	lp->miss_bcet_L2[ index + 1 ] = miss_arr_L2_2[ topo[num_topo-1]->bbid ];
	lp->unknow_bcet_L2[ index + 1 ] = unknow_arr_L2_2[ topo[num_topo-1]->bbid ];





      if( !lp->is_dowhile ) 
    {
	// one extra execution of loophead ?????????????????
	lp->bcet[index +1] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->bcost;

	//L1
	lp->hit_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index + 1]->hit;
	lp->miss_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index  + 1]->miss;
	lp->unknow_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index + 1]->unknow;

	//L2
	lp->hit_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->hit;
	lp->miss_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->miss;
	lp->unknow_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->unknow;



	if( lp->loophead->callpid != -1 )
	{
		lp->bcet[index + 1 ]  += p->bblist[lp->loophead->bbid]->proc_ptr->bcet[index + 1];

		//L1
		lp->hit_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->hit_bcet[index + 1];
		lp->miss_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->miss_bcet[index + 1];
		lp->unknow_bcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->unknow_bcet[index + 1];
		//L2
		lp->hit_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->hit_bcet_L2[index + 1];
		lp->miss_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->miss_bcet_L2[index + 1];
		lp->unknow_bcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->unknow_bcet_L2[index + 1];

	}
     }

		if(print)
		{
			printf("lp->bcet[%d] = %Lu\n", index, lp->bcet[index]);
			printf("lp->bcet[%d] = %Lu\n", index + 1, lp->bcet[index + 1]);

			printf("lp->hit_bcet = %Lu, %Lu\n",  lp->hit_bcet[ index ], lp->hit_bcet[ index + 1 ]);
			printf("lp->miss_bcet = %Lu, %Lu\n",  lp->miss_bcet[ index ], lp->miss_bcet[ index + 1 ]);
			printf("lp->unknow_bcet = %Lu, %Lu\n",  lp->unknow_bcet[ index ], lp->unknow_bcet[ index + 1 ]);

			printf("lp->hit_bcet_L2 = %Lu, %Lu\n",  lp->hit_bcet_L2[ index ], lp->hit_bcet_L2[ index + 1 ]);
			printf("lp->miss_bcet_L2 = %Lu, %Lu\n",  lp->miss_bcet_L2[ index ], lp->miss_bcet_L2[ index + 1 ]);
			printf("lp->unknow_bcet_L2 = %Lu, %Lu\n",  lp->unknow_bcet_L2[ index ], lp->unknow_bcet_L2[ index + 1 ]);

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

    free( bcet_arr_1 );
    free( bcet_arr_2 );
   //free( tmp_wpath );
  return 0;
}


/*
 * Determines WCET and WCET path of a procedure.
 * First analyse loops separately, starting from the inmost level.
 * When analysing an outer loop, the inner loop is treated as a black box.
 * Then analyse the procedure by treating the outmost loops as black boxes.
 */
void analyseProc_BCET( procedure *p ) {

  int  i, j;
  loop *lp;

  // analyse each loop from the inmost (reverse order from detection)
  for( i = p->num_loops - 1; i >= 0; i-- ) {
    lp = p->loops[i];

    // printf( "Analysis loop %d-%d\n", p->pid, lp->lpid ); fflush( stdout );
    // printLoop( lp ); fflush( stdout );
	for(j = 0; j < (1<<lp->level); j++)
		analyseDAGLoop_BCET(p, lp, j*2);
    // printf( "WCET: %d\n\n", lp->wcet );
  }

  // analyse procedure
  // printf( "Analysis proc %d\n", p->pid ); fflush( stdout );
  // printProc( p ); fflush( stdout );

for(j = 0; j < p->num_cost; j++)
  analyseDAGFunction_BCET( p, j);
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
int analysis_dag_BCET(MSC *msc) {

  int i, j, k;

  //cycle_time(0);

for(k = 0; k < msc->num_task; k++)
{
  // analyse each procedure in reverse topological order of call graph
  for( i = 0; i < msc->taskList[k].num_proc; i++ )
  	
  	for(j = 0; j < msc->taskList[k].proc_cg_ptr[i].num_proc; j++)
  	{
    		analyseProc_BCET(msc->taskList[k].proc_cg_ptr[i].proc[j]);
	}
  // for loading frequency calculation in dynamic locking
 // fprintf( stderr, "P%d\n", main_id );
	msc->taskList[k].bcet = msc->taskList[k].main_copy->bcet[0];


	msc->taskList[k].hit_bcet = msc->taskList[k].main_copy->hit_bcet[0];
	msc->taskList[k].miss_bcet = msc->taskList[k].main_copy->miss_bcet[0];
	msc->taskList[k].unknow_bcet = msc->taskList[k].main_copy->unknow_bcet[0];

	msc->taskList[k].hit_bcet_L2 = msc->taskList[k].main_copy->hit_bcet_L2[0];
	msc->taskList[k].miss_bcet_L2 = msc->taskList[k].main_copy->miss_bcet_L2[0];
	msc->taskList[k].unknow_bcet_L2 = msc->taskList[k].main_copy->unknow_bcet_L2[0];


	if(print)
	{
		printf("msc->taskList[%d].bcet = %Lu\n", k, msc->taskList[k].bcet);
	}


}
  //double t = cycle_time(1);
  //printf( "\nTime taken (analysis): %f ms\n", t/CYCLES_PER_MSEC ); fflush( stdout );

  return 0;
}


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
