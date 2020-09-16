#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

int 
vasprintf(char **ret, const char *format, va_list args)
{
    *ret = NULL;
    
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (size == -1)
        return size;
    
    char* str = malloc(size+1);
    if (str == NULL)
        return -1;

    va_start(args, format);
    size = vsnprintf(str, (size_t) size + 1, format, args);
    va_end(args);   
    
    if (size == -1)
        free(str);
    
    *ret = str;

    return size;
}

int 
asprintf(char **ret, const char *format, ...)
{
  int r;
  va_list args;
  r = vasprintf(ret, format, args);
  return(r);
}
