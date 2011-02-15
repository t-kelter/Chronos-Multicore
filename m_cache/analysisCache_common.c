// Include standard library headers
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "analysisCache_common.h"


int
logBase2(int n)
{
  DSTART( "logBase2" );

  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
    {
    DOUT("log2() only works for positive power of two values\n");
    DRETURN( -1 );
    }
  while (n >>= 1)
    power++;

  DRETURN( power );
}


char
isInWay(int entry, int *entries, int num_entry)
{
  int i;
  for(i = 0; i < num_entry; i++)
    if(entry == entries[i])
      return 1;

  return 0;
}
