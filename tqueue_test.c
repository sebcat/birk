#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "test.h"
#include "tqueue.h"

#define WORKLOAD_USECS	100000

static void *ww(void *data) {
	long i = (long)data;
	usleep(WORKLOAD_USECS); /* simulate work */
	i++;
	return (void*)i;
}

void timed_testfunc(int nthreads, long nitems) {
	tqueue_t *q;
	long i, res, expected;
	struct tqueue_options opts;

	memset(&opts, 0, sizeof(opts));
	opts.nthreads = nthreads;
	opts.worker = &ww;
	q = tqueue_new(&opts);
	EXPECT(tqueue_get(q, 0) == NULL, "expected NULL result on tqueue_get");

	for(i=0; i<nitems; i++) {
		EXPECT(tqueue_put(q, (void*)(i+1)) == 0, "tqueue_put failure");
	}

	expected = 0;
	for(i=res=expected=0; i<nitems; i++) {
		expected += i+2;
		res += (long)tqueue_get(q, 1);
	}

	EXPECT(res == expected, "sum: expected %ld, was %ld", expected, res);
	tqueue_free(q);
}

int main() {
	int i;
	struct timeval t1, t2, res;
	static struct {
		int nthreads;
		long nitems;
		useconds_t usec_min;
		useconds_t usec_max;
	} timed_tests[]= {
		{1, 4, WORKLOAD_USECS*4, WORKLOAD_USECS*(4+1)},
		{2, 4, WORKLOAD_USECS*2, WORKLOAD_USECS*(2+1)},
		{4, 4, WORKLOAD_USECS, WORKLOAD_USECS*2},
		{8, 4, WORKLOAD_USECS, WORKLOAD_USECS*2},
		{0,0,0,0},
	};

	for(i=0; timed_tests[i].nthreads != 0; i++) {
		gettimeofday(&t1, NULL);
		timed_testfunc(timed_tests[i].nthreads, timed_tests[i].nitems);
		gettimeofday(&t2, NULL);
		timersub(&t2, &t1, &res);
		printf("timed_tests[%d]: tv_sec: %lu tv_usec: %lu "
				"min: %u max: %u\n",
				i, res.tv_sec, res.tv_usec, timed_tests[i].usec_min,
				timed_tests[i].usec_max);
		EXPECT(res.tv_sec == 0 &&
				res.tv_usec > timed_tests[i].usec_min &&
				res.tv_usec < timed_tests[i].usec_max, "FAIL");
	}

	return 0;
}
