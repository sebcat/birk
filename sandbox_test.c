#include <stdio.h>
#include "birk.h"
#include "sandbox.h"
#include "test.h"

int main() {
	FILE *fp;

	EXPECT((fp = fopen("/dev/zero", "rb")) != NULL,
			"unable to open /dev/zero");
	EXPECT(fclose(fp) == 0, "unexpected return from fclose");
	EXPECT(sandbox_enter() == BIRK_OK, "sandbox_enter() != BIRK_OK");
	EXPECT((fp = fopen("/dev/zero", "rb")) == NULL,
			"unexpected successful return from fopen");
	return EXIT_SUCCESS;
}
