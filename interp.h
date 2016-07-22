#ifndef __BIRK_INTERP_H
#define __BIRK_INTERP_H

typedef struct interp interp_t;

interp_t *interp_new();
void interp_free(interp_t *interp);
int interp_load(interp_t *interp, const char *modname);
int interp_eval(interp_t *interp, const char *chunk);
const char *interp_errstr(interp_t *interp);

#endif
