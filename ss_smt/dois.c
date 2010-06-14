/* features:

	SMT4 = 4 threads
        Width = 2x8 = 16
        Fu number = 17
        DecodeDepth = total
        RUU/LSQ = distributed (4x16/4x8) = total 64/32
        dtlb = 4 x 512 = total 2M  : a=8
        itlb = 4 x 256 = total 1M  : a=8

        il1 total = 2x1x32=64 : a=4
        dl1 total = 1x64=64 : a=4
        ul2 total = 1x1024=1M : a=8

*/
        

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#define N 1

char pathname[N][1024];

char pid_fname[64] = "../sbac.out.2/dois.pid";

void init(void)
{

/* prediction forced at 100% */
  
  strcpy(pathname[0],"nohup ss_smt 13 16 -redir:sim ../sbac.out.2/res.smt4.13-16.100 -redir:prog ../sbac.out.2/saida.smt4.13-16.100 -fastfwd 50000000 -max:inst 200000000 -issue:inorder FALSE -ruulsq:type distributed -fu:type hetero -imodules:num 2 -fetch:width 8 -fetch:ifqsize 8 -decode:width 16 -issue:width 16 -commit:width 16 -mem:width 8 -ruu:size 16 -lsq:size 8 -cache:dl1 dl1:1:64:64:4:l -cache:dl2 ul2:1:1024:128:8:l -cache:il1 il1:1:32:64:4:l -cache:il2 dl2 -tlb:dtlb dtlb:512:4096:8:l -tlb:itlb itlb:256:4096:8:l -bpred:type forced:100 -res:ialu 5 -res:imult 2 -res:memport 3 -res:fpalu 5 -res:fpmult 2 PROGRAM > ../sbac.out.2/nohup.smt4.13-16.100 &");

}
  
int obtain_pid(void)
{ FILE *fd;
  char line[1024], fname[12];
  int pid;

  fd = fopen(pid_fname,"r");

  if (!fd) 
  { printf("\nnao existe arquivo arq.pid\n"); return 0;}

  while (!feof(fd))
  {
       line[0]='\n';
       fgets(line, 1024, fd);

       sscanf(line, "%s %d", fname, &pid);

       /* printf("\nfname = %s pid = %d\n",fname,pid); */

       if ( (fname[0]=='s')&&
            (fname[1]=='s')&&
            (fname[2]=='_')&&
            (fname[3]=='s')&&
            (fname[4]=='m')&&
            (fname[5]=='t') )
       {   fclose(fd); return pid; }

  }
  
  fclose(fd);
  return 0;
}

main()
{ char get_ps[256], fname[64],c;
  int i,pid,lives;
  FILE *fd;

  init();

  for (i=0;i<N;i++)
  {
      system(pathname[i]); /* fork a simulation */
   
      lives = 1;
      while (lives)
      {
          system("sleep 300"); /* sleep 5 minutes */

          strcpy(get_ps,"ps -u ronaldog -o fname -o pid >");
          sprintf(get_ps,"%s %s",get_ps, pid_fname);

          system(get_ps); /* create a file with a pid */

          system("sleep 60"); /* sleep 1 minute */

          lives = pid = obtain_pid();

       }
  }

}

