
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "err.h"

const char* libutil_getprogname (void);

void libutil_vwarn(const char *fmt, va_list args) {
    fprintf(stderr, "%s: ", libutil_getprogname());
    if (fmt) {
        vfprintf(stderr, fmt, args);
        fprintf(stderr, ": %s\n", strerror(errno));
    }
    else {
        fprintf(stderr, "\n");
    }
}

void libutil_vwarnx(const char *fmt, va_list args) {
    fprintf(stderr, "%s: ", libutil_getprogname());
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
