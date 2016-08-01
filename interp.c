#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include "interp.h"
#include "birk.h"
#include "ipc.h"

#define BIRK_ERRSZ	512

#define BIRK_IPCNAME "__birkipc"

#define BIRK_LOADERR(interp) \
	do { \
		const char *__msg; \
		__msg = lua_tostring((interp)->L, -1); \
		snprintf((interp)->err, BIRK_ERRSZ, "%s", __msg); \
		lua_pop((interp)->L, 1); \
	} while(0)

struct interp {
	lua_State *L;
	int ipcfd;
	char err[BIRK_ERRSZ];
};

static int bipc_ready(lua_State *L) {
	struct ipcmsg msg;
	interp_t *interp;

	if ((interp = lua_touserdata(L, -1)) == NULL) {
		return luaL_error(L, "invalid IPC self");
	}

	assert(interp->ipcfd >= 0);
	IPC_MREADY(&msg);
	if (ipc_sendmsg(interp->ipcfd, &msg) <= 0) {
		return luaL_error(L, "ipc_sendmsg failure");
	}

	return 0;
}

static int bipc_connect(lua_State *L) {
	struct ipcmsg msg;
	interp_t *interp;
	const char *host;
	size_t hostlen=0;
	int port;

	if ((interp = lua_touserdata(L, -3)) == NULL) {
		return luaL_error(L, "invalid IPC self");
	}

	if ((host = luaL_checklstring(L, -2, &hostlen)) == NULL
			|| *host == '\0') {
		return luaL_error(L, "invalid host");
	}

	port = luaL_checkint(L, -1);
	if (port <= 0 || port > 65535) {
		return luaL_error(L, "invalid port number");
	}

	if (sizeof(port)+hostlen > IPC_VALSZ) {
		return luaL_error(L, "host name too long");
	}

	assert(interp->ipcfd >= 0);
	IPC_MCONNECT(&msg, host, hostlen+1, port);
	if (ipc_sendmsg(interp->ipcfd, &msg) <= 0) {
		return luaL_error(L, "ipc_sendmsg failure");
	}

	return 0;
}

interp_t *interp_new(int flags, int ipcfd) {
	interp_t *interp = NULL;
	lua_State *L = NULL;
	static const luaL_Reg ipcfuncs[] = {
		{"ready", bipc_ready},
		{"connect", bipc_connect},
		{NULL, NULL},
	};

	L = luaL_newstate();
	if (L == NULL) {
		return NULL;
	}

	luaopen_base(L);
	luaopen_coroutine(L);
	luaopen_table(L);
	luaopen_string(L);
#if (LUA_VERSION_NUM >= 503)
	luaopen_utf8(L);
#endif
	luaopen_bit32(L);
	luaopen_math(L);
	luaopen_debug(L);
	luaopen_package(L);
	/* not opened: io, os */

	/* create a userdata object that represents the interpreter */
	interp = (interp_t*)lua_newuserdata(L, sizeof(interp_t));
	if (interp == NULL) {
		lua_close(L);
		return NULL;
	}

	memset(interp, 0, sizeof(interp_t));
	interp->ipcfd = ipcfd;
	interp->L = L;

	/* Build a metatable with the methods for our userdata object.
	 * Assign the userdata object to BIRK_IPCNAME */
	luaL_newmetatable(L, BIRK_IPCNAME);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, ipcfuncs, 0);
	lua_setmetatable(L, -2);
	lua_setglobal(L, BIRK_IPCNAME);

	if ((flags & INTERPF_LOADBIRK) && interp_load(interp, "birk") != BIRK_OK) {
		/* error message is set, caller should check it on return */
		return interp;
	}

	return interp;
}

void interp_free(interp_t *interp) {
	if (interp != NULL && interp->L != NULL) {
		lua_close(interp->L);
	}
}

int interp_load(interp_t *interp, const char *modname) {
	assert(interp != NULL);
	assert(interp->L != NULL);
	assert(modname != NULL);

	interp->err[0] = '\0';
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

int interp_eval(interp_t *interp, const char *code) {
	assert(interp != NULL);
	assert(interp->L != NULL);
	assert(code != NULL);

	interp->err[0] = '\0';
	if (luaL_dostring(interp->L, code) != LUA_OK) {
		BIRK_LOADERR(interp);
		return BIRK_ERROR;
	}

	return BIRK_OK;
}

const char *interp_errstr(interp_t *interp) {
	assert(interp != NULL);
	return interp->err;
}
