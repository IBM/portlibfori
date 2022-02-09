#ifndef LIBUTIL_STDLIB_H
#define LIBUTIL_STDLIB_H

/* Include the real stdlib.h. We don't want to recreate all the stuff */
/* it contains - just add libutil_isatty declaration to it */
#include_next <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char* libutil_getprogname (void);
extern void libutil_setprogname(const char *);

#ifdef __cplusplus
}
#endif

#endif
