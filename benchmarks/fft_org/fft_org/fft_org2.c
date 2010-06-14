#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

//#include "math.h"
#define MAXSIZE 256
//#define MAXSIZE 512
//#define MAXSIZE 1024
#define MAXWAVES 8

#define PADDLE 0
//int coeff[MAXWAVES];
//int amp[MAXWAVES];

#define  DDC_PI  (3)
#define  PI DDC_PI

/* static double cos(double rad)
{
  double sin();

  return (sin (PI / 2.0 - rad));
}

static double sin(rad)
double rad;
{
  double app;

  double diff;
  int inc = 1;

  //while (rad > 2*PI)
	rad -= 2*PI;
  //while (rad < -2*PI)
    rad += 2*PI;
  app = diff = rad;
   diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  //while(fabs(diff) >= 0.00001) {
    diff = (diff * (-(rad*rad))) /
      ((2.0 * inc) * (2.0 * inc + 1.0));
    app = app + diff;
    inc++;
  //}

  return(app);
} */

unsigned NumberOfBitsNeeded ( unsigned PowerOfTwo )
{
    unsigned i;
    if (PowerOfTwo<2) return 0;
    for ( i=0; i<32 ; i++ )
    {
        if ( PowerOfTwo & (1 << i) )
            return i;
    }
}

unsigned ReverseBits ( unsigned index, unsigned NumBits )
{
    unsigned i, rev;

    for ( i=rev=0; i < NumBits; i++ )
    {
        rev = (rev << 1) | (index & 1);
        index >>= 1;
    }

    return rev;
}


int main(int argc, char *argv[]) {
    int RealIn[MAXSIZE];
    int ImagIn[MAXSIZE];
    int RealOut[MAXSIZE];
    int ImagOut[MAXSIZE];
    int invfft=0;
    int coeff,amp;
    unsigned InverseTransform,NumSamples;
    unsigned NumBits;    /* Number of bits needed to store indices */
    unsigned i, j, k, n;
    unsigned BlockSize, BlockEnd;

    int angle_numerator = 2.0 * DDC_PI;
    int tr, ti;     /* temp real, temp imaginary */


    InverseTransform = 0;
    NumSamples = MAXSIZE;

    for (i=0;i<MAXSIZE;i++) {
        coeff = ((i+39)*91)%1000;
        amp = ((i+71)*83)%1000;
        RealIn[i]=coeff*sin(amp);
        ImagIn[i]=0;
    }
    if ( InverseTransform )
        angle_numerator = -angle_numerator;

    NumBits = NumberOfBitsNeeded ( NumSamples );

    /*
    **   Do simultaneous data copy and bit-reversal ordering into outputs...
    */

    for ( i=0; i < NumSamples; i++ )
    {
        j = ReverseBits ( i, NumBits );
        RealOut[j] = RealIn[i];
        ImagOut[j] = ImagIn[i];
    }

    /*
    **   Do the FFT itself...
    */

    BlockEnd = 1;
    for ( BlockSize = 2; BlockSize <= NumSamples; BlockSize <<= 1 )
    {
        int delta_angle = angle_numerator / (int)BlockSize;
        int sm2 = sin ( -2 * delta_angle );
        int sm1 = sin ( -delta_angle );
        int cm2 = cos ( -2 * delta_angle );
        int cm1 = cos ( -delta_angle );
        int w = 2 * cm1;
        int temp;
        int ar[3], ai[3];

        for ( i=0; i <= NumSamples; i += BlockSize )
        {
            ar[2] = cm2;
            ar[1] = cm1;

            ai[2] = sm2;
            ai[1] = sm1;

            for ( j=i, n=0; n < BlockEnd; j++, n++ )
            {
                ar[0] = w*ar[1] - ar[2];
                ar[2] = ar[1];
                ar[1] = ar[0];

                ai[0] = w*ai[1] - ai[2];
                ai[2] = ai[1];
                ai[1] = ai[0];

                k = j + BlockEnd;
                tr = ar[0]*RealOut[k] - ai[0]*ImagOut[k];
                ti = ar[0]*ImagOut[k] + ai[0]*RealOut[k];

                RealOut[k] = RealOut[j] - tr;
                ImagOut[k] = ImagOut[j] - ti;

                RealOut[j] += tr;
                ImagOut[j] += ti;
            }
        }

        BlockEnd = BlockSize;
    }

    /*forward FFT only*/
}
