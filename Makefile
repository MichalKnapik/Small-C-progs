switches = -Wall -O0 -g -lm
glibincludeflags = `pkg-config --cflags glib-2.0`
gliblibflags = `pkg-config --libs glib-2.0`
gcc = gcc

all:
	$(gcc) $(switches) cache-measure.c -o cache-measure
	$(gcc) $(switches) btrees.c -o btrees
	$(gcc) $(switches) ts_queue_oth.c -o ts_queue_oth -pthread

test:
	@echo Compiling tests.
	@$(gcc) $(switches) $(glibincludeflags) -DAS_LIB ts_queue_oth.c gtests.c -c
	@$(gcc) ts_queue_oth.o gtests.o  $(switches) $(gliblibflags) -pthread -o gtest
	@echo Starting tests.
	@./gtest
clean:
	rm -rf *.o *~ #~
