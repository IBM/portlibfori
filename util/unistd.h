#ifndef LIBUTIL_UNISTD_H
#define LIBUTIL_UNISTD_H

/* Include the real unistd.h. We don't want to recreate all the stuff */
/* it contains - just add libutil_isatty declaration to it */
#include_next <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int libutil_isatty(int fd);

#ifdef __cplusplus
}
#endif

#endif
