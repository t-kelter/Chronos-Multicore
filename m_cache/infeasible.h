#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


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


char regName[NO_REG][OP_LEN];
deri_tree reg2Mem[NO_REG];     // reg2Mem[i]: current memory address of register i

int    **num_assign;           // num_assign[i][j]: #assign effects in proc i block j
assign ****assignlist;         // assignlist[i][j]: list of assign effects (ptr) in proc i block j
branch ***branchlist;          // branchlist[i][j]: branch effect (ptr) associated with proc i block j

int num_BA;                    // #potential BA conflict pairs
int num_BB;                    // #potential BB conflict pairs

int  *num_paths;
path ***pathlist;              // pathlist[i]: list of potential wcet paths collected at block i

int  max_paths; 
char *pathFreed;

