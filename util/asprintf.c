#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

int libutil_vasprintf(char **ret, const char *format, va_list args)
{
    *ret = NULL;

    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (size == -1) return size;
    
    char* str = malloc(size+1);
    
    if (str == NULL) return -1;

    size = vsnprintf(str, (size_t) size + 1, format, args);
    
    if (size == -1)
    {
        free(str);
        return -1;
    }

    *ret = str;

    return size;
}

int libutil_asprintf(char **ret, const char *format, ...)
{
  int r;
  va_list args;
  va_start(args, format);
  r = libutil_vasprintf(ret, format, args);
  va_end(args);
  return r;
}
