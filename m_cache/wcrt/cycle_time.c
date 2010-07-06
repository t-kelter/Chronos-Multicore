#include "cycle_time.h"

static void 
access_counter(unsigned *hi, unsigned *lo)
{
    /* Get cycle counter */
    asm("rdtsc; movl %%edx,%0; movl %%eax,%1"
	    : "=r" (*hi), "=r" (*lo)
	    : /* No input */
	    : "%edx", "%eax");
}



double
cycle_time(int start_end)
{
  //static int	lo1, hi1, lo2, hi2;
    static unsigned int	lo1, hi1, lo2, hi2;
    unsigned	ulo, uhi, borrow;
    double	t;

    if (start_end == 0) {
	access_counter(&hi1, &lo1);
	return 0;
    }
    else {
	access_counter(&hi2, &lo2);
	ulo = lo2 - lo1;
	borrow = ulo > lo2;
	uhi = hi2 - hi1 - borrow;
	t = (double) uhi * (1 << 30) * 4 + ulo;
	return t;
    }
}
