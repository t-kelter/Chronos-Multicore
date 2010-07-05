/*****************************************************************************
 *
 * $Id: common.h,v 1.7 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: common.h,v $
 * Revision 1.7  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.6  2004/05/08 03:03:30  lixianfe
 * May 8th, 2004
 *
 * Revision 1.5  2004/04/22 08:55:41  lixianfe
 * Version free of flush (except interprocedural flushes)
 *
 * Revision 1.4  2004/03/06 03:50:24  lixianfe
 * corrected a bug in reading user constraints
 *
 * Revision 1.3  2004/02/24 12:58:31  lixianfe
 * Initial ILP formulation is done
 *
 * Revision 1.2  2004/02/17 04:02:17  lixianfe
 * implemented function of finding flush points
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <assert.h>

#define SHOW_PROGRESS	0

#define	MAX_INST	1024000	/* maximum # of instructions can be processed */
#define MAX_BB		81920	/* maximum # of blocks can be processed */
//#define	MAX_INST	102400	/* maximum # of instructions can be processed */
//#define MAX_BB		8192	/* maximum # of blocks can be processed */
//#define	MAX_INST	10240	/* maximum # of instructions can be processed */
//#define MAX_BB		4096	/* maximum # of blocks can be processed */
// #define	MAX_INST	5120	/* maximum # of instructions can be processed */
// #define MAX_BB		1024	/* maximum # of blocks can be processed */
#define	PATH_MAX_INST	32	/* thershold of path length for WCET calc */
#define	STACK_ELEMS	1024	/* initial stack capacity in terms of elements */
#define	QUEUE_ELEMS	1024    

#define CHECK_MEM(p) \
if ((p) == NULL) { \
    fprintf(stderr, "Out of memory in %s:%d\n", __FILE__, __LINE__); \
    exit(1); \
}

#define	inst_num(code, size)	((size) / SS_INST_SIZE)
#define inst_size(code, n)	((n) * SS_INST_SIZE)
#define BETWEEN(x, a, b)	(((x) >= (a)) && ((x) <= (b)))
#define INSIDE(x, a, b)		(return (((x) > (a)) && ((x) < (b))))
#define SET_FLAG(x, flag_msk)	((x) |= (flag_msk))
#define RESET_FLAG(x, flag_msk)	((x) &= (~(flag_msk)))
#define TEST_FLAG(x, flag_msk)	((x) & (flag_msk))


#define GOOD_RANGE	0
#define BAD_RANGE	1

#define SAME_GENERAL	0
#define LESS_GENERAL	1
#define	MORE_GENERAL	2
#define NO_GENERAL	3   // generality uncomparable
// an interval [lo..hi]
typedef struct {
    int	    lo, hi;
} range_t;

// compare which one is more general
int
cmp_general(range_t *x, range_t *y);

// intersect operation
int
range_isect(range_t *x, range_t *y);

// union operation
void
range_union(range_t *x, range_t *y);



typedef struct stack_t {
    void    *base;
    void    *top;
    int	    esize;	/* element size */
    int	    capt;	/* capacity */
} Stack;



#ifndef QUEUE
#define QUEUE
typedef struct queue_t {
    void    *base;
    void    *head, *tail;   /* head points to oldest element */
    int	    esize;	    /* element size */
    int	    capt;	    /* capacity */
} Queue;
#endif


const void *
my_bsearch(const void *key, const void *base, size_t n, size_t size,
	int (*cmp)(const void *k, const void *datum));

void
my_insert(const void *x, void *base, void *y, int *nelem, int size);



int
bits(unsigned x);


#endif
