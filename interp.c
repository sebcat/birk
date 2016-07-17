#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include "interp.h"

#define BIRK_ERRSZ	512

#define BIRK_LOADERR(interp) \
	do { \
		const char *__msg; \
		__msg = lua_tostring((interp)->L, -1); \
		snprintf((interp)->err, BIRK_ERRSZ, "%s", __msg); \
		lua_pop((interp)->L, 1); \
	} while(0)


typedef struct interp_t {
	lua_State *L;
	char err[BIRK_ERRSZ];
} interp_t;

interp_t *interp_new() {
	interp_t *interp;

	interp = malloc(sizeof(interp_t));
	if (interp == NULL) {
		return NULL;
	}

	memset(interp, 0, sizeof(interp_t));
	interp->L = luaL_newstate();
	if (interp->L == NULL) {
		return NULL;
	}

	luaopen_base(interp->L);
	luaopen_coroutine(interp->L);
	luaopen_table(interp->L);
	luaopen_string(interp->L);
	luaopen_utf8(interp->L);
	luaopen_bit32(interp->L);
	luaopen_math(interp->L);
	luaopen_debug(interp->L);
	luaopen_package(interp->L);
	/* not opened: io, os */
	return interp;
}

void interp_free(interp_t *interp) {
	if (interp != NULL) {
		if (interp->L != NULL) {
			lua_close(interp->L);
		}

		interp->L = NULL;
		free(interp);
	}
}

int interp_load(interp_t *interp, const char *modname) {
	assert(interp != NULL);
	assert(interp->L != NULL);
	assert(modname != NULL);

	lua_getglobal(interp->L, "require");
	lua_pushstring(interp->L, modname);
	if (lua_pcall(interp->L, 1, 1, 0) != LUA_OK) {
		BIRK_LOADERR(interp);
		return BIRK_ERROR;
	} else {
		lua_setglobal(interp->L, modname);
	}

	return BIRK_OK;
}

const char *interp_errstr(interp_t *interp) {
	assert(interp != NULL);
	return interp->err;
}
