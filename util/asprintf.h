#ifndef LIBUTIL_SYS_FILE_H
#define LIBUTIL_SYS_FILE_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int vasprintf(char **ret, const char *format, va_list args);

int asprintf(char **ret, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
