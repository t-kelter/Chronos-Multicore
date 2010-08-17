/*! This is a header file of the Chronos timing analyzer. */

/*
 * This compilation unit provides a modular representation for TDMA offsets.
 * The user of the header can switch between offset ranges, delimited by a lower
 * and an upper bound and offset sets, which explicitly list the offsets. The
 * former is the default, while the latter is only activated when
 *
 * CHRONOS_USE_TDMA_OFFSET_SETS
 *
 * is defined. The resulting datatype is called 'offset_data' in any case.
 */

#ifndef __CHRONOS_OFFSET_DATA_H
#define __CHRONOS_OFFSET_DATA_H

#include "header.h"

// ######### Macros #########


/* A convenience macro to iterate over the offsets in an offset data object. */
#define ITERATE_OFFSET_BOUND( bound, iteration_variable ) \
  for ( iteration_variable  = bound.lower_bound; \
        iteration_variable <= bound.upper_bound; \
        iteration_variable++ )


// ######### Datatype declarations  ###########


/* A data type to represent an offset set. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint *offsets;
  uint num_offsets;
} tdma_offset_set;

/* A data type to represent an offset range. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint lower_bound;
  uint upper_bound;
} tdma_offset_bounds;


/* Publish the selected data type. */
#ifdef CHRONOS_USE_TDMA_OFFSET_SETS
typedef tdma_offset_set offset_data;
#else
typedef tdma_offset_bounds offset_data;
#endif


// ######### Function declarations  ###########


/* Verifies that the bound information is valid. */
_Bool checkOffsetBound( const tdma_offset_bounds *b );

/* Returns the union of the given offset bounds. */
tdma_offset_bounds mergeOffsetBounds( const tdma_offset_bounds *b1, const tdma_offset_bounds *b2 );

/* Returns
 * - a negative number : if 'lhs' is no subset of 'rhs', nor are they equal
 * - 0                 : if 'lhs' == 'rhs'
 * - a positive number : if 'lhs' is a subset of 'rhs' */
int isOffsetBoundSubsetOrEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs );

/* Returns
 * - 1 : if 'lhs' == 'rhs'
 * - 0 : if 'lhs' != 'rhs' */
_Bool isOffsetBoundEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs );

/* Returns '1' if the given offset bounds represent the maximal offset range,
 * else return '0'. */
_Bool isMaximalOffsetRange( const tdma_offset_bounds *b );


#endif
