// Include standard library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "infeasible.h"
#include "findConflicts.h"


int initRegSet() {

  strcpy( regName[0] , "$0"  ); strcpy( regName[1] , "$1"  ); 
  strcpy( regName[2] , "$2"  ); strcpy( regName[3] , "$3"  );	
  strcpy( regName[4] , "$4"  ); strcpy( regName[5] , "$5"  ); 
  strcpy( regName[6] , "$6"  ); strcpy( regName[7] , "$7"  );	
  strcpy( regName[8] , "$8"  ); strcpy( regName[9] , "$9"  ); 
  strcpy( regName[10], "$10" ); strcpy( regName[11], "$11" );	
  strcpy( regName[12], "$12" ); strcpy( regName[13], "$13" ); 
  strcpy( regName[14], "$14" ); strcpy( regName[15], "$15" );	
  strcpy( regName[16], "$16" ); strcpy( regName[17], "$17" ); 
  strcpy( regName[18], "$18" ); strcpy( regName[19], "$19" );	
  strcpy( regName[20], "$20" ); strcpy( regName[21], "$21" ); 
  strcpy( regName[22], "$22" ); strcpy( regName[23], "$23" );	
  strcpy( regName[24], "$24" ); strcpy( regName[25], "$25" ); 
  strcpy( regName[26], "$26" ); strcpy( regName[27], "$27" );	
  strcpy( regName[28], "$28" ); strcpy( regName[29], "$29" ); 
  strcpy( regName[30], "$30" ); strcpy( regName[31], "$31" );
  
  strcpy( regName[34], "$f0"  ); strcpy( regName[35], "$f1"  ); 
  strcpy( regName[36], "$f2"  ); strcpy( regName[37], "$f3"  );
  strcpy( regName[38], "$f4"  ); strcpy( regName[39], "$f5"  ); 
  strcpy( regName[40], "$f6"  ); strcpy( regName[41], "$f7"  );
  strcpy( regName[42], "$f8"  ); strcpy( regName[43], "$f9"  ); 
  strcpy( regName[44], "$f10" ); strcpy( regName[45], "$f11" );
  strcpy( regName[46], "$f12" ); strcpy( regName[47], "$f13" ); 
  strcpy( regName[48], "$f14" ); strcpy( regName[49], "$f15" );
  strcpy( regName[50], "$f16" ); strcpy( regName[51], "$f17" ); 
  strcpy( regName[52], "$f18" ); strcpy( regName[53], "$f19" );
  strcpy( regName[54], "$f20" ); strcpy( regName[55], "$f21" ); 
  strcpy( regName[56], "$f22" ); strcpy( regName[57], "$f23" );
  strcpy( regName[58], "$f24" ); strcpy( regName[59], "$f25" ); 
  strcpy( regName[60], "$f26" ); strcpy( regName[61], "$f27" );
  strcpy( regName[62], "$f28" ); strcpy( regName[63], "$f29" ); 
  strcpy( regName[64], "$f30" ); strcpy( regName[65], "$f31" );
  
  strcpy( regName[66], "$fcc" );	
  
  reg2Mem[0].mem_addr[0] = '\0';
  reg2Mem[0].value = 0;
  reg2Mem[0].valid = 1;
  reg2Mem[0].instr = NIL;
  
  strcpy( reg2Mem[28].mem_addr, "$28" );
  reg2Mem[28].value = 0;
  reg2Mem[28].valid = 0;
  reg2Mem[28].instr = NIL;
  
  strcpy( reg2Mem[30].mem_addr, "$30" );
  reg2Mem[30].value = 0;
  reg2Mem[30].valid = 0;
  reg2Mem[30].instr = NIL;

  return 0;
}


int clearReg() {

  int i;
  for( i = 1; i < NO_REG; i++ ) {
   
    if( i == 28 || i == 30 )
      continue;

    reg2Mem[i].mem_addr[0] = '\0';
    reg2Mem[i].value = 0;
    reg2Mem[i].valid = 0;
    reg2Mem[i].instr = NIL;
  }
  return 0;
}


int findReg( char key[] ) {

  int i;
  for( i = 0; i < NO_REG; i++ ) { 
    if( strcmp( key, regName[i] ) == 0 )
      return i; 
  }

  return -1;
}


int neg( int a ) {

  if( a == BT ) return SE;
  if( a == BE ) return ST;
  if( a == SE ) return BT;
  if( a == ST ) return BE;
  if( a == EQ ) return NE;
  if( a == NE ) return EQ;
  return NA;
}


/*
 * Testing conflict between "X a rhs_a" and "X b rhs_b".
 */
char testConflict( int a, int rhs_a, int b, int rhs_b ) {

  if( a == BT ) { 
    if( b == ST || b == SE || b == EQ ) {
      if( rhs_a < rhs_b ) return 0;
      else return 1;
    }
    else return 0;
  } 
  if( a == ST ) { 
    if( b == BT || b == BE || b == EQ ) {
      if( rhs_a > rhs_b ) return 0;
      return 1;
    }
    else return 0;
  }            
  if( a == BE ) { 
    if( b == ST ) {
      if( rhs_a < rhs_b ) return 0;
      else return 1;
    }
    else if( b == SE || b == EQ ) {
      if( rhs_a <= rhs_b ) return 0;
      else return 1;
    }
    else return 0; 
  }
  if( a == SE ) {
    if( b == BT ) {
      if( rhs_a > rhs_b ) return 0;
      else return 1;
    }
    else if( b == BE || b == EQ ) {
      if( rhs_a >= rhs_b ) return 0;
      else return 1;
    }
    else return 0;
  } 
  if( a == EQ ) { 
    if( b == NE ) {
      if( rhs_a != rhs_b ) return 0;
      else return 1;
    }
    if( b == EQ ) {
      if( rhs_a == rhs_b ) return 0;
      else return 1;
    }
    else if( b == ST ) {
      if( rhs_a < rhs_b ) return 0;
      else return 1;		
    }
    else if( b == BT ) {
      if( rhs_a > rhs_b ) return 0;
      else return 1;		
    }
    else if( b == SE ) {
      if( rhs_a <= rhs_b ) return 0;
      else return 1;		
    }
    else if( b == BE ) {
      if( rhs_a >= rhs_b ) return 0;
      else return 1;		
    }
  }	
  if( a == NE ) {
    if( b == EQ && ( rhs_a == rhs_b ))
      return 1; 
    else return 0;
  } 
  return 0;	
}


/*
 * Testing conflict between A and B.
 * r1, r2 are relational operators associated with A, B respectively.
 * Returns 1 if conflict, 0 otherwise.
 */
char isBBConflict( branch *A, branch *B, int r1, int r2 ) {  

  return testConflict( r1, A->rhs, r2, B->rhs );
}


char isBAConflict( assign *A, branch *B, int r ) {  

  if( A->rhs_var ) return 0;
  return testConflict( EQ, A->rhs, r, B->rhs ); 	
}


/*
 * Initializes infeasible path detection.
 */
int initInfeas()
{
  DSTART( "initInfeas" );

  int i, j;
  procedure *p;

  DOUT( "\nInitializing register set...\n" ); fflush(stdout);
  initRegSet();

  DOUT( "Allocating data structures...\n" ); fflush( stdout );
  MALLOC( num_assign, int**, num_procs * sizeof(int*), "num_assign" );
  MALLOC( assignlist, assign****, num_procs * sizeof(assign***), "assignlist" );
  MALLOC( branchlist, branch***, num_procs * sizeof(branch**),  "branchlist" );

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];

    CALLOC( num_assign[i], int*, p->num_bb, sizeof(int), "num_assign elm" );
    MALLOC( assignlist[i], assign***, p->num_bb * sizeof(assign**), "assignlist elm" );
    MALLOC( branchlist[i], branch**, p->num_bb * sizeof(branch*),  "branchlist elm" );

    for( j = 0; j < p->num_bb; j++ ) {
      assignlist[i][j] = NULL;
      branchlist[i][j] = NULL;
    }
  }

  DOUT( "Detecting effects...\n" ); fflush( stdout );
  execute();

  DOUT( "\nFinding related effects...\n" ); fflush( stdout );
  detectConflicts();

  DOUT( "Detected %d BB and %d BA\n", num_BB, num_BA );
  DRETURN( 0 );
}
