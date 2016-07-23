all: birk-agent test

CC?=clang
RM?=rm -f
# lua-5.2 instead of lua-5.3 because there's currently (2016-07-22) no
# lua53-lpeg in the ports tree
CFLAGS=-Wall -Werror -O1 -g -I/usr/local/include/lua52/
LDFLAGS=-L/usr/local/lib -llua-5.2


.PHONY: test

test: interp_test sandbox_test

interp_test: interp_test.c interp.c
	$(CC) $(CFLAGS) -o interp_test interp_test.c interp.c $(LDFLAGS) && ./interp_test

sandbox_test: sandbox_test.c sandbox.c
	$(CC) $(CFLAGS) -o sandbox_test sandbox_test.c sandbox.c $(LDFLAGS) && ./sandbox_test

birk-agent: interp.c interp.h sandbox.c sandbox.h birk-agent.c birk.h
	$(CC) $(CFLAGS) -o birk-agent interp.c sandbox.c birk-agent.c $(LDFLAGS)

clean:
	$(RM) interp_test sandbox_test birk-agent
