/* This file is meant for the simulation of a shared cache hierarchy */
/* The shared cache is implemented as part of a shared segment in the 
 * physical memory where each set belonging to the shared cache 
 * corresponds to a unique shared memory segment in the physical memory */
/* We assume the following while simulating the shared cache */
/* 1. Each set must be accessed in a mutually exclusive manner */
/* 2. Thus each set can be accessed through a guard of semaphore */
/* 3. Different sets in the shared cache can be accessed in parallel */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* random id */
int last_id = 5678;

/* Size of the shared memory segment */
#define SHMZ 

/* returns an identity for the allocated shared memory 
 * segment */
key_t get_magic_number()
{
	 return (key_t)last_id++;	  
}

int main()
{
	 key_t key;		/* key to be passed to the shared memory */
	 int shmflag;	/* shmflg to be passed to shmget */
	 int shmid; 	/* return value from shmget() */
	 int size;		/* size to be passed to shmget() */

	 int id;

	 /* Parameters for the shared cache among different cores */	  
	 int nsets;
	 int nassoc;
	 int block_size;

	 nsets = get_no_of_sets();
	 nassoc = get_assoc();
	 block_size = get_block_size();

	 /* Allocate the shared memory for all the set-s in the shared 
	  * cache */
	 for(id = 0; id < nsets; id++)
	 {
		  key = get_magic_number();

		  /* Create the segment */
		  if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
			 fprintf(stdout, "shmget failed\n");
			 exit(-1);
		  }
	 }
}
