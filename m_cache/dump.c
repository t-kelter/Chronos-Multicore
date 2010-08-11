#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "dump.h"
#include "handler.h"


int printBlock( const block * const bb ) {

  int i;

  printf( "%3d %08x (%3d)",// (%3d) [R%2d]",
	  bb->bbid, bb->startaddr, bb->size);//, bb->cost, bb->regid );

  for( i = 0; i < bb->num_outgoing; i++ )
    printf( " %3d", bb->outgoing[i] );

  for( i = bb->num_outgoing; i < 2; i++ )
    printf( "    " );

  if( bb->callpid != -1 )
    printf( "  P%d", bb->callpid );
  else
    printf( "    " );

  printf("	bb->proc_ptr: %u\n", (unsigned int)(uintptr_t)bb->proc_ptr);

  if( bb->loopid != -1 ) {
    printf( "  L%d", bb->loopid );
    if( bb->is_loophead )
      printf( "*" );
  }

  printf( "\n" );

  return 0;
}


int printProc( const procedure * const p ) {

  int i;

  printf( "Proc: %d\n", p->pid );
  for( i = 0; i < p->num_bb; i++ ) {
    const block * const bb = p->bblist[i];
    if( !bb )
      prerr( "Null basic block at %d\n", i );
    printBlock( bb );
  }
  if( p->num_calls ) {
    printf( "Calls procedures:" );
    for( i = 0; i < p->num_calls; i++ )
      printf( " %d", p->calls[i] );
    printf( "\n" );
  }

  if( p->num_topo ) {
    printf( "Topo order: " );
    for( i = 0; i < p->num_topo; i++ )
      printf( " %d", p->topo[i]->bbid );
    printf( "\n" );
  }
  printf( "\n" );

  return 0;
}


int print_cfg()
{
  int i;

  int max_num_bb, max_proc;

  max_num_bb = 0;
  max_proc = -1;

  for( i = 0; i < num_procs; i++ ) {
    const procedure * const p = procs[i];
    if( !p )
      printf( "Null procedure at %d\n", i ), exit(1);
    printProc( p );
    if( p->num_bb > max_num_bb ) {
      max_num_bb = p->num_bb;
      max_proc = i;
    }
  }
  printf( "\n%d procedures (main: %d), %d basic blocks in total (max %d in proc %d)\n", 
	  num_procs, main_id, total_bb, max_num_bb, max_proc );

  if( proc_cg != NULL ) {
    printf( "Reverse-topo ordered call graph:" );
    for( i = 0; i < num_procs; i++ ) {
      const procedure * const p = procs[ proc_cg[i] ];
      if( !p )
	printf( "Null procedure at topo order %d\n", i ), exit(1);
      printf( " %d", p->pid );
    }
    printf( "\n" );
  }
  printf( "\n--------------------------------\n" );

  return 0;
}


int printLoop( const loop * const lp ) {

  int i;

  printf( "Loop %d-%d (%d, %d) [%d, %d, %d]", lp->pid, lp->lpid, 
	  lp->level, lp->nest, lp->loophead->bbid, lp->loopsink->bbid, 
	  ( lp->loopexit ? lp->loopexit->bbid : -1 ));

  if( lp->is_dowhile )
    printf( " dw" );
  else
    printf( " nm" );
  printf( " %d\n", lp->loopbound );

  // print loop components
  for( i = 0; i < lp->num_topo; i++ )
    printBlock( lp->topo[i] );

  printf( "\n" );

  return 0;
}


int print_loops()
{
  int i, j;

  for( i = 0; i < num_procs; i++ ) {
    printf( "\nproc %d:\n\n", i );
    const procedure * const p = procs[i];
    if( !p )
      printf( "Null procedure at %d\n", i ), exit(1);

    for( j = 0; j < p->num_loops; j++ ) {
      const loop * const lp = p->loops[j];
      if( !lp )
	printf( "Null loop at %d proc %d\n", j, i ), exit(1);
      printLoop( lp );
    }
  }
  printf( "\n--------------------------------\n" );

  return 0;
}


int print_topo()
{
  int i, j, k;
  for( i = 0; i < num_procs; i++ ) {
    const procedure * const p = procs[i];

    // procedure blocks
    printf( "Proc %d:", p->pid );
    for( j = 0; j < p->num_topo; j++ )
      printf( " %d", p->topo[j]->bbid );
    printf( "\n" );

    // loops
    for( j = 0; j < p->num_loops; j++ ) {
      const loop * const lp = p->loops[j];
      printf( "- Loop %d-%d:", lp->pid, lp->lpid );
      for( k = 0; k < lp->num_topo; k++ )
	printf( " %d", lp->topo[k]->bbid );
      printf( "\n" );
    }
  }
  printf( "\n--------------------------------\n" );

  return 0;
}


int printWCETLoop( const loop * const lp ) {

  printf( "- Loop %d-%d: %Lu (%s)\n", lp->pid, lp->lpid, lp->wcet[0], lp->wpath );
  return 0;
}


int printWCETProc( const procedure * const p ) {

  printf( "Proc %d: %Lu (%s)\n", p->pid, p->wcet[0], p->wpath );
  return 0;
}


int printInstr( FILE *fptr, const instr * const insn ) {

  fprintf( fptr, "%s %s %s %s %s\n", insn->addr, insn->op, insn->r1, insn->r2, insn->r3 );
  return 0;
}

int printBlockInstr( FILE *fptr, const block * const bb ) {

  int i;
  instr *insn;

  for( i = 0; i < bb->num_instr; i++ ) {
    insn = bb->instrlist[i];
    fprintf( fptr, "%d %d %d ", bb->pid, bb->bbid, i );
    printInstr( fptr, insn );
  }
  return 0;
}


int print_instrlist() {

  int i, j;
  FILE *fptr = openfile( "blks", "w" );

  // printf( "Instruction list:\n\n" );

  for( i = 0; i < num_procs; i++ ) {
    const procedure * const p = procs[i];
    for( j = 0; j < p->num_bb; j++ ) {
      // printBlockInstr( stdout, p->bblist[j] );
      printBlockInstr( fptr, p->bblist[j] );      
    }
    // printf( "\n" );
  }
  // printf( "\n--------------------------------\n" );

  fclose( fptr );
  return 0;
}


int printPath( const path * const pt ) {

  int i;

  printf( "(%Lu)", pt->cost );
  for( i = 0; i < pt->bb_len; i++ )
    printf( " %d", pt->bb_seq[i] );
  printf( " [" );
  for( i = 0; i < pt->branch_len; i++ )
    printf( " %d(%d)", pt->branch_eff[i]->bb->bbid, pt->branch_dir[i] );
  printf( "]\n" );

  return 0;
}


int printBranch( const branch * const br, const char printcf ) {

  int i;

  if( br == NULL )
    return -1;

  printf( "B <%d,%3d> ", br->bb->pid, br->bb->bbid );

  printf( "rhs:" );
  if( br->rhs_var )
    printf( "var " );
  else
    printf( "%3d ", br->rhs );

  printf( "jump:%d deri:%s\n", br->jump_cond, br->deri_tree );

  if( !printcf )
    return 0;

  if( br->num_conflicts ) {
    printf( "- outgoing conflicts:" );
    for( i = 0; i < br->num_conflicts; i++ ) {

      if( br->conflictdir_jump[i] != -1 )
	printf( " %d(j%d)", br->conflicts[i]->bb->bbid, br->conflictdir_jump[i] );
      //if( BBdir_jump[br->bb->pid][br->bb->bbid][i] != -1 )
      //		BBdir_jump[br->bb->pid][br->bb->bbid][i] );

      if( br->conflictdir_fall[i] != -1 )
	printf( " %d(f%d)", br->conflicts[i]->bb->bbid, br->conflictdir_fall[i] );
      //if( BBdir_fall[br->bb->pid][br->bb->bbid][i] != -1 )
      //		BBdir_fall[br->bb->pid][br->bb->bbid][i] );
    }
    printf( "\n" );
  }
  if( br->num_incfs ) {
    printf( "- incoming conflicts:" );
    for( i = 0; i < procs[br->bb->pid]->num_bb; i++ )
      if( br->in_conflict[i] )
	printf( " %d", i );
    printf( "\n" );
  }
  return 0;
}

int printAssign( const assign * const assg, const int id, const char printcf ) {

  int i;

  printf( "A <%d,%3d,%3d> ", assg->bb->pid, assg->bb->bbid, assg->lineno );

  printf( "rhs:" );
  if( assg->rhs_var )
    printf( "var " );
  else
    printf( "%3d ", assg->rhs );

  printf( "deri:%s\n", assg->deri_tree );

  if( !printcf )
    return 0;

  if( assg->num_conflicts ) {
    printf( "- outgoing conflicts:" );
    for( i = 0; i < assg->num_conflicts; i++ )
      printf( " %d(%d)", assg->conflicts[i]->bb->bbid, assg->conflictdir[i] );
	      //	      BAdir[assg->bb->pid][assg->bb->bbid][id][i] );
    printf( "\n" );
  }

  return 0;
}


int print_effects() {

  int i, j, k;

  for( i = 0; i < num_procs; i++ ) {
    for( j = 0; j < procs[i]->num_bb; j++ ) {

      for( k = 0; k < num_assign[i][j]; k++ )
	printAssign( assignlist[i][j][k], k, 1 );

      printBranch( branchlist[i][j], 1 );
    }
  }
  printf( "\n--------------------------------\n" );

  return 0;
}


int dump_loops()
{
  int i, j, k;
  FILE *f = openfile( "loops", "w" );

  for( i = 0; i < num_procs; i++ ) {
    for( j = 0; j < procs[i]->num_loops; j++ ) {
      const loop * const lp = procs[i]->loops[j];
      fprintf( f, "Loop %d %d  %d %d %d %d  %d\n", lp->pid, lp->lpid,
	       lp->level, lp->nest, lp->loophead->bbid, lp->loopbound, lp->num_topo );

      for( k = 0; k < lp->num_topo; k++ )
	fprintf( f, " %d", lp->topo[k]->bbid );
      fprintf( f, "\n" );
    }
  }
  fclose( f );

  return 0;
}


int dump_callgraph()
{
  int i;

  if( proc_cg == NULL )
    return 1;

  FILE *f = openfile( "cg", "w" );

  for( i = 0; i < num_procs; i++ )
    fprintf( f, "%d ", procs[proc_cg[i]]->pid );
  fprintf( f, "\n" );

  fclose( f );

  return 0;
}

void dumpCacheState( const cache_state * const cs )
{
	int i, j, k, n;
	//cache_state *copy = NULL;

	//int lp_level = cs->loop_level;
	
	printf("\nCache State\n");	
	//printf("\nloop level = %d\n", lp_level);

	/*
  int copies;
	if( lp_level == -1)
		copies = 1;
	else
		copies = (2<<lp_level);

	printf("\nTotal copies = %d\n", copies);
	*/

	//printf("\nThis is copies NO %d\n", copies);
	printf("\nMUST\n");
	
	for(i = 0; i < cache.na; i++)
		printf("	way %d		", i);
	printf("\n");
	
	for(j = 0; j < cache.ns; j++)
	{
		printf("set %d	", j);

		for( k = 0; k < cache.na; k++)
		{
			if(cs->must[j][k]->num_entry!=0)
			{
				//printf("\nNO of way = %d \n", k);

				//printf("\nMust \n");

				for(n = 0; n < cs->must[j][k]->num_entry; n++)
					printf(" %d ", cs->must[j][k]->entry[n]);
				printf(";	");
			}
			else
				printf("		");
		}  //end for(i)
		printf("\n");		
	} //end for(j)


	printf("\nMAY\n");
	
	for(i = 0; i < cache.na; i++)
		printf("	way %d		", i);
	printf("\n");
	
	for(j = 0; j < cache.ns; j++)
	{
		printf("set %d	", j);

		for( k = 0; k < cache.na; k++)
		{
			if(cs->may[j][k]->num_entry!=0)
			{
				//printf("\nNO of way = %d \n", k);

				//printf("\nMust \n");

				for(n = 0; n < cs->may[j][k]->num_entry; n++)
					printf(" %d ", cs->may[j][k]->entry[n]);
				printf(";	");


				//printf("\nMay \n");

			}
			else
				printf("		");
		}  //end for(i)
		printf("\n");		
	} //end for(j)

}


void dumpCacheState_L2( const cache_state * const cs )
{
	int i, j, k, n;
	//cache_state *copy = NULL;

	//int lp_level = cs->loop_level;
	
	printf("\nCache State_L2\n");	
	//printf("\nloop level = %d\n", lp_level);

	/*
  int copies;
	if( lp_level == -1)
		copies = 1;
	else
		copies = (2<<lp_level);

	printf("\nTotal copies = %d\n", copies);
	*/

	//printf("\nThis is copies NO %d\n", copies);
	printf("\nMUST\n");
	
	for(i = 0; i < cache_L2.na; i++)
		printf("	way %d		", i);
	printf("\n");
	
	for(j = 0; j < cache_L2.ns; j++)
	{
		printf("set %d	", j);

		for( k = 0; k < cache_L2.na; k++)
		{
			if(cs->must[j][k]->num_entry!=0)
			{
				//printf("\nNO of way = %d \n", k);

				//printf("\nMust \n");

				for(n = 0; n < cs->must[j][k]->num_entry; n++)
					printf(" %d ", cs->must[j][k]->entry[n]);
				printf(";	");
			}
			else
				printf("		");
		}  //end for(i)
		printf("\n");		
	} //end for(j)


	printf("\nMAY\n");
	
	for(i = 0; i < cache_L2.na; i++)
		printf("	way %d		", i);
	printf("\n");
	
	for(j = 0; j < cache_L2.ns; j++)
	{
		printf("set %d	", j);

		for( k = 0; k < cache_L2.na; k++)
		{
			if(cs->may[j][k]->num_entry!=0)
			{
				//printf("\nNO of way = %d \n", k);

				//printf("\nMust \n");

				for(n = 0; n < cs->may[j][k]->num_entry; n++)
					printf(" %d ", cs->may[j][k]->entry[n]);
				printf(";	");


				//printf("\nMay \n");

			}
			else
				printf("		");
		}  //end for(i)
		printf("\n");		
	} //end for(j)

}


void dump_prog_info( const procedure * const proc)
{
  int i,j,k;
  FILE* fp = stdout;

  fprintf(fp, "Printing incoming block information\n");
  for(i = 0; i < proc->num_bb; i++) {
    assert(proc->bblist[i]);
    const block * const bb = proc->bblist[i];
    fprintf(fp, "Basic block id = (%d.%d.0x%x.start=%Lu.finish=%Lu)\n", proc->pid,
        bb->bbid, (unsigned int)(uintptr_t)bb, bb->start_time, bb->finish_time);
    fprintf(fp, "Incoming blocks (Total = %d)======> \n", bb->num_incoming);
    for(j = 0; j < bb->num_incoming; j++) {
      fprintf(fp, "(bb=%d.%x)\n", bb->incoming[j],
          (unsigned int)(uintptr_t)proc->bblist[bb->incoming[j]]);
    }
  }
  fprintf(fp, "Printing Topological information ====>\n");
  for(i = proc->num_topo - 1; i >= 0; i--) {
    const block * const bb = proc->topo[i];
    fprintf(fp, "(proc=%d.bb=%d)\n", bb->pid, bb->bbid);
  }
  fprintf(fp, "Printing Loop information ====>\n");
  for(i = 0; i < proc->num_loops; i++) {
    const loop * const lp = proc->loops[i];
    assert(lp->loophead);
    fprintf(fp, "(proc=%d.loop=%d.loophead=%d.loopsink=%d.loopexit=%d)\n",
        proc->pid, lp->lpid, lp->loophead->bbid, lp->loopsink->bbid,
        lp->loopexit ? lp->loopexit->bbid : -1);
    fprintf(fp, "Loop Nodes ====>\n");
    for(j = lp->num_topo - 1; j >= 0; j--) {
      assert(lp->topo[j]);
      fprintf(fp, "(bb=%d)", lp->topo[j]->bbid);
    }
    fprintf(fp,"\n");
  }

  fprintf(fp, "Printing instruction call relationship\n");
  for(i = 0; i < proc->num_bb; i++) {
    const block * const bb = proc->bblist[i];

    for(j = 0; j < bb->num_instr; j++) {
      const instr * const inst = bb->instrlist[j];
      if(IS_CALL(inst->op)) {
        fprintf(fp, "PROCEDURE CALL ENCOUNTERED at %s\n", GET_CALLEE(inst));
        /* Ignore library calls */
        if(!proc->calls)
          continue;
        for(k = 0; k < proc->num_calls; k++) {
          uint callee_id = proc->calls[k];
          const procedure * const callee = procs[callee_id];
          assert(callee);
          fprintf(fp, "Got start address = %x\n",
              callee->topo[callee->num_topo - 1]->startaddr);
        }
      }
    }
  }
}

/* This is for debugging. Dumped chmc info after preprocessing */
void dump_pre_proc_chmc( const procedure * const proc, const enum AccessScenario scenario )
{
  fprintf( stdout, "%s-case CHMC info: \n\n", 
      scenario == ACCESS_SCENARIO_BCET ? "Best" : "Worst" );

  int i;
  for(i = 0; i < proc->num_bb; i++) {
    const block * const bb = proc->bblist[i];

    int j;
    for(j = 0; j < bb->num_instr; j++) {
      const instr * const inst = bb->instrlist[j];
      fprintf( stdout, "Instruction address = %s ==>\n", inst->addr );

      int k;
      for(k = 0; k < bb->num_chmc; k++) {
        const acc_type acc = check_hit_miss( bb, inst, k, scenario );
        if( acc == L1_HIT )
          fprintf( stdout, "L1_HIT\n" );
        else if( acc == L2_HIT )
          fprintf( stdout, "L2_HIT\n");
        else 
          fprintf( stdout, "L2_MISS\n");
      }
    }
  }
}
