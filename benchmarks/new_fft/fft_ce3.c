#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#define CACHE_SET 64
#define CACHE_ASSO 1
#define BLK_SIZE 8
//set_shift = logbase2 (blk_rangeJ in byte)
#define SET_SHIFT  5
#define CACHE_SIZE CACHE_SET*CACHE_ASSO*BLK_SIZE

#define BLK_MASK (BLK_SIZE*4-1)
#define SET_MASK  (CACHE_SET-1)
#define SET_BLK(addr) ( (((int)addr)>>(SET_SHIFT))&SET_MASK )



#include "math.h"
//#define MAXSIZE 256
#define MAXSIZE 512
#define MAXWAVES 8

int p1,p2,p3,p4;
int CopyBuf[BLK_SIZE*2];
int BufROJ[BLK_SIZE];
int BufIOJ[BLK_SIZE];
int BufROK[BLK_SIZE];
int BufIOK[BLK_SIZE];

int RealIn[MAXSIZE];
int ImagIn[MAXSIZE];
int RealOut[MAXSIZE];
int ImagOut[MAXSIZE];
int ar[3], ai[3];
//int coeff[MAXWAVES];
//int amp[MAXWAVES];
#define min(a,b) (a<b ? a:b)

#define block_copy(B, A, size) {\
    int _ix;\
    for (_ix = 0; _ix<size; _ix++) {\
        B[_ix] = A[_ix];\
    }\
}

#define  DDC_PI  (3)
#define  PI DDC_PI
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
    int invfft=0;
    int coeff,amp;
    unsigned InverseTransform,NumSamples;
    unsigned NumBits;    /* Number of bits needed to store indices */
    int i, j, k, n;
    int ii, jj, kk;
    unsigned BlockSize, BlockEnd;

    int angle_numerator = 2.0 * DDC_PI;
    int tr, ti;     /* temp real, temp imaginary */
    int rangeJ;

    InverseTransform = 0;
    NumSamples = MAXSIZE;

    for (i=0;i<MAXSIZE;i++) {
        coeff = ((i+39)*91)%1000;
        amp = ((i+71)*83)%1000;
        RealIn[i]=coeff*(int)sin(amp);
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
        int sm2 = (int)sin ( -2 * delta_angle );
        int sm1 = (int)sin ( -delta_angle );
        int cm2 = (int)cos ( -2 * delta_angle );
        int cm1 = (int)cos ( -delta_angle );
        int w = 2 * cm1;
        int temp;

        for ( i=0; i < NumSamples; i += BlockSize )
        {
            ar[2] = cm2;
            ar[1] = cm1;

            ai[2] = sm2;
            ai[1] = sm1;

            rangeJ = min(BlockEnd,BLK_SIZE);/*to avoid mess up addr range*/
            for ( jj=i, n=0; n < BlockEnd; jj+=rangeJ, n+=rangeJ )
            {
                k = j + BlockEnd;

                block_copy(BufROJ,(RealOut+jj),BLK_SIZE);
                block_copy(BufIOJ,(ImagOut+jj),BLK_SIZE);
                block_copy(BufROK,(RealOut+kk),BLK_SIZE);
                block_copy(BufIOK,(ImagOut+kk),BLK_SIZE);
                rangeJ = min(BLK_SIZE, BlockEnd);
                for (j=0; j<rangeJ; j++) {
                    ar[0] = w*ar[1] - ar[2];
                    ar[2] = ar[1];
                    ar[1] = ar[0];

                    ai[0] = w*ai[1] - ai[2];
                    ai[2] = ai[1];
                    ai[1] = ai[0];

                    //RealOut[k] - ai[0]*ImagOut[k];
                    //ImagOut[k] + ai[0]*RealOut[k];
                    tr = ar[0]*BufROK[j] - ai[0]*BufIOK[j];
                    ti = ar[0]*BufIOK[j] + ai[0]*BufROK[j];

                    //RealOut[k] = RealOut[j] - tr;
                    BufROK[j] = BufROJ[j] - tr;
                    //ImagOut[k] = ImagOut[j] - ti;
                    BufIOK[j] = BufIOJ[j] - ti;
                    

                    //RealOut[j] += tr;
                    BufROJ[j] += tr;
                    //ImagOut[j] += ti;
                    BufIOJ[j] += ti;
                }
                block_copy((RealOut+jj),BufROJ,BLK_SIZE);
                block_copy((ImagOut+jj),BufIOJ,BLK_SIZE);
                block_copy((RealOut+kk),BufROK,BLK_SIZE);
                block_copy((ImagOut+kk),BufIOK,BLK_SIZE);
            }
        }

        BlockEnd = BlockSize;
    }

    /*forward FFT only*/
}
#if 0
#define block_copy(B, A, size) {\
    int iC;\
    int i;\
    if ( SET_BLK(B) == SET_BLK(A) ) {\
        if ( SET_BLK(A) == SET_BLK(&CopyBuf[0]) ) {\
            iC=BLK_SIZE;\
        }\
        else iC = 0;\
        for (i=0;i<size;i++) CopyBuf[iC+i] = A[i];\
        for (i=0;i<size;i++) B[i] = CopyBuf[iC+i];\
    }\
    else {\
        for (i=0;i<size;i++) {\
            B[i] = A[i];\
        }\
    }\
}
#endif
