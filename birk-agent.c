#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "interp.h"
#include "sandbox.h"
#include "birk.h"

/* option flags */
#define OPT_NOSANDBOX	(1 << 0)

struct options {
	int flags;
	char *chunk;
};

static void usage() {
	fprintf(stderr,
		"usage: birk [opts]\n"
		"opts:\n"
		"  -s            disable sandbox (only use for debugging)\n"
		"  -e <chunk>    code chunk to evaluate\n"
		"  -h            this text\n");
}

static void opts_or_die(int argc, char *argv[], struct options *opts) {
	int opt;

	/* set default options */
	memset(opts, 0, sizeof(struct options));

	/* override defaults with command arguments */
	while ((opt = getopt(argc, argv, "hse:")) != -1) {
		switch(opt) {
			case 's':
				opts->flags |= OPT_NOSANDBOX;
				break;
			case 'e':
				opts->chunk = strdup(optarg);
				break;
			case 'h':
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

}

static int do_that_thing(struct options *opts) {
	interp_t *interp = NULL;
	int ret = EXIT_FAILURE;

	interp = interp_new();
	if (interp == NULL) {
		perror("interp_new");
		goto fail;
	}

	if (interp_load(interp, "birk") != BIRK_OK) {
		fprintf(stderr, "%s\n", interp_errstr(interp));
		goto fail;
	}

	if (opts->flags & OPT_NOSANDBOX) {
		fprintf(stderr, "Warning: sandbox disabled\n");
	} else {
		if (sandbox_enter() != BIRK_OK) {
			perror("sandbox_enter");
			goto fail;
		}
	}

	if (opts->chunk) {
		if (interp_eval(interp, opts->chunk) != BIRK_OK) {
			fprintf(stderr, "%s\n", interp_errstr(interp));
			goto fail;
		}
	}

	ret = EXIT_SUCCESS;
fail:
	if (opts->chunk != NULL) {
		free(opts->chunk);
	}

	if (interp != NULL) {
		interp_free(interp);
	}

	return ret;
}

int main(int argc, char *argv[]) {
	struct options opts;

	opts_or_die(argc, argv, &opts);
	return do_that_thing(&opts);
}
