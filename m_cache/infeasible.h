/*! This is a header file of the Chronos timing analyzer. */

/*
 * Supporting functions for infeasible path detection.
 *
 * When DEF_INFEASIBILITY_GLOBALS is defined, then this file defines all the
 * global variables used during infeasibility analysis, else it declares them as
 * external.
 */

#ifndef __CHRONOS_INFEASIBLE_H
#define __CHRONOS_INFEASIBLE_H

// ######### Macros #########


#define REG_RETURN regName[2]
// the register where return value of a function call is stored (by observation)

#define DERI_LEN 800   // length of register derivation tree
#define INSN_LEN 50    // length of assembly line
#define NO_REG   68

#define JUMP 1
#define FALL 2

#define BT 1      // bigger
#define BE 2      // bigger or equal
#define ST 3      // smaller
#define SE 4      // smaller or equal
#define EQ 5      // equal
#define NE 6      // not equal
#define NA 0      // no decision made

// instr types in deri_tree: to determine branch jump direction

#define KO  -2    // a situation not supported
#define NIL -1
#define SLTI 1
#define SLT  2


// ######### Datatype declarations  ###########


typedef struct {
  char mem_addr[DERI_LEN];     // a sequence of operations on mem. accesses
  int  value;                  // a constant; 0 if mem. access (not resolved)
  char valid;                  // 1 if value is a valid constant (not unresolved mem. access)
  char instr;                  // any special kind of instruction; -1 if not specified 	
} deri_tree;                   // derivation of the value of a register

typedef struct brch {
  char   deri_tree[DERI_LEN];  // derivation tree of tested variable
  int    rhs;                  // the rhs constant
  char   rhs_var;              // 1 if rhs is a variable
  char   jump_cond;            // condition that makes a branch instruction jump
  block  *bb;                  // associated block
  int    num_active_incfs;     // #unvisited blocks (assign/branch) with incoming conflict
  int    num_incfs;            // #blocks (assign/branch) with incoming conflict
  char   *in_conflict;         // in_conflict[i] = 1 if block i is incoming conflict, i: 0...proc->num_bb
  int    num_conflicts;
  struct brch **conflicts;     // conflicting branches (outgoing)
  char   *conflictdir_jump;    // directions of branches conflicting with this branch's jump direction 
  char   *conflictdir_fall;    // directions of branches conflicting with this branch's fall direction 
} branch;

typedef struct {
  char   deri_tree[DERI_LEN];  // derivation tree of affected variable
  int    rhs;                  // the rhs constant
  char   rhs_var;              // 1 if rhs is a variable
  block  *bb;                  // associated block
  int    lineno;               // line number in bb
  int    num_conflicts;
  branch **conflicts;          // conflicting branches (outgoing)
  char   *conflictdir;         // directions of the conflicting branches
} assign;


typedef struct {
  ull    cost;
  int    bb_len;
  int    *bb_seq;              // block id-s
  int    branch_len;
  branch **branch_eff;         // branch effects, NULL if the corresponding branch has no effect
  char   *branch_dir;          // direction taken by each branch in this path
} path;


// ######### Function declarations  ###########


int initRegSet();

int clearReg();

int findReg( char key[] );

int neg( int a );

/*
 * Testing conflict between "X a rhs_a" and "X b rhs_b".
 */
char testConflict( int a, int rhs_a, int b, int rhs_b );

/*
 * Testing conflict between A and B.
 * r1, r2 are relational operators associated with A, B respectively.
 * Returns 1 if conflict, 0 otherwise.
 */
char isBBConflict( branch *A, branch *B, int r1, int r2 );

char isBAConflict( assign *A, branch *B, int r );

/*
 * Initializes infeasible path detection.
 */
int initInfeas();


// ######### Global variables declarations / definitions ###########


#ifdef DEF_INFEASIBILITY_GLOBALS
#define EXTERN 
#else
#define EXTERN extern
#endif

EXTERN char regName[NO_REG][OP_LEN];
EXTERN deri_tree reg2Mem[NO_REG];     // reg2Mem[i]: current memory address of register i

EXTERN int    **num_assign;           // num_assign[i][j]: #assign effects in proc i block j
EXTERN assign ****assignlist;         // assignlist[i][j]: list of assign effects (ptr) in proc i block j
EXTERN branch ***branchlist;          // branchlist[i][j]: branch effect (ptr) associated with proc i block j

EXTERN int num_BA;                    // #potential BA conflict pairs
EXTERN int num_BB;                    // #potential BB conflict pairs

EXTERN int  *num_paths;
EXTERN path ***pathlist;              // pathlist[i]: list of potential wcet paths collected at block i

EXTERN int  max_paths; 
EXTERN char *pathFreed;


#endif
