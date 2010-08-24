// Include standard library headers
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include local library headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <debugmacros/debugmacros.h>

// Include local headers
#include "busSchedule.h"
#include "handler.h"


/* Prints read TDMA bus schedule */

static void print_core_specific_data(core_sched_p* head_core, int ncore, FILE* fp)
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

static void print_TDMA_sched( FILE* fp )
{
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

/* Find proper segment given a list of segments and a starting time */
static segment_p find_segment(segment_p* head_seg, int nsegs, ull start_time)
{
	/* FIXME*/	  
	return NULL;	  
}

/* Return the global TDMA bus schedule set previously */
sched_p getSchedule()
{
	return global_sched_data;	  
}

/* Gets the schedule for the core with index 'core_index' at time 'time'
 * in the current global TDMA schedule. */
core_sched_p getCoreSchedule( uint core_index, ull time )
{
  const sched_p glob_sched = getSchedule();
  assert(glob_sched && core_index < glob_sched->n_cores &&
      "Internal error: Invalid data structures!" );

  /* Find the proper segment for start time in case there are
   * multiple segments present in the full bus schedule */
  segment_p cur_seg = ( glob_sched->type != SCHED_TYPE_1 )
    ? find_segment( glob_sched->seg_list, glob_sched->n_segments, time )
    : glob_sched->seg_list[0];
  const core_sched_p core_schedule = cur_seg->per_core_sched[core_index];
  assert(core_schedule &&
      "Internal error: Invalid data structures!" );
  return core_schedule;
}

/* Set core specific TDMA bus schedule data in a segment */
static void set_core_specific_data(core_sched_p* head_core, int ncore, FILE* fp)
{
  assert(head_core);

	int i;	  
	int delimiter;

	for(i = 0; i < ncore; i++) {
		 CALLOC( head_core[i], core_sched_p, 1, sizeof(core_sched_s),
             "head_core[i]" );
		 fscanf(fp, "%Lu", &(head_core[i]->start_time));
		 fscanf(fp, "%u", &(head_core[i]->interval));
		 fscanf(fp, "%u", &(head_core[i]->slot_len));
		 fscanf(fp, "%u", &delimiter);
		 if(delimiter > 0)
			prerr("Error: TDMA bus schedule file is in wrong format");		 
	}
}


static void freeScheduleData( sched_p schedule )
{
  uint i;
  for( i = 0; i < schedule->n_segments; i++ ) {
    segment_p const seg = schedule->seg_list[i];

    uint j;
    for ( j = 0; j < schedule->n_cores; j++ ) {
      free( seg->per_core_sched[j] );
    }
    free( seg->per_core_sched );
    free( seg );
  }
  free( schedule->seg_list );
  free( schedule );
}


void setSchedule(const char* sched_file)
{
  DSTART( "setSchedule" );

	FILE * const fp = fopen(sched_file, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open file: %s (busSchedule.c:108)\n", sched_file);
    exit (1);
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

  if ( global_sched_data ) {
    freeScheduleData( global_sched_data );
  }
  CALLOC( global_sched_data, sched_p, 1, sizeof(sched_s), "global_schedule_data" );

	/* Read the type of the schedule */
  uint getdata;
	fscanf(fp, "%u", &getdata);
	global_sched_data->type = getdata;
	
	/* Set the number of segments in the schedule. If it is type 0
	 * schedule then number of segment is 1 , otherwise read the 
	 * number of segments from the TDMA schedule file */
  uint n_segs;
	if(getdata == 0) {
	  n_segs = 1;	  
	} else {
	  fscanf(fp, "%u", &n_segs);
	}
  global_sched_data->n_segments = n_segs;
	/* Now allocate data for all segments in the schedule here */
	CALLOC( global_sched_data->seg_list, segment_p*, n_segs, sizeof(segment_p),
	        "global_sched_data->seg_list" );

	/* Read total number of cores */
  uint ncore;
	fscanf(fp, "%u", &ncore);
	global_sched_data->n_cores = ncore;

	/* Now here read all information about different segments and core
    * specific schedule */
	uint cur_seg = 0;
	while(1) {
		 /* Allocate a segment */ 
		 segment_p seg;
		 CALLOC( seg, segment_p, 1, sizeof(segment_s), "seg" );
		 global_sched_data->seg_list[cur_seg++] = seg;
		 if(n_segs > 1) {
			 /* Now read the start time and end time of the segment */
			 fscanf(fp, "%Lu", &(seg->seg_start));
			 fscanf(fp, "%Lu", &(seg->seg_end));
		 } 		
		 /* Allocate the memory for core specific data */
		 CALLOC( seg->per_core_sched, core_sched_p*, ncore,
             sizeof(core_sched_p), "seg->per_core_sched" );
		 set_core_specific_data(seg->per_core_sched, ncore, fp);

		 /* Traversal of all segments done. Terminate the loop */
		 if(cur_seg == n_segs)
			break;		  
	}

	fclose(fp);

	DACTION( print_TDMA_sched( stdout ); );
	DEND();
}
