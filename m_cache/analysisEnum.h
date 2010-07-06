/*! This is a header file of the Chronos timing analyzer. */

/*
 * WCET analysis via path enumeration.
 */

#ifndef __CHRONOS_ANALYSIS_ENUM_H
#define __CHRONOS_ANALYSIS_ENUM_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


char enum_in_seq_bef( int bbid, int target, ushort *bb_seq, ushort bb_len );

int enum_effectCancelled( branch *br, assign *assg, ushort *bb_seq, ushort len,
    block **bblist, int num_bb );

int enum_findBranch( branch *br, ushort *bb_seq, ushort len );

char enum_BBconflictInPath( branch *bru, char direction, block *bv, 
			    ushort *bb_seq, ushort len, block **bblist, int num_bb );

char enum_BAconflictInPath( block *bu, ushort *bb_seq, ushort len, i
    block **bblist, int num_bb );

int analyseEnumDAG( char objtype, void *obj );

int analyseEnumProc( procedure *p );

int analysis_enum();


#endif
