#define DEBUG 0
#if DEBUG
#include "stdio.h"
FILE* f;
#endif

#define CACHE_SET 32
#define CACHE_ASSO 1
#define BLOCK_SIZE 8
//set_shift = logbase2 (blk_size in byte)
#define SET_SHIFT  5
#define CACHE_SIZE CACHE_SET*CACHE_ASSO*BLOCK_SIZE
#define BLK_MASK (BLOCK_SIZE*4-1)
#define SET_MASK  (CACHE_SET-1)
#define SET_BLK(addr) ( (((int)addr)>>(SET_SHIFT))&SET_MASK )

#define SIZE 512*32
#define MAXVALUE 0xfffffff

#define bounded_copy(B, iB, A, iA, size) {\
    int _iX;\
    for (_iX=0; _iX<size; _iX++) B[iB+_iX] = A[iA+_iX];\
}
/*
#define bounded_copy(B, A, size) {\
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
*/

#define BLK_SIZE (CACHE_SIZE/4)
#define min(a,b) ( a<b ? a : b)
int BufI[BLK_SIZE];
int BufJ[BLK_SIZE];
int BufOut[BLK_SIZE];

int arrayIn[SIZE];
int arrayOut[SIZE];

void main() {
    int ii,jj,kk;
    int i,j,k,h;
    int min;
    int low,mid,high;
    int runsize,num;
    for (i=0;i<SIZE;i++) {
       //if (i%2==0) arrayIn[i] = i; else arrayIn[SIZE-i]=i;
       arrayIn[i]=i;
       //arrayIn[i] = SIZE-i;
       //arrayIn[i] = (i+39)*41%2059;
       //arrayIn[i] = (i+71)*47 % 14279;
       //arrayIn[i] = (i+51)*53 %1551;
    }
    ii=0;
    while (ii<SIZE) {
        bounded_copy(BufI,0,arrayIn,ii,BLK_SIZE);
        i=0;j=0;min=0;k=0;
        runsize=1;
        while (runsize<BLK_SIZE) {
            num=0;
            while (num*runsize<BLK_SIZE) {
                low = runsize*num;num++;
                mid = runsize*num-1;num++;
                high= runsize*num-1;
                i=low;j=mid+1;k=low;
                while (i<=mid) {
                    if(j<=high) {
                    if (i<=mid && j<=high) {
                        if (BufI[i]<=BufI[j]) min = BufI[i++];
                        else min = BufI[j++];
                    }
                    else if (i<=mid) min = BufI[i++];
                    else min = BufI[j++];
                    BufOut[k++] = min;
                }
                }
                for (i=low;i<=high;i++) BufI[i] = BufOut[i];
            }
            runsize*=2;
        }
        bounded_copy(arrayIn,ii,BufI,0,BLK_SIZE);
        ii+=BLK_SIZE;
    }

    ii=0;jj=0;kk=0;
    i=0;j=0;k=1;
    runsize=BLK_SIZE;
    while (runsize<SIZE) {
        num=0;kk=0;
        while(num*runsize<SIZE && kk<SIZE && jj< SIZE) {
            low = min(SIZE,runsize*num);num++;
            mid = runsize*num;num++;
            high= runsize*num;
            ii=low; jj=mid;kk=low; 
            min=0;i=BLK_SIZE;j=BLK_SIZE;k=0;
            while (min!=MAXVALUE) {
                BufI[0]=0;
                if ( (ii<mid||jj<high) ) {
                if (ii<mid-BLK_SIZE && i==BLK_SIZE) {
                    bounded_copy(BufI,0,arrayIn,ii,BLK_SIZE);
                    ii+=BLK_SIZE;i=0;
                }
                else if (i==BLK_SIZE){
                    bounded_copy(BufI,0,arrayIn,ii,mid-ii);
                    BufI[mid-ii+1]=MAXVALUE;i=0;ii=mid;
                }
                if (jj<high-BLK_SIZE && j==BLK_SIZE) {
                    bounded_copy(BufJ,0,arrayIn,jj,BLK_SIZE);
                    jj+=BLK_SIZE;j=0;
                }
                else if (j==BLK_SIZE){
                    bounded_copy(BufJ,0,arrayIn,jj,high-jj);
                    BufJ[high-jj+1]=MAXVALUE;j=0;jj=high;
                }
                while (i<BLK_SIZE && j<BLK_SIZE
                        && k<BLK_SIZE && min<MAXVALUE) {
                    if (BufI[i]<BufJ[j]) min = BufI[i++];
                    else min = BufJ[j++];
                    if (min<MAXVALUE) BufOut[k++]=min;
                }
                if (min==MAXVALUE || k==BLOCK_SIZE) {
                    bounded_copy(arrayOut,kk,BufOut,0,BLK_SIZE);
                    kk+=BLK_SIZE;k=0;
                }
                }
            }
        }
        runsize*=2;
    }
}





























/*
#define bounded_copy(B, A, size) {\
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
*/

