#include <stdio.h>
#include <stdlib.h>
#include "interp.h"
#include "birk.h"
#include "test.h"

int main() {
	interp_t *interp;

	setenv("LUA_PATH", "?/init.lua;?.lua;?", 1);
	interp_free(NULL); /* should not crash */
	interp = interp_new(0, -1);
	EXPECT(*interp_errstr(interp) == '\0',
			"%s", interp_errstr(interp));
	EXPECT(interp != NULL, "interp was NULL");
	EXPECT(interp_load(interp, "IAHUHF.EAHFE.FHE.FH") == BIRK_ERROR,
			"unexpected success of nonexistent library");
	EXPECT(interp_load(interp, "birk.irc_test") == BIRK_OK,
			"%s", interp_errstr(interp));
	interp_free(interp);
	return EXIT_SUCCESS;
}
