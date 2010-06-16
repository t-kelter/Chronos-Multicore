#define PRINT 0
#if PRINT
#include "stdio.h"
#include "stdlib.h"
#endif

//suppose array size=MERGE_K^2*CACHE_SIZE/2, CACHE_SIZE = 2^x for simplicity
#define SIZE 2048
#define CACHE_SET 32
#define ASSO 1
//size in integer = 4 bytes
#define BLOCK_SIZE 8
//set_shift = logbase2 (blk_size in byte)
#define SET_SHIFT  5
#define CACHE_SIZE CACHE_SET*ASSO*BLOCK_SIZE
#define BLK_MASK (BLOCK_SIZE*4-1)
#define SET_MASK  (CACHE_SET-1)

//k: degree of merge,in IO efficient theoretically K=MEM_SIZE/BLOCK_SIZE, practically 4
#define MERGE_K 4

#if PRINT
FILE *out;
void printArray(int *a, int low, int high) {
    int i;
    for (i=low;i<=high;i++) {
        if (a[i] == 0x0fffffff) break;
        fprintf(out,"%5d ",a[i]);
    }
    fprintf(out,"\n");
}
#endif

int Buf[CACHE_SIZE]; //CACHE_SIZE contiguous memory address for buffering
int mem0[SIZE],mem1[SIZE];
int *array0, *array1;

int outIndex;

typedef struct int_pair{
    int val1;
    int val2;
} pair;
pair priorityQ[MERGE_K];

/* Bounded copy 1 block from B[iB] to A[iA]                 */
/* Assumption                                               */
/* - size <= BLOCK_SIZE                                     */
/* - A[iA+size] don't exceed block boundary                 */
/* - B[iB+size] don't exceed block boundary                 */
/* May require wrapper to bounded copy multiple blocks      */
#define SET_BLK(addr) ( ((addr)>>(SET_SHIFT))&SET_MASK )
int CopyBuf[BLOCK_SIZE*2];
void bounded_copy(int* A, int iA, int* B, int iB, int size) {
    int i;
    if ( SET_BLK( ((int)&(A[iA])) ) == SET_BLK( ((int)&(B[iB])) )) {
        if ( SET_BLK( ((int)&(B[iB]) )) == SET_BLK( ((int)&(CopyBuf[0])) )) {
        //A[iA] & B[iB] & CopyBuf[0] mapped to the same set -> copy via set (CopyBuf[0]+1)
            for (i=0;i<size;i++) {
                CopyBuf[BLOCK_SIZE+i] = B[iB + i];
            }
            for (i=0;i<size;i++) {
                A[iA+i] = CopyBuf[i+BLOCK_SIZE];
            }
        }
        else {//A[iA] & B[iB] mapped to the same set -> copy via CopyBuf[0]
            for (i=0;i<size;i++) {
                CopyBuf[i] = B[iB+i];
            }
            for (i=0;i<size;i++) {
                A[iA+i] = CopyBuf[i];
            }

        }
    }
    else {//Buf[iB] && A[iA] mapped to the different set
        for (i=0;i<size;i++) {
            A[iA+i] = B[iB+i];
        }
    }
}

//Priority queue via binary heap
void enqueue(int val, int src, int *Val, int *Src, int *size) {
    int i;
    int tmp; //tmpVal, tmpSrc;
    i= ++*size;
    Val[i]=val;
    Src[i]=src;
    while ( (i>0) && (Val[i/2]>Val[i]) ) { 
       tmp = Val[i/2];
       Val[i/2]=Val[i];
       Val[i]=tmp;
       tmp = Src[i/2];
       Src[i/2]=Src[i];
       Src[i]=tmp;
       i/=2;
    }
}
void dequeue(int *val, int *src, int* Val,int *Src,int *size) {
    int i,j;
    int tmp;

    *val = Val[0];
    *src = Src[0];
    Val[0] = Val[*size];
    Src[0] = Src[*size];
    *size = *size-1;
    i=0;
    j=2*i+1;
    while (j<=*size) {
        if (j+1<=*size && Val[j]>Val[j+1]) {
            j++;    
        }
        if (Val[i] > Val[j]) {
            tmp = Val[j];
            Val[j] = Val[i];
            Val[i] = tmp;
            tmp = Src[j];
            Src[j] = Src[i];
            Src[i] = tmp;
        }
        else break;
        j=2*i+1;
    }
}

void merge_sort(int size) {
    int bigrunnum, bigrunsize;
    int low, high, mid;
    int runsize;
    int qsize;
    int mergenum, start;
    int i,j,k;
    int iBuf;
    int val, runNo;

    /* pointer that mark portion of the Buf*/
    int *input, *output;
    int *inBuf[MERGE_K], *outBuf;
    int *runIndex;
    int *heapVal, *heapRunNo;         //
    int next;                       //use to allocate the Buf

    /* First phase: sorting individual run */
    /* Divide large array into several big runs, each size CACHE_SIZE/2 */
    /* Merge sort from src [0..CACHE_SIZE/2] to dst [CACHE_SIZE/2..CACHE_SIZE */
    /* Since Buf is inside the cache, there should be no cache miss */
    /* After finish, copy [0..CACHE_SIZE/2] back to memory */
    input = (int*)(&Buf[0]);
    output= (int*)(&Buf[CACHE_SIZE/2]);
    bigrunsize = CACHE_SIZE/2;
    bigrunnum = 0;
    while (bigrunnum*bigrunsize<size) {
        start = bigrunnum*bigrunsize;
        for (iBuf=0;iBuf<bigrunsize;iBuf+=BLOCK_SIZE) {
            bounded_copy(input,iBuf,array0,start+iBuf,BLOCK_SIZE);
        }
        #if 0
        fprintf(out,"Sorting BigRun: ");
        printArray(input,0,bigrunsize-1);
        #endif
        runsize=1;
        while (runsize<bigrunsize) {
            mergenum=0;
            while (mergenum*runsize<bigrunsize) {
                low = mergenum*runsize;
                mid = (mergenum+1)*runsize-1;
                high= (mergenum+2)*runsize-1;
                mergenum+= 2;

                //merge 2 runs
                i=low;j=mid+1;
                k=0;
                while (i<=mid && j<=high) {
                    if (input[i]<=input[j]) {
                        output[k]=input[i];
                        i++;
                    }
                    else { //array0[i]>array0[j]
                        output[k]=input[j];
                        j++;
                    }
                    k++;
                }
                while (i<=mid) {
                    output[k]=input[i];
                    k++;i++;
                }
                while (j<=high) {
                    output[k]=input[j];
                    k++;j++;
                }
                j=0;
                for (i=low;i<=high;i++) {
                    input[i]=output[j++];
                }
            }
            runsize*=2;
        }
        for (iBuf=0;iBuf<bigrunsize;iBuf+=BLOCK_SIZE) {
            bounded_copy(array0,start+iBuf,input,iBuf,BLOCK_SIZE);
        }
        #if PRINT
        fprintf(out,"Sorted BigRun: %d %d\n",start,start+bigrunsize-1);
        printArray(array0,start,start+bigrunsize-1);
        fflush(out);
        #endif
        bigrunnum++;
    }

    /* Phase 2: merge MERGE_K big run into sorted array */
    /* Cache design for merging phase */
    /* BufferOut = Buf[0...BLOCK_SIZE-1] */
    /* BufferIn  = Buf[...(MERGE_K+1)*BLOCK_SIZE-1] */
    /* RunIndex = Buf[..MERGE_K-1] */
    /* Heap: Value= Buf[.. +MERGE_K-1],Source[..MERGE_K-1]*/
    /* merge level (MERGE_K) is chosen to maximize cache usage */
    /* becareful not to overuse the buffer */

    /*Allocate memory in Buffer for different usage */
    next = 0;
    //allocate Buffer to buffer output array
    outBuf = (int*)(&Buf[next]);
    next += BLOCK_SIZE;
    //allocate Buffer to buffer input array
    for (i=0;i<MERGE_K;i++) {
        inBuf[i] = (int*)(&Buf[next] );
        next += BLOCK_SIZE;
    }
    //allocate Buffer for the binary heap to sort element
    runIndex = (int*)(&Buf[next] );
    next += MERGE_K;
    heapVal = (int*)(&Buf[next]);
    next += MERGE_K;
    heapRunNo = (int*)(&Buf[next]);
    next += MERGE_K;
    

    while (bigrunsize<size) {
        mergenum=0;
        while (MERGE_K*bigrunsize*mergenum < size) {
            start = mergenum*MERGE_K*bigrunsize;
            #if PRINT
            fprintf(out,"Merging %d BigRun: %d %d\n",MERGE_K,start,start+MERGE_K*bigrunsize-1);
            for (i=0;i<MERGE_K;i++) {
                printArray(array0,start+i*bigrunsize,start+(i+1)*bigrunsize-1);
            }
            fflush(out);
            #endif
            qsize=-1;
            for (i=0;i<MERGE_K;i++) {
                bounded_copy(inBuf[i],0,array0,start+i*bigrunsize,BLOCK_SIZE);
                runIndex[i]=1;
                enqueue(inBuf[i][0],i,heapVal,heapRunNo,&qsize);
            }

            outIndex=0;
            while (heapVal[0]!=0x0fffffff) {
                dequeue(&val, &runNo, heapVal, heapRunNo, &qsize);
                outBuf[outIndex%BLOCK_SIZE]=val;
                outIndex++;

                if (outIndex%BLOCK_SIZE==0) {
                    bounded_copy(array1,outIndex-BLOCK_SIZE,outBuf,0,BLOCK_SIZE);
                }
                if (runIndex[runNo]%BLOCK_SIZE==0) {
                    if (runIndex[runNo]<bigrunsize) {
                        bounded_copy(inBuf[runNo],0,array0,
                            start+runNo*bigrunsize+runIndex[runNo],BLOCK_SIZE);
                        val = inBuf[runNo][runIndex[runNo]%BLOCK_SIZE]; 
                        runIndex[runNo]++;
                    }
                    else val= 0x0fffffff;
                }
                else {
                    //fprintf(out,"runNo %d, runIndex %d\n",
                    //    runNo,runIndex[runNo]%BLOCK_SIZE);fflush(out);
                    val = inBuf[runNo][runIndex[runNo]%BLOCK_SIZE];
                    runIndex[runNo]++;
                }
                enqueue(val,runNo,heapVal,heapRunNo,&qsize);
            }
            bounded_copy(array1,outIndex-(outIndex%BLOCK_SIZE),
                outBuf,0,outIndex%BLOCK_SIZE);
            for (k=0;k<MERGE_K*bigrunsize;k+=BLOCK_SIZE) {
                bounded_copy(array0,start+k,array1,k,BLOCK_SIZE);
            }
            mergenum++;
#if PRINT
            fprintf(out,"Merged %d big run: %d %d\n",MERGE_K,start,start+MERGE_K*bigrunsize-1);
            //printArray(array0,start,start+MERGE_K*bigrunsize-1);
            fflush(out);
#endif
        }
        bigrunsize*=MERGE_K;
    }
}
int main() {
    int i;
    int cnt;

    //init
    cnt = 1;
    array0 = (int*)(&mem0[0]);
    array1 = (int*)(&mem1[0]);
    for (i=0;i<SIZE;i++) {
       //array0[i] = SIZE-i;
       cnt = (cnt+7)*38 % SIZE;
       //cnt = (cnt +71)*29 % SIZE;
    }
    #if PRINT
    out = fopen("merging.txt","w");
    fprintf(out,"Original Array: ");
    printArray(array0,0,SIZE-1);
    #endif

    merge_sort(SIZE);
    
    #if PRINT
    fprintf(out,"Merged Array: ");
    printArray(array0,0,SIZE-1);
    fclose(out);
    #endif

    return 0;
}
