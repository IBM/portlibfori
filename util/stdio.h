  
#ifndef LIBUTIL_STDIO_H
#define LIBUTIL_STDIO_H

#include_next <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Amazingly, this works on XLC 13.1.2 */
extern int asprintf(char **ret, const char *format, ...)  __asm__("libutil_asprintf");
extern int vasprintf(char **ret, const char *format, va_list args)  __asm__("libutil_vasprintf");

#ifdef __cplusplus
}
#endif

#endif
