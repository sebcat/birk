#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "tqueue.h"

struct queue_element {
	void *data;
	struct queue_element *next;
	struct queue_element *prev;
};

struct queue {
	struct queue_element *head, *tail;
	size_t nelems;
};

static void *queue_put(struct queue *q, void *data) {
	struct queue_element *el;

	el = calloc(1, sizeof(struct queue_element));
	if (el == NULL) {
		return NULL;
	}

	el->data = data;
	if (q->head != NULL) {
		q->head->prev = el;
	}

	if (q->tail == NULL) {
		q->tail = el;
	}

	el->next = q->head;
	el->prev = NULL;
	q->head = el;
	q->nelems++;
	return el;
}

static void *queue_get(struct queue *q) {
	struct queue_element *el;
	void *data;

	if ((el = q->tail) == NULL) {
		return NULL;
	} else if (el->prev == NULL) {
		q->head = NULL;
		q->tail = NULL;
	} else {
		q->tail = el->prev;
	}

	data = el->data;
	free(el);
	q->nelems--;
	return data;
}

#define queue_nelems(q) ((q)->nelems)

static void queue_cleanup(struct queue *q, void(*freer)(void*)) {
	struct queue_element *curr, *next;

	if (q != NULL) {
		for(curr = q->head; curr != NULL; curr = next) {
			if (freer) {
				freer(curr->data);
			}

			next = curr->next;
			free(curr);
		}

		q->head = NULL;
		q->tail = NULL;
	}
}

struct tqueue_t {
	struct tqueue_options opts;

	pthread_barrier_t init_barrier;

	struct queue inq;
	pthread_cond_t inq_haselems;
	pthread_mutex_t inq_m;

	struct queue outq;
	pthread_cond_t outq_haselems;
	pthread_mutex_t outq_m;

	pthread_t *threads;
	pthread_mutex_t shutdown;
};

static void *worker(void *data) {
	int lockret;
	tqueue_t *q = (tqueue_t*)data;
	pthread_barrier_wait(&q->init_barrier);
	void *q_data = NULL, *outdata = NULL;

	while((lockret = pthread_mutex_trylock(&q->shutdown)) == EBUSY) {
		pthread_mutex_lock(&q->inq_m);
		if (queue_nelems(&q->inq) == 0) {
			pthread_cond_wait(&q->inq_haselems, &q->inq_m);
		}

		q_data = queue_get(&q->inq);
		pthread_mutex_unlock(&q->inq_m);
		if (q_data != NULL && q->opts.worker != NULL) {
			outdata = q->opts.worker(q_data);
		}

		if (outdata != NULL) {
			pthread_mutex_lock(&q->outq_m);
			queue_put(&q->outq, outdata);
			pthread_mutex_unlock(&q->outq_m);
			pthread_cond_signal(&q->outq_haselems);
		}
	}

	if (lockret == 0) {
		pthread_mutex_unlock(&q->shutdown);
	}

	return NULL;
}

tqueue_t *tqueue_new(struct tqueue_options *opts) {
	tqueue_t *q;
	int i;

	q = calloc(1, sizeof(tqueue_t));
	if (q == NULL) {
		return NULL;
	}

	memcpy(&q->opts, opts, sizeof(struct tqueue_options));
	q->threads = calloc(opts->nthreads, sizeof(pthread_t));
	if (q->threads == NULL) {
		free(q);
		return NULL;
	}

	/* initialization barrier */
	pthread_barrier_init(&q->init_barrier, NULL, opts->nthreads+1);

	/* init queue syncronization primitives */
	pthread_mutex_init(&q->inq_m, NULL);
	pthread_cond_init(&q->inq_haselems, NULL);
	pthread_mutex_init(&q->outq_m, NULL);
	pthread_cond_init(&q->outq_haselems, NULL);

	/* the shutdown mutex is unlocked at shutdown */
	pthread_mutex_init(&q->shutdown, NULL);
	pthread_mutex_lock(&q->shutdown);

	/* spawn the threads */
	for(i=0; i<q->opts.nthreads; i++) {
		if (pthread_create(&q->threads[i], NULL, worker, q) != 0) {
			q->opts.nthreads = i;
			tqueue_free(q);
			return NULL;
		}
	}

	pthread_barrier_wait(&q->init_barrier);
	return q;
}

void tqueue_free(tqueue_t *q) {
	int i;

	if (q != NULL) {
		pthread_mutex_unlock(&q->shutdown);
		/* clear input queue */
		pthread_mutex_lock(&q->inq_m);
		queue_cleanup(&q->inq, q->opts.in_freer);
		pthread_mutex_unlock(&q->inq_m);

		/* signal haselems condition in order to trigger check for shutdown */
		pthread_cond_broadcast(&q->inq_haselems);

		/* wait for worker threads */
		for(i=0; i<q->opts.nthreads; i++) {
			pthread_join(q->threads[i], NULL);
		}

		/* clear output queue */
		pthread_mutex_lock(&q->outq_m);
		queue_cleanup(&q->outq, q->opts.out_freer);
		pthread_mutex_unlock(&q->outq_m);

		free(q->threads);
		free(q);
	}
}

int tqueue_put(tqueue_t *q, void *el) {
	int ret;
	pthread_mutex_lock(&q->inq_m);
	ret = queue_put(&q->inq, el) == NULL ? -1 : 0;
	pthread_mutex_unlock(&q->inq_m);
	pthread_cond_signal(&q->inq_haselems);
	return ret;
}

void *tqueue_get(tqueue_t *q, int shouldblock) {
	void *ret = NULL;

	pthread_mutex_lock(&q->outq_m);
	if (shouldblock && queue_nelems(&q->outq) == 0) {
		pthread_cond_wait(&q->outq_haselems, &q->outq_m);
	}

	ret = queue_get(&q->outq);
	pthread_mutex_unlock(&q->outq_m);
	return ret;
}

