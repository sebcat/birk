all: test

CC?=clang
RM?=rm -f
CFLAGS=-Wall -Werror -O1 -g
LDFLAGS=-llua

.PHONY: test

test:
	$(CC) $(CFLAGS) -o interp_test interp_test.c interp.c $(LDFLAGS) && ./interp_test

clean:
	$(RM) interp_test
