#ifdef __powerpc64__
#define __XCOFF64__
#define __LDINFO_PTRACE64__
#else
#define __XCOFF32__
#define __LDINFO_PTRACE32__
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <execinfo.h>

#include <sys/ldr.h>
#include <xcoff.h>

#include <assert.h>

// The AIX ABI defines the stack layout like so:
/*
High Address
            +-> Back chain
            |   Floating point register save area
            |   General register save area
            |   VRSAVE save word (32-bits)
            |   Alignment padding (4 or 12 bytes)
            |   Vector register save area (quadword aligned)
            |   Local variable space
            |   Parameter save area    (SP + 48/24)
            |   TOC save area          (SP + 40/20)
            |   link editor doubleword (SP + 32/16)
            |   compiler doubleword    (SP + 24/12)
            |   LR save area           (SP + 16/8)
            |   CR save area           (SP + 8/4)
  SP  --->  +-- Back chain             (SP + 0)
*/
// We can treat the stack pointer as a void* array. The first
// entry is the previous stack pointer (back chain), the second is the saved
// condition register (CR), the third is the saved link register (LR) ...
//
// We can then walk the stack using the back chain to the previous stack pointer
// and so on until we get to the bottom and the back chain reference is NULL.

static void** getsp()
{
    // POWER ABI reserves r1 for the stack pointer
    register void** sp asm ("r1");
    
    // return parent's stack pointer
    return (void**)sp[0];
    
}

size_t backtrace(void** frames, size_t count)
{
    size_t i;
    
    // We ignore the current stack frame (this function)
    void** sp = getsp()[0];
    
    // Walk the stack up to count times or we hit the bottom
    // of the stack (whichever is first)
    for(i = 0; i < count && sp; ++i, sp = (void**) sp[0]) {
        frames[i] = sp[2];
    }
    
    return i;
}

#ifdef __powerpc64__
#define FMT "0xFFFFFFFFFFFFFFFF"
#else
#define FMT "0xFFFFFFFF"
#endif

static inline int symbol_to_string(char* buffer, const void* symbol) {
    return sprintf(buffer, "0x%p", symbol);
}

char** backtrace_symbols(void* const* frames, size_t count)
{
    if(!count) return NULL;

    size_t data_size = count * (sizeof(char*) + sizeof(FMT));
    char* data = (char*) malloc(data_size);
    
    char* array = data;
    char* str = data + (count * sizeof(char*));
    
    for(size_t i = 0; i < count; ++i)
    {
        int chars_written = symbol_to_string(str, frames[i]);
        if(chars_written < 0) {
            free(data);
            return NULL;
        }
        memcpy(array, &str, sizeof(char*));
        
        // NOTE: chars_written doesn't include null terminator
        str += chars_written+1;
        array += sizeof(char*);
    }
    
    return (char**) data;
}

void backtrace_symbols_fd(void *const *frames, size_t count, int fd) {
    for(size_t i = 0; i < count; ++i) {
        char buff[sizeof(FMT)];

        int chars_written = symbol_to_string(buff, frames[i]);
        if(chars_written >= 0) {
            buff[chars_written] = '\n';
            write(fd, buff, chars_written+1);
        }
        else {
            // This only happens if there's an encoding error
#define ERRSTR "*** error ***\n"
            write(fd, ERRSTR, sizeof(ERRSTR)-1);
        }

    }
}
