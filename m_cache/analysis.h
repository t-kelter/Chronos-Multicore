/*
 * Declarations for WCET analysis.
 */

#define PROC 1
#define LOOP 2


FILE *ilpf;

ull *enum_paths_proc;  // number of paths in each procedure
ull *enum_paths_loop;  // number of paths in each loop (in currently analysed procedure)

unsigned short ****enum_pathlist;  // enum_pathlist[p][b]: list of enumerated paths kept at proc. p block b
unsigned short ***enum_pathlen;    // enum_pathlen[p][b][n]: length of the n-th path in enum_pathlist[p][b]

char do_inline = 0;

