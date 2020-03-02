#ifndef LIBUTIL_SYS_FILE_H
#define LIBUTIL_SYS_FILE_H

/* Include the real sys/file.h. We don't want to recreate all the stuff */
/* it contains - just override flock */
#include_next <sys/file.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Amazingly, this works on XLC 13.1.2 */
extern int flock(int fd, int operation)  __asm__("libutil_flock");

#ifdef __cplusplus
}
#endif

#endif
