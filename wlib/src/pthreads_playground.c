// https://computing.llnl.gov/tutorials/pthreads/samples/hello.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

const int NUM_THREADS = 4;

const int N = 1000;
const int CHUNK_SIZE = N/NUM_THREADS;

static double x[N];
static double y[N];
static double z[N];

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

double sum = 0.0;

void *add(void *threadid)
{
	long tid;
	tid = (long)threadid;
	for (int i=tid*CHUNK_SIZE; i<tid*CHUNK_SIZE+CHUNK_SIZE; i++) {
		pthread_mutex_lock(&mymutex);
		sum += x[i] + y[i];
		pthread_mutex_unlock(&mymutex);
	}
	printf("Hello World! It's me, thread #%ld!\n", tid);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (long t=0; t<NUM_THREADS; t++) {
//		printf("In main: creating thread %ld\n", t);
		int rc = pthread_create(&threads[t], &attr, add, (void *)t);
		if ( rc ) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	pthread_attr_destroy(&attr);

	for (long t=0; t<NUM_THREADS; t++) {
		void *status;
		int rc = pthread_join(threads[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
//		printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}

//	printf("Main: program completed. Exiting.\n");

	/* Last thing that main() should do */
	pthread_exit(NULL);
}