#ifdef PRINT
#include "stdio.h"
#include "stdlib.h"
#endif

#define N 32
int A[N][N],B[N][N],C[N][N];
void matrix_multiplication(int n) {
    int i,j,k;

    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            for (k=0; k<n; k++) {
                C[i][j] += A[i][k]*B[k][j];
            }
        }
    }
}

int main() {
    int i,j,k;
    
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
    matrix_multiplication(N);
    return 0;
}
