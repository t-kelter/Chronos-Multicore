/*
 * Returns 1 if deri_tree is modified inside lp, 0 otherwise.
 * Potentially recursive.
 */
char modifiedInLoop( loop *lp, char *deri_tree ) {

  int i, j;
  block *bb;

  //char nested;

  loop **nested;
  int num_nested;

  nested = NULL;
  num_nested = 0;

  for( i = 0; i < lp->num_topo; i++ ) {
    bb = lp->topo[i];
    for( j = 0; j < num_assign[lp->pid][bb->bbid]; j++ ) {
      if( strstr( deri_tree, assignlist[lp->pid][bb->bbid][j]->deri_tree ) != NULL ) {
	free( nested );
	return 1;
      }
    }
    // remember nested loops
    if( i < lp->num_topo-1 && bb->is_loophead ) {
      num_nested++;
      nested = (loop**) REALLOC( nested, num_nested * sizeof(loop*), "nested" );
      nested[num_nested-1] = procs[bb->pid]->loops[bb->loopid];
    }
  }
  // check nested loop: may take long so do only after all other blocks done
  for( i = 0; i < num_nested; i++ ) {
    if( modifiedInLoop( nested[i], deri_tree )) {
      free( nested );
      return 1;
    }
  }

  free( nested );
  return 0;
}


/*
 * Recursive function.
 * Returns 1 if there is a path from bblist[srcid] to bblist[destid] in the CFG without
 * passing through a nested loophead nor an assignment to a component of deri_tree
 * (other than assg, which is the one being tested, if any).
 * Returns 0 otherwise.
 */
char reachableNoCancel( int pid, int srcid, int destid, char *deri_tree,
			assign *assg, block **bblist, int num_bb, char *visited ) {
  int i, id;
  block *bb;

  if( srcid == destid )
    return 1;

  // check cancellation of effect
  bb = bblist[srcid];

  // by intermediary assignment
  if( assg == NULL || bb->bbid != assg->bb->bbid ) {
    for( i = 0; i < num_assign[pid][bb->bbid]; i++ )
      if( strstr( deri_tree, assignlist[pid][bb->bbid][i]->deri_tree ) != NULL )
	return 0;
  }

  // by nested loop
  if( srcid != num_bb - 1 && bb->is_loophead
      && modifiedInLoop( procs[pid]->loops[bb->loopid], deri_tree ))
    return 0;

  visited[bb->bbid] = 1;

  for( i = 0; i < bb->num_outgoing; i++ ) {
    if( !visited[ bb->outgoing[i] ] ) {
      id = getblock( bb->outgoing[i], bblist, 0, srcid-1 );
      if( id != -1 && reachableNoCancel( pid, id, destid, deri_tree, assg, bblist, num_bb, visited ))
	return 1;
    }
  }
  return 0;
}


/*
 * Returns 1 if there is a path from bblist[srcid] to bblist[destid] in the CFG without
 * passing through a nested loophead nor an assignment to a component of deri_tree
 * (other than assg, which is the one being tested, if any).
 * Returns 0 otherwise.
 */
char isReachableNoCancel( int pid, int srcid, int destid, char *deri_tree,
			  assign *assg, block **bblist, int num_bb ) {

  char *visited;  // bit array to mark visited nodes
  int  res;

  if( srcid == destid )
    return 1;

  visited = (char*) CALLOC( visited, procs[pid]->num_bb, sizeof(char), "visited" );
  res = reachableNoCancel( pid, srcid, destid, deri_tree, assg, bblist, num_bb, visited );

  free( visited );
  return res;
}


int addAssign( char deri_tree[], block *bb, int lineno, int rhs, char rhs_var ) {

  int i;
  assign *assg;

  // check if there is previous assignment to the same memory address in the same block
  for( i = 0; i < num_assign[bb->pid][bb->bbid]; i++ ) {
    assg = assignlist[bb->pid][bb->bbid][i];

    if( strcmp( assg->deri_tree, deri_tree ) == 0 ) {
      // overwrite this assignment
      assg->lineno  = lineno;
      assg->rhs     = rhs;
      assg->rhs_var = rhs_var;
      return 1;
    }
  }

  // else add new
  assg = (assign*) MALLOC( assg, sizeof(assign), "assign" );
  strcpy( assg->deri_tree, deri_tree );
  assg->rhs           = rhs;
  assg->rhs_var       = rhs_var;
  assg->bb            = bb;
  assg->lineno        = lineno;
  assg->num_conflicts = 0;
  assg->conflicts     = NULL;
  assg->conflictdir   = NULL;

  num_assign[bb->pid][bb->bbid]++;

  i = num_assign[bb->pid][bb->bbid];
  assignlist[bb->pid][bb->bbid] = (assign**)
    REALLOC( assignlist[bb->pid][bb->bbid], i * sizeof(assign*), "assignlist[p][b]" );
  assignlist[bb->pid][bb->bbid][i-1] = assg;

  return 0;
}

int addBranch( char deri_tree[], block *bb, int rhs, char rhs_var, char jump_cond ) {

  branch *br;

  br = (branch*) MALLOC( br, sizeof(branch), "branch" );
  strcpy( br->deri_tree, deri_tree );
  br->rhs              = rhs;
  br->rhs_var          = rhs_var;
  br->jump_cond        = jump_cond;
  br->bb               = bb;
  br->num_conflicts    = 0;
  br->conflicts        = NULL;
  br->conflictdir_jump = NULL;
  br->conflictdir_fall = NULL;
  br->num_incfs        = 0;
  br->num_active_incfs = 0;
  br->in_conflict = (char*)
    CALLOC( br->in_conflict, procs[bb->pid]->num_bb, sizeof(char), "branch in_conflict" );

  branchlist[bb->pid][bb->bbid] = br;

  return 0;
}


int setReg2Mem( int pos, char mem_addr[], int value, int instr ) {

  reg2Mem[pos].mem_addr[0] = '\0';  // reset
  strcpy( reg2Mem[pos].mem_addr, mem_addr );
  reg2Mem[pos].value = value;
  reg2Mem[pos].valid = 1;
  reg2Mem[pos].instr = instr;

  return 0;
}


/*
 * Go through the entire list of instructions and collect effects (assignments, branches).
 * Calculate and store register values (in the form of deri_tree) for each effect.
 */
int execute() {

  int i, j, k, m;
  int power;

  int regPos1, regPos2, regPos3;
  int regval;
  int in_test;

  char tmp[ DERI_LEN ];
  char offset[ DERI_LEN ];

  char jal, ignore;  // procedure call cancels conflict

  procedure *p;
  block *bb;
  instr *insn;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];
    jal = 0;

    // clear registers (parameter passing not handled)
    clearReg();

    for( j = 0; j < p->num_bb; j++ ) {
      bb = p->bblist[j];
      
      for( k = 0; k < bb->num_instr; k++ ) {
	insn = bb->instrlist[k];
	// printf( "%d-%d-%d\n", i, j, k );
	// printInstr( insn ); fflush( stdout );

	ignore = jal;
	jal = 0;

	tmp[0] = '\0';
	if( strlen( insn->addr ) < 2 ) break;

	// First we test the previous effects on the current instruction.
	if( strcmp( insn->op, "beq" ) == 0 || strcmp( insn->op, "bne" ) == 0 ) {

	  if( ignore )
	    continue;

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  if( ( reg2Mem[regPos1].instr == SLTI || reg2Mem[regPos1].instr == SLT ) &&
	      strlen( reg2Mem[regPos2].mem_addr ) == 0 && reg2Mem[regPos2].value == 0 ) {

	    // r1's value is the result of slt/slti operation (0 or 1)
	    // compared against zero (r2)
	    
	    if( strcmp( insn->op, "beq" ) == 0 )
	      // jump if r1 == 0, i.e. slt is false
	      addBranch( reg2Mem[regPos1].mem_addr, bb, reg2Mem[regPos1].value, 0, BE );
	    else
	      // jump if r1 != 0, i.e. slt is true
	      addBranch( reg2Mem[regPos1].mem_addr, bb, reg2Mem[regPos1].value, 0, ST );
	  }
	  else if( reg2Mem[regPos1].instr == NIL && reg2Mem[regPos2].instr == NIL ) {
	    
	    // r1 and r2 are constants or results of some calculation

	    // skip if both are mem. accesses or both are constants
	    // ( we only handle conflicts of type var-rel-const )
	    if( ( strlen( reg2Mem[regPos1].mem_addr ) > 0 && strlen( reg2Mem[regPos2].mem_addr ) > 0 ) ||
		( strlen( reg2Mem[regPos1].mem_addr ) == 0 && strlen( reg2Mem[regPos2].mem_addr ) == 0 ))
	      continue;
	    
	    if( strlen( reg2Mem[regPos1].mem_addr ) > 0 && strlen( reg2Mem[regPos2].mem_addr ) == 0 ) {
	      // r1 expr, r2 const
	      in_test = regPos1;
	      regval = reg2Mem[regPos2].value - reg2Mem[regPos1].value;
	    }
	    else if( strlen( reg2Mem[regPos1].mem_addr ) == 0 && strlen( reg2Mem[regPos2].mem_addr ) > 0 ) {
		// r1 const, r2 expr
	      in_test = regPos2;
	      regval = reg2Mem[regPos1].value - reg2Mem[regPos2].value;
	    }

	    if( strcmp( insn->op, "bne" ) == 0 )
	      addBranch( reg2Mem[in_test].mem_addr, bb, regval, 0, NE );
	    else
	      addBranch( reg2Mem[in_test].mem_addr, bb, regval, 0, EQ );
	  }			
	}  		
	else if( strcmp( insn->op, "bgez" ) == 0 || strcmp( insn->op, "blez" ) == 0 ||
		 strcmp( insn->op, "bgtz" ) == 0 ) {

	  if( ignore )
	    continue;

	  // only one register used, its value compared against zero
	  regPos1 = findReg( insn->r1 );
	  // printf( "reg1(%d): %s\n", regPos1, reg2Mem[regPos1].mem_addr );

	  if( strlen( reg2Mem[regPos1].mem_addr ) > 0 ) {

	    if( strcmp( insn->op, "bgez" ) == 0 )
	      addBranch( reg2Mem[regPos1].mem_addr, bb, 0 - reg2Mem[regPos1].value, 0, BE );
	    else if( strcmp( insn->op, "bgtz" ) == 0 )
	      addBranch( reg2Mem[regPos1].mem_addr, bb, 0 - reg2Mem[regPos1].value, 0, BT );
	    else
	      addBranch( reg2Mem[regPos1].mem_addr, bb, 0 - reg2Mem[regPos1].value, 0, SE );
	  }
	}	

	// If the update occurs on ra[31], s8[30], sp[29], gp[28] skip the instruction 
	// (those are system processing)
	else if( strcmp( insn->r1, "$31" ) == 0 || strcmp( insn->r1, "$30" ) == 0 ||
		 strcmp( insn->r1, "$29" ) == 0 || strcmp( insn->r1, "$28" ) == 0 ||
		 strcmp( insn->r3, "$29" ) == 0 )
	  continue;

	// If there is no problem with transition from current instruction to the next, 
	// execute the current instruction.

	else if( strcmp( insn->op, "lw"  ) == 0 || strcmp( insn->op, "lb"  ) == 0 ||
		 strcmp( insn->op, "lbu" ) == 0 || strcmp( insn->op, "l.d" ) == 0 ||
		 strcmp( insn->op, "lh"  ) == 0 ) {

	  // load operation
	  // r1: destination register, r2: index from base, r3: base register

	  regPos1 = findReg( insn->r1 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos3, reg2Mem[regPos3].mem_addr );

	  // 'calculate' mem. address that is being loaded
	  sprintf( offset, "%d", reg2Mem[regPos3].value + atoi( insn->r2 ));
	  strcat( tmp, offset );
	  if( strlen( reg2Mem[regPos3].mem_addr ) > 0 ) {
	    strcat( tmp, "pl" );
	    strcat( tmp, reg2Mem[regPos3].mem_addr );
	  }
	  setReg2Mem( regPos1, tmp, 0, NIL );
	}
	else if( strcmp( insn->op, "sw" ) == 0 || strcmp( insn->op, "sb"  ) == 0 ||
		 strcmp( insn->op, "sh" ) == 0 || strcmp( insn->op, "sbu" ) == 0 ) {

	  // store operation ( assignment )
	  // r1: source register, r2: index from base, r3: base register

	  regPos1 = findReg( insn->r1 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos3, reg2Mem[regPos3].mem_addr );

	  // the target memory address is deri_tree[r2].mem_addr+'+'+offset
	  sprintf( offset, "%d", reg2Mem[regPos3].value + atoi( insn->r2 ));
	  strcat( tmp, offset ); 
	  if( strlen( reg2Mem[regPos3].mem_addr ) > 0 ) {
	    strcat( tmp, "pl" );
	    strcat( tmp, reg2Mem[regPos3].mem_addr );
	  }
	  if( ignore && strcmp( insn->r1, REG_RETURN ) == 0 )
	    // is a return value from some function call
	    addAssign( tmp, bb, k, 0, 1 );

	  else if( strlen( reg2Mem[regPos1].mem_addr ) == 0 && reg2Mem[regPos1].valid )
	    // a constant
	    addAssign( tmp, bb, k, reg2Mem[regPos1].value, 0 );

	  else
	    addAssign( tmp, bb, k, 0, 1 );
	}
	else if( strcmp( insn->op, "lui" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  // printf( "reg1(%d): %s\n", regPos1, reg2Mem[regPos1].mem_addr );

	  setReg2Mem( regPos1, tmp, atoi( insn->r2 ) * 65536, NIL );
	}
	else if( strcmp( insn->op, "addiu" ) == 0 || strcmp( insn->op, "addi" ) == 0 ) {

	  // When calling the instruction "addiu r1, r2, i" 

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  strcat( tmp, reg2Mem[regPos2].mem_addr );
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value + atoi( insn->r3 ), NIL );
	}
	else if( strcmp( insn->op, "ori" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  strcat( tmp, reg2Mem[regPos2].mem_addr );				
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value | atoi( insn->r3 ), NIL );
	}
	else if( strcmp( insn->op, "andi" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  strcat( tmp, reg2Mem[regPos2].mem_addr );				
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value & atoi( insn->r3 ), NIL );
	}
	else if( strcmp( insn->op, "sll" ) == 0 ) {
	  // r1 = r2 * ( 2^r3 )

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );	  
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  if( strlen( reg2Mem[regPos2].mem_addr ) > 0 ) {
	    // r2 is a variable
	    strcat( tmp, reg2Mem[regPos2].mem_addr );
	    strcat( tmp, "sll" );
	    strcat( tmp, insn->r3 );
	  }
	  regval = reg2Mem[regPos2].value;
	  power = hexValue( insn->r3 );
	  for( m = 0; m < power ; m++ )
	    regval *= 2;
	  
	  setReg2Mem( regPos1, tmp, regval, NIL );
	}
	else if( strcmp( insn->op, "srl" ) == 0 ) {
	  // r1 = r2 / ( 2^r3 )

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  if( strlen( reg2Mem[regPos2].mem_addr ) > 0 ) {
	    // r2 is an variable
	    strcat( tmp, reg2Mem[regPos2].mem_addr );
	    strcat( tmp, "srl" );
	    strcat( tmp, insn->r3 );	
	  }
	  regval = reg2Mem[regPos2].value;
	  power = hexValue( insn->r3 );
	  for( m = 0; m < power ; m++ )
	    regval /= 2;

	  setReg2Mem( regPos1, tmp, regval, NIL );
	}
	else if( strcmp( insn->op, "slti" ) == 0 || strcmp( insn->op, "sltiu" ) == 0 ) {

	  // set r1 to 1 if r2 < r3 ( r3 constant )
	  // r2 <  r3 -> r1 is 1 -> value >  0
	  // r2 >= r3 -> r1 is 0 -> value <= 0

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  // printf( "reg1(%d): %s; reg2(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr );

	  strcat( tmp, reg2Mem[regPos2].mem_addr );
	  setReg2Mem( regPos1, tmp, atoi( insn->r3 ) - reg2Mem[regPos2].value, SLTI );
	}
	else if( strcmp( insn->op, "slt" ) == 0 || strcmp( insn->op, "sltu" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg2(%d):%s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr, regPos3, reg2Mem[regPos3].mem_addr );

	  if( strlen( reg2Mem[regPos3].mem_addr ) == 0 ) {
	    // r3 is a constant
	    strcat( tmp, reg2Mem[regPos2].mem_addr );
	    setReg2Mem( regPos1, tmp, reg2Mem[regPos3].value - reg2Mem[regPos2].value, SLT );
	  }
	  else {
	    strcat( tmp, reg2Mem[regPos2].mem_addr );
	    strcat( tmp, "slt" );
	    strcat( tmp, reg2Mem[regPos3].mem_addr );
	    setReg2Mem( regPos1, tmp, reg2Mem[regPos3].value - reg2Mem[regPos2].value, KO );
	  }
	}
	else if( strcmp( insn->op, "addu" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg2(%d):%s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr, regPos3, reg2Mem[regPos3].mem_addr );

	  if( strlen( reg2Mem[regPos2].mem_addr ) == 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 )
	    // r2 const, r3 expr
	    strcat( tmp, reg2Mem[regPos3].mem_addr );

	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) == 0 )
	    // r2 expr, r3 const
	    strcat( tmp, reg2Mem[regPos2].mem_addr );				

	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 ) {
	    // both expr
	    strcat( tmp, reg2Mem[regPos2].mem_addr );
	    strcat( tmp, "pl" );
	    strcat( tmp, reg2Mem[regPos3].mem_addr );				
	  }
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value + reg2Mem[regPos3].value, NIL );
	}
	else if( strcmp( insn->op, "subu" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg2(%d):%s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr, regPos3, reg2Mem[regPos3].mem_addr );

	  if( strlen( reg2Mem[regPos2].mem_addr ) == 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 )
	    // r2 const, r3 expr
	    strcat( tmp, reg2Mem[regPos3].mem_addr );				

	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) == 0 )
	    // r2 expr, r3 const
	    strcat( tmp, reg2Mem[regPos2].mem_addr );				
	  
	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 ) {
	    // both expr
	    strcat( tmp, reg2Mem[regPos2].mem_addr );				
	    strcat( tmp, "sub" );				
	    strcat( tmp, reg2Mem[regPos3].mem_addr );				
	  }
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value - reg2Mem[regPos3].value, NIL );
	}
	else if( strcmp( insn->op, "or" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );
	  regPos2 = findReg( insn->r2 );
	  regPos3 = findReg( insn->r3 );
	  // printf( "reg1(%d): %s; reg2(%d):%s; reg3(%d):%s\n", regPos1, reg2Mem[regPos1].mem_addr, 
	  // regPos2, reg2Mem[regPos2].mem_addr, regPos3, reg2Mem[regPos3].mem_addr );

	  if( strlen( reg2Mem[regPos2].mem_addr ) == 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 )
	    // r2 const, r3 expr
	    strcat( tmp, reg2Mem[regPos3].mem_addr );

	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) == 0 )
	    // r2 expr, r3 const
	    strcat( tmp, reg2Mem[regPos2].mem_addr );				
	  
	  else if( strlen( reg2Mem[regPos2].mem_addr ) > 0 && strlen( reg2Mem[regPos3].mem_addr ) > 0 ) {
	    // both expr
	    strcat( tmp, reg2Mem[regPos2].mem_addr );				
	    strcat( tmp, "or" );				
	    strcat( tmp, reg2Mem[regPos3].mem_addr );				
	  }
	  setReg2Mem( regPos1, tmp, reg2Mem[regPos2].value | reg2Mem[regPos3].value, NIL );
	}
	else if( strcmp( insn->op, "jal" ) == 0 )
	  jal = 1;

	else if( strcmp( insn->op, "j" ) == 0 );

	else if( debug == 2 )
	  printf( "Ignoring opcode: %s\n", insn->op );

	/*
	else if( strcmp( insn->op, "mtc1" ) == 0 || strcmp( insn->op, "cvt.d.w" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );			
	  regPos2 = findReg( insn->r2 );

	  strcat( tmp, insn->op );
	  strcat( tmp, reg2Mem[regPos1] );
	  reg2Mem[regPos2][0] = '\0';
	  strcpy( reg2Mem[regPos2], tmp );
	}
	else if( strcmp( insn->op, "c.eq.d" ) == 0 ) {

	  regPos1 = findReg( insn->r1 );			
	  regPos2 = findReg( insn->r2 );

	  strcat( tmp, reg2Mem[regPos1] );	
	  strcat( tmp, insn->op );
	  strcat( tmp, reg2Mem[regPos2] );	
	  reg2Mem[66][0] = '\0';
	  strcpy( reg2Mem[66], tmp );
	}
	*/

      } // end for instr

    } // end for bb

  } // end for procs

  return 1;	
}


/*
 * Records conflict between assg (at index id of assignlist) and br taking direction dir.
 */
int setBAConflict( assign *assg, branch *br, int id, char dir ) {

  int num, pid, bbid;

  num_BA++;
  pid  = assg->bb->pid;
  bbid = assg->bb->bbid;

  assg->num_conflicts++;
  num = assg->num_conflicts;

  assg->conflicts = (branch**)
    REALLOC( assg->conflicts, num * sizeof(branch*), "assignlist conflicts" );
  assg->conflicts[num-1] = br;

  assg->conflictdir = (char*)
    REALLOC( assg->conflictdir, num * sizeof(char), "assignlist conflictdir" );
  assg->conflictdir[num-1] = dir;

  // set incoming conflict
  if( !br->in_conflict[bbid] ) {
    br->in_conflict[bbid] = 1;
    br->num_incfs++;
    br->num_active_incfs++;
  }
  return 0;
}


/*
 * Records conflict between br1 taking direction dir1 and br2 taking direction dir2.
 */
int setBBConflict( branch *br1, branch *br2, char dir1, char dir2, char new ) {

  int num, pid, bbid;

  num_BB++;
  pid  = br1->bb->pid;
  bbid = br1->bb->bbid;

  if( new ) {
    // each branch added once (even if both edges have their own conflicts)
    br1->num_conflicts++;
    num = br1->num_conflicts;
    br1->conflicts = (branch**)
      REALLOC( br1->conflicts, num * sizeof(branch*), "branchlist conflicts" );
    br1->conflicts[num-1] = br2;

    br1->conflictdir_jump = (char*)
      REALLOC( br1->conflictdir_jump, num * sizeof(char), "branchlist conflictdir_jump" );
    br1->conflictdir_jump[num-1] = -1;

    br1->conflictdir_fall = (char*)
      REALLOC( br1->conflictdir_fall, num * sizeof(char), "branchlist conflictdir_fall" );
    br1->conflictdir_fall[num-1] = -1;
  }

  if( dir1 == JUMP )
    br1->conflictdir_jump[br1->num_conflicts-1] = dir2;
  else if( dir1 == FALL )
    br1->conflictdir_fall[br1->num_conflicts-1] = dir2;

  // set incoming conflict
  if( !br2->in_conflict[bbid] ) {
    br2->in_conflict[bbid] = 1;
    br2->num_incfs++;
    br2->num_active_incfs++;
  }
  return 0;
}


/*
 * Checks pairwise effects to identify conflicts.
 * Special handling of loopheads:
 * - when detecting for the loop, consider only the iteration edge (taken branch)
 * - when detecting for its nesting procedure/loop (treated as black box),
 *   consider only the exit edge (non-taken branch)
 * lpid is given to identify the loop being analyzed.
 */
int detectConflictTopo( procedure *p, block **bblist, int num_bb, int lpid ) {

  int  i, j, k, id;
  char self, newconflict;

  branch *bri, *brj;
  assign *assg;

  int jump, fall;
  block *bj;

  // test each branch
  for( i = 0; i < num_bb; i++ ) {
    id  = bblist[i]->bbid;
    bri = branchlist[p->pid][id];

    if( bri == NULL || p->bblist[id]->is_loophead )
      continue;

    // printBranch( bri, 0 );

    // check self-effect
    self = 0;
    for( k = 0; k < num_assign[p->pid][id]; k++ ) {
      assg = assignlist[p->pid][id][k];
      // printAssign( assg, k, 0 );

      // effect due to assignment modifying a variable in the expression
      if( strstr( bri->deri_tree, assg->deri_tree ) != NULL )
	self = 1;

      // possible conflict only if the whole expression is identical
      if( strcmp( bri->deri_tree, assg->deri_tree ) == 0 ) {

	if( isBAConflict( assg, bri, bri->jump_cond ))
	  setBAConflict( assg, bri, k, bri->jump_cond );

	else if( isBAConflict( assg, bri, neg( bri->jump_cond )))
	  setBAConflict( assg, bri, k, neg( bri->jump_cond ));

	// stop once a match is found (can have at most one match)
	// Note also that if this block is entered, self must have been set to 1.
	break;
      }
    }
    // in case of self-effect, no need to test for further conflicts
    if( self )
      continue;

    // check against every effect coming before it in the CFG
    for( j = i + 1; j < num_bb; j++ ) {
      bj  = bblist[j];
      brj = branchlist[p->pid][bj->bbid];

      // check BA conflict
      for( k = 0; k < num_assign[p->pid][bj->bbid]; k++ ) {
	assg = assignlist[p->pid][bj->bbid][k];
	// printAssign( assg, k, 0 );

	if( strcmp( assg->deri_tree, bri->deri_tree ) == 0 ) {

	  if( isReachableNoCancel( p->pid, j, i, bri->deri_tree, assg, bblist, num_bb )) {

	    if( isBAConflict( assg, bri, bri->jump_cond ))
	      setBAConflict( assg, bri, k, bri->jump_cond );

	    else if( isBAConflict( assg, bri, neg( bri->jump_cond )))
	      setBAConflict( assg, bri, k, neg( bri->jump_cond ));
	  }
	  // stop once a match is found (can have at most one match)
	  break;
	}
      }
      if( brj == NULL )
	continue;

      // printf( "- " ); printBranch( brj, 0 );

      // check BB conflict
      if( strcmp( brj->deri_tree, bri->deri_tree ) == 0 ) {

	jump = getblock( bj->outgoing[1], bblist, 0, j-1 );
	fall = getblock( bj->outgoing[0], bblist, 0, j-1 );

	// special treatment for loophead
	if( bj->is_loophead ) {
	  if( bj->loopid == lpid )
	    // own loophead: consider only iteration edge
	    fall = -1;
	  else
	    // loop treated as black box: consider only exit edge
	    jump = -1;
	}

	// test all combinations of edge directions
	newconflict = 1;

	if( jump != -1 && isReachableNoCancel( p->pid, jump, i, bri->deri_tree, NULL, bblist, num_bb )) {
	  if( isBBConflict( brj, bri, brj->jump_cond, bri->jump_cond )) {
	    setBBConflict( brj, bri, JUMP, bri->jump_cond, newconflict );
	    newconflict = 0;
	    //printf( "jump-jump conflict\n" );
	  }
	  else if( isBBConflict( brj, bri, brj->jump_cond, neg( bri->jump_cond ))) {
	    setBBConflict( brj, bri, JUMP, neg( bri->jump_cond ), newconflict );
	    newconflict = 0;
	    //printf( "jump-fall conflict\n" );
	  }
	}
	if( fall != -1 && isReachableNoCancel( p->pid, fall, i, bri->deri_tree, NULL, bblist, num_bb )) {
	  if( isBBConflict( brj, bri, neg( brj->jump_cond ), bri->jump_cond )) {
	    setBBConflict( brj, bri, FALL, bri->jump_cond, newconflict );
	    newconflict = 0;
	    //printf( "fall-jump conflict\n" );
	  }
	  else if( isBBConflict( brj, bri, neg( brj->jump_cond ), neg( bri->jump_cond ))) {
	    setBBConflict( brj, bri, FALL, neg( bri->jump_cond ), newconflict );
	    newconflict = 0;
	    //printf( "fall-fall conflict\n" );
	  }
	}
      }
    } // end for each incoming block
  } // end for each branch

  return 0;
}


/*
 * Checks pairwise effects to identify conflicts.
 */
int detectConflicts() {

  int i, j;
  procedure *p;
  loop *lp;

  num_BA = 0;
  num_BB = 0;

  for( i = 0; i < num_procs; i++ ) {
    p = procs[i];

    // test for p's own topo
    detectConflictTopo( p, p->topo, p->num_topo, -1 );

    // test for each loop in p
    for( j = 0; j < p->num_loops; j++ ) {
      lp = p->loops[j];
      detectConflictTopo( p, lp->topo, lp->num_topo, lp->lpid );
    }
  }
  return 0;
}
