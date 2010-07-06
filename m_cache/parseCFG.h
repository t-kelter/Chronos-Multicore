/*! This is a header file of the Chronos timing analyzer. */

/*
 * CFG processing functions.
 */

#ifndef __CHRONOS_PARSE_CFG_H
#define __CHRONOS_PARSE_CFG_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


static void  calculate_incoming();

/*
 * Initializes a basic block. Memory allocation is done in caller.
 */
int createBlock( block *bb, int pid, int bbid, int startaddr, int tb, int nb,
    int callpid, int loopid );

/*
 * Initializes a loop. Memory allocation is done in caller.
 */
int createLoop( loop *lp, int pid, int lpid, int level, block *loophead );

/*
 * Initializes a procedure. Memory allocation is done in caller.
 */
int createProc( procedure *p, int pid );

/*
 * Initializes an instruction. Memory allocation is done in caller.
 */
int createInstr( instr *i, int addr, char *op, char *r1, char *r2, char *r3 );
 
/*
 * Reads [filename].arg and extract info on:
 * - last instruction address (for counting instructions purpose)
 * - start address of main procedure (for identifying main procedure id)
 *
 * File format is:
 *   <start_addr> <end_addr> <start_main> <cache_parameters>...
 */
int read_arg( int *lastaddr, int *mainaddr );

/*
 * Given a basic block bb (knowing its start address), 
 * calculates the number of bytes in the basic block preceeding it.
 * The result is assigned to the size of the preceeding bb, and returned.
 */
int countInstrPrev( block *bb );

/*
 * Calculates the number of instructions in the last basic block in the last procedure.
 * The result is assigned to the size of the last bb, and returned.
 */
int countInstrLast( int lastaddr );

/*
 * Returns the id of the main procedure, -1 if none found.
 */
int findMainProc( int mainaddr );

/*
 * Determines a reverse topological ordering of the procedure call graph.
 * Result is stored in global array proc_cg.
 */
int topo_call();

/*
 * Parses CFG.
 * Input : [filename].cfg
 * Format: <proc_id> <bb_id> <bb_startaddr> <taken_branch> <nontaken_branch> <called_proc>
 */
int read_cfg();

/*
 * Reads in assembly instructions and stores them in the corresponding basic block.
 * For purpose of conflict detection for infeasible path detection.
 * At the point of call, cfg should have been parsed.
 *
 * Input : [filename].md
 * Format: <insn_addr> <op> <r1> <r2> <r3>
 */
int readInstr();


#endif
