/*
 * tqueue - syncronized worker thread pool
 *
 * mode of operation:
 *
 *                worker threads
 * input queue        _(W)_      output queue
 * +---+---+---+     /     \     +---+---+---+
 * |   |   |   | -> <--(W)--> -> |   |   |   |
 * +---+---+---+     \     /     +---+---+---+
 *                    -(W)-
 *
 * Order of output depends on thread scheduling
 *
 * All methods must be called from the same thread
 *
 */

#ifndef __BIRK_TQUEUE_H
#define __BIRK_TQUEUE_H

typedef struct tqueue_t tqueue_t;

struct tqueue_options {
	/* number of worker threads */
	int nthreads;

	/* worker callback, needs to be reentrant */
	void *(*worker)(void*);

	/* callback called for freeing an element of the input queue,
	 * can be NULL */
	void (*in_freer)(void*);

	/* callback called for freeing an element of the output queue,
	 * can be NULL */
	void (*out_freer)(void*);
};

tqueue_t *tqueue_new(struct tqueue_options *opts);
void tqueue_free(tqueue_t *q); /* XXX: Only call once! */

/* puts an element in the front of the input queue. Returns -1 on error,
 * 0 on success */
int tqueue_put(tqueue_t *q, void *el);

/* takes an element from the end of the output queue. Returns a pointer to
 * the element on success, NULL on failure or no output available if
 * shouldblock is set to zero */
void *tqueue_get(tqueue_t *q, int shouldblock);

#endif
