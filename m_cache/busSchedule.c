#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Improper exit with error message */
void prerr(char* msg)
{
	fprintf(stdout, "%s\n", msg);
	fflush(stdout);
	exit(-1);
}

/* Prints read TDMA bus schedule */
#ifdef _DEBUG
void print_core_specific_data(core_sched_p* head_core, int ncore, FILE* fp)
{
	int i;

	for(i = 0; i < ncore; i++)
	{ 
		 fprintf(fp, "Schedule for core %d ======> \n", i); 
		 fprintf(fp, "********************************************\n");
		 fprintf(fp, "Core slot start time = %Lu\n", head_core[i]->start_time);
		 fprintf(fp, "Core slot interval = %u\n", head_core[i]->interval);
		 fprintf(fp, "Core slot length = %u\n", head_core[i]->slot_len);
		 fprintf(fp, "********************************************\n");
	}
}

void print_TDMA_sched()
{
	FILE* fp = stdout;
	int i;
	segment_p seg;

	/* Global schedule data must be set here */
	assert(global_sched_data);
	fprintf(fp, "\n****************TDMA BUS SCHEDULE INFO********************\n\n");
	fprintf(fp, "Schedule type = %u\n", global_sched_data->type);	  
	fprintf(fp, "Number of segments = %u\n\n", global_sched_data->n_segments);
	fprintf(fp, "Number of cores = %u\n\n", global_sched_data->n_cores);

	/* Now print each segment */
	for(i = 0; i < global_sched_data->n_segments; i++)
	{
		seg = global_sched_data->seg_list[i];  
		fprintf(fp, "Segment id = %d ====> \n", i);
		fprintf(fp, "************************************************\n");
		if(global_sched_data->n_segments > 1)
		{
			fprintf(fp, "Segment start time = %Lu\n", seg->seg_start);
			fprintf(fp, "Segment end time = %Lu\n", seg->seg_end);
		}
		/* Print core specific data here */
		print_core_specific_data(seg->per_core_sched, global_sched_data->n_cores, fp);
	}
	fprintf(fp, "\n****************END TDMA BUS SCHEDULE INFO********************\n\n");
	fprintf(fp, "\n");
}
#endif

/* Find proper segment given a list of segments and a starting time */
segment_p find_segment(segment_p* head_seg, int nsegs, ull start_time)
{
	/* FIXME*/	  
	return NULL;	  
}

/* return the global TDMA bus schedule set previously */
sched_p getSchedule()
{
	return global_sched_data;	  
}

/* Set core specific TDMA bus schedule data in a segment */
static void set_core_specific_data(core_sched_p* head_core, int ncore, FILE* fp)
{
	int i;	  
	int delimiter;

	assert(head_core);

	for(i = 0; i < ncore; i++)
	{
		 head_core[i] = (core_sched_p)malloc(sizeof(core_sched_s));
		 if(!head_core[i])
			prerr("Error: Out of memory");
		 memset(head_core[i], 0, sizeof(core_sched_s));
		 fscanf(fp, "%Lu", &(head_core[i]->start_time));
		 fscanf(fp, "%u", &(head_core[i]->interval));
		 fscanf(fp, "%u", &(head_core[i]->slot_len));
		 fscanf(fp, "%u", &delimiter);
		 if(delimiter > 0)
			prerr("Error: TDMA bus schedule file is in wrong format");		 
	}
}

void setSchedule(char* sched_file)
{
	FILE* fp;
	uint getdata;
	uint ncore;
	uint n_segs, cur_seg = 0;
	segment_p seg;

	fp = fopen(sched_file, "r");
	if(!fp)
	{
		fprintf(stdout, "Internal file opening failed\n");  
		exit(-1);
	}	
	/******************Type of the file********************************************/ 
	/*  					(For non-segmented schedule)						 					*/	
	/* <sched_type> <core_no> <\n> <start_time> <interval> <slot_length> 			*/
	/*  					   (For segmented schedule)						 					*/
	/* <sched_type> <\n> <n_segments> <\n> <core_no> <\n>									*/
	/* <seg_start_0> <seg_end_0> <start_time_0> <interval_0> <slot_length_0> <\n>	*/
	/* <seg_start_1> <seg_end_1> <start_time_1> <interval_1> <slot_length_1> <\n>	*/
	/* .																									*/
	/* .																									*/
	/* <seg_start_n> <seg_end_n> <start_time_n> <interval_n> <slot_length_n> <\n>	*/
	/* (Schedule for different cores are separated by a "-1" number)					*/
	/******************************************************************************/

	global_sched_data = (sched_p)malloc(sizeof(sched_s));
	if(!global_sched_data)
		prerr("Error: Out of memory");
	memset(global_sched_data, 0, sizeof(sched_s));	

	/* Read the type of the schedule */
	fscanf(fp, "%u", &getdata);
	global_sched_data->type = getdata;
	
	/* Set the number of segments in the schedule. If it is type 0
	 * schedule then number of segment is 1 , otherwise read the 
	 * number of segments from the TDMA schedule file */
	if(getdata == 0)
	{ 
	  n_segs = 1;	  
	  global_sched_data->n_segments = 1;	  
	}  
	else
	{
	  fscanf(fp, "%u", &n_segs);
	  global_sched_data->n_segments = n_segs; 	  
	}  
	/* Now allocate data for all segments in the schedule here */
	global_sched_data->seg_list = (segment_p *)malloc(n_segs * sizeof(segment_p));
	if(!(global_sched_data->seg_list))
		prerr("Error: Out of memory");  
	memset(global_sched_data->seg_list, 0, n_segs * sizeof(segment_p));

	/* Read total number of cores */
	fscanf(fp, "%u", &ncore);
	global_sched_data->n_cores = ncore;

	/* Now here read all information about different segments and core
    * specific schedule */
	while(1)
	{
		 /* Allocate a segment */ 
		 seg = (segment_p)malloc(sizeof(segment_s)); 
		 if(!seg)
			prerr("Error: Out of memory");
		 memset(seg, 0, sizeof(segment_s));	
		 global_sched_data->seg_list[cur_seg++] = seg;
		 if(n_segs > 1)
		 {
			 /* Now read the start time and end time of the segment */
			 fscanf(fp, "%Lu", &(seg->seg_start));
			 fscanf(fp, "%Lu", &(seg->seg_end));
		 } 		
		 /* Allocate the memory for core specific data */
		 seg->per_core_sched = (core_sched_p *)malloc(ncore * sizeof(core_sched_p));
		 if(!(seg->per_core_sched))
			 prerr("Error: Out of memory");
		 memset(seg->per_core_sched, 0, ncore * sizeof(core_sched_p));
		 set_core_specific_data(seg->per_core_sched, ncore, fp);

		 /* Traversal of all segments done. Terminate the loop */
		 if(cur_seg == n_segs)
			break;		  
	}

	fclose(fp);

#ifdef _DEBUG
	print_TDMA_sched();
#endif
}
