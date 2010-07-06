/*! This is a header file of the Chronos timing analyzer. */

/*! This unit holds utility functions (f.e. for list management) */

#ifndef __CHRONOS_UTIL_H
#define __CHRONOS_UTIL_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Tests for membership in an integer array.
 */
char inList( int element, int *arr, int len );

/*
 * Returns the index of element in arr.
 */
int getIndexInList( int element, int *arr, int len );

/*
 * Inserts element into integer array arr, and updates len.
 */
int enqueue( int element, int **arr, int *len );

/*
 * Inserts element into integer array arr and updates len, if it is not already in.
 */
int enqueueUnique( int element, int **arr, int *len );

/*
 * Inserts element1 into integer array arr1, element2 into arr2, and updates len.
 */
int enqueuePair( int element1, int **arr1, int element2, int **arr2, int *len );

/*
 * Removes and returns the first element of integer array arr, and updates len.
 * *len has to be > 0; no check is done here.
 */
int dequeue( int **arr, int *len );

/*
 * Returns (a mod b) where a may be a negative integer. b must be a positive integer.
 * Definition: a mod b = c means a = k.b + c, k an integer (+/-/0) s.t. 0 <= c < b
 */
int mymod( int a, int b );

int imin( int a, int b );

#endif
