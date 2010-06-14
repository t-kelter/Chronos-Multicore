#ifdef PRINT
#include "stdio.h"
#include "stdlib.h"
#endif

#define SIZE 1024*32
int array0[SIZE];
int array1[SIZE];

#ifdef PRINT
FILE *out;
void printArray(int a[], int size) {
    int i;
    for (i=0;i<size;i++) {
        if (a[i] == 0x0fffffff) break;
        fprintf(out,"%4d ",a[i]);
    }
    fprintf(out,"\n");
}
#endif

//iterative version of merge sort, (so that it is) cache inefficient
void merge_sort(int size) {
    int num,runsize, low, high,mid;
    int i,j,k;

    runsize=1;
    while (runsize<size) {
        num = 0;
        while (num*runsize<size) {
            low = num*runsize;
            mid = (num+1)*runsize-1;
            high= (num+2)*runsize-1;
            num+= 2;

            #ifdef PRINT
            fprintf(out,"Merging Array: %d %d\n",low,high);
            printArray(array0,SIZE);
            fflush(out);
            #endif
            i=low;j=mid+1;
            k=low;
            while (i<=mid && j<=high) {
                if (array0[i]<=array0[j]) {
                    array1[k]=array0[i];
                    i++;
                }
                else { //array0[i]>array0[j]
                    array1[k]=array0[j];
                    j++;
                }
                k++;
            }
            while (i<=mid) {
                array1[k]=array0[i];
                k++;i++;
            }
            while (j<=high) {
                array1[k]=array0[j];
                k++;j++;
            }
            k=low;
            for (k=low;k<=high;k++) {
                array0[k]=array1[k];
            }
            #ifdef PRINT
            printArray(array0,SIZE);
            fflush(out);
            #endif
        }
        runsize = runsize*2;
    }
}


int main() {
    int i,j,k;
    int cnt;

    //init
    cnt = 1;
    for (i=0;i<SIZE;i++) {
       if (i%2==0) array0[i] = i; else array0[SIZE-i]=i;
       //array0[i]=i;
       //array0[i] = SIZE-i;
       //array0[i] = (i+39)*41%2059;
       //array0[i] = (i+71)*47 % 14279;
       //array0[i] = (i+51)*53 %1551;
    }
    #ifdef PRINT
    out = fopen("merging.txt","w");
    fprintf(out,"Original Array: ");
    printArray(array0,SIZE);
    #endif

    merge_sort(SIZE-1);
    
    #ifdef PRINT
    fprintf(out,"Merged Array: ");
    printArray(array1,SIZE);
    fclose(out);
    #endif

    return 0;
}
