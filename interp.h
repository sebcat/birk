#ifndef __BIRK_INTERP_H
#define __BIRK_INTERP_H

#define BIRK_ERROR	0
#define BIRK_OK		1

typedef struct interp_t interp_t;

interp_t *interp_new();
void interp_free(interp_t *interp);
int interp_load(interp_t *interp, const char *modname);
const char *interp_errstr(interp_t *interp);

#endif
