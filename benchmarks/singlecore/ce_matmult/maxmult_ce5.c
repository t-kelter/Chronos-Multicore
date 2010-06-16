#ifdef PRINT
#include "stdio.h"
#include "stdlib.h"
FILE *f;
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

#define block_copy(B, A, size) {\
    int iC;\
    int iX;\
    if ( SET_BLK(B) == SET_BLK(A) ) {\
        if ( SET_BLK(A) == SET_BLK(&CopyBuf[0]) ) {\
            iC=BLOCK_SIZE;\
        }\
        else iC = 0;\
        for (iX=0;iX<size;iX++) CopyBuf[iC+iX] = A[iX];\
        for (iX=0;iX<size;iX++) B[iX] = CopyBuf[iC+iX];\
    }\
    else {\
        for (iX=0;iX<size;iX++) {\
            B[iX] = A[iX];\
        }\
    }\
}
#define min(a,b) (a>b) ? b : a
#define N 32
int CopyBuf[BLOCK_SIZE*2];
int BufA[BLOCK_SIZE];
int BufC[BLOCK_SIZE];
int BufB[BLOCK_SIZE*8];
int A[N][N],B[N][N],C[N][N];

int main() {
    int i,j,k;
    int kk,jj;
    /*just assume aligned access*/ // int kbound, jbound;
    #ifdef PRINT
    f = fopen("array.txt","w");
    #endif
    
    for (i=0;i<N;i++) {
        for (j=0;j<N;j++) {
            A[i][j] = i*N+j;
        }
    }
    for (i=0;i<N;i++) {
        for (j=0;j<N;j++) {
            B[i][j] = j*N+i;
        }
    }
    for (i=0;i<N;i++) {
        for (j=0;j<N;j++) {
            C[i][j] = 0;
        }
    }

    for ( kk=0; kk<N; kk+=BLOCK_SIZE) {
        for ( jj=0; jj<N; jj+=BLOCK_SIZE) {
            //block
            #ifdef PRINT
            fprintf(f,"Block: i[0-%d],j[%d-%d],k[%d-%d]\n",
                N-1,jj,jj+BLOCK_SIZE-1, kk, kk+BLOCK_SIZE-1);fflush(f); 
            #endif
            for (k=0;k<BLOCK_SIZE;k++) {
                block_copy((BufB+k*BLOCK_SIZE),(B[k+kk]+jj),BLOCK_SIZE);
            }
            #ifdef PRINT
            for (i=0;i<BLOCK_SIZE;i++) {
                for (j=0;j<BLOCK_SIZE;j++) fprintf(f,"%d ",BufB[j+i*BLOCK_SIZE]);
                fprintf(f,"\n");
            }
            #endif
            for (i=0; i<N; i++) {
                //copy to buffer
                block_copy(BufA,(A[i]+kk),BLOCK_SIZE);
                block_copy(BufC,(C[i]+jj),BLOCK_SIZE);
                #ifdef PRINT
                for (j=0;j<BLOCK_SIZE;j++) fprintf(f,"%d ",BufA[j]);fprintf(f,"| A[%d][%d]\n",i,kk);
                for (j=0;j<BLOCK_SIZE;j++) fprintf(f,"%d ",BufC[j]);fprintf(f,"| C[%d][%d]\n",i,jj);
                #endif
                //Buf[sA+(k-kk)]=A[i][k];
                for (k=0; k<BLOCK_SIZE; k++) {
                    for (j=0; j<BLOCK_SIZE; j++) {
                        //manipulate in buffer
                        //C[i][j] += A[i][k]*B[k][j]
                        BufC[j] += BufA[k]*BufB[k*BLOCK_SIZE+j];
                    }
                }
                block_copy((C[i]+jj),BufC,BLOCK_SIZE);
            }
        }
    }
    #ifdef PRINT
    for (i=0;i<N;i++) {
        for (j=0; j<N; j++) {
            fprintf(f,"%d ",A[i][j]);
        }
        fprintf(f,"\n");
    }
    for (i=0;i<N;i++) {
        for (j=0; j<N; j++) {
            fprintf(f,"%d ",B[i][j]);
        }
        fprintf(f,"\n");
    }
    for (i=0;i<N;i++) {
        for (j=0; j<N; j++) {
            fprintf(f,"%d ",C[i][j]);
        }
        fprintf(f,"\n");
    }
    #endif
    return 0;
}
