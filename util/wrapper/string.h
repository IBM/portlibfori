#ifndef LIBUTIL_STRING_H
#define LIBUTIL_SSTRING_H

/* Include the real string.h. We don't want to recreate all the stuff */
/* it contains - just more stuff */
#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XXX: Add back the bounded attributes? */
size_t	 strlcat(char *, const char *, size_t) __asm__("libutil_strlcat");
size_t	 strlcpy(char *, const char *, size_t) __asm__("libutil_strlcpy");

#ifdef __cplusplus
}
#endif

#endif
