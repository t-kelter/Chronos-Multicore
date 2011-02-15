#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parseCFG.h"
#include "block.h"
#include "handler.h"
#include "infeasible.h"

/*
 * Initializes a basic block. Memory allocation is done in caller.
 */
int createBlock( block *bb, int pid, int bbid, int startaddr, int tb, int nb, int callpid, int loopid ) {

  bb->bbid         = bbid;
  bb->pid          = pid;
  bb->startaddr    = startaddr;
  bb->outgoing     = NULL;
  bb->num_outgoing = 0;
  bb->num_outgoing_copy = 0;
  	
  bb->callpid      = callpid;
  bb->size         = 0;
//  bb->wcost         = 0;
//  bb->bcost 	= 0;
  //bb->reset = 0;

  //bb->regid        = -1;
  bb->loopid       = loopid;
  bb->is_loophead  = 0;
  bb->instrlist    = NULL;
  bb->num_instr    = 0;
  bb->num_cache_state = 0;
  bb->num_cache_state_L2 = 0;
  bb->num_chmc = 0;
  bb->num_chmc_L2 = 0;  
  bb->bb_cache_state = NULL;
  bb->bb_cache_state_L2= NULL;

  //  bb->hit = 0;
//  bb->miss = 0;
//  bb->unknow = 0;
  bb->proc_ptr = NULL;

  // outgoing edges
  if( tb != -1 ) {
    bb->num_outgoing++;
    bb->num_outgoing_copy++;
	
    REALLOC( bb->outgoing, int*, bb->num_outgoing * sizeof(int), "bb outedge" );
    bb->outgoing[ bb->num_outgoing - 1 ] = tb;
  }
  if( nb != -1 ) {
    bb->num_outgoing++;
    bb->num_outgoing_copy++;
	
    REALLOC( bb->outgoing, int*, bb->num_outgoing * sizeof(int), "bb outedge" );
    bb->outgoing[ bb->num_outgoing - 1 ] = nb;
  }

  return 0;
}


/*
 * Initializes a loop. Memory allocation is done in caller.
 */
int createLoop( loop *lp, int pid, int lpid, int level, block *loophead ) {

  lp->lpid         = lpid;
  lp->pid          = pid;
  lp->loopbound    = 0;
  lp->loophead     = loophead;
  lp->loopsink     = NULL;
  lp->loopexit     = NULL;
  lp->num_exits    = 0;
  lp->exits        = NULL;
  lp->level        = level;
  lp->nest         = -1;
  lp->is_dowhile   = 0;
  MALLOC( lp->topo, block**, sizeof(block*), "loop topo" );
  lp->topo[0]      = loophead;
  lp->num_topo     = 1;
  lp->wcet         = 0;
  lp->wpath        = NULL;
  lp->num_cost = 0;

  return 0;
}


/*
 * Initializes a procedure. Memory allocation is done in caller.
 */
int createProc( procedure *p, int pid ) {

  p->pid          = pid;
  p->bblist       = NULL;
  p->num_bb       = 0;
  p->loops        = NULL;
  p->num_loops    = 0;
  p->calls        = NULL;
  p->num_calls    = 0;
  p->topo         = NULL;
  p->num_topo     = 0;
  p->wcet         = 0;
  p->wpath        = NULL;
  p->num_cost = 0;


  return 0;
}


/*
 * Initializes an instruction. Memory allocation is done in caller.
 */
int createInstr( instr *i, int addr, char *op, char *r1, char *r2, char *r3 ) {

  sprintf( i->addr, "%08x", addr );
  strcpy( i->op, op );
  strcpy( i->r1, r1 );
  strcpy( i->r2, r2 );
  strcpy( i->r3, r3 );

  return 0;
}


/*
 * Reads [filename].arg and extract info on:
 * - last instruction address (for counting instructions purpose)
 * - start address of main procedure (for identifying main procedure id)
 *
 * File format is:
 *   <start_addr> <end_addr> <start_main> <cache_parameters>...
 */
int read_arg( int *lastaddr, int *mainaddr ) {

  FILE *fptr;
  int scan;

  fptr = openfile( "arg", "r" );

  fscanf( fptr, "%x", &scan );     // discard start address info
  fscanf( fptr, "%x", lastaddr );
  fscanf( fptr, "%x", mainaddr );

  // ignore the rest
  fclose( fptr );

  return 0;
}


/*
 * Given a basic block bb (knowing its start address), 
 * calculates the number of bytes in the basic block preceeding it.
 * The result is assigned to the size of the preceeding bb, and returned.
 */
int countInstrPrev( block *bb ) {

  procedure *p;
  block *prev;

  if( !bb )
    printf( "Null basic block in countInstrPrev\n" ), exit(1);

  if( bb->bbid == 0 ) { // first bb in the procedure

    if( bb->pid == 0 ) // no previous block
      return 0;

    // previous bb is in previous procedure
    p = procs[ bb->pid - 1 ];
    prev = p->bblist[ p->num_bb - 1 ];
  }
  else {
    // previous bb is in same procedure
    p = procs[ bb->pid ];
    prev = p->bblist[ p->num_bb - 2 ];
  }
  prev->size = bb->startaddr - prev->startaddr;

  return prev->size;
}


/*
 * Calculates the number of instructions in the last basic block in the last procedure.
 * The result is assigned to the size of the last bb, and returned.
 */
int countInstrLast( int lastaddr ) {

  procedure *p;
  block *bb;

  p = procs[ num_procs - 1];
  if( !p )
    printf( "Null last procedure\n" ), exit(1);

  bb = p->bblist[ p->num_bb - 1 ];
  if( !bb )
    printf( "Null last basic block\n" ), exit(1);

  // Address of last instruction is given in lastaddr.
  // This last instruction is inclusive in last bb.

  bb->size = lastaddr - bb->startaddr + INSN_SIZE;

  return bb->size;
}


/*
 * Returns the id of the main procedure, -1 if none found.
 */
int findMainProc( int mainaddr ) {

  procedure *p;
  int i;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];

    if( !p || !p->num_bb )
      return -1;

    // start address of main procedure is given in mainaddr
    if( mainaddr == p->bblist[0]->startaddr )
      return i;
  }
  return -1;
}


/*
 * Determines a reverse topological ordering of the procedure call graph.
 * Result is stored in global array proc_cg.
 */
int topo_call() {

  int i;
  procedure *p;

  procedure **comefrom;  // keep the proc from which we came, to return to
  int  comefromsize;
  char *markfin;         // markfin: 0: unvisited, 1: visited but unfinished, 2: finished
  char fin;

  int countdone;

  comefrom     = NULL;
  comefromsize = 0;
  CALLOC( markfin, char*, num_procs, sizeof(char), "topo fin array" );
  REALLOC( proc_cg, int*, num_procs * sizeof(int), "proc topo" );

  countdone = 0;

  // start from main procedure
  p = procs[ main_id ];

  while( countdone < num_procs ) {

    fin = 1;
    for( i = 0; fin && i < p->num_calls; i++ )
      if( markfin[ p->calls[i] ] != 2 )
	fin = 0;

    if( !p->num_calls || fin ) {  // finished
      markfin[ p->pid ] = 2;
      proc_cg[ countdone++ ] = p->pid;

      // returning
      if( !comefromsize )
	break;

      p = comefrom[ --comefromsize ];
      continue;
    }  // end if

    markfin[ p->pid ] = 1;
    REALLOC( comefrom, procedure**, (comefromsize + 1) * sizeof(procedure*), "topo comefrom array" );
    comefrom[ comefromsize++ ] = p;

    // go to next unvisited procedure call
    fin = 1;
    for( i = 0; fin && i < p->num_calls; i++ ) {
      if( markfin[ p->calls[i] ] != 2 ) {
	p = procs[ p->calls[i] ];
	fin = 0;
      } // end if
    } // end for
    if( fin )
      printf( "Unvisited called procedure not detected.\n" ), exit(1);

  } // end while( countdone < num_bb )

  free( comefrom );
  free( markfin );

  return 0;
}


/* 
 * Parses CFG.
 * Input : [filename].cfg
 * Format: <proc_id> <bb_id> <bb_startaddr> <taken_branch> <nontaken_branch> <called_proc>
 */
int read_cfg() {

  FILE *fptr;

  int pid, bbid, bbsa, tb, nb, cpid;
  procedure *p;
  block *bb;

  int lastaddr;
  int mainaddr;

  fptr = openfile( "cfg", "r" );
  int scan_result;

  while( scan_result = fscanf( fptr, "%d %d %x %d %d %d", &pid, &bbid, &bbsa, &tb, &nb, &cpid ),
         scan_result != EOF ) {

    if ( scan_result != 6 ) {
      prerr( "CFG file had invalid format!" );
    }

    //pid start at 0; if a new proc
    if( pid >= num_procs ) {

      // start a new procedure
      MALLOC( p, procedure*, sizeof(procedure), "procedure" );
		/* FIXME: Always reset memory to zero after allocating a procedure */
		memset(p, 0, sizeof(procedure));
      createProc( p, pid );

      num_procs++;
      REALLOC( procs, procedure**, num_procs * sizeof(procedure*), "procedure list" );
      procs[ num_procs - 1 ] = p;
    }

    p = procs[pid];
    if( !p )
      printf( "Null procedure at %d\n", pid ), exit(1);

    // create a new basic block within proc [pid]
    MALLOC( bb, block*, sizeof(block), "basic block" );
	 memset(bb, 0, sizeof(block));
    createBlock( bb, pid, bbid, bbsa, tb, nb, cpid, -1 );

    p->num_bb++;
    REALLOC( p->bblist, block**, p->num_bb * sizeof(block*), "bblist" );
    p->bblist[ p->num_bb - 1 ] = bb;

    // called procedures
    if( cpid != -1 ) {
      p->num_calls++;
      REALLOC( p->calls, int*, p->num_calls * sizeof(int), "calls" );
      p->calls[ p->num_calls - 1 ] = cpid;
    }

    // Now that we know the start address of this bb, 
    // we can determine the size (i.e. #instructions) of the previous bb.
    countInstrPrev( bb );

    total_bb++;
  }
  fclose( fptr );

  read_arg( &lastaddr, &mainaddr );

  // determine size of last bb
  countInstrLast( lastaddr );

  main_id = findMainProc( mainaddr );
  if( main_id == -1 )
    printf( "Main procedure not found.\n" ), exit(1);

  // construct reverse topological order of procedure call graph
  topo_call();
  

  
  return 0;
}


/*
 * Reads in assembly instructions and stores them in the corresponding basic block.
 * For purpose of conflict detection for infeasible path detection.
 * At the point of call, cfg should have been parsed.
 *
 * Input : [filename].md
 * Format: <insn_addr> <op> <r1> <r2> <r3>
 */
int readInstr() {

  FILE *fptr;

  block *bb;
  instr *insn;

  char line[ INSN_LEN ];
  char op[9], r1[9], r2[9], r3[9];
  int  addr;

  fptr = openfile( "md", "r" );

  while( fgets( line, INSN_LEN, fptr )) {

    op[0] = '\0';
    r1[0] = '\0';
    r2[0] = '\0';
    r3[0] = '\0';

    sscanf( line, "%x %s %s %s %s\n", &addr, op, r1, r2, r3 );

    bb = findBlock( addr );
    if( !bb ) {
      // could be that addr is from a procedure that is never called (thus not in CFG)
      fprintf( stderr, "Warning: Ignored out-of-range address [%x] %s %s %s %s\n", addr, op, r1, r2, r3 );
      continue;
    }

    MALLOC( insn, instr*, sizeof(instr), "instruction" );	 
    createInstr( insn, addr, op, r1, r2, r3 );

    bb->num_instr++;
    REALLOC( bb->instrlist, instr**, bb->num_instr * sizeof(instr*), "instruction list" );
    bb->instrlist[ bb->num_instr - 1 ] = insn;
  }
  fclose( fptr );

  return 0;
}

void
calculate_incoming()
{
	int i, j, k;
	 block *src_bb, *dst_bb;
	 procedure *p;

	 for(k = num_procs -1; k >= 0; k--)
	  {
	  	//compute incoming for procedures
		 p = procs[k];
		//initialize number of incoming
		 for( i = 0; i < p->num_bb; i++ )
		 {
		 	p->bblist[i]->num_incoming= 0;
			CALLOC(p->bblist[i]->incoming, int*, 1, sizeof(int), "incoming");
		 }

		 for( i = 0; i < p->num_bb; i++ )
		 {
		 	src_bb = p->bblist[i];
		 	for( j = 0; j < src_bb->num_outgoing; j++ )
		 	{	
		 		dst_bb = p->bblist[src_bb->outgoing[j]];
				
				dst_bb->num_incoming++;
				REALLOC( dst_bb->incoming, int*, dst_bb->num_incoming * sizeof(int), "bb inedge" );
	   			 dst_bb->incoming[ dst_bb->num_incoming - 1 ] = src_bb->bbid;
			} // end for (j)
		} // end for(i)
	 }
	 //exit(1);
	/*
	 	 for(k = 0; k < num_procs; k++)
		{
			 p = procs[k];
			 for( i = 0; i < p->num_bb; i++ )
		 	{
		 		int x;
		 		printf( "bb[%d][%d] (%d):", k, i, p->bblist[i]->num_incoming );
				for( x= 0; x < p->bblist[i]->num_incoming; x++ )
					printf(" %d", p->bblist[i]->incoming[x] );
				printf("\n");
			 }
	 	 }
		exit(1);
		*/
}
