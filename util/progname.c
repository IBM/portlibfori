#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* __progname = NULL;

const char* libutil_getprogname (void) {
    return __progname;
}

void libutil_setprogname (const char* progname) {
    const char *p;
    p = strrchr (progname, '/');
    if (p != NULL) {
        __progname = p + 1;
    } else {
        __progname = progname;
    }
    return;
}
