# Small-C-progs
A collection of small C programs that illustrate some basic concepts. 

1. cache-measure.c: A program that tries to torture cache levels, following the ideas by Remzi and Andrea C. Arpaci-Dusseau.
   		    Fixes a CPU and makes a torture array that covers NUMPAGES pages of memory and accesses each page LOOPS
		    times. With LOOPS set to 1000000 it gives more or less stable results on my machine. You can start with
		    NUMPAGES=10 and increase until you see a drop in mem access time. This should correspond to using different
		    cache levels. Also, I decided to measure the number of CPU ticks instead of time.

2. btrees.c:	    A direct implementation of B-trees, for didactic purposes. The program can produce B-trees of arbirtrary sizes
   		    in dot-compatible output. Feel free to use the pictures without attribution. The code is more or less what
		    you can find in CLRS (with one small fix, I think).

3. ts_queue_oth.c:  An implementation of hand-over-hand locking thread safe queue together with a simple experiment. For didactic
   		    purposes.
                    		    