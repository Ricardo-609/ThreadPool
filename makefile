CC=g++
CXXFLAGS=-std=c++0x
CFLAGS=-I

test_threadpool:test_threadpool.cc
	$(CC) -o test_threadpool ./test_threadpool.cc -std=c++11 -pthread

test_threadpool_time:test_threadpool_time.cc
	$(CC) -o test_threadpool_time ./test_threadpool_time.cc -std=c++11 -pthread

clean:
	rm -f ./*.o