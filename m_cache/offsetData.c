// Include standard library headers
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "offsetData.h"
#include "handler.h"


// ############################################################
// #### Local data type definitions (will not be exported) ####
// ############################################################


/* Internal marker for minimum current offset. */
#define MINIMUM_OFFSET 0
/* Internal marker for maximum current offset. */
#define MAXIMUM_OFFSET max_offset


// #########################################
// #### Declaration of static variables ####
// #########################################


/* The maximum possible offset that is used in the current setup. */
static uint max_offset;


// ##################################################
// #### Forward declarations of static functions ####
// ##################################################


// #########################################
// #### Definitions of static functions ####
// #########################################


// #########################################
// #### Definitions of public functions ####
// #########################################


/* Creates an empty set-type offset data object. */
offset_data createOffsetDataSet( void )
{
  offset_data result;
  result.type = OFFSET_DATA_TYPE_SET;
  memset( result.content.offset_set.offsets, 0,
      TECHNICAL_OFFSET_MAXIMUM * sizeof( _Bool ) );
  return result;
}


/* Creates an offset data object of given type which
 * represents the given offset interval. */
offset_data createOffsetDataFromOffsetBounds( enum OffsetDataType type,
    uint lower_bound, uint upper_bound )
{
  assert( lower_bound <= MAXIMUM_OFFSET &&
          upper_bound <= MAXIMUM_OFFSET &&
          lower_bound <= upper_bound &&
          "Invalid arguments!" );

  offset_data result;
  result.type = type;

  if ( type == OFFSET_DATA_TYPE_RANGE ) {
    result.content.offset_range.lower_bound = lower_bound;
    result.content.offset_range.upper_bound = upper_bound;
  } else if ( type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    result.content.time_range.lower_bound = lower_bound;
    result.content.time_range.upper_bound = upper_bound;
  } else if ( type == OFFSET_DATA_TYPE_SET ) {
    memset( result.content.offset_set.offsets, 0,
        TECHNICAL_OFFSET_MAXIMUM * sizeof( _Bool ) );

    uint i;
    for ( i = lower_bound; i <= upper_bound; i++ ) {
      result.content.offset_set.offsets[i] = 1;
    }
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }

  return result;
}


/* Creates an offset data object of given type which
 * represents the given time interval. */
offset_data createOffsetDataFromTimeBounds( enum OffsetDataType type,
    ull minTime, ull maxTime )
{
  assert( minTime <= maxTime && "Invalid arguments" );

  /* Very simple for time range type. */
  if ( type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    offset_data result;
    result.type = type;
    result.content.time_range.lower_bound = minTime;
    result.content.time_range.upper_bound = maxTime;
    return result;
  }

  // TODO: This won't work for segmented schedules

  // Split up the time values into multiplier and remainder
  const uint minTime_factor    = minTime / ( MAXIMUM_OFFSET + 1 );
  const uint minTime_remainder = minTime % ( MAXIMUM_OFFSET + 1 );
  const uint maxTime_factor    = maxTime / ( MAXIMUM_OFFSET + 1 );
  const uint maxTime_remainder = maxTime % ( MAXIMUM_OFFSET + 1 );

  /* We want to find the offsets in the interval [minTime, maxTime]
   * To do that efficiently we must consider multiple cases:
   *
   * a) Between the two time values lies a full TDMA iteration
   *    --> all offsets are possible */
  const uint factorDifference = maxTime_factor - minTime_factor;
  if ( factorDifference > 1 ) {
    return createOffsetDataFromOffsetBounds( type,
        MINIMUM_OFFSET, MAXIMUM_OFFSET );

  /* b) The interval [minTime, maxTime] crosses exactly one TDMA interval boundary
   *    --> Not all offsets may be possible (depending on the remainders) but
   *        offset MINIMUM_OFFSET and MAXIMUM_OFFSET are definitely possible and
   *        thus our offset range representation does not allow a tighter bound
   *        than [MINIMUM_OFFSET, MAXIMUM_OFFSET]. The set representation can be
   *        more precise here. */
  } else if ( factorDifference == 1 ) {

    if ( type == OFFSET_DATA_TYPE_RANGE ) {
      return createOffsetDataFromOffsetBounds( type,
          MINIMUM_OFFSET, MAXIMUM_OFFSET );
    } else {
      offset_data result = createOffsetDataSet();
      addOffsetDataOffsetRange( &result, MINIMUM_OFFSET, maxTime_remainder );
      addOffsetDataOffsetRange( &result, minTime_remainder, MAXIMUM_OFFSET );
      return result;
    }

  /* c) The interval [minTime, maxTime] lies inside a single TDMA interval
   *    --> Use the remainders as the offset bounds */
  } else {
    assert( minTime_factor == maxTime_factor &&
            minTime_remainder <= maxTime_remainder &&
            "Invalid internal state!" );
    return createOffsetDataFromOffsetBounds( type,
        minTime_remainder, maxTime_remainder );
  }
}

/* If 'd' is of a type which represents the offsets only implicitly,
 * then this function will convert the contents of 'd' such that it
 * only contains the represented offsets, and no other information.
 * For the time representations this means, that the absolute times
 * are taken modulo the TDMA interval to get the offsets. These are
 * then stored in 'd'. For explicit offset representations, this
 * function does nothing.
 *
 * Explicit offset representations:
 * - OFFSET_DATA_TYPE_RANGE
 * - OFFSET_DATA_TYPE_SET
 *
 * Implicit offset representations:
 * - OFFSET_DATA_TYPE_TIME_RANGE_RANGE
 * - OFFSET_DATA_TYPE_TIME_RANGE_SET
 */
void convertOffsetDataToExplicitOffsets( offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    return;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    time_bounds * const b = &d->content.time_range;
    b->lower_bound %= ( MAXIMUM_OFFSET + 1 );
    b->upper_bound %= ( MAXIMUM_OFFSET + 1 );
    if ( b->lower_bound > b->upper_bound ) {
      const uint temp = b->lower_bound;
      b->lower_bound = b->upper_bound;
      b->upper_bound = temp;
    }
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    return;
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }

}


/* Returns the currently used maximum offset. The offset
 * representation will not be able to deal with offsets bigger
 * than this. */
uint getOffsetDataMaxOffset( void )
{
  return MAXIMUM_OFFSET;
}


/* Sets the currently used maximum offsets. All offset
 * data objects which are created after this function has
 * been called will use the new maximum offset. */
void setOffsetDataMaxOffset( uint new_max_offset )
{
  assert( new_max_offset <= TECHNICAL_OFFSET_MAXIMUM &&
      "Maximum offset exceeeds technical maximum!" );
  MAXIMUM_OFFSET = new_max_offset;
}


/* Adds the given offset range to the given offset data object,
 * regardless of its type. */
void addOffsetDataOffsetRange( offset_data * const d,
                               uint lower_bound, uint upper_bound )
{
  assert( d && isOffsetDataValid( d ) &&
          lower_bound <= MAXIMUM_OFFSET &&
          upper_bound <= MAXIMUM_OFFSET &&
          lower_bound <= upper_bound && "Invalid arguments!" );
  assert( d->type != OFFSET_DATA_TYPE_TIME_RANGE && "Unsupported operation!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    tdma_offset_bounds * const b = &d->content.offset_range;
    if ( lower_bound < b->lower_bound ) {
      b->lower_bound = lower_bound;
    } else
    if ( upper_bound > b->upper_bound ) {
      b->upper_bound = upper_bound;
    }
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    tdma_offset_set * const s = &d->content.offset_set;
    uint i;
    for ( i = lower_bound; i <= upper_bound; i++ ) {
      s->offsets[i] = 1;
    }
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }
}


/* Sets the given offset data's contents such that it contains the most
 * unprecise offset information possible. */
void setOffsetDataMaximal( offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );
  assert( d->type != OFFSET_DATA_TYPE_TIME_RANGE && "Unsupported operation!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    tdma_offset_bounds * const b = &d->content.offset_range;
    b->lower_bound = MINIMUM_OFFSET;
    b->upper_bound = MAXIMUM_OFFSET;
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    tdma_offset_set * const s = &d->content.offset_set;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      s->offsets[i] = 1;
    }
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }
}


/* Computes new offsets which would be reached from "startOffsets" after
 * 'bcTimeElapsed' time units have passed in the best case and 'wcTimeElapsed'
 * time units have passed in the worst case. The function writes the resulting
 * offsets to "targetOffsets". If "append" is true, then the new offsets
 * will be merged with those in "targetOffsets", else they will overwrite
 * the previous content of "targetOffsets".
 *
 * "sourceOffsets" may be equal to "targetOffsets"
 * "bcTimeElapsed" may be bigger than "wcTimeElapsed" */
void updateOffsetData( offset_data * const targetOffsets,
    const offset_data * const sourceOffsets, const uint bcTimeElasped,
    const uint wcTimeElapsed, const _Bool append )
{
  assert( targetOffsets && isOffsetDataValid( targetOffsets ) &&
          sourceOffsets && isOffsetDataValid( sourceOffsets ) &&
          "Invalid arguments!" );
  assert( targetOffsets->type == sourceOffsets->type && "Unsupported operation!" );

  /* For time representations this is absolutely trivial. */
  if ( sourceOffsets->type == OFFSET_DATA_TYPE_TIME_RANGE ) {

    const time_bounds * const sourceTime = &sourceOffsets->content.time_range;
    time_bounds       * const targetTime = &targetOffsets->content.time_range;
    targetTime->lower_bound = sourceTime->lower_bound + bcTimeElasped;
    targetTime->upper_bound = sourceTime->upper_bound + wcTimeElapsed;

  } else {

    const uint newLowerBound = getOffsetDataMinimumOffset( sourceOffsets ) + bcTimeElasped;
    const uint newUpperBound = getOffsetDataMaximumOffset( sourceOffsets ) + wcTimeElapsed;

    // Split up the time values into multiplier and remainder
    const uint bcTime_factor    = newLowerBound / ( MAXIMUM_OFFSET + 1 );
    const uint bcTime_remainder = newLowerBound % ( MAXIMUM_OFFSET + 1 );
    const uint wcTime_factor    = newUpperBound / ( MAXIMUM_OFFSET + 1 );
    const uint wcTime_remainder = newUpperBound % ( MAXIMUM_OFFSET + 1 );

    /* We want to find the offsets in the interval [newLowerBound, newUpperBound]
     * To do that efficiently we must consider multiple cases:
     *
     * a) Between the two time values lies a full TDMA iteration
     *    --> all offsets are possible */
    const uint factorDifference = abs( (signed int)wcTime_factor -
                                       (signed int)bcTime_factor );
    if ( factorDifference > 1 ) {
      setOffsetDataMaximal( targetOffsets );

      /* b) The interval [newLowerBound, newUpperBound] crosses at most one
       *    TDMA interval boundary. */
    } else {

      if ( sourceOffsets->type == OFFSET_DATA_TYPE_RANGE ) {
        tdma_offset_bounds * const b = &targetOffsets->content.offset_range;

        /* b1) The interval [newLowerBound, newUpperBound] crosses exactly one
         *     TDMA interval boundary
         *     --> Not all offsets may be possible (depending on the remainders) but
         *         offset MINIMUM_OFFSET and MAXIMUM_OFFSET are definitely possible and
         *         thus our offset range representation does not allow a tighter bound
         *         than [MINIMUM_OFFSET, MAXIMUM_OFFSET]. */
        if ( factorDifference == 1 ||
             bcTime_factor > wcTime_factor ) {
          setOffsetDataMaximal( targetOffsets );

        /* b2) The interval [newLowerBound, newUpperBound] lies inside a single
         *    TDMA interval
         *    --> Use the remainders as the offset bounds */
        } else {
          assert( bcTime_factor == wcTime_factor && "Invalid internal state!" );
          if ( bcTime_remainder <= wcTime_remainder ) {
            b->lower_bound = bcTime_remainder;
            b->upper_bound = wcTime_remainder;
          } else {
            setOffsetDataMaximal( targetOffsets );
          }
        }

      } else if ( sourceOffsets->type == OFFSET_DATA_TYPE_SET ) {

        /* For sets we can immediately compute the resulting offsets. We could
         * do it with this algorithm in all cases (also a)), but it is
         * time-consuming, and therefore we only do it if we can achieve better
         * precision which is the case here, when there are offsets which are
         * potentially NOT in the result set. */

        /* Determines whether we must first buffer the results. */
        _Bool bufferResultsFirst = ( sourceOffsets == targetOffsets );
        const _Bool * const inputPointer = sourceOffsets->content.offset_set.offsets;
              _Bool * const finalOutputPointer = targetOffsets->content.offset_set.offsets;
        _Bool *preliminaryOutputPointer;

        /* Determine and prepare the output area. */
        const int output_area_size = ( MAXIMUM_OFFSET + 1 ) * sizeof( _Bool );
        _Bool *offsetResultBuffer;
        if ( bufferResultsFirst ) {
          CALLOC( offsetResultBuffer, _Bool*, MAXIMUM_OFFSET + 1,
                sizeof( _Bool ), "newOffsets" );
          preliminaryOutputPointer = offsetResultBuffer;
          if ( append ) {
            memcpy( preliminaryOutputPointer, inputPointer,
                output_area_size );
          }
        } else {
          preliminaryOutputPointer = finalOutputPointer;
          if ( !append ) {
            memset( preliminaryOutputPointer, 0, output_area_size );
          }
        }

        // Compute the new offsets
        uint i, j;
        for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
          if ( inputPointer[i] ) {
            for ( j = bcTimeElasped; j <= wcTimeElapsed; j++ ) {
              preliminaryOutputPointer[ (i + j) % (MAXIMUM_OFFSET + 1) ] = 1;
            }
          }
        }

        // Copy from temporary array to the offset object if needed
        if ( preliminaryOutputPointer != finalOutputPointer ) {
          for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
            finalOutputPointer[i] = preliminaryOutputPointer[i];
          }
          free( offsetResultBuffer );
        }
      } else {
        assert( 0 && "Unsupported offset data type!" );
      }
    }

  }
}


/* Returns the union of 'd1' nad 'd2' into 'result'. */
offset_data mergeOffsetData( const offset_data * const d1,
                             const offset_data * const d2 )
{
  assert( d1 && isOffsetDataValid( d1 ) &&
          d2 && isOffsetDataValid( d2 ) && "Invalid arguments!" );
  assert( d1->type == d2->type && "Unsupported operation!" );
  offset_data result;
  result.type = d1->type;

  if ( d1->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b1 = &d1->content.offset_range;
    const tdma_offset_bounds * const b2 = &d2->content.offset_range;

    result.content.offset_range.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
    result.content.offset_range.upper_bound = MAX( b1->upper_bound, b2->upper_bound );
  } else if ( d1->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b1 = &d1->content.time_range;
    const time_bounds * const b2 = &d2->content.time_range;

    result.content.time_range.lower_bound = MIN( b1->lower_bound, b2->lower_bound );
    result.content.time_range.upper_bound = MAX( b1->upper_bound, b2->upper_bound );
  } else if ( d1->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s1 = &d1->content.offset_set;
    const tdma_offset_set * const s2 = &d2->content.offset_set;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      const _Bool newValue = s1->offsets[i] | s2->offsets[i];
      result.content.offset_set.offsets[i] = newValue;
    }
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }

  assert( isOffsetDataValid( &result ) && "Invalid result!" );
  return result;
}


/* Returns
 * - a negative number : if 'lhs' is no subset of 'rhs', nor are they equal
 * - 0                 : if 'lhs' == 'rhs'
 * - a positive number : if 'lhs' is a subset of 'rhs' */
int isOffsetDataSubsetOrEqual( const offset_data * const lhs,
                               const offset_data * const rhs )
{
  assert( lhs && isOffsetDataValid( lhs ) &&
          rhs && isOffsetDataValid( rhs ) && "Invalid arguments!" );
  assert( lhs->type == rhs->type && "Unsupported operation!" );

  if ( lhs->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b1 = &lhs->content.offset_range;
    const tdma_offset_bounds * const b2 = &rhs->content.offset_range;

    // Check equality
    if ( b1->lower_bound == b2->lower_bound &&
         b1->upper_bound == b2->upper_bound ) {
      return 0;
    } else {
      // Check for subset
      if ( b1->lower_bound >= b2->lower_bound &&
           b1->upper_bound <= b2->upper_bound ) {
        return 1;
      } else {
        return -1;
      }
    }
  } else if ( lhs->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b1 = &lhs->content.time_range;
    const time_bounds * const b2 = &rhs->content.time_range;

    // Check equality
    if ( b1->lower_bound == b2->lower_bound &&
         b1->upper_bound == b2->upper_bound ) {
      return 0;
    } else {
      // Check for subset
      if ( b1->lower_bound >= b2->lower_bound &&
           b1->upper_bound <= b2->upper_bound ) {
        return 1;
      } else {
        return -1;
      }
    }
  } else if ( lhs->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s1 = &lhs->content.offset_set;
    const tdma_offset_set * const s2 = &rhs->content.offset_set;

    _Bool allEqual = 1;
    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      if ( s1->offsets[i] > s2->offsets[i] ) {
        return -1;
      } else
      if ( s1->offsets[i] < s2->offsets[i] ) {
        allEqual = 0;
      }
    }
    return allEqual;
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }
}


/* Returns
 * - 1 : if 'lhs' == 'rhs'
 * - 0 : if 'lhs' != 'rhs' */
_Bool isOffsetDataEqual( const offset_data * const lhs,
                         const offset_data * const rhs )
{
  return isOffsetDataSubsetOrEqual( lhs, rhs ) == 0;
}


/* Returns '1' if the given offset data represent the maximal offset
 * range, else return '0'. */
_Bool isOffsetDataMaximal( const offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    return b->lower_bound == MINIMUM_OFFSET &&
           b->upper_bound == MAXIMUM_OFFSET;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    offset_data rangedata = createOffsetDataFromTimeBounds(
        OFFSET_DATA_TYPE_RANGE, b->lower_bound, b->upper_bound );
    return isOffsetDataMaximal( &rangedata );
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      if ( !s->offsets[i] ) {
        return 0;
      }
    }
    return 1;
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }
}


/* Verifies that the offset data information is valid. */
_Bool isOffsetDataValid( const offset_data * const d )
{
  assert( d && "Invalid argument!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    return b->lower_bound <= b->upper_bound &&
           b->lower_bound >= MINIMUM_OFFSET && b->lower_bound <= MAXIMUM_OFFSET &&
           b->upper_bound >= MINIMUM_OFFSET && b->upper_bound <= MAXIMUM_OFFSET;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    return b->lower_bound <= b->upper_bound;
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    return 1;
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }
}


/* Returns whether the offset data information is empty. */
_Bool isOffsetDataEmpty( const offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) &&  "Invalid argument!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    return 0;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    return 0;
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      if ( s->offsets[i] ) {
        return 0;
      }
    }
    return 1;
  } else {
    assert( 0 && "Unsupported offset data type!" );
    return 0;
  }
}


/* Returns whether the given offset data represents a single value.
 *
 * If TRUE is returned, then the single value is written into
 * '*singleValue' is 'singleValue' is not NULL:
 * */
_Bool isOffsetDataSingleValue( const offset_data * const d,
    uint * const singleValue )
{
  assert( d && isOffsetDataValid( d ) &&  "Invalid argument!" );

  tdma_offset_bounds result;
  if ( isOffsetDataRangeValue( d, &result ) ) {
    if ( result.lower_bound == result.upper_bound ) {
      if ( singleValue != NULL ) {
        *singleValue = result.lower_bound;
      }
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }
}


/* Returns whether the given offset data represents a range value.
 * For offset range representations this is always true, for sets
 * it may be true. Empty offset data objects are not considered to
 * be range values.
 *
 * If TRUE is returned, then the offset range is written into
 * '*rangeValue' is 'rangeValue' is not NULL:
 * */
_Bool isOffsetDataRangeValue( const offset_data * const d,
    tdma_offset_bounds * const rangeValue )
{
  assert( d && isOffsetDataValid( d ) && "Invalid argument!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    if ( rangeValue != NULL ) {
      *rangeValue = d->content.offset_range;
    }
    return TRUE;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    if ( rangeValue != NULL ) {
      *rangeValue = createOffsetDataFromTimeBounds(
          OFFSET_DATA_TYPE_RANGE, b->lower_bound,
          b->upper_bound ).content.offset_range;
    }
    return TRUE;
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;
    tdma_offset_bounds result;
    _Bool foundFirstOffset = FALSE;
    _Bool foundLastOffset  = FALSE;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {

      // Current offset is in set
      if ( s->offsets[i] ) {
        if ( !foundFirstOffset ) {
          result.lower_bound = i;
          foundFirstOffset = TRUE;
        }
        if ( foundLastOffset ) {
          return FALSE;
        }

      // Current offset is not in set
      } else {
        if ( foundFirstOffset && !foundLastOffset ) {
          result.upper_bound = i - 1;
          foundLastOffset = TRUE;
        }
      }
    }

    // Offset set may be empty
    if ( !foundFirstOffset ) {
      return FALSE;
    } else {
      // If the set represents a range [x, max_offset], then
      // we must detect this here, because the loop will not
      // do so.
      if ( !foundLastOffset ) {
        result.upper_bound = MAXIMUM_OFFSET;
      }

      if ( rangeValue ) {
        *rangeValue = result;
      }
      return TRUE;
    }
  } else {
    assert( 0 && "Unsupported offset data type!" );
    return FALSE;
  }
}


/* Returns whether 'd' contains 'offset'. */
_Bool doesOffsetDataContainOffset( const offset_data * const d,
    const uint offset )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    return offset >= b->lower_bound &&
           offset <= b->upper_bound;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    offset_data rangedata = createOffsetDataFromTimeBounds(
        OFFSET_DATA_TYPE_RANGE, b->lower_bound, b->upper_bound );
    return doesOffsetDataContainOffset( &rangedata, offset );
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;
    return s->offsets[offset];
  } else {
    assert( 0 && "Unsupported offset data type!" );
    return 0; // To make compiler happy
  }
}


/* Returns the minimum offset which is included in the given data object.
 * If there is no minimum, because the offset object is empty, then this
 * functions throws an assertion. */
uint getOffsetDataMinimumOffset( const offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    return b->lower_bound;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    offset_data rangedata = createOffsetDataFromTimeBounds(
        OFFSET_DATA_TYPE_RANGE, b->lower_bound, b->upper_bound );
    return getOffsetDataMinimumOffset( &rangedata );
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;

    uint i;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      if ( s->offsets[i] ) {
        return i;
      }
    }
    assert( 0 && "Offset set was empty!" );
    return 0; // To make compiler happy
  } else {
    assert( 0 && "Unsupported offset data type!" );
    return 0; // To make compiler happy
  }
}


/* Returns the maximum offset which is included in the given data object.
 * If there is no maximum, because the offset object is empty, then this
 * functions throws an assertion. */
uint getOffsetDataMaximumOffset( const offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid arguments!" );

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    return b->upper_bound;
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    offset_data rangedata = createOffsetDataFromTimeBounds(
        OFFSET_DATA_TYPE_RANGE, b->lower_bound, b->upper_bound );
    return getOffsetDataMaximumOffset( &rangedata );
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;

    uint i;
    for ( i = MAXIMUM_OFFSET; i >= MINIMUM_OFFSET; i-- ) {
      if ( s->offsets[i] ) {
        return i;
      }
    }
    assert( 0 && "Offset set was empty!" );
    return 0; // To make compiler happy
  } else {
    assert( 0 && "Unsupported offset data type!" );
    return 0; // To make compiler happy
  }
}


/* Prints the offset data into an internal string and returns this string. */
char *getOffsetDataString( const offset_data * const d )
{
  assert( d && isOffsetDataValid( d ) && "Invalid argument!" );

  const uint string_size = 100 + ( max_offset + 1 ) *
                                 ( log( max_offset ) / log( 2 ) );
  uint printed_chars = 0;
  static char *output_string = NULL;
  REALLOC( output_string, char*, string_size, "output_string" );
  char *output_ptr = output_string;

  #define PRINT_TO_STRING( format, ... ) \
    { \
      const uint printed = snprintf( output_ptr, string_size - printed_chars, \
                                     format, ## __VA_ARGS__ ); \
      output_ptr += printed; \
      printed_chars += printed; \
    }

  if ( d->type == OFFSET_DATA_TYPE_RANGE ) {
    const tdma_offset_bounds * const b = &d->content.offset_range;
    PRINT_TO_STRING( "[%u,%u]", b->lower_bound, b->upper_bound );
  } else if ( d->type == OFFSET_DATA_TYPE_TIME_RANGE ) {
    const time_bounds * const b = &d->content.time_range;
    PRINT_TO_STRING( "[%llu,%llu]", b->lower_bound, b->upper_bound );
  } else if ( d->type == OFFSET_DATA_TYPE_SET ) {
    const tdma_offset_set * const s = &d->content.offset_set;
    PRINT_TO_STRING( "{ " );
    uint i;
    _Bool firstEntry = 1;
    for ( i = MINIMUM_OFFSET; i <= MAXIMUM_OFFSET; i++ ) {
      if ( s->offsets[i] ) {

        /* Slash mode. */
        if ( i >= MINIMUM_OFFSET+1 && s->offsets[i-1] ) {
          if ( i < MINIMUM_OFFSET+2 || !s->offsets[i-2] ) {
            PRINT_TO_STRING( "-" );
          }
          if ( i+1 > MAXIMUM_OFFSET || !s->offsets[i+1] ) {
            PRINT_TO_STRING( "%u", i );
          }
        /* Normal mode. */
        } else {
          if ( !firstEntry ) {
            PRINT_TO_STRING( ", " );
          } else {
            firstEntry = 0;
          }
          PRINT_TO_STRING( "%u", i );
        }
      }
    }
    PRINT_TO_STRING( " }" );
  } else {
    assert( 0 && "Unsupported offset data type!" );
  }

  #undef PRINT_TO_STRING

  return output_string;
}
