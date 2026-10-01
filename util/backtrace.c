#ifdef __powerpc64__
#define __XCOFF64__
#define __LDINFO_PTRACE64__
#else
#define __XCOFF32__
#define __LDINFO_PTRACE32__
#endif

#ifdef __powerpc64__
#define TRAMPOLINE_OFFSET 0xA8
#else
#define TRAMPOLINE_OFFSET 0x11C
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <execinfo.h>

#include <sys/ldr.h>
#include <sys/seg.h>
#include <sys/debug.h>
#include <sys/uio.h> // writev
#include <xcoff.h>

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
        void *lr = sp[2];
        frames[i] = lr;
        // This might be a signal handler frame, which means the back chain is
        // useless (missing/in the weeds), so look at what's in the frame. In
        // this case, what would be the back chain is in one of the fields of
        // the frame. We need to guess if this is a signal handler frame
        // though; our heuristic is the address being lower than the base of
        // text; the signal trampoline is around ~0x3680 under PASE, but AIX
        // has it at a different address around ~0x4800. The value we use for
        // the displacement is verified to be the proper offset by GDB, and we
        // employ a similar heuristic.
        // XXX: What about syscalls?
        if (lr < (void*)TEXTORG && lr != NULL) {
            sp = (void*)((uint64_t)sp + TRAMPOLINE_OFFSET);
        }
    }
    
    return i;
}

// The memory is allocated like a stack, with the char pointers at the top
// growing down and the char data they point to at the bottom, growing up.
// The pointers are initially stored as offsets from the end of the buffer,
// which lets us avoid having to  adjust the pointers when the buffer is
// realloc'd - we merely have to shift the char data to the end of the buffer.
// We keep track of metadata about the object using this struct which lives
// at the end of the memory allocation.
//
// NOTE: This may be more complicated than just allocating a bunch of individual
// char pointers and an array, then moving everything to a single buffer at the
// end, but it was fun to write.


typedef struct {
    // size of the entire stack
    uintptr_t size;
    // number of string pointers in the array
    size_t    count;
    // pointer to the start of the allocation
    char*     top;
    // offset from top to the string data stack
    uintptr_t bottom;
} _stack_t;

static inline size_t stack_bottom_size(_stack_t* stack) {
    return stack->size - stack->bottom;
}

static inline size_t stack_avail(_stack_t* stack) {
    size_t array_size = sizeof(void*) * stack->count;
    return stack->bottom - array_size;
}

static _stack_t* stack_init(size_t count, size_t size)
{
    const uintptr_t initial_size = sizeof(_stack_t) + count * (sizeof(char*) + size);
    char* top = malloc(initial_size);
    if (!top) return NULL;

    // The stack header goes at the end of the allocation
    _stack_t* stack = (_stack_t*) (top + initial_size - sizeof(_stack_t));
    stack->size = initial_size;
    stack->count = 0;
    stack->top = top;
    stack->bottom = (uintptr_t) stack - (uintptr_t) top;

    return stack;
}

static void stack_free(_stack_t* stack) {
    free(stack->top);
}


// Grow the stack
//
// The stack size is doubled until it is large enough to hold the given number
// of bytes. The existing stack is realloc'd to this new size and the trailing
// string data and stack info is moved to the end of the new allocation.
//
// NOTES:
// - the stack is always *at least* doubled, even if it's already large enough
// - because the offsets are stored relative to the end of the stack, they need
//   no adjustment
//
// Diagram of this operation:
// ╔════   old stack  ════╗                     ╔════   new stack  ════╗
// ║   ┍╸> offset 1       ║                     ║   ┍╸> offset 1       ║
// ║   │   offset 2       ║                     ║   │   offset 2       ║
// ║   │   offset 3       ║                     ║   │   offset 3       ║
// ║   │   ...            ║                     ║   │   ...            ║
// ║   │   offset N       ║                     ║   │   offset N       ║
// ║   │   ...            ║                     ║   │   ...            ║
// ║   │                  ║                     ║   │                  ║
// S   │                  ║                     ║   │                  ║
// ║   │ ┍╸> string data  ║ ━━━━━━━━━┓          ║   │                  ║
// ║   │ │   string data  ║          ┃          ║   │                  ║
// ║   │ │   string data  ║          ┃          ║   │                  ║
// ║   │ │   header       ║          ┃          ║   │                  ║
// ║   │ │     size (S)   ║          ┃          ║   │                  ║
// ║   │ │     count (N)  ║          ┃          ║   │                  ║
// ║   ┕╸│━━━━ top        ║          ┃          ║   │                  ║
// ║     ┕━━━━ bottom     ║          ┃          ║   │                  ║
// ╚══════════════════════╝ ━┓       ┃         S*2 ┈│┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈╢
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┃          ║   │                  ║
//                           ┃       ┗━━━━━━━>  ║   │ ┍╸> string data  ║
//                           ┃                  ║   │ │   string data  ║
//                           ┃                  ║   │ │   string data  ║
//                           ┃                  ║   │ │   header       ║
//                           ┃                  ║   │ │     size (S*2) ║
//                           ┃                  ║   │ │     count (N)  ║
//                           ┃                  ║   ┕╸│━━━━ top        ║
//                           ┃                  ║     ┕━━━━ bottom     ║
//                           ┗━━━━━━━━━━━━━━━>  ╚══════════════════════╝
static int stack_grow(_stack_t** stack_ptr, size_t needed)
{
    _stack_t* stack = *stack_ptr;

    size_t total_needed = stack->size + needed;
    uintptr_t newsize = stack->size;
    do {
        newsize *= 2;
    } while (newsize < total_needed);

    char* top = realloc(stack->top, newsize);
    if(!top) return -1;

    // NOTE: Since we doubled the size, memcpy will never overlap and is therefore
    // safe to use instead of memmove here
    char* oldbottom = top + stack->bottom;
    char* newbottom = oldbottom + stack->size;
    memcpy(newbottom, oldbottom, stack_bottom_size(stack));


    // We've moved the stack in the memcpy, but we need to update its fields
    _stack_t* newstack = (_stack_t*) (top + newsize - sizeof(_stack_t));
    newstack->size = newsize;
    newstack->top = top;
    newstack->bottom += (newsize - stack->size);

    *stack_ptr = newstack;
    return 0;
}


// Push a string into the stack
//
// The bottom pointer is moved back enough to store the string (str_len+1)
// bytes, then the offset to this location from the end of the stack is stored
// at the current string index and the index is incremented.
//
// NOTES:
// - str need not be null-terminated
// - stack will be grown as-needed
static int stack_push(_stack_t** stack_ptr, const char* str, size_t str_len)
{
    _stack_t* stack = *stack_ptr;

    // check if enough space available for the string and its pointer
    if (stack_avail(stack) < str_len + 1 + sizeof(char*)) {
        int rc = stack_grow(stack_ptr, str_len);
        if(rc < 0) return rc;
        stack = *stack_ptr;
    }

    // Adjust the offset up to hold the string, copy it, and null-terminate
    stack->bottom -= str_len + 1;
    memcpy(stack->top + stack->bottom, str, str_len);
    stack->top[stack->bottom + str_len] = 0;

    // Store the offset to the string in the array and increment the count.
    // NOTE: The offset needs to be from the end or it will be broken after the grow
    uintptr_t* arr = (uintptr_t*) stack->top;
    arr[stack->count++] = stack->size - stack->bottom;

    return 0;
}


// Finalize the stack for returning to the user
//
// The offsets stored in the string array are converted to pointers to the
// string data and the trailing stack data is cleared. The array of string
// pointers from the stack is returned, which must be freed by the user using
// free().
static char** stack_finalize(_stack_t* stack) {
    // loop through each entry and convert the offset to a real pointer
    char** arr = (char**) stack->top;
    for(size_t i = 0; i < stack->count; ++i)
    {
        arr[i] = stack->top + (stack->size - (uintptr_t) arr[i]);
    }

    // Clear the stack info so the stack cannot be re-used
    memset(stack, 0, sizeof(*stack));

    return arr;
}

typedef struct {
    const char* name;
    const char* file;
    const char* member;
    unsigned name_len;
    unsigned offset;
} sym_info_t;


// Retrieve symbol information for the given address
//
// The address is looked up in the info provided by loadquery() to find the
// module it was loaded from. If ldinfo is NULL or we can't find it, filename
// and member will be returned as NULL.
//
// From the given address, the instruction stream will be walked until a
// traceback table is found. Only up to 64k bytes (or until the end of the text
// segment, if known) will be searched. Once found, the function name and its
// length are set. The function name will not be null-terminated.
//
// If no traceback table is found or it doesn't contain a function name, -1 is
// returned. This should be rare, unless the function is *very* big or a
// traceback is not defined. The latter only happens on GCC (12.5) when one of
// the following options is specified:
// - -mtraceback=part or -mtraceback=no
// - -finhibit-size-directive
// - -Os
static int sym_info(struct ld_info* ldinfo, sym_info_t* info, const void* addr) {
    uintptr_t a = (uintptr_t) addr;
    info->file = NULL;
    info->member = NULL;

    uintptr_t start = 0;
    uintptr_t end = 0;

    // ldinfo may be NULL
    while (ldinfo) {
        start = (uintptr_t) ldinfo->ldinfo_textorg;
        end = start + ldinfo->ldinfo_textsize;

        if (a >= start && a < end) {
            size_t filename_len = strlen(ldinfo->ldinfo_filename);
            info->file = ldinfo->ldinfo_filename;
            info->member = ldinfo->ldinfo_filename + filename_len + 1;
            break;
        }

        if (!ldinfo->ldinfo_next) {
            // No more entries to check. Mark the start and end as invalid
            ldinfo = NULL;
            start = end = 0;
            break;
        }

        // ldinfo_next is not a pointer, but a *byte* offset from the current
        // ldinfo pointer
        ldinfo = (struct ld_info*) ((char*) ldinfo + ldinfo->ldinfo_next);
    }

    const struct tbtable_short* tb = NULL;

    // Search up to 64k bytes for the traceback table
    int search_bytes = 65536;
    if (end && a + search_bytes > end) {
        // We found the end of the text segment for this function and we could
        // go past it, limit the search to the end of the segment

        search_bytes = end - a;
    }
    // Convert bytes to instruction count
    int search_count = search_bytes / 4;

    const uint32_t* instr = addr;
    for (int i = 0; i < search_count; i++) {
        if (instr[i]) continue;

        tb = (const struct tbtable_short*) &instr[i+1];
        break;
    }

    if (!tb) return -1;

    // There are extended traceback table fields that start after
    // the traceback table. These fields may or may not exist and
    // flags in the traceback table must be checked.
    const char* tb_ext = (char*) &tb[1];

    // A function name is not present in the table, no need to check
    // any of the other flags just return now
    if (!tb->name_present) return -1;

    // parminfo (unsigned int): exists if fixedparms or floatparms != 0
    if (tb->fixedparms || tb->floatparms) {
        tb_ext += sizeof(unsigned);
    }

    // tb_offset (unsigned int): exists if has_tboff bit is set
    if (tb->has_tboff) {
        unsigned tb_offset = *(unsigned*) tb_ext;
        tb_ext += sizeof(unsigned);

        uintptr_t start = (uintptr_t) tb - tb_offset;
        info->offset = (uintptr_t) addr - start;
    }
    else {
        info->offset = 0;
    }

    // hand_mask (int): exists if int_hndl bit is set
    if (tb->int_hndl) {
        tb_ext += sizeof(int);
    }

    // ctl_info (int), ctl_info_disp (int[ctl_info]): exists if has_ctl bit is set
    if (tb->has_ctl) {
       int ctl_info = *(const int*) tb_ext;
       tb_ext += sizeof(ctl_info) + sizeof(int) * ctl_info;
    }

    // name_len (short), name (char[name_len]): exists if name_present bit is set
    // NOTE: We already checked this above
    unsigned short name_len = *(const unsigned short*) tb_ext;
    const char* name = tb_ext + sizeof(short);
    info->name = name;
    info->name_len = name_len;

    return 0;
}

// Format strings used based on what info we can gather about the address.
// If we happen to be at the first instruction of the function (offset 0)
// then the offset will not be printed.
// These are based off the FreeBSD formatting.

// Address only. When we can't find a traceback table
#define BT_FMT_ADDR "0x%p"
// Address and name only. We couldn't find it in the loadquery info.
#define BT_FMT_SYM "0x%p <%.*s>"
#define BT_FMT_SYM_OFFSET "0x%p <%.*s+0x%x>"
// Address, name, and file path.
#define BT_FMT_SYM_FILE "0x%p <%.*s> at %s"
#define BT_FMT_SYM_FILE_OFFSET "0x%p <%.*s+0x%x> at %s"
// Address, name, file path with member
#define BT_FMT_SYM_MEMBER "0x%p <%.*s> at %s(%s)"
#define BT_FMT_SYM_MEMBER_OFFSET "0x%p <%.*s+0x%x> at %s(%s)"

// Symbol loop macro for body of backtrace_symbols* functions
//
// This will call loadquery() to get the shared library info for symbol
// filenames and then loop over the frames to look up the symbol information.
// Depending on the information retrieved, it determines a format string and
// generates a string representation of the frame. If the line that would be
// generated is longer than 1024 bytes, it will be truncated and will end with
// "...".
//
// The string and its length (not including null terminator) will be passed to
// the provided ITER macro.
//
// NOTE: We currently only allocate 4096 bytes (1 page) for the loadquery
// buffer. Each entry is either 24 (32-bit) or 44 (64-bit) bytes plus filename
// and member name length. Assuming most libraries are in /QOpenSys/pkgs/lib or
// /QOpenSys/usr/lib, each entry should be about 75-100 bytes on average. This
// gives enough room for about 40-55 shared libraries, which is likely enough
// for nearly all cases. If we go beyond that, we simply don't output
// filenames currently.
//
// TODO: Allocate in a loop until we get all the data from loadquery

#define SYM_LOOP(FRAMES, COUNT, ITER) \
{ \
    size_t ldbuf_size = 4096; \
    char* ldbuf = malloc(ldbuf_size); \
    if (loadquery(L_GETINFO, ldbuf, ldbuf_size) < 0) { \
        free(ldbuf); \
        ldbuf = NULL; \
    } \
    struct ld_info* ldinfo = (struct ld_info*) ldbuf; \
    for(size_t _frame_idx = 0; _frame_idx < COUNT; ++_frame_idx) { \
        void* addr = FRAMES[_frame_idx]; \
        const char* fmt; \
        sym_info_t info; \
        if (sym_info(ldinfo, &info, addr) < 0 || !info.name) fmt = BT_FMT_ADDR; \
        else if (!info.file) fmt = info.offset ? BT_FMT_SYM_OFFSET : BT_FMT_SYM; \
        else if (!info.member[0]) fmt = info.offset ? BT_FMT_SYM_FILE_OFFSET : BT_FMT_SYM_FILE; \
        else fmt = info.offset ? BT_FMT_SYM_MEMBER_OFFSET : BT_FMT_SYM_MEMBER; \
        char buff[1024]; \
        size_t len = snprintf(buff, sizeof(buff), fmt, addr, info.name_len, info.name, info.offset, info.file, info.member); \
        if (len > sizeof(buff)) { \
            buff[sizeof(buff)-4] = buff[sizeof(buff)-3] = buff[sizeof(buff)-2] = '.'; \
            len = sizeof(buff)-1; \
        } \
        ITER(buff, len); \
    } \
    free(ldbuf); \
} \
(void) 0

char** backtrace_symbols(void* const* frames, size_t count)
{
    if(!count) return NULL;

    // Default to 100 characters per line
    _stack_t* stack = stack_init(count, 100);
    if (!stack) return NULL;

    // For each line, push it to the our stack object
#define SYMBOLS_ITER(buff, len) \
        if (stack_push(&stack, buff, len)) { \
            stack_free(stack); \
            return NULL; \
        } \
        (void) 0

    SYM_LOOP(frames, count, SYMBOLS_ITER);

    // Convert the stack object to the linear array format
    return stack_finalize(stack);
}

void backtrace_symbols_fd(void *const *frames, size_t count, int fd) {
    struct iovec vec[2];
    vec[1].iov_base = (void*) "\n";
    vec[1].iov_len = 1;

    // For each line, write it and a newline to the given fd
    //
    // NOTE: Using writev we don't have to copy the line to add the newline or
    // call write twice
#define SYMBOLS_FD_ITER(str, len) \
    vec[0].iov_base = str; \
    vec[0].iov_len = len; \
    writev(fd, vec, 2)

    SYM_LOOP(frames, count, SYMBOLS_FD_ITER);
}
