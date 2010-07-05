/*****************************************************************************
 *
 * $Id: common.c,v 1.5 2004/07/29 02:18:10 lixianfe Exp $
 *
 * $Log: common.c,v $
 * Revision 1.5  2004/07/29 02:18:10  lixianfe
 * program transformation (loop unroll, proc inline), and
 * cache categorization
 *
 * Revision 1.4  2004/03/06 03:50:24  lixianfe
 * corrected a bug in reading user constraints
 *
 * Revision 1.3  2004/03/02 02:58:14  lixianfe
 * results of sim/est doesn't match, this is due to the procedure call and sim
 * dosen't get the right path
 *
 * Revision 1.2  2004/02/17 04:02:17  lixianfe
 * implemented function of finding flush points
 *
 * Revision 1.1  2004/02/11 07:56:51  lixianfe
 * now generates call graph and cfg for each procedure
 *
 *
 ****************************************************************************/

#include "common.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* binary search, if not match, return the entry where key will be inserted in front
 */
const void *
my_bsearch(const void *key, const void *base, size_t n, size_t size,
	int (*cmp)(const void *k, const void *datum))
{
    int		low, high, mid;
    int		c;

    if (n == 0)
	return base;

    low = 0; high = n - 1;
    while (low <= high) {
	mid = (low + high) / 2;
	if (cmp == NULL)
	    c = (*((int *) key) - *((int *) (base + mid * size)));
	else
	    c = cmp(key, (const void *)(base + mid * size));
	if (c < 0)
	    high = mid - 1;
	else if (c > 0)
	    low = mid + 1;
	else
	    return (const void *)(base + mid * size);

    }
    return  (const void *)(base + low * size);
}



/* insert x into base in front of y. nelem is the current number of elements in base
 * and size is the size of each element */
void
my_insert(const void *x, void *base, void *y, int *nelem, int size)
{
    int	    nbytes;

    nbytes = (*nelem) * size - ((intptr_t)y - (intptr_t)base);
    assert(nbytes >= 0);
    memmove(y+size, y, nbytes);
    memcpy(y, x, size);
    (*nelem)++;
}



/* stack operations */

void
init_stack(Stack *stack, int elem_size)
{
    stack->base = (void *) malloc(STACK_ELEMS * elem_size);
    if (stack->base == NULL) {
	fprintf(stderr, "out of memory (__FILE__:__LINE__)\n");
	exit(1);
    }
    stack->esize = elem_size;
    stack->capt = STACK_ELEMS * elem_size;
    stack->top = stack->base;
}



void
free_stack(Stack *stack)
{
    if (stack->base != NULL) {
	free(stack->base);
	stack->base = NULL;
    }
    stack->capt = stack->esize = 0;
    stack->top = NULL;
}



void
stack_push(Stack *stack, void *x)
{
    if ((stack->top + stack->esize) > (stack->base + stack->capt)) {
	stack->capt *= 2;
	stack->base = (void *) realloc(stack->base, stack->capt);
	if (stack->base == NULL) {
	    fprintf(stderr, "out of memory (__FILE__:__LINE__)\n");
	    exit(1);
	}
    }
    memcpy(stack->top, x, stack->esize);
    stack->top += stack->esize;
}



int
stack_empty(Stack *stack)
{
    assert(stack->top >= stack->base);
    return (stack->top == stack->base) ? 1 : 0;
}



void *
stack_pop(Stack *stack)
{
    if (stack_empty(stack))
	return NULL;
    stack->top -= stack->esize;
    return stack->top;
}



/* clear elements to make stack empty, but don't free memory */
void
clear_stack(Stack *stack)
{
    stack->top = stack->base;
}



/* copy contents of stack x to stack y; y must have been initiated and not freed */
void
copy_stack(Stack *y, Stack *x)
{
    /* first free stack y */
    free_stack(y);
    
    y->base = (void *) malloc(x->capt);
    if (y->base == NULL) {
	fprintf(stderr, "out of memory (__FILE__:__LINE__)\n");
	exit(1);
    }
    memcpy(y->base, x->base, (x->top - x->base));
    y->esize = x->esize;
    y->capt = x->capt;
    y->top = y->base + (x->top - x->base);
}



/* queue operations */
void
init_queue(Queue *queue, int elem_size)
{
    queue->base = (void *) malloc(QUEUE_ELEMS * elem_size);
    if (queue->base == NULL) {
	fprintf(stderr, "out of memory (__FILE__:__LINE__)\n");
	exit(1);
    }
    queue->esize = elem_size;
    queue->capt = QUEUE_ELEMS * elem_size;
    queue->head = queue->tail = queue->base;
}



void
free_queue(Queue *queue)
{
    if (queue->base != NULL) {
	free(queue->base);
	queue->base = NULL;
    }
    queue->capt = queue->esize = 0;
    queue->head = queue->tail = NULL;
}



void
enqueue(Queue *queue, void *x)
{
    void    *newbase;
    int	    n;

    memcpy(queue->tail, x, queue->esize);
    queue->tail += queue->esize;
    if (queue->tail == (queue->base + queue->capt))
	queue->tail = queue->base;
    if (queue->tail != queue->head) /* queue not full, done */
	return;

    /* queue is full, allocate more memory and move data */
    newbase = (void *) malloc(queue->capt * 2);
    if (newbase == NULL) {
	fprintf(stderr, "out of memory (__FILE__:__LINE__)\n");
	exit(1);
    }
    n = queue->base + queue->capt - queue->head;
    memcpy(newbase, queue->head, n);
    if (queue->tail > queue->base)
	memcpy(newbase + n, queue->base, queue->tail - queue->base);
    free(queue->base);
    queue->base = newbase;
    queue->head = queue->base;
    queue->tail = queue->base + queue->capt;
    queue->capt *= 2;
}



int
queue_empty(Queue *queue)
{
    return (queue->head == queue->tail) ? 1 : 0;
}



void *
dequeue(Queue *queue)
{
    void    *p;
    if (queue_empty(queue))
	return NULL;
    p = queue->head;
    queue->head += queue->esize;
    if (queue->head == (queue->base + queue->capt))
	queue->head = queue->base;
    return p;
}



/* just clear elements to queue empty, but don't free memory */
void
clear_queue(Queue *queue)
{
    queue->head = queue->tail = queue->base;
}



int
bits(unsigned x)
{
    int	    i = 0;

    if (x == 0)
	return 0;
    while (x > 0) {
	i++;
	x >>= 1;
    }
    return i;
}



int
cmp_general(range_t *x, range_t *y)
{
    if (x->lo < y->lo) {
	if (x->hi < y->hi)
	    return NO_GENERAL;
	else
	    return MORE_GENERAL;
    } else if (x->lo > y->lo) {
	if (x->hi > y->hi)
	    return NO_GENERAL;
	else
	    return LESS_GENERAL;
    } else {	// x->lo == y->lo
	if (x->hi > y->hi)
	    return MORE_GENERAL;
	else if (x->hi < y->hi)
	    return LESS_GENERAL;
	else	// x->hi == y->hi
	    return SAME_GENERAL;
    }
}



int
range_isect(range_t *x, range_t *y)
{
    if (x->lo < y->lo)
	x->lo = y->lo;
    if (x->hi > y->hi)
	x->hi = y->hi;
    if (x->lo > x->hi)
	return BAD_RANGE;
    else
	return GOOD_RANGE;
}



// union operation
void
range_union(range_t *x, range_t *y)
{
    if (x->lo > y->lo)
	x->lo = y->lo;
    if (x->hi < y->hi)
	x->hi = y->hi;
}
