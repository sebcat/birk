#include <stdio.h>
#include <stdlib.h>
#include "interp.h"
#include "test.h"

int main() {
	interp_t *interp;

	setenv("LUA_PATH", "?.lua;?", 1);
	interp = interp_new();
	EXPECT(interp != NULL, "interp was NULL");
	EXPECT(interp_load(interp, "IAHUHF.EAHFE.FHE.FH") == BIRK_ERROR,
			"unexpected success of nonexistent library");
	EXPECT(interp_load(interp, "birk.irc_test") == BIRK_OK,
			interp_errstr(interp));
	interp_free(interp);
	return EXIT_SUCCESS;
}
