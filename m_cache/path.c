/*
 * Path manipulation functions. Support for DAG-based infeasible path detection.
 */


int freePath( int bbid, int ptid ) {

  if( pathlist[bbid][ptid] ) {
    free( pathlist[bbid][ptid]->bb_seq );
    free( pathlist[bbid][ptid]->branch_eff );
    free( pathlist[bbid][ptid]->branch_dir );
  }
  free( pathlist[bbid][ptid] );

  return 0;
}


int freePathsInNode( int bbid ) {

  int i;

  for( i = 0; i < num_paths[bbid]; i++ )
    freePath( bbid, i );
  free( pathlist[bbid] );
  return 0;
}

/*
char identical( path *p1, path *p2 ) {

  int i;

  if( p1->branch_len != p2->branch_len )
    return 0;

  if( p1->branch_len == 0 )
    return 1;

  for( i = 0; i < p1->branch_len; i++ )
    if( p1->branch_eff[i] != p2->branch_eff[i] ||
	p1->branch_dir[i] != p2->branch_dir[i] )
      return 0;

  return 1;
}
*/


/*
 * Returns 1 if p1 and p2 have exactly the same set of branches, 0 otherwise.
 */
char identicalConflict( path *p1, path *p2 ) {

  int i;

  for( i = 0; i < p1->branch_len; i++ )
    if( p1->branch_eff[i] != p2->branch_eff[i] ||
	p1->branch_dir[i] != p2->branch_dir[i] )
      return 0;

  return 1;
}


/*
 * Returns 1 if p contains branch br taking direction dir, 0 otherwise.
 */
char hasEdge( path *p, branch *br, char dir ) {

  int i;

  for( i = 0; i < p->branch_len; i++ ) {
    if( p->branch_eff[i] == br ) {
      if( p->branch_dir[i] == dir )
	return 1;
      else
	// opposite direction
	return 0;
    }
  }
  return 0;
}


/*
 * Returns 1 if p1's conflict list is a subset of p2's conflict list, 0 otherwise.
 * Note that the identical relation is included.
 */
char subsetConflict( path *p1, path *p2 ) {

  int i;

  if( p1->branch_len == 0 ) // emptyset is subset of any set
    return 1;

  if( p1->branch_len > p2->branch_len )
    return 0;

  if( p1->branch_len == p2->branch_len )
    // slightly more efficient
    return identicalConflict( p1, p2 );

  // checks if all branches in p1 occur in p2
  for( i = 0; i < p1->branch_len; i++ ) {
    if( p1->branch_eff[i] == NULL )
      continue;
    if( !hasEdge( p2, p1->branch_eff[i], p1->branch_dir[i] ))
      return 0;
  }
  return 1;
}


/*
 * Sorts pathlist according to increasing cost, then decreasing number of branches.
 */
int sortPath( path **pathlist, int num_paths ) {

  int  i;
  char changed;
  path *temp;

  changed = 1;
  while( changed ) {

    changed = 0;
    for( i = 0; i < num_paths - 1; i++ ) {

      if( pathlist[i]->cost > pathlist[i+1]->cost ||
	  ( pathlist[i]->cost == pathlist[i+1]->cost &&
	    pathlist[i]->branch_len < pathlist[i+1]->branch_len )) {

	// swap
	temp          = pathlist[i];
	pathlist[i]   = pathlist[i+1];
	pathlist[i+1] = temp;

	changed = 1;
      }
    }
  }
  return 0;
}


/*
int pathCompare( path *p1, path *p2 ) {

  int i;
  for( i = 0; i < p1->branch_len && i < p2->branch_len; i++ ) {
    if( p1->branch_eff[i]->bb->bbid < p2->branch_eff[i]->bb->bbid ) return -1;
    if( p1->branch_eff[i]->bb->bbid > p2->branch_eff[i]->bb->bbid ) return 1;
    if( p1->branch_dir[i] < p2->branch_dir[i] ) return -1;
    if( p1->branch_dir[i] > p2->branch_dir[i] ) return 1;
  }
  if( p1->branch_len > i ) return 1;
  if( p2->branch_len > i ) return -1;

  return 0;
}


int quickSort( path **pathlist, int lo, int hi ) {

  int i, j, mid;
  path *temp;

  mid = ( lo + hi ) / 2;
  i = lo;
  j = hi;

  do {
    while( i <= hi && pathCompare( pathlist[i], pathlist[mid] ) < 0 )
      i++;
    while( j >= lo && pathCompare( pathlist[j], pathlist[mid] ) > 0 )
      j--; 

    if( i <= j ) {
      // swap
      temp = pathlist[i];
      pathlist[i] = pathlist[j];
      pathlist[j] = temp;
      i++; j--;
    }
  } while( i <= j );

  if( lo < j ) quickSort( pathlist, lo, j );
  if( i < hi ) quickSort( pathlist, i, hi );

  return 0;
}
*/

/*
 * Copies p2's block and branch sequences into p1.
 */
int copySeq( path *p1, path *p2 ) {

  int i;
  for( i = 0; i < p2->bb_len; i++ )
    p1->bb_seq[i] = p2->bb_seq[i];
  
  for( i = 0; i < p2->branch_len; i++ ) {
    p1->branch_eff[i] = p2->branch_eff[i];
    p1->branch_dir[i] = p2->branch_dir[i];
  }
  return 0;
}


/*
 * Insert edge <br,dir> into pt's branch sequence,
 * maintaining increasing order of branch index.
 */
int sortedInsertBranch( path *pt, branch *br, char dir ) {

  int i;

  i = pt->branch_len - 1;
  while( i > 0 && pt->branch_eff[i-1]->bb->bbid > br->bb->bbid ) {
    pt->branch_eff[i] = pt->branch_eff[i-1];
    pt->branch_dir[i] = pt->branch_dir[i-1];
    i--;
  }
  pt->branch_eff[i] = br;
  pt->branch_dir[i] = dir;

  return 0;
}


/*
 * Removes the branch at index bri of pt's branch sequence.
 */
int removeBranch( path *pt, int bri ) {

  int i;

  // shift
  for( i = bri; i < pt->branch_len - 1; i++ ) {
    pt->branch_eff[i] = pt->branch_eff[i+1];
    pt->branch_dir[i] = pt->branch_dir[i+1];
  }
  pt->branch_len--;

  return 0;
}


/*
 * Returns the index at which br is found in the list branch_eff,
 * -1 if not found.
 */
int findBranch( branch *br, branch **branch_eff, int branch_len ) {

  int i;
  for( i = 0; i < branch_len; i++ )
    if( branch_eff[i] == br )
      return i;
  return -1;
}
