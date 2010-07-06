#include <stdlib.h>

#include "knapsack.h"
#include "header.h"

/*
 * Knapsack solution via Dynamic Programming.
 * Returns the optimal gain value, and updates the array alloc with the solution.
 * If only interested in the objective value, pass NULL as alloc.
 *
 * DP table: need to keep only the current row and the last one row.
 * For each cell we keep the gain and the allocation (as bit array).
 */
int DP_knapsack( int capacity, int num_items, int *gain, int *weight, char *alloc ) {

  int *prev_gain = NULL;
  int *curr_gain;
  char **prev_alloc = NULL;
  char **curr_alloc;

  int gainN, gainY;
  int i, w, space;
  int it;

  if( capacity <= 0 ) {
    if( alloc != NULL ) {
      for( i = 0; i < num_items; i++ )
	alloc[i] = 0;
    }
    return 0;
  }

  for( i = 0; i < num_items; i++ ) {

    curr_gain = (int*) CALLOC( curr_gain, capacity, sizeof(int), "curr_gain in DP" );

    if( alloc != NULL ) {
      curr_alloc = (char**) MALLOC( curr_alloc, capacity * sizeof(char*), "curr_alloc in DP" );
      for( w = 0; w < capacity; w++ ) {
	curr_alloc[w] = (char*) CALLOC( curr_alloc[w], num_items, sizeof(char), "curr_alloc elm in DP" );
      }
    }

    for( w = 0; w < capacity; w++ ) {

      // if not taking item i
      if( i > 0 )
	gainN = prev_gain[w];
      else
	gainN = 0;

      // if taking item i
      space = w + 1 - weight[i];
      if( space >= 0 ) {
	gainY = gain[i];
	if( space > 0 && i > 0 )
	  gainY += prev_gain[space-1];
      }
      else  // cannot fit
	gainY = 0;

      if( gainY >= gainN ) {
	curr_gain[w] = gainY;

	if( alloc != NULL ) {

	  if( space >= 0 ) {
	    // add item i
	    curr_alloc[w][i] = 1;

	    // copy previous allocation
	    if( space > 0 && i > 0 )
	      for( it = 0; it < i; it++ )
		curr_alloc[w][it] = prev_alloc[space-1][it];
	  }
        }
      }
      else {
	curr_gain[w] = gainN;

	// copy previous allocation
	if( alloc != NULL ) {
	  if( i > 0 ) {
	    for( it = 0; it < i; it++ )
	      curr_alloc[w][it] = prev_alloc[w][it];
	  }
	}
      }
      /*
      printf( "[%d][%d] %d\t", i, w, curr_gain[w] );
      for( it = 0; it < num_items; it++ )
	printf( "%d ", curr_alloc[w][it] );
      printf( "\n" );
      */
    }
    free( prev_gain );
    prev_gain = curr_gain;

    if( alloc != NULL ) {
      if( prev_alloc != NULL ) {
	for( w = 0; w < capacity; w++ )
	  free( prev_alloc[w] );
	free( prev_alloc );
      }
      prev_alloc = curr_alloc;
    }
  }
  gainY = curr_gain[capacity - 1];

  // resulting allocation
  if( alloc != NULL ) {
    for( i = 0; i < num_items; i++ )
      if( curr_alloc[capacity - 1][i] )
	alloc[i] = 1;
  }

  free( curr_gain );

  if( alloc != NULL ) {
    for( w = 0; w < capacity; w++ )
      free( curr_alloc[w] );
    free( curr_alloc );
  }

  return gainY;
}
