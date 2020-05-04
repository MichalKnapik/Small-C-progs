switches = -Wall -O0 -g
gcc = gcc
src = cache-measure

all:
	$(gcc) $(switches) $(src).c -o $(src)

clean:
	rm -rf *.o *~ #~
