#ifndef _EXECINFO_H
#define _EXECINFO_H

#include <stdlib.h>

size_t backtrace(void** buffer, size_t size);

char** backtrace_symbols(void* const* frames, size_t size);

#endif
