#ifndef LIBUTIL_STDLIB_H
#define LIBUTIL_STDLIB_H

/* Include the real stdlib.h. We don't want to recreate all the stuff */
/* it contains - just add libutil_isatty declaration to it */
#include_next <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* libutil_getprogname(void);
void libutil_setprogname(const char *);
const char* getprogname(void) __asm__ ("libutil_getprogname");
void setprogname(const char *) __asm__ ("libutil_setprogname");

#ifdef __cplusplus
}
#endif

#endif
