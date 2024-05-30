#ifndef LIBUTIL_FNMATCH_H
#define LIBUTIL_FNMATCH_H

/* Include the real fnmatch.h. We don't want to recreate all the stuff */
/* it contains - just overwrite the fnmatch function declaration in it */
#include_next <fnmatch.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _GNU_SOURCE
#define FNM_FILE_NAME FNM_PATHNAME // Preferred GNU name
#define FNM_CASEFOLD  0x4000000    // Some large power of 2 to avoid collisions
#endif

int fnmatch(const char *, const char*, int) __asm__ ("libutil_fnmatch");

#ifdef __cplusplus
}
#endif

#endif
