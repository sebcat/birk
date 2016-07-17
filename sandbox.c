#ifdef __linux__
#include <seccomp.h>
#endif
#include <errno.h>
#include <stdlib.h>

#include "birk.h"
#include "sandbox.h"

#if defined(__linux__)
int sandbox_enter() {
	int calls[] ={
		SCMP_SYS(brk),
        SCMP_SYS(epoll_create),
        SCMP_SYS(epoll_create1),
        SCMP_SYS(epoll_ctl),
        SCMP_SYS(epoll_wait),
        SCMP_SYS(exit),
        SCMP_SYS(exit_group),
        SCMP_SYS(fstat),
        SCMP_SYS(futex),
        SCMP_SYS(mmap),
        SCMP_SYS(mprotect),
        SCMP_SYS(munmap),
        SCMP_SYS(poll),
        SCMP_SYS(read),
		SCMP_SYS(sched_getaffinity),
		SCMP_SYS(sched_setaffinity),
        SCMP_SYS(sched_yield),
        SCMP_SYS(select),
        SCMP_SYS(write),
	};
	int i, ret = BIRK_ERROR;
	scmp_filter_ctx ctx = NULL;

	if ((ctx = seccomp_init(SCMP_ACT_ERRNO(EACCES))) == NULL) {
		goto fail;
	}

	for(i=0; i<sizeof(calls)/sizeof(int); i++) {
		if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, calls[i], 0) != 0) {
			goto fail;
		}
	}

	if (seccomp_load(ctx) < 0) {
		goto fail;
	}

	ret = BIRK_OK;
fail:
	if (ctx != NULL) {
		seccomp_release(ctx);
	}

	return ret;
}

#elif defined(__FreeBSD__)

#error "NYI"

#else

#error "NYI"

#endif
