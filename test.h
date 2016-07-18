#ifndef __BIRK_TEST_H
#define __BIRK_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define EXPECT(stmt, fmt, ...) \
	do {\
		if (!(stmt)) { \
			fprintf(stderr, "%s:%d -> " fmt "\n" , __FILE__, __LINE__, ##__VA_ARGS__); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#endif
