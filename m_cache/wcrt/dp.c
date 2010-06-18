#include <stdio.h>

double getval( int row, int col );

/*
 * Dynamic programming.
 */
// int dpmin( int N, int S ) {
int dpmin( int N, int S, double **val ) {

  int i, w, k, rem;
  int **dec;

  double **min;
  double new;

  min = (double**) malloc( (N+1) * sizeof(double) );
  dec = (int**) malloc( (N+1) * sizeof(int) );
  for( i = 0; i <= N; i++ ) {
    min[i] = (double*) malloc( (S+1) * sizeof(double) );
    dec[i] = (int*) malloc( (S+1) * sizeof(int) );
  }

  // init
  for( w = 0; w <= S; w++ )
    min[0][w] = 0;

  // calculation
  for( i = 1; i <= N; i++ ) {
    for( w = 0; w <= S; w++ ) {

      dec[i][w] = -1;

      for( k = 0; k <= w; k++ ) {
	if( val[i-1][k] == -1 || min[i-1][w-k] == -1 )
	  // illegal segment size (must be in the power of 2)
	  continue;

	// new = getval( i, k ) + min[i-1][w-k];
	// note that index for val starts from 0 for first element
	new = val[i-1][k] + min[i-1][w-k];

	if( dec[i][w] == -1 || min[i][w] >= new ) {
	  min[i][w] = new;
	  dec[i][w] = k;
	}
      }
      if( dec[i][w] == -1 )
	min[i][w] = -1;
      // printf( "[%d][%d] %Lf (%d)\n", i, w, min[i][w], dec[i][w] );
    }
  }
  printf( "Optimal value: %f\n", min[N][S] );

  // trace the solution set
  rem = S;
  for( i = N; i > 0; i-- ) {
    /*
    // printf( "Decision at %d: %d (%f)\n", i, dec[i][rem], getval( i-1, dec[i][rem] ));
    printf( "Decision at %d: %d (%f)\n", i, dec[i][rem], val[i-1][dec[i][rem]] );
    */
    segshare[i-1] = dec[i][rem];
    rem -= dec[i][rem];
  }
  printf( "\n" );


  for( i = 0; i <= N; i++ ) {
    free( min[i] );
    free( dec[i] );
  }
  free( min );
  free( dec );

  return 0;
}


/*
int main( int argc, char *argv[] ) {

  int N, S, i, w;
  double **val;

  FILE *f;

  if( argc < 2 )
    printf( "Usage: ./dp <inputfile>\n" ), exit(1);

  f = wcrt_openfile( argv[1], "r" );

  fscanf( f, "%d %d", &N, &S );
  // printf( "N: %d   S: %d\n", N, S );

  val = (double**) malloc( N * sizeof(double*) );
  for( i = 0; i < N; i++ )
    val[i] = (double*) malloc( (S+1) * sizeof(double) );

  for( i = 0; i < N; i++ )
    for( w = 0; w <= S; w++ )
      fscanf( f, "%lf", &(val[i][w]) );

  fclose( f );

  // for( i = 0; i <= N; i++ )
  //   for( w = 0; w <= S; w++ )
  //     printf( "val[%d][%d]: %lf\n", i, w, val[i][w] );

  dpmin( N, S, val );

  return 0;
}
*/
