#ifndef __BIRK_TEST_H
#define __BIRK_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define EXPECT(stmt, errstr) \
	do {\
		if (!(stmt)) { \
			fprintf(stderr, "%s:%d -> %s\n", __FILE__, __LINE__, errstr); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#endif
