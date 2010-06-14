/*
 * Math library emulation functions
 *  
 * Copyright (C) 2007 IRIT - UPS <casse@irit.fr>
 *
 * This file is part of papabench.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with papabench; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA. 
 *
 */
#include <math.h>

double pp_atan2(double x, double y)
{
	double coeff_1 = M_PI/4;
	double coeff_2 = 3*coeff_1;
	double abs_y = fabs(y)+1e-10;
	double angle, r;
	if(x>0)
	{
		r = (x - abs_y)/(x + abs_y);
		angle = coeff_1 - coeff_1*r;
	}
	else
	{
		r = (x + abs_y)/(abs_y - x);
		angle = coeff_2 - coeff_1*r;
	}
	if(y<0)
		return (-angle);
	else
		return angle;
}


/* Calculates sin(x), angle x must be in rad.
 * Range: -pi/2 <= x <= pi/2
 * Precision: +/- .000,000,005
 */
 
double pp_sin(double x)
{
	double xi, y, q, q2;
	int sign;

	xi = x; sign = 1;
	while (xi < -1.57079632679489661923) xi += 6.28318530717958647692;
	while (xi > 4.71238898038468985769) xi -= 6.28318530717958647692;
	if (xi > 1.57079632679489661923) {
		xi -= 3.141592653589793238462643;
		sign = -1;
	}
	q = xi / 1.57079632679; q2 = q * q;
	y = ((((.00015148419  * q2
	      - .00467376557) * q2
	      + .07968967928) * q2
	      - .64596371106) * q2
	      +1.57079631847) * q;
	return(sign < 0? -y : y);
}

double pp_sqrt(double x)
{
	typedef union {
		double d;
		unsigned u[4];
	} DBL;
	double re, p, q;
	unsigned ex; /*exponens*/
	DBL *xp;
	xp = (DBL*)&x;
	ex = xp->u[3] & ~0100017; /*save exponens*/
	re = ex & 020 ? 1.414235623730950488 : 1.0;
	ex = (ex - 037740) >> 1;
	ex &= ~0100017;
	xp->u[3] &= ~0177760; /*erase exponens and sign*/
	xp->u[3] |= 037740; /*arrange for mantissa in range 0.5 to 1.0*/
	if (xp->d < 0.7071067812) {  /* multiply by sqrt(2) if mantissa < 0.7 */
		xp->d *= 1.4142135623730950488;
		if (re > 1.0) re = 1.189207115002721;
		else re = 1.0 / 1.18920711500271;
	}
	p =	      0.54525387389085 +   /* Polynomial approximation */
	    xp->d * (13.65944682358639 +   /* from: COMPUTER APPROXIMATIONS*/
	    xp->d * (27.090122712655   +   /* Hart, J.F. et al. 1968 */
	    xp->d *   6.43107724139784));
	q =	      4.17003771413707 +
	    xp->d * (24.84175210765124 +
	    xp->d * (17.71411083016702 +
	    xp->d ));
	xp->d = p / q;
	xp->u[3] = xp->u[3] + (ex & ~0100000);
	xp->d *= re;
	return(xp->d);
}
