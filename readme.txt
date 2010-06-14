Source code
============
Source code of the shared cache and bus is in directory "m_cache"

To compile the code
======================
./make
It should produce a binary file named "opt"

To run the code
======================================================================
./opt <msc_path> <L1_cache_config> <L2_cache_config> <num_of_cores>

<msc_path> : Path of the MSC description running (A sample can be 
found at interfere/msc1)

<L1_cache_config> : L1 cache configuration, 
must be in the directory cache_config/
If L1 cache configuration file name is l1.config, give argument "l1"

<L2_cache_config> : L2 cache configuration, 
must be in the directory cache_config/
If L2 cache configuration file name is l2.config, give argument "l2"

<num_of_cores> : Total nuber of cores

**************
An example
**************

An example can be found in the script run

> cat run
> ./opt interferePath l1 l2 2

> cat interferePath
> interfere/msc1

This runs the analyzer with msc found at "interfere/msc1" with 
L1 cache configuration "cache_config/l1.config" and L2 cache 
configuration "cache_config/l2.config".

MSC format 
===========
> cat interfere/msc1

> 
2							/* Total number of tasks */
matmult/matmult		/* First task name aka path of the first task binary */
0							/* number of successors */
statemate/statemate	/* Second task name aka path of the second task binary */
0 							/* number of successors */
1 0 						/* dependency graph for all the tasks */
0 1 

Essentially, this example runs matmult program in one core and statemate program 
in another core independently.


CAUTION
========
The code is not very well written and currently we are trying to merge with 
our existing chronos tool features. Especially, the portion of sharedcache. 
Contact me (sudiptac@comp.nus.edu.sg) for any difficulties you face.


