switches = -Wall -O0 -g
gcc = gcc


all:
	$(gcc) $(switches) cache-measure.c -o cache-measure
	$(gcc) $(switches) btrees.c -o btrees
	$(gcc) $(switches) ts_queue_oth.c -o ts_queue_oth -pthread

clean:
	rm -rf *.o *~ #~
