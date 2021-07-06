switches = -Wall -O0 -g
gcc = gcc


all:
	$(gcc) $(switches) cache-measure.c -o cache-measure
	$(gcc) $(switches) btrees.c -o btrees

clean:
	rm -rf *.o *~ #~
