#include "pathTest.h"

int hasConflict(int effectNo){
  int k;
  for(k = 0; k < EFFECT_LEN; k ++){
    if(strlen(effectLst[k].deri_tree) < 2 || k == effectNo) continue;
    if(strcmp(effectLst[k].deri_tree,effectLst[effectNo].deri_tree) == 0){
      if(isConflict(effectLst[k],effectLst[effectNo]) == -1 ){
	return 1;
      }
    }
  }
  return 0;	
}

void cancelEffect(int effectNo){
  int k;
  for(k = 0; k < EFFECT_LEN; k ++){
    if(strlen(effectLst[k].deri_tree) < 2 || k == effectNo) continue;
    if(strcmp(effectLst[k].deri_tree,effectLst[effectNo].deri_tree) == 0){
      effectLst[k].deri_tree[0] = '\0';
    }
  }
}


int isFeasible(int p[],int length){
  int i, j ,k, m; int power; int direction; int regPos1,regPos2,regPos3; int in_test; int newEffect = 1;
  char tmp[MEM_LEN];	
  char offset[MEM_LEN];
  clearEffectLst();
  
  //printf("Initialization over...\n");
  effectNo = 0;
  /*for(m = 0; m < length; m++){
    printf("%d-",p[m]);
    }
    printf("\n"); */
       
  for(m = 0; m < length; m++){
    i = p[m];     
    //printf("%d->",p[m]);fflush(stdout);
    
    for(j = 0; j < NO_LINES; j++){
      tmp[0] = '\0';
      if(strlen(instrLst[i][j].line_no) < 2) break;
      //printf("\t<%s:%s>: ", instrLst[i][j].line_no,instrLst[i][j].op);fflush(stdout);
      
      // First we test the previous effects on the current instruction 
      if(strcmp(instrLst[i][j].op,"beq") == 0 ||
	 strcmp(instrLst[i][j].op,"bne") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	if((reg2MemLst[regPos1].instr == SLTI || reg2MemLst[regPos1].instr == SLT) &&
	   strlen(reg2MemLst[regPos2].mem_addr) == 0 && reg2MemLst[regPos2].value == 0	){
	  effectNo ++;
	  effectLst[effectNo].rhs = reg2MemLst[regPos1].value;
	  if(strcmp(instrLst[i][j].op,"beq") == 0){
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r3) == 0)
	      effectLst[effectNo].result = BE;
	    else
	      effectLst[effectNo].result = ST;						  	
	  }
	  else{
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r3) == 0)
	      effectLst[effectNo].result = ST;						 
	    else
	      effectLst[effectNo].result = BE;
	  }
	  
	  strcpy(effectLst[effectNo].deri_tree,reg2MemLst[regPos1].mem_addr);
	  effectLst[effectNo].type = BRANCH; 
	  effectLst[effectNo].blkNo = i; effectLst[effectNo].lnNo = j;
	  if(hasConflict(effectNo)) return 0;
	}
	else if(reg2MemLst[regPos1].instr == NIL && reg2MemLst[regPos2].instr == NIL){
	  
	  if((strlen(reg2MemLst[regPos1].mem_addr) > 0 && strlen(reg2MemLst[regPos2].mem_addr) > 0) ||
	     (strlen(reg2MemLst[regPos1].mem_addr) == 0 && strlen(reg2MemLst[regPos2].mem_addr)== 0))
	    continue;
	  
	  effectNo ++;
	  if(strlen(reg2MemLst[regPos1].mem_addr) > 0 && strlen(reg2MemLst[regPos2].mem_addr) == 0){
	    in_test = regPos1;
	    effectLst[effectNo].rhs = reg2MemLst[regPos2].value - reg2MemLst[regPos1].value;
	  }
	  else if(strlen(reg2MemLst[regPos1].mem_addr) == 0 && strlen(reg2MemLst[regPos2].mem_addr) > 0){
	    in_test = regPos2;
	    effectLst[effectNo].rhs = reg2MemLst[regPos1].value - reg2MemLst[regPos2].value;
	  }
	  if(strcmp(instrLst[i][j].op,"bne") == 0){
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r3) == 0)
	      effectLst[effectNo].result = NE;
	    else
	      effectLst[effectNo].result = EQ;
	  }
	  else{
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r3) == 0)
	      effectLst[effectNo].result = EQ;
	    else
	      effectLst[effectNo].result = NE;
	  }
	  strcpy(effectLst[effectNo].deri_tree,reg2MemLst[in_test].mem_addr);
	  effectLst[effectNo].type = BRANCH; effectLst[effectNo].blkNo = i; effectLst[effectNo].lnNo = j;
	  if(hasConflict(effectNo)) return 0;
	}			
      }  		
      else if(strcmp(instrLst[i][j].op,"bgez") == 0 || 
	      strcmp(instrLst[i][j].op,"blez") == 0 ||
	      strcmp(instrLst[i][j].op,"bgtz") == 0	){
	regPos1 = findReg(instrLst[i][j].r1);
	if(strlen(reg2MemLst[regPos1].mem_addr) > 0){
	  effectNo ++;
	  effectLst[effectNo].rhs = 0 - reg2MemLst[regPos1].value;
	  strcpy(effectLst[effectNo].deri_tree,reg2MemLst[regPos1].mem_addr);
	  effectLst[effectNo].type = BRANCH; 
	  effectLst[effectNo].blkNo = i; effectLst[effectNo].lnNo = j;
	  if(strcmp(instrLst[i][j].op,"bgez") == 0){
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r2) == 0)
	      effectLst[effectNo].result = BE;
	    else
	      effectLst[effectNo].result = ST;							
	  }
	  else if(strcmp(instrLst[i][j].op,"bgtz") == 0){
	    //printf("%s : %s\n", instrLst[p[m+1]][0].line_no,instrLst[i][j].r2); fflush(stdout);
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r2) == 0)
	      effectLst[effectNo].result = BT;
	    else
	      effectLst[effectNo].result = SE;							
	  }
	  else{
	    if(strcmp(instrLst[p[m+1]][0].line_no,instrLst[i][j].r2) == 0)
	      effectLst[effectNo].result = SE;
	    else
	      effectLst[effectNo].result = BT;
	    
	  }
	  if(hasConflict(effectNo)) return 0;
	}
	
      } 
      
      
      // If the update occurs on s8[30] or gp[28] skip the instruction
      if(strcmp(instrLst[i][j].r1,"$s8[30]") == 0 ||
	 strcmp(instrLst[i][j].r1,"$gp[28]") == 0)  continue;
      
      //printf("\t\tNot Updating Stack pointer: %s \n", instrLst[p[i]][j].op);  
      
      // If there is no problem with transition from current instruction to the next,
      //   execute the current instruction 
      if(strcmp(instrLst[i][j].op,"lw") == 0  || 
	 strcmp(instrLst[i][j].op,"lb") == 0  ||
	 strcmp(instrLst[i][j].op,"lbu") == 0 ||
	 strcmp(instrLst[i][j].op,"l.d") == 0 
	 ){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos3 = findReg(instrLst[i][j].r3);
	sprintf(offset,"%d",reg2MemLst[regPos3].value+atoi(instrLst[i][j].r2));
	strcat(tmp,offset); 
	if(strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,"plus");
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);
	}				
	reg2MemLst[regPos1].mem_addr[0] = '\0';
	reg2MemLst[regPos1].value = 0; 	
	reg2MemLst[regPos1].instr = NIL; 	
	strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	//printf("\t\t\t%s: %s\n",instrLst[p[i]][j].r1, reg2MemLst[regPos]);
      }
      else if(strcmp(instrLst[i][j].op,"sw") == 0 || 
	      strcmp(instrLst[i][j].op,"sb") == 0){
	regPos3 = findReg(instrLst[i][j].r3);
	sprintf(offset,"%d",reg2MemLst[regPos3].value+atoi(instrLst[i][j].r2));
	strcat(tmp,offset); 
	if(strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,"plus");
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);
	}				
	effectNo ++;
	strcpy(effectLst[effectNo].deri_tree,tmp);
	effectLst[effectNo].type = ASSIGN; effectLst[effectNo].blkNo = i; effectLst[effectNo].lnNo = j;
	
	regPos1 = findReg(instrLst[i][j].r1);
	if(strlen(reg2MemLst[regPos1].mem_addr) == 0){
	  effectLst[effectNo].rhs = reg2MemLst[regPos1].value;
	  effectLst[effectNo].rhs_var = 0;
	}
	else{
	  effectLst[effectNo].rhs_var = 1;
	}
	cancelEffect(effectNo);
	
      }
      else if(strcmp(instrLst[i][j].op,"lui") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	reg2MemLst[regPos1].value = atoi(instrLst[i][j].r2) * 65536; 	
	reg2MemLst[regPos1].instr = NIL;
	//printf("\t\t\t%s: %s\n",instrLst[p[i]][j].r1, reg2MemLst[regPos]);
      }
      else if(strcmp(instrLst[i][j].op,"addiu") == 0 || 
	      strcmp(instrLst[i][j].op,"addi") == 0 ){
	
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	reg2MemLst[regPos1].value =  reg2MemLst[regPos2].value + atoi(instrLst[i][j].r3);
	reg2MemLst[regPos1].instr = NIL; 
	//printf("\t\t\t%s: %s\n",instrLst[i][j].r1, reg2MemLst[regPos]);
      }
      else if(strcmp(instrLst[i][j].op,"ori") == 0 ){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	reg2MemLst[regPos1].value =  reg2MemLst[regPos2].value | atoi(instrLst[i][j].r3);
	reg2MemLst[regPos1].instr = NIL; 
	//printf("\t\t\t%s: %s\n",instrLst[i][j].r1, reg2MemLst[regPos]);
      }
      else if( strcmp(instrLst[i][j].op,"sll") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	if(strlen(reg2MemLst[regPos2].mem_addr) > 0){    //r2 is an varaible
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);
	  strcat(tmp,"sll"); strcat(tmp,instrLst[i][j].r3);	
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else    					//r2 is an constant
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	
	reg2MemLst[regPos1].value =  reg2MemLst[regPos2].value;
	sscanf(instrLst[i][j].r3,"%x",&power);
	for(k = 0; k < power ; k++){ reg2MemLst[regPos1].value *= 2; }
	reg2MemLst[regPos1].instr = NIL; 
      }
      else if( strcmp(instrLst[i][j].op,"srl") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	if(strlen(reg2MemLst[regPos2].mem_addr) > 0){    //r2 is an varaible
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);
	  strcat(tmp,"srl"); strcat(tmp,instrLst[i][j].r3);	
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else    					//r2 is an constant
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	
	reg2MemLst[regPos1].value =  reg2MemLst[regPos2].value;
	sscanf(instrLst[i][j].r3,"%x",&power);
	for(k = 0; k < power ; k++){ reg2MemLst[regPos1].value /= 2; }
	reg2MemLst[regPos1].instr = NIL; 
      }
      else if(strcmp(instrLst[i][j].op,"slti") == 0 || 
	      strcmp(instrLst[i][j].op,"sltiu") == 0){
	//        deri_tree[r1].mem_addr = deri_tree[r2].mem_addr
	//        deri_tree[r1].value    = i - deri_tree[r2].value ;
	//	   deri_tree[r1].instr    = SLTI;		
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	strcat(tmp,reg2MemLst[regPos2].mem_addr);
	reg2MemLst[regPos1].mem_addr[0] = '\0';
	strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	reg2MemLst[regPos1].value = atoi(instrLst[i][j].r3) - reg2MemLst[regPos2].value;
	reg2MemLst[regPos1].instr = SLTI;
      }
      else if(strcmp(instrLst[i][j].op,"slt") == 0 ||
	      strcmp(instrLst[i][j].op,"sltu") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	regPos3 = findReg(instrLst[i][j].r3);
	if(strlen(reg2MemLst[regPos3].mem_addr) == 0){			//r3 is a const
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);
	  reg2MemLst[regPos1].mem_addr[0] = '\0';
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	  reg2MemLst[regPos1].value = reg2MemLst[regPos3].value - reg2MemLst[regPos2].value;
	  reg2MemLst[regPos1].instr = SLT;
	}
	else{
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);
	  strcat(tmp,"slt");
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);
	  reg2MemLst[regPos1].mem_addr[0] = '\0';
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	  reg2MemLst[regPos1].value = reg2MemLst[regPos3].value - reg2MemLst[regPos2].value;
	  reg2MemLst[regPos1].instr = KO;
	}
      }
      else if(strcmp(instrLst[i][j].op,"addu") == 0){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	regPos3 = findReg(instrLst[i][j].r3);
	//printf("%s:%d | %s:%d\n",reg2MemLst[regPos2].mem_addr, reg2MemLst[regPos2].value,
	//			reg2MemLst[regPos3].mem_addr, reg2MemLst[regPos3].value);
	//fflush(stdout); 
	if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  reg2MemLst[regPos1].mem_addr[0] = '\0';
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  strcat(tmp,"plus");				
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	reg2MemLst[regPos1].value = reg2MemLst[regPos2].value + reg2MemLst[regPos3].value;
	reg2MemLst[regPos1].instr = NIL;
	
      } 
      else if(strcmp(instrLst[i][j].op,"subu") == 0 ){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	regPos3 = findReg(instrLst[i][j].r3);
	if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  reg2MemLst[regPos1].mem_addr[0] = '\0';
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  strcat(tmp,"sub");				
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	reg2MemLst[regPos1].value = reg2MemLst[regPos2].value - reg2MemLst[regPos3].value;
	reg2MemLst[regPos1].instr = NIL;
	//printf("\t\t\t%s: %s\n",instrLst[p[i]][j].r1, reg2MemLst[regPos]);
	
      }
      else if(strcmp(instrLst[i][j].op,"or") == 0 ){
	regPos1 = findReg(instrLst[i][j].r1);
	regPos2 = findReg(instrLst[i][j].r2);
	regPos3 = findReg(instrLst[i][j].r3);
	if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  reg2MemLst[regPos1].mem_addr[0] = '\0';
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) == 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) == 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	else if(strlen(reg2MemLst[regPos2].mem_addr) > 0 && strlen(reg2MemLst[regPos3].mem_addr) > 0){
	  strcat(tmp,reg2MemLst[regPos2].mem_addr);				
	  strcat(tmp,"or");				
	  strcat(tmp,reg2MemLst[regPos3].mem_addr);				
	  reg2MemLst[regPos1].mem_addr[0] = '\0'; 	
	  strcpy(reg2MemLst[regPos1].mem_addr,tmp);
	}
	reg2MemLst[regPos1].value = reg2MemLst[regPos2].value | reg2MemLst[regPos3].value;
	reg2MemLst[regPos1].instr = NIL;
	//printf("\t\t\t%s: %s\n",instrLst[p[i]][j].r1, reg2MemLst[regPos]);
      }
      //printf("\t\tUpdate effect over\n");  fflush(stdout);
      
      
    }	
  }	
  return 1;	
}

