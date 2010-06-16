#ifdef PRINT
#include "stdio.h"
#include "stdlib.h"
#endif

#define N 8
#define CACHE_SET 32
#define CACHE_ASSO 1
#define BLOCK_SIZE 8
//set_shift = logbase2 (blk_size in byte)
#define SET_SHIFT  5
#define CACHE_SIZE CACHE_SET*CACHE_ASSO*BLOCK_SIZE

#define BLK_MASK (BLOCK_SIZE*4-1)
#define SET_MASK  (CACHE_SET-1)

//#define min(a,b) (a>b) ? b : a

typedef struct matrix_range {
    int top;
    int left;
    int row;
    int col;
} range;
typedef struct int_matrix {
    int **m; 
} matrix;

#ifdef PRINT
FILE *out;
#endif


int *buf;
int Buf[CACHE_SIZE];
int A[N][N],B[N][N],C[N][N];

int min(int a, int b) {
    if (a>b) return b;
    else return a;
}

#define SET_BLK(addr) ( ((addr)>>(SET_SHIFT))&SET_MASK )
int CopyBuf[BLOCK_SIZE*2];
void block_copy(int Buf[], int iB, int A[], int iA, int size) {
    int i;
    #ifdef PRINT
    //fprintf(out,"block_copy: Buf %d A %d, size %d\n",iB, iA, size);fflush(out);
    //fprintf(out,"Addr buf:%08x Addr A:%08x\n",( (int)&(Buf[iB]) ), (int)(&A[iA])); fflush(out);
    #endif
    if ( SET_BLK( ((int)&(Buf[iB])) ) == SET_BLK( ((int)&(A[iA])) )) {//Buf[iB] && A[iA] mapped to the same block
        #ifdef PRINT
        fprintf(out,"Set buf:%4d Set A:%4d\n",SET_BLK((int)&(Buf[iB])) , SET_BLK((int)(&A[iA])) ); fflush(out);
        fprintf(out,"Block Conflict\n");fflush(out);
        #endif
        if ( SET_BLK( ((int)&(A[iA]) )) == SET_BLK( ((int)&(CopyBuf[0])) )) {
            for (i=0;i<size;i++) {
                CopyBuf[BLOCK_SIZE+i] = A[iA + i];
            }
            for (i=0;i<size;i++) {
                Buf[iB+i] = CopyBuf[i+BLOCK_SIZE];
            }
        }
        else {//same_blk( Buf[iB],A[iA] ) && diff_blk( A[iA],CopyBuf[0] )
            for (i=0;i<size;i++) {
                CopyBuf[i] = A[iA+i];
            }
            for (i=0;i<size;i++) {
                Buf[iB+i] = CopyBuf[i];
            }

        }
    }
    else {//Buf[iB] && A[iA] mapped to the different block
        for (i=0;i<size;i++) {
            Buf[iB+i] = A[iA+i];
        }
    }
}
#if 0
void block_copy_add(int Buf[], int iB, int A[], int iA, int size) {
    int i;
    if ( SET_BLK( ((int)&(Buf[iB])) ) == SET_BLK( ((int)&(A[iA])) )) {//Buf[iB] && A[iA] mapped to the same block
        if ( SET_BLK( ((int)&(A[iA]) )) == SET_BLK( ((int)&(CopyBuf[0])) )) {
            for (i=0;i<size;i++) {
                CopyBuf[BLOCK_SIZE+i] += A[iA + i];
            }
            for (i=0;i<size;i++) {
                Buf[iB+i] += CopyBuf[i+BLOCK_SIZE];
            }
        }
        else {//same_blk( Buf[iB],A[iA] ) && diff_blk( A[iA],CopyBuf[0] )
            for (i=0;i<size;i++) {
                CopyBuf[i] += A[iA+i];
            }
            for (i=0;i<size;i++) {
                Buf[iB+i] += CopyBuf[i];
            }

        }
    }
    else {//Buf[iB] && A[iA] mapped to the different block
        for (i=0;i<size;i++) {
            Buf[iB+i] += A[iA+i];
        }
    }
}
#endif
void matrix_multiplication(int n) {
    int i,j,k;
    int kk,jj;
    int sA,sC;
    int kbound, jbound;
    int r;
    int i1,j1;

    int iBuf, jBuf;

    for ( kk=0; kk<n; kk+=BLOCK_SIZE) {
        for ( jj=0; jj<n; jj+=BLOCK_SIZE) {
            //block
            //fprintf(out,"Block: i[0-%d],j[%d-%d],k[%d-%d]\n",n-1,jj,jj+BLOCK_SIZE-1, kk, kk+BLOCK_SIZE-1);fflush(out); 
            kbound = min(kk+BLOCK_SIZE,n);
            jbound = min(jj+BLOCK_SIZE,n);
            for (k=kk;k<kbound;k++) {
                block_copy(Buf,(k-kk)*BLOCK_SIZE,B[k],jj,jbound-jj);
            }
            sA = BLOCK_SIZE*BLOCK_SIZE;
            sC = sA + BLOCK_SIZE;
            for (i=0; i<n; i++) {
                //copy to buffer
                block_copy(Buf,sA,A[i],kk,kbound-kk);
                block_copy(Buf,sC,C[i],jj,jbound-jj);
                //Buf[sA+(k-kk)]=A[i][k];
                for (k=0; k<kbound-kk; k++) {
                    for (j=0; j<jbound-jj; j++) {
                        //manipulate in buffer
                        //C[i][j] += A[i][k]*B[k][j]
                        Buf[sC+j] += Buf[sA+k]*Buf[k*BLOCK_SIZE+j];
                    }
                }
                block_copy(C[i],jj,Buf,sC,jbound-jj);
                //block_copy_add(C[i],jj,Buf,sC,jbound-jj);
            }
        }
    }
}

int main() {
    //matrix A,B,C;
    range aR, bR, cR;
    int i,j,k;
    
    #ifdef PRINT
    out = fopen("maxmult1.txt","w");
    #endif
    
    #ifdef PRINT
    block_copy(Buf,0,C[0],0,8);
    for (i=0;i<8;i++) {
        fprintf(out,"buf[%d]: addr:%08x set:%4d\n",i,( (int)&(Buf[i]) ), SET_BLK((int)&(Buf[i])) ); fflush(out);
    }
    for (i=0;i<8;i++) {
        fprintf(out,"A[0][%d]: addr:%08x set:%4d\n",i,( (int)&(A[0][i]) ), SET_BLK((int)&(A[0][i])) ); fflush(out);
    }
    for (i=0;i<8;i++) {
        fprintf(out,"B[0][%d]: addr:%08x set:%4d\n",i,( (int)&(B[0][i]) ), SET_BLK((int)&(B[0][i])) ); fflush(out);
    }
    for (i=0;i<8;i++) {
        fprintf(out,"C[0][%d]: addr:%08x set:%4d\n",i,( (int)&(C[0][i]) ), SET_BLK((int)&(C[0][i])) ); fflush(out);
    }
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
    aR.top=0; aR.left=0; aR.row=N; aR.col=N;
    #ifdef TMP
    fprintf(out,"Initial matrix A\n");fflush(out);
    printMatrix(&A,aR);
    fprintf(out,"Initial matrix B\n");fflush(out);
    printMatrix(&B,aR);
    #endif


    matrix_multiplication(N);

    #ifdef TMP
    fprintf(out,"Result matrix C\n");fflush(out);
    printMatrix(&C,aR);
    #endif

                #if 0
                fprintf(out,"A[%d][%d-%d]:\n",i,kk,kbound);
                for (i1=0;i1<kbound-kk;i1++) {
                    fprintf(out," %4d",Buf[sA+i1]);
                }
                fprintf(out,"\n");
                #endif
    return 0;
}
