#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "analysisDAG_WCET.h"
#include "analysisDAG_common.h"
#include "busSchedule.h"

// Forward declarations of static functions

static void computeWCET_loop(loop* lp, procedure* proc);

static void computeWCET_block(block* bb, procedure* proc, loop* cur_lp);

static void computeWCET_proc(procedure* proc, ull start_time);



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
int analyseDAGFunction_WCET(procedure *proc, int index)
{

  ull *wcet_arr = NULL;

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
  char max;
  block *bb;

  procedure *p;
  loop *lpn;

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
    
    wcet_arr[ bb->bbid ] = lpn->wcet[index * 2] + lpn->wcet[index * 2 + 1];

    hit_arr[ bb->bbid ] = lpn->hit_wcet[index * 2] + lpn->hit_wcet[index * 2 + 1];
    miss_arr[ bb->bbid ] = lpn->miss_wcet[index * 2] + lpn->miss_wcet[index * 2 + 1];
    unknow_arr[ bb->bbid ] = lpn->unknow_wcet[index * 2] + lpn->unknow_wcet[index * 2 + 1];
    
    hit_arr_L2[ bb->bbid ] = lpn->hit_wcet_L2[index * 2] + lpn->hit_wcet_L2[index * 2 + 1];
    miss_arr_L2[ bb->bbid ] = lpn->miss_wcet_L2[index * 2] + lpn->miss_wcet_L2[index * 2 + 1];
    unknow_arr_L2[ bb->bbid ] = lpn->unknow_wcet_L2[index * 2] + lpn->unknow_wcet_L2[index * 2 + 1];
    
  // add wcet at loopexit ???
    wcet_arr[ bb->bbid ] += wcet_arr[ lpn->loopexit->bbid ];

    hit_arr[ bb->bbid ] += hit_arr[ lpn->loopexit->bbid ];
    miss_arr[ bb->bbid ] += miss_arr[ lpn->loopexit->bbid ];
    unknow_arr[ bb->bbid ] += unknow_arr[ lpn->loopexit->bbid ];
    
    hit_arr_L2[ bb->bbid ] += hit_arr_L2[ lpn->loopexit->bbid ];
    miss_arr_L2[ bb->bbid ] += miss_arr_L2[ lpn->loopexit->bbid ];
    unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ lpn->loopexit->bbid ];


  if(print)
    {
      printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);

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
    wcet_arr[ bb->bbid ] = bb->chmc_L2[ index ]->wcost;

    hit_arr[ bb->bbid ] = bb->chmc[ index ]->hit;
    miss_arr[ bb->bbid ] = bb->chmc[ index ]->miss;
    unknow_arr[ bb->bbid ] = bb->chmc[ index ]->unknow;
    
    hit_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->hit;
    miss_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->miss;
    unknow_arr_L2[ bb->bbid ] = bb->chmc_L2[ index ]->unknow;

    
  if(print)
    {
      printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);

      printf("bb->chmc[%d]->hit = %d\n", index, bb->chmc[ index ]->hit);
      printf("bb->chmc[%d]->miss = %d\n", index, bb->chmc[ index ]->miss);
      printf("bb->chmc[%d]->unknow = %d\n", index, bb->chmc[ index ]->unknow);
      printf("bb->chmc_L2[%d]->hit = %d\n", index, bb->chmc_L2[ index ]->hit);
      printf("bb->chmc_L2[%d]->miss = %d\n", index, bb->chmc_L2[ index ]->miss);
      printf("bb->chmc_L2[%d]->unknow = %d\n", index, bb->chmc_L2[ index ]->unknow);

    }

  //---- successors ----
  switch( bb->num_outgoing ) {

  case 0:
    break;

  case 1:  // just add the weight of successor
    wcet_arr[ bb->bbid ] += wcet_arr[ bb->outgoing[0] ];

    hit_arr[ bb->bbid ] += hit_arr[ bb->outgoing[0] ];
    miss_arr[ bb->bbid ] += miss_arr[ bb->outgoing[0] ];
    unknow_arr[ bb->bbid ] += unknow_arr[ bb->outgoing[0] ];
  
    hit_arr_L2[ bb->bbid ] += hit_arr_L2[ bb->outgoing[0] ];
    miss_arr_L2[ bb->bbid ] += miss_arr_L2[ bb->outgoing[0] ];
    unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ bb->outgoing[0] ];


  if(print)
  {
      printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);

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
      max = 1;
    else if(bb->loopid != p->bblist[bb->outgoing[1]]->loopid)
      max = 0;
    else
    */
    
    {
    if( wcet_arr[ bb->outgoing[0] ] >= wcet_arr[ bb->outgoing[1] ] )
          max = 0;
      else
          max = 1;
    }
    wcet_arr[ bb->bbid ] += wcet_arr[ bb->outgoing[(int)max] ];

    hit_arr[ bb->bbid ] += hit_arr[ bb->outgoing[(int)max] ];
    miss_arr[ bb->bbid ] += miss_arr[ bb->outgoing[(int)max] ];
    unknow_arr[ bb->bbid ] +=  unknow_arr[ bb->outgoing[(int)max] ];
  
    hit_arr_L2[ bb->bbid ] += hit_arr_L2[ bb->outgoing[(int)max] ];
    miss_arr_L2[ bb->bbid ] += miss_arr_L2[ bb->outgoing[(int)max] ];
    unknow_arr_L2[ bb->bbid ] += unknow_arr_L2[ bb->outgoing[(int)max] ];

    
  if(print)
    {
      printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);

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
      wcet_arr[ bb->bbid ] += bb->proc_ptr->wcet[ index ];

    hit_arr[ bb->bbid ] += bb->proc_ptr->hit_wcet[ index ];
    miss_arr[ bb->bbid ] += bb->proc_ptr->miss_wcet[ index ];
    unknow_arr[ bb->bbid ] += bb->proc_ptr->unknow_wcet[ index ];

    hit_arr_L2[ bb->bbid ] += bb->proc_ptr->hit_wcet_L2[ index ];
    miss_arr_L2[ bb->bbid ] += bb->proc_ptr->miss_wcet_L2[ index ];
    unknow_arr_L2[ bb->bbid ] += bb->proc_ptr->unknow_wcet_L2[ index ];

  if(print)
    printf( "bb %d-%d procedure call %d wcet: %Lu\n", bb->pid, bb->bbid, bb->callpid, bb->proc_ptr->wcet[index] ); fflush( stdout );

  if(print)
    {
      printf("wcet_arr[%d] = %Lu\n", bb->bbid, wcet_arr[bb->bbid]);

      printf( "p-> hit_L2[ %d ]%Lu\n", index, bb->proc_ptr->hit_wcet_L2[ index ]); 
      printf( "p-> miss_L2[ %d ]%Lu\n", index, bb->proc_ptr->miss_wcet_L2[ index ]); 
      printf( "p-> unknow_L2[ %d ]%Lu\n", index, bb->proc_ptr->unknow_wcet_L2[ index ]); 

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
    char *wpath;
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
      char fn[100];
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
    int freq = 1;
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

  p->hit_wcet[index] = hit_arr[ topo[num_topo-1]->bbid ];
  p->miss_wcet[index] = miss_arr[ topo[num_topo-1]->bbid ];
  p->unknow_wcet[index] = unknow_arr[ topo[num_topo-1]->bbid ];

  p->hit_wcet_L2[index] = hit_arr_L2[ topo[num_topo-1]->bbid ];
  p->miss_wcet_L2[index] = miss_arr_L2[ topo[num_topo-1]->bbid ];
  p->unknow_wcet_L2[index] = unknow_arr_L2[ topo[num_topo-1]->bbid ];

  
  if(print)
    {
      printf("p->wcet[%d] = %Lu\n", index, p->wcet[index]);

      printf( "p-> hit[ %d ]%Lu\n", index, p->hit_wcet[ index ]); 
      printf( "p-> miss[ %d ]%Lu\n", index, p->miss_wcet[ index ]); 
      printf( "p-> unknow[ %d ]%Lu\n", index, p->unknow_wcet[ index ]); 

      printf( "p-> hit_L2[ %d ]%Lu\n", index, p->hit_wcet_L2[ index ]); 
      printf( "p-> miss_L2[ %d ]%Lu\n", index, p->miss_wcet_L2[ index ]); 
      printf( "p-> unknow_L2[ %d ]%Lu\n", index, p->unknow_wcet_L2[ index ]); 
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



int analyseDAGLoop_WCET(procedure *proc, loop *lop, int index ) 
{
  ull *wcet_arr_1 = NULL;
  ull *wcet_arr_2 = NULL;


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
    
    wcet_arr_1[ bb->bbid ] = lpn->wcet[ index ];
    wcet_arr_2[ bb->bbid ] = lpn->wcet[ index + 1 ] * (lp->loopbound -1);

    hit_arr_1[ bb->bbid ] = lpn->hit_wcet[ index ];
    miss_arr_1[ bb->bbid ] = lpn->miss_wcet[ index ];
    unknow_arr_1[ bb->bbid ] = lpn->unknow_wcet[ index  ];

    hit_arr_L2_1[ bb->bbid ] = lpn->hit_wcet_L2[ index ];
    miss_arr_L2_1[ bb->bbid ] = lpn->miss_wcet_L2[ index ];
    unknow_arr_L2_1[ bb->bbid ] = lpn->unknow_wcet_L2[ index ];



    hit_arr_2[ bb->bbid ] = lpn->hit_wcet[ index + 1 ] * (lp->loopbound -1);
    miss_arr_2[ bb->bbid ] = lpn->miss_wcet[ index + 1  ] * (lp->loopbound -1);
    unknow_arr_2[ bb->bbid ] = lpn->unknow_wcet[ index + 1  ] * (lp->loopbound -1);

    hit_arr_L2_2[ bb->bbid ] = lpn->hit_wcet_L2[ index + 1 ] * (lp->loopbound -1);
    miss_arr_L2_2[ bb->bbid ] = lpn->miss_wcet_L2[ index + 1  ] * (lp->loopbound -1);
    unknow_arr_L2_2[ bb->bbid ] = lpn->unknow_wcet_L2[ index + 1  ] * (lp->loopbound -1);



  // add wcet at loopexit ?????????????????????
    //wcet_arr_1[ bb->bbid ] += wcet_arr_1[ lpn->loopexit->bbid ];
    wcet_arr_2[ bb->bbid ] += wcet_arr_2[ lpn->loopexit->bbid ];

    hit_arr_2[ bb->bbid ] += hit_arr_2[ lpn->loopexit->bbid ];
    miss_arr_2[ bb->bbid ] += miss_arr_2[ lpn->loopexit->bbid ];
    unknow_arr_2[ bb->bbid ] += unknow_arr_2[ lpn->loopexit->bbid ];
    
    hit_arr_L2_2[ bb->bbid ] += hit_arr_L2_2[ lpn->loopexit->bbid ];
    miss_arr_L2_2[ bb->bbid ] += miss_arr_L2_2[ lpn->loopexit->bbid ];
    unknow_arr_L2_2[ bb->bbid ] += unknow_arr_L2_2[ lpn->loopexit->bbid ];



    if(print)
    {
      printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
      printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);


    }


      } // end if nested loophead

      else {
    //cost of this bb
    
  //cost of bb excluding loophead of other loops
    wcet_arr_1[ bb->bbid ] = bb->chmc_L2[index]->wcost;
    wcet_arr_2[ bb->bbid ] = bb->chmc_L2[index + 1]->wcost * (lp->loopbound -1);

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
      printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
      printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);

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

  case 1:  // just add the weight of successor
    wcet_arr_1[ bb->bbid ] += wcet_arr_1[ bb->outgoing[0] ];
    wcet_arr_2[ bb->bbid ] += wcet_arr_2[ bb->outgoing[0] ];

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
      printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
      printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);

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
    wcet_arr_1[ bb->bbid ] += wcet_arr_1[ bb->outgoing[(int)max1] ];
    wcet_arr_2[ bb->bbid ] += wcet_arr_2[ bb->outgoing[(int)max2] ];


    hit_arr_1[ bb->bbid ] +=  hit_arr_1[ bb->outgoing[(int)max1] ];
    miss_arr_1[ bb->bbid ] += miss_arr_1[ bb->outgoing[(int)max1] ];
    unknow_arr_1[ bb->bbid ] +=  unknow_arr_1[ bb->outgoing[(int)max1] ];
  
    hit_arr_L2_1[ bb->bbid ] +=  hit_arr_L2_1[ bb->outgoing[(int)max1] ];
    miss_arr_L2_1[ bb->bbid ] += miss_arr_L2_1[ bb->outgoing[(int)max1] ];
    unknow_arr_L2_1[ bb->bbid ] += unknow_arr_L2_1[ bb->outgoing[(int)max1] ];


    hit_arr_2[ bb->bbid ] +=  hit_arr_2[ bb->outgoing[(int)max2] ];
    miss_arr_2[ bb->bbid ] += miss_arr_2[ bb->outgoing[(int)max2] ];
    unknow_arr_2[ bb->bbid ] +=  unknow_arr_2[ bb->outgoing[(int)max2] ];
  
    hit_arr_L2_2[ bb->bbid ] +=  hit_arr_L2_2[ bb->outgoing[(int)max2] ];
    miss_arr_L2_2[ bb->bbid ] += miss_arr_L2_2[ bb->outgoing[(int)max2] ];
    unknow_arr_L2_2[ bb->bbid ] += unknow_arr_L2_2[ bb->outgoing[(int)max2] ];

    if(print)
    {
      printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
      printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);

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
      wcet_arr_1[ bb->bbid ] += bb->proc_ptr->wcet[ index ];
      wcet_arr_2[ bb->bbid ] += bb->proc_ptr->wcet[ index + 1 ] * (lp->loopbound -1);


    hit_arr_1[ bb->bbid ] += bb->proc_ptr->hit_wcet[ index ];
    miss_arr_1[ bb->bbid ] += bb->proc_ptr->miss_wcet[ index ];
    unknow_arr_1[ bb->bbid ] += bb->proc_ptr->unknow_wcet[ index ];

    hit_arr_L2_1[ bb->bbid ] += bb->proc_ptr->hit_wcet_L2[ index ];
    miss_arr_L2_1[ bb->bbid ] += bb->proc_ptr->miss_wcet_L2[ index ];
    unknow_arr_L2_1[ bb->bbid ] += bb->proc_ptr->unknow_wcet_L2[ index ];


    hit_arr_2[ bb->bbid ] += bb->proc_ptr->hit_wcet[ index + 1 ] * (lp->loopbound -1);
    miss_arr_2[ bb->bbid ] += bb->proc_ptr->miss_wcet[ index + 1 ] * (lp->loopbound -1);
    unknow_arr_2[ bb->bbid ] += bb->proc_ptr->unknow_wcet[ index + 1 ] * (lp->loopbound -1);

    hit_arr_L2_2[ bb->bbid ] += bb->proc_ptr->hit_wcet_L2[ index + 1 ] * (lp->loopbound -1);
    miss_arr_L2_2[ bb->bbid ] += bb->proc_ptr->miss_wcet_L2[ index + 1 ] * (lp->loopbound -1);
    unknow_arr_L2_2[ bb->bbid ] += bb->proc_ptr->unknow_wcet_L2[ index + 1 ] * (lp->loopbound -1);

  
  if(print)
    printf( "bb %d-%d procedure call %d wcet: 1 %Lu 2 %Lu\n", bb->pid, bb->bbid, bb->callpid,  bb->proc_ptr->wcet[index * 2],  bb->proc_ptr->wcet[index * 2 + 1]); fflush( stdout );
  if(print)
    {
      printf("wcet_arr_1[%d] = %Lu\n", bb->bbid, wcet_arr_1[bb->bbid]);
      printf("wcet_arr_2[%d] = %Lu\n", bb->bbid, wcet_arr_2[bb->bbid]);

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
      lp->wcet[ index ] = wcet_arr_1[ topo[ num_topo-1 ]->bbid ] + lp->num_fm_L2 * IC_MISS_L2 + lp->num_fm * IC_MISS;
      lp->wcet[ index + 1 ] = wcet_arr_2[ topo[num_topo-1]->bbid ];

  //first iteration, L1
  lp->hit_wcet[ index ] = hit_arr_1[ topo[num_topo-1]->bbid ];
  lp->miss_wcet[ index ] = miss_arr_1[ topo[num_topo-1]->bbid ];
  lp->unknow_wcet[ index ] = unknow_arr_1[ topo[num_topo-1]->bbid ];

  //first iteration, L2
  lp->hit_wcet_L2[ index ] = hit_arr_L2_1[ topo[num_topo-1]->bbid ];
  lp->miss_wcet_L2[ index ] = miss_arr_L2_1[ topo[num_topo-1]->bbid ];
  lp->unknow_wcet_L2[ index ] = unknow_arr_L2_1[ topo[num_topo-1]->bbid ];


  //next iteration, L1
  lp->hit_wcet[ index + 1 ] = hit_arr_2[ topo[num_topo-1]->bbid ];
  lp->miss_wcet[ index + 1 ] = miss_arr_2[ topo[num_topo-1]->bbid ];
  lp->unknow_wcet[ index + 1 ] = unknow_arr_2[ topo[num_topo-1]->bbid ];

  //next iteration, L2
  lp->hit_wcet_L2[ index + 1 ] = hit_arr_L2_2[ topo[num_topo-1]->bbid ];
  lp->miss_wcet_L2[ index + 1 ] = miss_arr_L2_2[ topo[num_topo-1]->bbid ];
  lp->unknow_wcet_L2[ index + 1 ] = unknow_arr_L2_2[ topo[num_topo-1]->bbid ];

  if(print)
  {
    printf(" lp->wcet = %Lu, %Lu\n", lp->wcet[index], lp->wcet[ index + 1 ]);

    printf(" lp->hit_wcet = %Lu, %Lu\n", lp->hit_wcet[index], lp->hit_wcet[ index + 1 ]);
    printf(" lp->miss_wcet = %Lu, %Lu\n", lp->miss_wcet[index], lp->miss_wcet[ index + 1 ]);
    printf(" lp->unknow_wcet = %Lu, %Lu\n", lp->unknow_wcet[index], lp->unknow_wcet[ index + 1 ]);

    printf(" lp->hit_wcet_L2 = %Lu, %Lu\n", lp->hit_wcet_L2[index], lp->hit_wcet_L2[ index + 1 ]);
    printf(" lp->miss_wcet_L2 = %Lu, %Lu\n", lp->miss_wcet_L2[index], lp->miss_wcet_L2[ index + 1 ]);
    printf(" lp->unknow_wcet_L2 = %Lu, %Lu\n", lp->unknow_wcet_L2[index], lp->unknow_wcet_L2[ index + 1 ]);

  }


      if( !lp->is_dowhile ) 
    {
  // one extra execution of loophead ?????????????????
  lp->wcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->wcost;

  //L1
  lp->hit_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index + 1]->hit;
  lp->miss_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index  + 1]->miss;
  lp->unknow_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->chmc[index + 1]->unknow;

  //L2
  lp->hit_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->hit;
  lp->miss_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->miss;
  lp->unknow_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->chmc_L2[index + 1]->unknow;


  if( lp->loophead->callpid != -1 )
  {
    lp->wcet[ index +1 ]  += p->bblist[lp->loophead->bbid]->proc_ptr->wcet[index + 1];

    //L1
    lp->hit_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->hit_wcet[index + 1];
    lp->miss_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->miss_wcet[index + 1];
    lp->unknow_wcet[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->unknow_wcet[index + 1];
    //L2
    lp->hit_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->hit_wcet_L2[index + 1];
    lp->miss_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->miss_wcet_L2[index + 1];
    lp->unknow_wcet_L2[index +1 ] += p->bblist[lp->loophead->bbid]->proc_ptr->unknow_wcet_L2[index + 1];

  }
     }

    if(print)
    {
      printf("lp->wcet[%d] = %Lu\n", index, lp->wcet[index]);
      printf("lp->wcet[%d] = %Lu\n", index+1, lp->wcet[index + 1]);

      printf("lp->hit_wcet = %Lu, %Lu\n",  lp->hit_wcet[ index ], lp->hit_wcet[ index + 1 ]);
      printf("lp->miss_wcet = %Lu, %Lu\n",  lp->miss_wcet[ index ], lp->miss_wcet[ index + 1 ]);
      printf("lp->unknow_wcet = %Lu, %Lu\n",  lp->unknow_wcet[ index ], lp->unknow_wcet[ index + 1 ]);

      printf("lp->hit_wcet_L2 = %Lu, %Lu\n",  lp->hit_wcet_L2[ index ], lp->hit_wcet_L2[ index + 1 ]);
      printf("lp->miss_wcet_L2 = %Lu, %Lu\n",  lp->miss_wcet_L2[ index ], lp->miss_wcet_L2[ index + 1 ]);
      printf("lp->unknow_wcet_L2 = %Lu, %Lu\n",  lp->unknow_wcet_L2[ index ], lp->unknow_wcet_L2[ index + 1 ]);

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
void analyseProc_WCET( procedure *p ) {

  int  i, j;
  loop *lp;

  // analyse each loop from the inmost (reverse order from detection)
  for( i = p->num_loops - 1; i >= 0; i-- ) {
    lp = p->loops[i];

    // printf( "Analysis loop %d-%d\n", p->pid, lp->lpid ); fflush( stdout );
    // printLoop( lp ); fflush( stdout );
  for(j = 0; j < (1<<lp->level); j++)
    analyseDAGLoop_WCET(p, lp, j*2);
    // printf( "WCET: %d\n\n", lp->wcet );
  }

  // analyse procedure
  // printf( "Analysis proc %d\n", p->pid ); fflush( stdout );
  // printProc( p ); fflush( stdout );

for(j = 0; j < p->num_cost; j++)
  analyseDAGFunction_WCET( p, j);
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
int analysis_dag_WCET(MSC *msc) {
  int i, j, k;

  //cycle_time(0);

  for(k = 0; k < msc->num_task; k++)
  {
     /* analyse each procedure in reverse topological order of call graph */
     for( i = 0; i < msc->taskList[k].num_proc; i++ )
     for(j = 0; j < msc->taskList[k].proc_cg_ptr[i].num_proc; j++)
     {
      analyseProc_WCET(msc->taskList[k].proc_cg_ptr[i].proc[j]);
     }
   
  /* for loading frequency calculation in dynamic locking
  fprintf( stderr, "P%d\n", main_id ); */
  msc->taskList[k].wcet = msc->taskList[k].main_copy->wcet[0];

  msc->taskList[k].hit_wcet = msc->taskList[k].main_copy->hit_wcet[0];
  msc->taskList[k].miss_wcet = msc->taskList[k].main_copy->miss_wcet[0];
  msc->taskList[k].unknow_wcet = msc->taskList[k].main_copy->unknow_wcet[0];

  msc->taskList[k].hit_wcet_L2 = msc->taskList[k].main_copy->hit_wcet_L2[0];
  msc->taskList[k].miss_wcet_L2 = msc->taskList[k].main_copy->miss_wcet_L2[0];
  msc->taskList[k].unknow_wcet_L2 = msc->taskList[k].main_copy->unknow_wcet_L2[0];

  
  if(print)
  {
    printf("msc->taskList[%d].wcet = %Lu\n", k, msc->taskList[k].wcet);
   }
 }
 
 /* double t = cycle_time(1);
  printf( "\nTime taken (analysis): %f ms\n", t/CYCLES_PER_MSEC ); 
  fflush( stdout ); */

  return 0;
}

/***********************************************************************/
/* sudiptac:: This part of the code is only used for the WCET and
 * BCET analysis in presence shared data bus. All procedures in the
 * following is used only for this purpose and therefore can safely 
 * be ignored for analysis which does not include shared data bus 
 */ 
/***********************************************************************/

/* sudiptac:: Determines WCET of a procedure in presence of shared data
 * bus. We assume the shared cache analysis at this point and CHMC 
 * classification for every instruction has already been computed. We 
 * also assume a statically generated TDMA bus schedule and the 
 * worst/best case starting time of the procedure since in presence of 
 * shared data bus worst/best case execution time of a procedure/loop 
 * depends on its starting time */

#ifdef _DEBUG
static void dump_prog_info(procedure* proc)
{
  loop* lp;
  block* bb;
  instr* inst;
  int i,j,k;
  FILE* fp = stdout;

  fprintf(fp, "Printing incoming block information\n");   
  for(i = 0; i < proc->num_bb; i++)
  {
    assert(proc->bblist[i]); 
    bb = proc->bblist[i];
    fprintf(fp, "Basic block id = (%d.%d.0x%lx.start=%Lu.finish=%Lu)\n", proc->pid, 
      bb->bbid, (uintptr_t)bb, bb->start_time, bb->finish_time); 
    fprintf(fp, "Incoming blocks (Total = %d)======> \n", bb->num_incoming); 
    for(j = 0; j < bb->num_incoming; j++)
    {
       fprintf(fp, "(bb=%d.%lx)\n", bb->incoming[j],
        (uintptr_t)proc->bblist[bb->incoming[j]]);
    }
  }
  fprintf(fp, "Printing Topological information ====>\n");    
  for(i = proc->num_topo - 1; i >= 0; i--)
  {
     bb = proc->topo[i]; 
     fprintf(fp, "(proc=%d.bb=%d)\n", bb->pid, bb->bbid); 
  }
  fprintf(fp, "Printing Loop information ====>\n");   
  for(i = 0; i < proc->num_loops; i++)
  {
    lp = proc->loops[i]; 
    assert(lp->loophead);
    fprintf(fp, "(proc=%d.loop=%d.loophead=%d.loopsink=%d.loopexit=%d)\n", 
      proc->pid, lp->lpid, lp->loophead->bbid, lp->loopsink->bbid,
      lp->loopexit ? lp->loopexit->bbid : -1); 
    fprintf(fp, "Loop Nodes ====>\n");  
    for(j = lp->num_topo - 1; j >= 0; j--)
    {
      assert(lp->topo[j]);     
       fprintf(fp, "(bb=%d)", lp->topo[j]->bbid);
    }
    fprintf(fp,"\n");
  }
  
  fprintf(fp, "Printing instruction call relationship\n");
  for(i = 0; i < proc->num_bb; i++)
  {
     bb = proc->bblist[i]; 

     for(j = 0; j < bb->num_instr; j++)
     {
       inst = bb->instrlist[j];
       if(IS_CALL(inst->op))
       {
        fprintf(fp, "PROCEDURE CALL ENCOUNTERED at %s\n", GET_CALLEE(inst));   
        /* Ignore library calls */
        if(!proc->calls)
          continue; 
          for(k = 0; k < proc->num_calls; k++)
        {
           uint callee_id = proc->calls[k];
           procedure* callee = procs[callee_id];
           assert(callee);
           fprintf(fp, "Got start address = %x\n", 
              callee->topo[callee->num_topo - 1]->startaddr);
        }
      } 
    }
  }
}
#endif

#ifdef _DEBUG
/* This is for debugging. Dumped chmc info after preprocessing */
static void dump_pre_proc_chmc(procedure* proc)
{
  int i,j,k;
  block* bb;
  instr* inst;

  for(i = 0; i < proc->num_bb; i++)
  {
    bb = proc->bblist[i];     
    
    for(j = 0; j < bb->num_instr; j++)
    {
      inst = bb->instrlist[j];
       fprintf(stdout, "Instruction address = %s ==>\n", inst->addr);
      for(k = 0; k < bb->num_chmc; k++)
      {
        if(inst->acc_t[k] == L1_HIT)
          fprintf(stdout, "L1_HIT\n");
        else  
          fprintf(stdout, "L2_MISS\n");
      }    
    }
  }
}
#endif

/* Attach hit/miss classification for L2 cache to the instruction */
static void classify_inst_L2(instr* inst, CHMC** chmc_l2, int n_chmc_l2, int inst_id)
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

    if(!chmc_l2[i]->hitmiss_addr) continue;

    if((chmc_l2[i]->hitmiss_addr[inst_id] == ALWAYS_HIT) &&
           inst->acc_t[i] != L1_HIT)
    {
      inst->acc_t_l2[i] = L2_HIT;  
    }
  }
}

/* Attach hit/miss classification to the instruction */
static void classify_inst(instr* inst, CHMC** chmc, int n_chmc, int inst_id)
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
  
    /* FIXME: I think this is possible for a buggy implementation
     * in cache analysis */
    if(!chmc[i]->hitmiss_addr) { 
      continue;
    }

    if(chmc[i]->hitmiss_addr[inst_id] == ALWAYS_HIT)
    {
      inst->acc_t[i] = L1_HIT;   
    }
  }
}

/* Attach chmc classification for L2 cache to the instruction 
 * data structure */
static void preprocess_chmc_L2(procedure* proc)
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
      classify_inst_L2(inst, bb->chmc_L2, bb->num_chmc_L2, j);     
    }
  }
}

/* Attach chmc classification to the instruction data structure */
static void preprocess_chmc(procedure* proc)
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
      classify_inst(inst, bb->chmc, bb->num_chmc, j);    
    }
  }
}

/* This sets the latest starting time of a block
 * during WCET calculation */
static void set_start_time(block* bb, procedure* proc)
{
  int i, in_index;
  ull max_start = bb->start_time;
  /* ull max_start = 0; */

  assert(bb);

  for(i = 0; i < bb->num_incoming; i++)
  {
     in_index = bb->incoming[i];
     assert(proc->bblist[in_index]);
     /* Determine the predecessors' latest finish time */
     if(max_start < proc->bblist[in_index]->finish_time)
      max_start = proc->bblist[in_index]->finish_time;     
  }

  /* Now set the starting time of this block to be the latest 
   * finish time of predecessors block */
  bb->start_time = max_start;

#ifdef _DEBUG
  printf("Setting max start of bb %d = %Lu\n", bb->bbid, max_start);
#endif
}

static void set_start_time_opt(block* bb, procedure* proc, uint context)
{
  int i, in_index;
  ull max_start = bb->start_opt[context];
  /* ull max_start = 0; */

  assert(bb);

  for(i = 0; i < bb->num_incoming; i++)
  {
     in_index = bb->incoming[i];
     assert(proc->bblist[in_index]);
     /* Determine the predecessors' latest finish time */
     if(max_start < proc->bblist[in_index]->fin_opt[context]) {
       /* If a bus-miss has been encountered in predecessor basic block 
       * propagate the information to the current block */
      if(proc->bblist[in_index]->latest_bus[context])
          bb->latest_bus[context] = proc->bblist[in_index]->latest_bus[context]; 
      if(proc->bblist[in_index]->latest_latency[context])
          bb->latest_latency[context] = proc->bblist[in_index]->latest_latency[context]; 
      max_start = proc->bblist[in_index]->fin_opt[context]; 
     }  
  }

  /* Now set the starting time of this block to be the latest 
   * finish time of predecessors block */
  bb->start_opt[context] = max_start;

#ifdef _DEBUG
  printf("Setting max start of bb %d = %Lu\n", bb->bbid, max_start);
#endif
}

/* Determine approximate memory/bus delay for a L1 cache miss */

uint determine_latency(block* bb, uint context, uint bb_cost, acc_type type)
{
  ull delay = 0;
  ull offset;
  ull interval;
  uint slot_len;
  uint latency;
  uint p_latency;
  int n;
  int ncore = 4;

  offset = bb->start_opt[context] + bb_cost - bb->latest_bus[context];

  /* FIXME: Core number is hard-coded */
  slot_len = global_sched_data->seg_list[0]->per_core_sched[0]->slot_len;
  interval = global_sched_data->seg_list[0]->per_core_sched[0]->interval;
  
  /* Return maximum if no bus is modeled */   
  if(g_no_bus_modeling)
  {
    if(type == L2_HIT)  
      return ((ncore - 1) * interval + 2 * L2_HIT_LATENCY);
    else if(type == L2_MISS)  
      return ((ncore - 1) * interval + 2 * MISS_PENALTY);
  }

  /* CAUTION :::: Following code is a bit of mathematical manipulation.     
   * Hard to understand without the draft */
  if(!bb->latest_bus[context]) {
    bb->latest_bus[context] = bb->start_opt[context] + bb_cost;  
    if(type == L2_HIT) {
      bb->latest_bus[context] += L2_HIT_LATENCY;
      bb->latest_latency[context] = L2_HIT_LATENCY;  
      delay = (interval - slot_len) + L2_HIT_LATENCY;
    }  
    else if(type == L2_MISS) {
      bb->latest_bus[context] += MISS_PENALTY;
      bb->latest_latency[context] = MISS_PENALTY;  
      delay = (interval - slot_len) + MISS_PENALTY;
    }  
  }  
  else if((offset < slot_len - L2_HIT_LATENCY) && type == L2_HIT)
    delay = L2_HIT_LATENCY;  
  else if((offset < slot_len - MISS_PENALTY) && type == L2_MISS)
    delay = MISS_PENALTY;  
  /* Mathematics, mathematics and more mathematics */ 
  else
  {
    if(type == L2_HIT)
      latency = L2_HIT_LATENCY;
    else
      latency = MISS_PENALTY;

    p_latency = (bb->latest_latency[context]);

    n = ((offset + p_latency) / (ncore * slot_len)) + 1; 
    /* Check whether can be served in an outstanding bus slot */
    if(offset <= ((n - 1) * ncore + 1) * slot_len - latency - p_latency)
      delay = latency;  
    /* Otherwise create a new bus slot and schedule */  
    else {
      delay = (n * ncore * slot_len) + latency - p_latency - offset;
      bb->latest_bus[context] = bb->start_opt[context] + bb_cost + delay;  
      bb->latest_latency[context] = latency;  
    }  
  }
#ifdef _PRINT
  /* Exceeds maximum possible delay......print it out */    
  if(delay > slot_len * (ncore - 1) + 2 * MISS_PENALTY) 
  {
     fprintf(stdout, "Bus delay exceeded maximum limit (%Lu)\n", delay); 
  }
#endif
  return delay;   
}

/* Computes end alignment cost of the loop */
ull endAlign(ull fin_time)
{
  ull interval = global_sched_data->seg_list[0]->per_core_sched[0]->interval;

  if(fin_time % interval == 0)  
  {
#ifdef _DEBUG
    printf("End align = 0\n");
#endif
    return 0;
  } 
  else
  {
#ifdef _DEBUG
    printf("End align = %Lu\n", (fin_time / interval + 1) * interval - fin_time);  
#endif
    return ((fin_time / interval + 1) * interval - fin_time);  
  } 
}

/* computes start alignment cost */
uint startAlign()
{
#ifdef _DEBUG
    printf("Start align = %u\n", global_sched_data->seg_list[0]->per_core_sched[0]->interval);
#endif
    return global_sched_data->seg_list[0]->per_core_sched[0]->interval;
}

/* Preprocess one loop for optimized bus aware WCET calculation */
/* This takes care of the alignments of loop at te beginning and at the 
 * end */
static void preprocess_one_loop(loop* lp, procedure* proc, ull start_time)
{
  int i,j,k;
  block* bb;
  loop* inlp = NULL;
  CHMC* cur_chmc;
  CHMC* cur_chmc_L2;
  instr* inst;
  uint bb_cost = 0;  
  uint latency = 0;
  uint lpbound;
  uint max_fin[64] = {0};

  /* We can assume the start time to be always zero */   
  start_time = 0;

  /* Compute only once */   
  if(lp->wcet_opt[0])
    return;   

#ifdef _NPRINT
       fprintf(stdout, "Visiting loop = %d.%d.0x%x\n", lp->pid,lp->lpid,(unsigned)lp);
#endif
  
  /* FIXME: correcting loop bound */    
  lpbound = lp->loopexit ? lp->loopbound : (lp->loopbound + 1);

  /* Traverse all the blocks in topological order. Topological
   * order does not assume internal loops. Thus all internal 
   * loops are considered to be black boxes */
  for(i = lp->num_topo - 1; i >= 0; i--)
  {
    bb =  lp->topo[i];
    /* bb cannot be empty */
    assert(bb);
    /* initialize basic block cost */
    bb_cost = 0;
    memset(max_fin, 0, 64);

    /* Traverse over all the CHMC-s of this basic block */
    for(j = 0; j < bb->num_chmc; j++)  
    {
      cur_chmc = (CHMC *)bb->chmc[j]; 
      cur_chmc_L2 = (CHMC *)bb->chmc_L2[j];
      /* Reset start , finish time and cost */
      bb->start_opt[j] = 0;         
      bb->fin_opt[j] = 0;         
      bb_cost = 0;
    
      /* Check whether this basic block is the header of some other 
       * loop */
        if((inlp = check_loop(bb, proc)) && i != lp->num_topo - 1)
       {
        /* FIXME: do I need this ? */
        /* set_start_time_opt(bb, proc, j); */
        preprocess_one_loop(inlp, proc, bb->start_opt[j]);
        bb->fin_opt[j] = bb->start_opt[j] + startAlign() + inlp->wcet_opt[2*j] + 
           startAlign() + (inlp->wcet_opt[2*j+1] + endAlign(inlp->wcet_opt[2*j+1])) 
           * inlp->loopbound;
        continue;
        }
      
      if(i == lp->num_topo - 1)
        bb->start_opt[j] = start_time;
      /* Otherwise, set the maximum of the finish time of predecesssor 
       * basic blocks */
      else  
        set_start_time_opt(bb, proc, j); 
#ifdef _NPRINT
       fprintf(stdout, "Current CHMC = 0x%x\n", (unsigned)cur_chmc);
       fprintf(stdout, "Current CHMC L2 = 0x%x\n", (unsigned)cur_chmc_L2);
#endif
      for(k = 0; k < bb->num_instr; k++)
      {
        inst = bb->instrlist[k];
        /* Instruction cannot be empty */
        assert(inst);
        /* Check for a L1 miss */
#ifdef _NPRINT
        if(cur_chmc->hitmiss_addr[k] != ALWAYS_HIT)
          fprintf(stdout, "L1 miss at 0x%x\n", (unsigned)bb->startaddr); 
#endif
        /* first check whether the instruction is an L1 hit or not */
        /* In that easy case no bus access is required */
        if(cur_chmc->hitmiss_addr[k] == ALWAYS_HIT)
        {
           bb_cost += L1_HIT_LATENCY;
        }
        /* Otherwise if it is an L2 hit */
        else if (cur_chmc_L2->hitmiss_addr[k] == ALWAYS_HIT)
        {
           /* access shared bus */
           latency = determine_latency(bb, j, bb_cost, L2_HIT);
#ifdef _NPRINT
           fprintf(stdout, "Latency = %u\n", latency);
#endif
           bb_cost += latency;
        }
        /* Else it is an L2 miss */
        else
        {
           /* access shared bus */
           latency = determine_latency(bb, j, bb_cost, L2_MISS);
#ifdef _NPRINT
           fprintf(stdout, "Latency = %u\n", latency);
#endif
           bb_cost += latency;
        }
         /* Handle procedure call instruction */
        if(IS_CALL(inst->op))
         {  
           procedure* callee = getCallee(inst, proc); 
         
          /* For ignoring library calls */
          if(callee)
          {
             /* Compute the WCET of the callee procedure here.
            * We dont handle recursive procedure call chain 
             */
             computeWCET_proc(callee, bb->start_opt[j] + bb_cost);
            /* Single cost for call instruction */
             bb_cost += callee->running_cost;  
          }
         }
      }
      /* Set finish time of the basic block */
      bb->fin_opt[j] = bb->start_opt[j] + bb_cost;
      /* Set max finish time */
      if(max_fin[j] < bb->fin_opt[j])
        max_fin[j] = bb->fin_opt[j];   
    }
  }
  for(j = 0; j < lp->loophead->num_chmc; j++)
  {
     lp->wcet_opt[j] = (max_fin[j] - 1);  
#ifdef _PRINT
     fprintf(stdout, "WCET of loop (%d.%d.0x%lx)[%d] = %Lu\n", lp->pid, lp->lpid,
        (uintptr_t)lp, j, lp->wcet_opt[j]);
#endif
  }
}

/* Preprocess each loop for optimized bus aware WCET calculation */
static void preprocess_all_loops(procedure* proc)
{
  int i;    

  /* Preprocess loops....in reverse topological order i.e. in reverse 
   * order of detection */
  for(i = proc->num_loops - 1; i >= 0; i--)
  {
    /* Preprocess only outermost loop, inner ones will be processed 
     * recursively */
    if(proc->loops[i]->level == 0) 
      preprocess_one_loop(proc->loops[i], proc, 1); 
  }
}

/* Computes the latest finish time and worst case cost of a loop.
 * This procedure fully unrolls the loop virtually during computation.
 * Can be optimized for specific bus schedule ? */
static void computeWCET_loop(loop* lp, procedure* proc)
{
  int i, lpbound;
  int j;
  block* bb;

#ifdef _DEBUG
  fprintf(stdout, "Visiting loop = (%d.%lx)\n", lp->lpid, (uintptr_t)lp);
#endif

  /* FIXME: correcting loop bound */    
  lpbound = lp->loopexit ? lp->loopbound : (lp->loopbound + 1);
  
  /* For computing wcet of the loop it must be visited 
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
        * set the start time to be the latest finish time 
        * of predecessor otherwise latest finish time of 
        * loop sink */
       if(bb->bbid == lp->loophead->bbid && i == 0)
        set_start_time(bb, proc);    
       else if(bb->bbid == lp->loophead->bbid)
       {
        assert(lp->loopsink);  
        bb->start_time = (bb->start_time < lp->loopsink->finish_time) 
                    ? lp->loopsink->finish_time
                    : bb->start_time;
#ifdef _DEBUG
        printf("Setting loop %d finish time = %Lu\n", lp->lpid,
           lp->loopsink->finish_time);
#endif
       }  
       else
        set_start_time(bb, proc);    
       computeWCET_block(bb, proc, lp);    
     }
  }
   /* CAUTION: Update the current context */ 
  cur_context /= 2;
}

/* Compute worst case finish time and cost of a block */
static void computeWCET_block(block* bb, procedure* proc, loop* cur_lp)
{
  int i;    
  loop* inlp;
  instr* inst;
  uint acc_cost = 0;
  acc_type acc_t;
  procedure* callee;

#ifdef _DEBUG
  fprintf(stdout, "Visiting block = (%d.%lx)\n", bb->bbid, (uintptr_t)bb);
#endif

   /* Check whether the block is some header of a loop structure. 
    * In that case do separate analysis of the loop */
   /* Exception is when we are currently in the process of analyzing 
    * the same loop */
   if((inlp = check_loop(bb,proc)) && (!cur_lp || (inlp->lpid != cur_lp->lpid)))
   {
    if(!g_optimized)  
     computeWCET_loop(inlp, proc);
    /* optimized version. Compute WCET of loop once */ 
    else 
    {
      bb->finish_time = bb->start_time + startAlign() + inlp->wcet_opt[0]  
               + startAlign() 
               + (inlp->wcet_opt[1] + endAlign(inlp->wcet_opt[1])) 
               * inlp->loopbound;    
    }
   }  

   /* It's not a loop. Go through all the instructions and
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
             computeWCET_proc(callee, bb->start_time + acc_cost);
            /* Single cost for call instruction */
             acc_cost += (callee->running_cost + 1);   
         }
       }
       /* No proedure call ---- normal instruction */
       else
       {
         all_inst++;   
         /* If its a L1 hit add only L1 cache latency */
         if((acc_t = check_hit_miss(bb, inst)) == L1_HIT)
          acc_cost += (L1_HIT_LATENCY);
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
#ifdef _DEBUG
  printf("Setting block %d finish time = %Lu\n", bb->bbid,
         bb->finish_time);
#endif
}

static void computeWCET_proc(procedure* proc, ull start_time)
{
  int i;    
  block* bb;
  ull max_f_time = 0;

  /* Initialize current context. Set to zero before the start 
   * of each new procedure */   
  cur_context = 0;

  /* Preprocess CHMC classification for each instruction inside
   * the procedure */
  preprocess_chmc(proc);
  
  /* Preprocess CHMC classification for L2 cache for each 
   * instruction inside the procedure */
  preprocess_chmc_L2(proc);

  /* Preprocess all the loops for optimized WCET calculation */
  /********CAUTION*******/
  preprocess_all_loops(proc);

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
      set_start_time(bb, proc);
    computeWCET_block(bb, proc, NULL);
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
     if(max_f_time < proc->topo[i]->finish_time)
       max_f_time = proc->topo[i]->finish_time;    
  }  

  proc->running_finish_time = max_f_time;
  proc->running_cost = max_f_time - start_time;
#ifdef _DEBUG
  fprintf(stdout, "Set worst case cost of the procedure %d = %Lu\n", 
        proc->pid, proc->running_cost);     
#endif        
}

/* This is a top level call and always start computing the WCET from 
 * "main" function */

void computeWCET(ull start_time)
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
   computeWCET_proc(procs[top_func], start_time); 

#ifdef _PRINT
  fprintf(stdout, "\n\n**************************************************************\n");    
  fprintf(stdout, "Latest start time of the program = %Lu start_time\n", start_time);
  fprintf(stdout, "Latest finish time of the program = %Lu cycles\n", 
      procs[top_func]->running_finish_time);
  if(g_shared_bus)    
      fprintf(stdout, "WCET of the program with shared bus = %Lu cycles\n", 
        procs[top_func]->running_cost);
  else      
      fprintf(stdout, "WCET of the program without shared bus = %Lu cycles\n", 
        procs[top_func]->running_cost);
  fprintf(stdout, "**************************************************************\n\n");    
#endif
}

/* Given a MSC and a task inside it, this function computes 
 * the latest start time of the argument task. Finding out 
 * the latest start time is important as the bus aware WCET
 * analysis depends on the same */
static void update_succ_latest_time(MSC* msc, task_t* task)
{
  int i;    
  uint sid;

#ifdef _DEBUG
  fprintf(stdout, "Number of Successors = %d\n", task->numSuccs);
#endif
  for(i = 0; i < task->numSuccs; i++)
  {
#ifdef _DEBUG
    fprintf(stdout, "Successor id with %d found\n", task->succList[i]);
#endif
    sid = task->succList[i];
    if(msc->taskList[sid].l_start < (task->l_start + task->wcet))
      msc->taskList[sid].l_start = task->l_start + task->wcet;
#ifdef _DEBUG
    fprintf(stdout, "Updating latest start time of successor = %Lu\n", 
        msc->taskList[sid].l_start);       
#endif
  }
}

/* Returns the latest starting of a task in the MSC */
/* Latest starting time of a task is computed as the maximum
 * of the latest finish times of all its predecessor tasks 
 * imposed by the partial order of the MSC */
static ull get_latest_start_time(task_t* cur_task, uint core)
{   
  ull start;

  /* If independent task mode return 0 */
  if(g_independent_task)
    return 0;  

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
#ifdef _DEBUG
  fprintf(stdout, "Assigning the latest starting time of the task = %Lu\n", start);   
#endif

  return start;   
}

/* Analyze worst case execution time of all the tasks inside 
 * a MSC. The MSC is given by the argument */
void compute_bus_WCET_MSC(MSC *msc, const char *tdma_bus_schedule_file) {
  int k;
  ull start_time = 0;
  procedure* proc;
        
  /* Set the global TDMA bus schedule */      
  setSchedule(tdma_bus_schedule_file);

  /* Reset the latest time of all cores */
  memset(latest, 0, num_core * sizeof(ull)); 
  /* reset latest time of all tasks */
  reset_all_task(msc);

  for(k = 0; k < msc->num_task; k++)
  {
    acc_bus_delay = 0;
    /* Get the main procedure */ 
#ifdef _PRINT
    fprintf(stdout, "Analyzing Task WCET %s......\n", msc->taskList[k].task_name);
    fflush(stdout);
#endif
    all_inst = 0;   
    cur_task = &(msc->taskList[k]);   
    proc = msc->taskList[k].main_copy;
    /* Return the core number in which the task is assigned */
    ncore = get_core(cur_task);
    /* First get the latest start time of task id "k" in this msc 
    * because the bus aware WCET analysis depends on the latest 
    * starting time of the corresponding task */
    start_time = get_latest_start_time(cur_task, ncore);
    computeWCET_proc(proc, start_time);
    /* Set the worst case cost of this task */
    msc->taskList[k].wcet = msc->taskList[k].main_copy->running_cost;
    /* Now update the latest starting time in this core */
    latest[ncore] = start_time + msc->taskList[k].wcet;
    /* Since the interference file for a MSC was dumped in topological 
    * order and read back in the same order we are assured of the fact 
    * that we analyze the tasks inside a MSC only after all of its 
    * predecessors have been analyzed. Thus After analyzing one task 
    * update all its successor tasks' latest time */
    update_succ_latest_time(msc, cur_task);
#ifdef _PRINT
  fprintf(stdout, "\n\n**************************************************************\n");    
  fprintf(stdout, "Latest start time of the program = %Lu start_time\n", start_time);
  fprintf(stdout, "Latest finish time of the task = %Lu cycles\n", 
      proc->running_finish_time);
  if(g_shared_bus)    
      fprintf(stdout, "WCET of the task with shared bus = %Lu cycles\n", 
          proc->running_cost);
  else        
      fprintf(stdout, "WCET of the task without shared bus = %Lu cycles\n", 
          proc->running_cost);
  fprintf(stdout, "**************************************************************\n\n");    
  fflush(stdout);
#endif
   }
  /* DONE :: WCET computation of the MSC */
}
