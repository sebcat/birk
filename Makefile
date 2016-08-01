all: birk-agent test

CC?=clang
RM?=rm -f
CFLAGS=-Wall -Werror -O1 -g
LDFLAGS=-llua -lseccomp

.PHONY: test

test: interp_test sandbox_test

interp_test: interp_test.c interp.c
	$(CC) $(CFLAGS) -o interp_test interp_test.c ipc.c interp.c $(LDFLAGS) && ./interp_test

sandbox_test: sandbox_test.c sandbox.c
	$(CC) $(CFLAGS) -o sandbox_test sandbox_test.c sandbox.c $(LDFLAGS) && ./sandbox_test

birk-agent: ipc.c ipc.h interp.c interp.h sandbox.c sandbox.h birk-agent.c birk.h
	$(CC) $(CFLAGS) -o birk-agent ipc.c interp.c sandbox.c birk-agent.c $(LDFLAGS)

clean:
	$(RM) interp_test sandbox_test birk-agent
