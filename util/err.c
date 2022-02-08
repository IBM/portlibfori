
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <procinfo.h>

#include "err.h"

void libutil_vwarn(const char *fmt, va_list args) {
    fprintf(stderr, "%s: ", libuitl_getprogname());
    if (fmt) {
        vfprintf(stderr, fmt, args);
        fprintf(stderr, ": %s\n", strerror(errno));
    }
    else {
        fprintf(stderr, "\n");
    }
}

void libutil_vwarnx(const char *fmt, va_list args) {
    fprintf(stderr, "%s: ", libuitl_getprogname());
    if (fmt) {
        vfprintf(stderr, fmt, args);
    }
    fprintf(stderr, "\n");
}

void libutil_verr(int eval, const char *fmt, va_list args) {
    vwarn(fmt, args);
    exit(eval);
}

void libutil_verrx(int eval, const char *fmt, va_list args) {
    vwarnx(fmt, args);
    exit(eval);
}

void libutil_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vwarn(fmt, args);
    va_end(args);
}

void libutil_warnx(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vwarnx(fmt, args);
    va_end(args);
}

void libutil_err(int eval, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    verr(eval, fmt, args);
}

void libutil_errx(int eval, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    verrx(eval, fmt, args);
}

extern int getargs(void *, int, char *, int);

static const char* __progname = NULL;

const char* libuitl_getprogname (void) {
    return __progname;
}

void libuitl_setprogname (const char* progname) {
    const char *p;
    p = strrchr (progname, '/');
    if (p != NULL) {
        __progname = p + 1;
    } else {
        __progname = progname;
    }
    return;
}
