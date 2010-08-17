// Include standard library headers
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "offsetData.h"
#include "busSchedule.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


// #########################################
// #### Declaration of static variables ####
// #########################################


// ##################################################
// #### Forward declarations of static functions ####
// ##################################################


// #########################################
// #### Definitions of static functions ####
// #########################################


// #########################################
// #### Definitions of public functions ####
// #########################################


/* Verifies that the bound information is valid. */
_Bool checkOffsetBound( const tdma_offset_bounds *b )
{
  // TODO: This won't work correctly for segmented schedules
  const uint interval = getCoreSchedule( ncore, 0 )->interval;
  return b->lower_bound <= b->upper_bound &&
         b->lower_bound >= 0 && b->lower_bound < interval &&
         b->upper_bound >= 0 && b->upper_bound < interval;
}


/* Returns the union of the given offset bounds. */
tdma_offset_bounds mergeOffsetBounds( const tdma_offset_bounds *b1, const tdma_offset_bounds *b2 )
{
  assert( b1 && checkOffsetBound( b1 ) && b2 && checkOffsetBound( b2 ) &&
          "Invalid arguments!" );

  tdma_offset_bounds result;
  result.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
  result.upper_bound = MAX( b1->upper_bound, b2->upper_bound );

  assert( checkOffsetBound( &result ) && "Invalid result!" );
  return result;
}


/* Returns
 * - a negative number : if 'lhs' is no subset of 'rhs', nor are they equal
 * - 0                 : if 'lhs' == 'rhs'
 * - a positive number : if 'lhs' is a subset of 'rhs' */
int isOffsetBoundSubsetOrEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs )
{
  assert( lhs && checkOffsetBound( lhs ) && rhs && checkOffsetBound( rhs ) &&
          "Invalid arguments!" );

  // Check equality
  if ( lhs->lower_bound == rhs->lower_bound &&
       lhs->upper_bound == rhs->upper_bound ) {
    return 0;
  } else {
    // Check for subset
    if ( lhs->lower_bound >= rhs->lower_bound &&
         lhs->upper_bound <= rhs->upper_bound ) {
      return 1;
    } else {
      return -1;
    }
  }
}


/* Returns
 * - 1 : if 'lhs' == 'rhs'
 * - 0 : if 'lhs' != 'rhs' */
_Bool isOffsetBoundEqual( const tdma_offset_bounds *lhs, const tdma_offset_bounds *rhs )
{
  assert( lhs && checkOffsetBound( lhs ) && rhs && checkOffsetBound( rhs ) &&
          "Invalid arguments!" );

  return lhs->lower_bound == rhs->lower_bound &&
         lhs->upper_bound == rhs->upper_bound;
}


/* Returns '1' if the given offset bounds represent the maximal offset range,
 * else return '0'. */
_Bool isMaximalOffsetRange( const tdma_offset_bounds *b )
{
  assert( b && checkOffsetBound( b ) && "Invalid arguments!" );

  // TODO: This won't work correctly for segmented schedules
  const uint interval = getCoreSchedule( ncore, 0 )->interval;

  return b->lower_bound == 0 &&
         b->upper_bound == interval - 1;
}
