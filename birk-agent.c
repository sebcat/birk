#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>

#include "interp.h"
#include "sandbox.h"
#include "birk.h"

/* option flags */
#define OPT_NOSANDBOX	(1 << 0)

/* default maximum number of sequential sandbox reloads */
#define DFL_MAXRETRIES	2
/* default RLIMIT_NOFILE limit, set in parent */
#define DFL_MAXFDS		10
/* default RLIMIT_AS limit, set in parent */
#define DFL_VADDRLIM	134217728L

struct options {
	int flags;
	int maxretries;
	int maxfds;
	long vaddrlim;
};

static void usage() {
	fprintf(stderr,
		"usage: birk [opts]\n"
		"opts:\n"
		"  -c <lim>      maximum no. of file descriptors (def: %d)\n"
		"  -h            this text\n"
		"  -r <num>      max consecutive respawns (def: %d)\n"
		"  -s            disable sandbox (only use for debugging)\n"
		"  -m <lim>      maximum vaddr space, in bytes (def: %lu)\n",
		DFL_MAXFDS,
		DFL_MAXRETRIES,
		DFL_VADDRLIM);
	exit(EXIT_FAILURE);
}

static void opts_or_die(struct options *opts, int argc, char *argv[]) {
	int opt;
	long l;
	char *cptr;

	/* set default options */
	memset(opts, 0, sizeof(struct options));
	opts->maxfds = DFL_MAXFDS;
	opts->maxretries = DFL_MAXRETRIES;
	opts->vaddrlim = DFL_VADDRLIM;

	/* override defaults with command arguments */
	while ((opt = getopt(argc, argv, "hsr:c:m:")) != -1) {
		switch(opt) {
			case 's':
				opts->flags |= OPT_NOSANDBOX;
				break;
			case 'r':
				errno = 0;
				cptr = NULL;
				l = strtol(optarg, &cptr, 10);
				if (errno == ERANGE || *cptr != '\0' || l < 0 ||
						l == LONG_MAX) {
					fprintf(stderr, "Invalid maxretries option\n");
					usage();
				}

				opts->maxretries = l;
				break;
			case 'c':
				errno = 0;
				cptr = NULL;
				l = strtol(optarg, &cptr, 10);
				if (errno == ERANGE || *cptr != '\0' || l < 0 ||
						l == LONG_MAX) {
					fprintf(stderr, "Invalid maxfds option\n");
					usage();
				}

				opts->maxfds = l;
				break;
			case 'm':
				errno = 0;
				cptr = NULL;
				l = strtol(optarg, &cptr, 10);
				if (errno == ERANGE || *cptr != '\0' || l < 0 ||
						l == LONG_MAX) {
					fprintf(stderr, "Invalid vaddrlim option\n");
					usage();
				}

				opts->vaddrlim = l;
				break;
			case 'h':
			default:
				usage();
		}
	}
}

static int child_event_loop(interp_t *interp, int parentfd) {
	char buf[2048];
	struct timeval tv;
	fd_set rfds;
	int ret, maxfd, stdin_fileno;

	stdin_fileno = fileno(stdin);
	while(1) {

		/* receive messages (if any) from the parent */
		maxfd = parentfd > stdin_fileno ? parentfd : stdin_fileno;
		FD_ZERO(&rfds);
		FD_SET(parentfd, &rfds);
		FD_SET(stdin_fileno, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(maxfd+1, &rfds, NULL, NULL, &tv);
		if (ret < 0 && errno != EINTR) {
			perror("select");
			break;
		} else if (ret > 0) {
			if (FD_ISSET(parentfd, &rfds)) {
				/* TODO: handle message from parent */
			} else if (FD_ISSET(stdin_fileno, &rfds)) {
				/*XXX: assumes line buffered STDIN */
				if (fgets(buf, sizeof(buf)-1, stdin) == NULL) {
					break;
				} else if (interp_eval(interp, buf) != BIRK_OK) {
					fprintf(stderr, "%s\n", interp_errstr(interp));
				}
			}
		}
	}

	return EXIT_FAILURE;
}

static pid_t start_interp_proc(struct options *opts, int *fd) {
	interp_t *interp = NULL;
	pid_t pid = -1;
	int pair[2], ret;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
		perror("socketpair");
		return -1;
	}

	if ((pid = fork()) < 0) {
		perror("fork");
		return -1;
	} else if (pid > 0) {
		close(pair[0]);
		*fd = pair[1];
		return pid;
	} else /* pid == 0 */ {
		close(pair[1]);
		signal(SIGCHLD, SIG_DFL);
		signal(SIGUSR1, SIG_DFL);

		/* we load the interpreter early on so that
		 * we can initialize things outside of the sandbox. This
		 * assumes that we consider the code to be trusted */
		interp = interp_new();
		if (interp == NULL) {
			perror("interp_new");
			exit(EXIT_FAILURE);
		} else if (*interp_errstr(interp) != '\0') {
			fprintf(stderr, "%s\n", interp_errstr(interp));
			exit(EXIT_FAILURE);
		}

		if (opts->flags & OPT_NOSANDBOX) {
			fprintf(stderr, "Warning: sandbox disabled\n");
		} else {
			if (sandbox_enter() != BIRK_OK) {
				perror("sandbox_enter");
				exit(EXIT_FAILURE);
			}
		}

		/* we don't pass on option data to the event loop.
		 * If option data is to be present further on, it
		 * should be made accessible through the interpreter */
		ret = child_event_loop(interp, pair[0]);
		close(pair[0]);
		interp_free(interp);
		exit(ret);
	}

	return ret;
}

/* parent signal handler */
#define _CHILD_DEAD	(1<<0) /* SIGCHLD passed to signal handler */
#define _CHILD_KILL (1<<1) /* SIGUSR1 passed to signal handler */
volatile sig_atomic_t _child_status = 0;
static void child_handler(int sig, siginfo_t *si, void *arg) {
	if (sig == SIGCHLD) {
		_child_status |= _CHILD_DEAD;
	} else if (sig == SIGUSR1) {
		_child_status |= _CHILD_KILL;
	}
}

static int parent_event_loop(struct options *opts, pid_t chldpid, int chldfd) {
	struct timeval tv;
	fd_set rfds;
	int ret, maxfd, nretries=0;

	while(1) {

		/* receive messages (if any) from the child */
		maxfd = chldfd;
		FD_ZERO(&rfds);
		FD_SET(chldfd, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(maxfd+1, &rfds, NULL, NULL, &tv);
		if (ret < 0 && errno != EINTR) {
			perror("select");
			break;
		} else if (ret > 0) {
			if (FD_ISSET(chldfd, &rfds)) {
				/* TODO: read child message, do something with it */
			}
		}

		/* manage child process status  and reloading w/ retry limit */
		if (_child_status & _CHILD_DEAD) {
			_child_status = 0;
			close(chldfd);
			while (waitpid(-1, NULL, WNOHANG) > 0);
			nretries++;
			if (nretries >= opts->maxretries) {
				break;
			} else {
				chldpid = start_interp_proc(opts, &chldfd);
				if (chldpid < 0) {
					break;
				}
			}
		} else if (_child_status & _CHILD_KILL) {
			kill(chldpid, SIGKILL);
		} else {
			if (nretries > 0) {
				nretries--;
			}
		}
	}

	return EXIT_FAILURE;
}

static void rlimits_or_die(struct options *opts) {
	struct rlimit rlim;

	/* Set RLIMIT_NOFILE if the max fd limit is lower than the current one */
	if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
		perror("getrlimit RLIMIT_NOFILE");
		exit(EXIT_FAILURE);
	}

	if (rlim.rlim_max > opts->maxfds) {
		rlim.rlim_cur = opts->maxfds;
		rlim.rlim_max = opts->maxfds;
		if (setrlimit(RLIMIT_NOFILE, &rlim) < 0) {
			perror("setrlimit RLIMIT_NOFILE");
			exit(EXIT_FAILURE);
		}
	}

	/* Set RLIMIT_NOFILE if the max fd limit is lower than the current one */
	if (getrlimit(RLIMIT_AS, &rlim) < 0) {
		perror("getrlimit RLIMIT_AS");
		exit(EXIT_FAILURE);
	}

	if (rlim.rlim_max > opts->vaddrlim) {
		rlim.rlim_cur = opts->vaddrlim;
		rlim.rlim_max = opts->vaddrlim;
		if (setrlimit(RLIMIT_AS, &rlim) < 0) {
			perror("setrlimit RLIMIT_AS");
			exit(EXIT_FAILURE);
		}
	}

}

int main(int argc, char *argv[]) {
	struct options opts;
	struct sigaction sa;
	int childfd = -1;
	pid_t pid;

	/* SIGCHLD signal handler */
	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sa.sa_flags = SA_SIGINFO|SA_RESTART|SA_NOCLDSTOP;
	sa.sa_sigaction = &child_handler;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction SIGCHLD");
		return EXIT_FAILURE;
	}

	/* SIGUSR1 signal handler, sinals the parent to reload the child */
	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO|SA_RESTART;
	sa.sa_sigaction = &child_handler;
	if (sigaction(SIGUSR1, &sa, NULL) < 0) {
		perror("sigaction SIGUSR1");
		return EXIT_FAILURE;
	}

	opts_or_die(&opts, argc, argv);
	rlimits_or_die(&opts);

	/* start the child process running the sandboxed Lua interpreter */
	if ((pid = start_interp_proc(&opts, &childfd)) < 0) {
		return EXIT_FAILURE;
	}

	/* have the parent enter its event loop */
	return parent_event_loop(&opts, pid, childfd);
}
