/*! This is a header file of the Chronos timing analyzer. */

/*
 * This compilation unit provides a modular representation for TDMA offsets.
 * The user of the header can switch between offset ranges, delimited by a lower
 * and an upper bound and offset sets, which explicitly list the offsets. The
 * objects can be created via special createXY functions. There is no need to
 * free them, as they have no dynamic data structures.
 *
 * ALWAYS call "setOffsetDataMaxOffset" before using this data type!
 */

#ifndef __CHRONOS_OFFSET_DATA_H
#define __CHRONOS_OFFSET_DATA_H

#include "header.h"

// ######### Macros #########


/* Technical offset limit (storage limitation). */
#define TECHNICAL_OFFSET_MAXIMUM 100


/* A convenience macro to iterate over the offsets in an offset data object. */
#define ITERATE_OFFSETS( offset_data, iteration_variable, loop_body_stmts ) \
  { uint iteration_variable; \
    if ( offset_data.type == OFFSET_DATA_TYPE_RANGE ) { \
      for ( iteration_variable  = offset_data.content.offset_range.lower_bound; \
            iteration_variable <= offset_data.content.offset_range.upper_bound; \
            iteration_variable++ ) { \
        loop_body_stmts \
      } \
    } else \
    if ( offset_data.type == OFFSET_DATA_TYPE_SET ) { \
      const uint max_offset = getOffsetDataMaxOffset(); \
      _Bool hadHit = 0; \
      for ( iteration_variable = 0; \
            iteration_variable <= max_offset; \
            iteration_variable++ ) { \
        if ( offset_data.content.offset_set.offsets[iteration_variable] ) { \
          hadHit = 1; \
          loop_body_stmts \
        } \
      } \
      assert( hadHit && "Offset set was empty!" ); \
    } else  {\
      assert( 0 && "Unknown offset data object type!" ); \
    } \
  }


// ######### Datatype declarations  ###########


/* A data type to represent an offset set. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  /* If the set contains offset i, then offsets[i] is 1 else it is 0. */
  _Bool offsets[TECHNICAL_OFFSET_MAXIMUM];
} tdma_offset_set;

/* A data type to represent an offset range. The offsets are measured from the
 * beginning of the TDMA slot of the first core. */
typedef struct {
  uint lower_bound;
  uint upper_bound;
} tdma_offset_bounds;

/* A data type to represent an absolute time range. */
typedef struct {
  ull lower_bound;
  ull upper_bound;
} time_bounds;

/* The type of an abstract offset representation. */
enum OffsetDataType {
  OFFSET_DATA_TYPE_SET,
  OFFSET_DATA_TYPE_RANGE,
  OFFSET_DATA_TYPE_TIME
};

/* The datatype which represents an abstract amount of offsets. */
typedef struct {
  /* Which of the two fields is active at the moment is given
   * by the value of the 'type' field. */
  union {
    tdma_offset_set offset_set;
    tdma_offset_bounds offset_range;
    time_bounds time_range;
  } content;
  enum OffsetDataType type;
} offset_data;


// ######### Function declarations  ###########


/* Creates an empty set-type offset data object. */
offset_data createOffsetDataSet( void );

/* Creates an offset data object of given type which
 * represents the given offset interval. */
offset_data createOffsetDataFromOffsetBounds( enum OffsetDataType type,
    uint lower_bound, uint upper_bound );

/* Creates an offset data object of given type which
 * represents the given time interval. */
offset_data createOffsetDataFromTimeBounds( enum OffsetDataType type,
    ull minTime, ull maxTime );

/* Returns the currently used maximum offset. The offset
 * representation will not be able to deal with offsets bigger
 * than this. */
uint getOffsetDataMaxOffset( void );
/* Sets the currently used maximum offsets. All offset
 * data objects which are created after this function has
 * been called will use the new maximum offset. */
void setOffsetDataMaxOffset( uint new_max_offset );

/* Adds the given offset range to the given offset data object,
 * regardless of its type. */
void addOffsetDataOffsetRange( offset_data * const d,
                               uint lower_bound, uint upper_bound );

/* Sets the given offset data's contents such that it contains the most
 * unprecise offset information possible. */
void setOffsetDataMaximal( offset_data * const d );

/* Returns the union of 'd1' nad 'd2'. */
offset_data mergeOffsetData( const offset_data * const d1,
                             const offset_data * const d2 );

/* Computes new offsets which would be reached from "startOffsets" after
 * [minTimeElasped,maxTimeElapsed] time units have passed and writes those
 * offsets to "targetOffsets". If "append" is true, then the new offsets
 * will be merged with those in "targetOffsets", else they will overwrite
 * the previous content of "targetOffsets".
 *
 * "sourceOffsets" may be equal to "targetOffsets" */
void updateOffsetData( offset_data * const targetOffsets,
    const offset_data * const sourceOffsets, const uint minTimeElasped,
    const uint maxTimeElapsed, const _Bool append );

/* Returns
 * - a negative number : if 'lhs' is no subset of 'rhs', nor are they equal
 * - 0                 : if 'lhs' == 'rhs'
 * - a positive number : if 'lhs' is a subset of 'rhs' */
int isOffsetDataSubsetOrEqual( const offset_data * const lhs,
                               const offset_data * const rhs );

/* Returns
 * - 1 : if 'lhs' == 'rhs'
 * - 0 : if 'lhs' != 'rhs' */
_Bool isOffsetDataEqual( const offset_data * const lhs,
                         const offset_data * const rhs );

/* Returns '1' if the given offset data represent the maximal offset
 * range, else return '0'. */
_Bool isOffsetDataMaximal( const offset_data * const d );

/* Verifies that the offset data information is valid. */
_Bool isOffsetDataValid( const offset_data * const d );

/* Returns whether the offset data information is empty. */
_Bool isOffsetDataEmpty( const offset_data * const d );

/* Returns whether the given offset data represents a single value.
 *
 * If TRUE is returned, then the single value is written into
 * '*singleValue' is 'singleValue' is not NULL:
 * */
_Bool isOffsetDataSingleValue( const offset_data * const d,
    uint * const singleValue );

/* Returns whether the given offset data represents a range value.
 * For offset range representations this is always true, for sets
 * it may be true. Empty offset data objects are not considered to
 * be range values.
 *
 * If TRUE is returned, then the offset range is written into
 * '*rangeValue' is 'rangeValue' is not NULL:
 * */
_Bool isOffsetDataRangeValue( const offset_data * const d,
    tdma_offset_bounds * const rangeValue );

/* Returns whether 'd' contains 'offset'. */
_Bool doesOffsetDataContainOffset( const offset_data * const d,
    const uint offset );

/* Returns the minimum offset which is included in the given data object.
 * If there is no minimum, because the offset object is empty, then this
 * functions throws an assertion. */
uint getOffsetDataMinimumOffset( const offset_data * const d );

/* Returns the maximum offset which is included in the given data object.
 * If there is no maximum, because the offset object is empty, then this
 * functions throws an assertion. */
uint getOffsetDataMaximumOffset( const offset_data * const d );

/* Prints the offset data into an internal string and returns this string. */
char *getOffsetDataString( const offset_data * const d );


#endif
