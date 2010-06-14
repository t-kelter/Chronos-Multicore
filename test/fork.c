
/* Includes */
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>     /* General Utilities */
 
int main()
{
    pid_t childpid; /* variable to store the child's pid */
    int retval;     /* child process: user-provided return code */
    int status;     /* parent process: child's exit status */

    /* only 1 int variable is needed because each process would have its
       own instance of the variable
       here, 2 int variables are used for clarity */
        
    /* now create new process */
    childpid = fork();
    
    if (childpid >= 0) /* fork succeeded */
    {
		  if(childpid == 0)	/* Child process */
		  {
				int i;

				for(i = 0; i < 10000000; i++)
					printf("I am child and my pid is %u\n", getpid()); 

		  }
		  else					/* Parent process */
		  {
				childpid = fork();	 
				if(childpid == 0) {
					 int i;
					 for(i = 0; i < 100000000; i++)
								printf("I am child and my pid is %u\n", getpid()); 
				}
				else
					 printf("I am parent\n");	 
		  }
    }
    else /* fork returns -1 on failure */
    {
        perror("fork"); /* display error message */
        exit(0); 
    }
}


