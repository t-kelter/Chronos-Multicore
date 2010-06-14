#include <stdio.h>

int main()
{
	unsigned x;	  
	FILE* fp = fopen("TDMA_bus_sched.db", "r");

	while(!feof(fp)) {	  
	   fscanf(fp, "%u", &x);
		fprintf(stdout, "x=%d\n", x);
	}	

	fclose(fp);
	system("./solve wcrtsyn.ilp");
}
