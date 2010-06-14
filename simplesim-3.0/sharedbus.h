#ifndef __BUS_SCHEDULE_H
#define __BUS_SCHEDULE_H

typedef unsigned long long ull;
typedef unsigned int uint;

/* Types of TDMA bus schedule */
#define PARTITION_CORE -1
typedef enum {
	SCHED_TYPE_1 = 0,
	SCHED_TYPE_2,
	SCHED_TYPE_3
} sched_type;

struct core_sched {
	ull start_time; /* starting time of the first slot for the core */	
	uint interval;	 /* time interval between two consecutive slots of the same core */ 
	uint slot_len;	 /* slot length of the core in this segment */	
};
typedef struct core_sched core_sched_s;
typedef struct core_sched* core_sched_p;

struct segment {
	ull seg_start;	 /* starting time of the segment */	
	ull seg_end;	 /* ending time of the segment */	
	core_sched_p* per_core_sched;	/* Core wise schedule information */
};
typedef struct segment segment_s;
typedef struct segment* segment_p;

struct schedule {
	sched_type type;	   /* type of the TDMA bus schedule */
	segment_p* seg_list; /* list of segments in the whole schedule length */	
	uint n_segments;	   /* number of segments in the full schedule */ 	
	uint n_cores;	      /* number of cores active */ 	
};

/* Defined type for global TDMA bus schedule */
typedef struct schedule sched_s;
typedef struct schedule* sched_p;

void print_TDMA_sched();
segment_p find_segment(segment_p* head_seg, int nsegs, ull start_time);
sched_p getSchedule();
void setSchedule(char* sched_file);
uint compute_bus_delay(ull start_time, uint ncore);

#endif
