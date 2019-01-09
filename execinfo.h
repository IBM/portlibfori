#ifndef _EXECINFO_H
#define _EXECINFO_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
size_t backtrace(void** buffer, size_t size);

char** backtrace_symbols(void* const* frames, size_t size);

#ifdef __cplusplus
}
#endif
#endif