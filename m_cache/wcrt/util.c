#include <stdlib.h>

#include "util.h"
#include "header.h"

/*
 * Tests for membership in an integer array.
 */
char inList( int element, int *arr, int len ) {

  int i;
  for( i = 0; i < len; i++ )
    if( arr[i] == element )
      return 1;
  return 0;
}


/*
 * Returns the index of element in arr.
 */
int getIndexInList( int element, int *arr, int len ) {

  int i;
  for( i = 0; i < len; i++ )
    if( arr[i] == element )
      return i;
  return -1;
}


/*
 * Inserts element into integer array arr, and updates len.
 */
int enqueue( int element, int **arr, int *len ) {

  (*len)++;
  REALLOC( *arr, int*, (*len) * sizeof(int), "arr" );
  (*arr)[(*len)-1] = element;

  return *len;
}


/*
 * Inserts element into integer array arr and updates len, if it is not already in.
 */
int enqueueUnique( int element, int **arr, int *len ) {

  if( *arr != NULL && inList( element, *arr, *len ) )
    return *len;

  (*len)++;
  REALLOC( *arr, int*, (*len) * sizeof(int), "arr" );
  (*arr)[(*len)-1] = element;

  return *len;
}


/*
 * Inserts element1 into integer array arr1, element2 into arr2, and updates len.
 */
int enqueuePair( int element1, int **arr1, int element2, int **arr2, int *len ) {

  (*len)++;

  REALLOC( *arr1, int*, (*len) * sizeof(int), "arr1" );
  (*arr1)[(*len)-1] = element1;

  REALLOC( *arr2, int*, (*len) * sizeof(int), "arr2" );
  (*arr2)[(*len)-1] = element2;

  return *len;
}


/*
 * Removes and returns the first element of integer array arr, and updates len.
 * *len has to be > 0; no check is done here.
 */
int dequeue( int **arr, int *len ) {

  int i;
  int element = (*arr[0]);
  for( i = 0; i < (*len)-1; i++ )
    (*arr)[i] = (*arr)[i+1];
  (*len)--;

  return element;
}


/*
 * Returns (a mod b) where a may be a negative integer. b must be a positive integer.
 * Definition: a mod b = c means a = k.b + c, k an integer (+/-/0) s.t. 0 <= c < b
 */
int mymod( int a, int b ) {

  int c;  

  if( a >= 0 )
    return a % b;

  c = a;
  while( c < 0 )
    c += b;

  return c;
}


int imin( int a, int b ) {

  if( a <= b )
    return a;
  return b;
}

time_t tmax( time_t a, time_t b ) {

  if( a >= b )
    return a;
  return b;
}
