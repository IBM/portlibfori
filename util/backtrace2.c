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

// On AIX the stack layout is like so:
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
// condition register (CR), and the third being the saved link register (LR)
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

size_t backtrace(void** buffer, size_t count)
{
    size_t i;
    
    // We ignore the current stack frame (this function)
    void** sp = getsp()[0];
    
    // Walk the stack up to count times or we hit the bottom
    // of the stack (whichever is first)
    for(i = 0; i < count && sp; ++i, sp = (void**) sp[0]) {
        buffer[i] = sp[2];
    }
    
    return i;
}

// The memory is allocated like a stack, with the char pointers at the top
// growing down and the char data they point to at the bottom, growing up.
// The pointers are initially stored as offsets from the end of the buffer,
// which lets us avoid having to change the adjust the pointers when the
// buffer is realloc'd - we merely have to shift the char data to the end of
// the buffer.
//
// To keep track of the metadata, we offset the first 4 entries to the end of
// the buffer and store a header in its place at the beginning. The header
// consistes of an eyecatcher, length of the buffer, array count, and offset
// from the end of the buffer the the free space.
//
// NOTE: This may be more complicated than just allocating a bunch of individual
// char pointers and an array, then moving everything to a single buffer at the
// end, but it was fun to write.


typedef struct {
    uintptr_t eyecatcher;
    uintptr_t length;
    uintptr_t count;
    uintptr_t bottom;
} stack_header_t;


#define STACK_MAGIC (0x5374436b)

static void* stack_init()
{
    const uintptr_t initial_size = (10 * (50 + sizeof(char*)));
    stack_header_t header = {
        STACK_MAGIC,
        initial_size,
        0,
        sizeof(header)
    };
    
    void* stack = malloc(header.length);
    memset(stack, 0, header.length);
    memcpy(stack, &header, sizeof(header));
    
    return stack;
}

static int stack_grow(void** stack)
{
    assert(*(uintptr_t*)*stack == STACK_MAGIC);
    stack_header_t header;
    memcpy(&header, *stack, sizeof(header));
    
    uintptr_t newlen = header.length * 2;
    
    void* newstack = realloc(*stack, newlen);
    if(!newstack) return -1;
    
    void* oldbottom = ((char*)newstack) + (header.length - header.bottom);
    void* newbottom = ((char*)newstack) + (newlen - header.bottom);
    
    memcpy(newbottom, oldbottom, header.bottom);
    
    header.length = newlen;
    memcpy(newstack, &header, sizeof(header));
    
    
    *stack = newstack;
    
    return 0;
}

static int stack_push(void** stack, const char* str, size_t str_len)
{
    assert(*(uintptr_t*)*stack == STACK_MAGIC);
    stack_header_t header;
    memcpy(&header, *stack, sizeof(header));
    
    char* stack_buf = (char*)*stack;
    
    // check if not enough space available
    if (header.length - header.bottom - (header.count * sizeof(char*)) < str_len + 1 + sizeof(char*))
    {
        int rc = stack_grow(stack);
        if(rc < 0) return rc;
        stack_buf = (char*)*stack;
        memcpy(&header, stack_buf, sizeof(header));
    }
    
    char** arr = (char**)stack_buf;
    if(header.count < 4) {
        // the first 4 we store at the end of the buffer
        arr = (char**) (stack_buf + (header.length - sizeof(header)));
    }
    
    header.bottom += str_len + 1;
    char* addr = stack_buf + (header.length - header.bottom);
    
    memcpy(addr, str, str_len);
    addr[str_len] = 0;
    
    arr[header.count++] = (char*) header.bottom;
    
    memcpy(*stack, &header, sizeof(header));
    
    return 0;
}

static char** stack_finalize(void** stack)
{
    assert(*(uintptr_t*)*stack == STACK_MAGIC);
    
    stack_header_t header;
    memcpy(&header, *stack, sizeof(header));
    
    char* stack_buf = (char*)*stack;
    
    // replace the header with the swapped first 4 indexes
    memcpy(stack_buf, stack_buf + (header.length - sizeof(header)), sizeof(header));
    
    // loop through each entry and convert the offset to a real pointer
    char** arr = (char**)stack_buf;
    for(size_t i = 0; i < (size_t) header.count; ++i)
    {
        arr[i] = stack_buf + (header.length - (uintptr_t) arr[i]);
    }
    
    return arr;
}

const char* cls_type(int cls)
{
    switch(cls) {
        case C_EXT: return "C_EXT";
        case C_HIDEXT: return "C_HIDEXT";
        case C_WEAKEXT: return "C_WEAKEXT";
        case C_FCN: return "C_FCN";
        case C_BCOMM: return "C_BCOMM";
        case C_BINCL: return "C_BINCL";
        case C_BLOCK: return "C_BLOCK";
        case C_BSTAT: return "C_BSTAT";
        case C_DECL: return "C_DECL";
//         case C_DWARF: return "C_DWARF";
        case C_ECOML: return "C_ECOML";
        case C_ECOMM: return "C_ECOMM";
        case C_EINCL: return "C_EINCL";
        case C_ENTRY: return "C_ENTRY";
        case C_ESTAT: return "C_ESTAT";
        case C_FILE: return "C_FILE";
        case C_FUN: return "C_FUN";
        case C_GSYM: return "C_GSYM";
        case C_GTLS: return "C_GTLS";
        case C_INFO: return "C_INFO";
        case C_LSYM: return "C_LSYM";
        case C_NULL: return "C_NULL";
        case C_PSYM: return "C_PSYM";
        case C_RPSYM: return "C_RPSYM";
        case C_RSYM: return "C_RSYM";
        case C_STAT: return "C_STAT";
        case C_STSYM: return "C_STSYM";
        case C_STTLS: return "C_STTLS";
        case C_TCSYM: return "C_TCSYM";
            
    }
}

const char* get_symbol_name(const struct ld_xinfo* ld, const void* addr, size_t* offset)
{
    // The mmap'd address has been saved in the ldinfo_dataorg
    const struct xcoffhdr* xcoff = (const struct xcoffhdr*) ld->ldinfo_dataorg;
    
    // sanity check: ensure this is an XCOFF file
    // TODO: implement sanity
    
#ifdef __powerpc64__
#define XCOFFMAGIC U64_TOCMAGIC
#else
#define XCOFFMAGIC U802TOCMAGIC
#endif
    
#define CHECK_INVAL_ADDR(addr, start, len) ((char*)addr < (char*) start || (char*)addr > (char*) start + len)

    // If we haven't sanity checked yet
    if(1)
    {
        // not an XCOFF file, bail
//         if(XCOFFMAGIC != *(uint32_t*)xcoff) return NULL;
        
//         if(ld->ldinfo_datasize < sizeof(*xcoff)) return NULL;
    }
    
    const struct filehdr* fh = &xcoff->filehdr;
    
    // If there's no symbol table, we can't look it up 
//     if(!fh->f_nsyms) return NULL;
    
    const struct aouthdr* oh = &xcoff->aouthdr;
    
    
    const struct scnhdr* sh = (struct scnhdr*)(((char*)oh) + fh->f_opthdr);
    
    
//     if(
    const struct scnhdr* th = sh + (oh->o_sntext-1);
    

    // Get the real starting address of the text section
    uintptr_t text_start = th->s_paddr - th->s_scnptr;
    
    // TODO: is this right?
    const char* loadmodule = (const char*) ld->ldinfo_textorg;
    
    // convert the real address to the relocatable address inside the object
    uintptr_t address = text_start + (uint32_t)(addr - ld->ldinfo_textorg);
    
    const char* symtab = loadmodule + fh->f_symptr;
    const char* string_tbl = symtab + (fh->f_nsyms * SYMESZ);
    
    struct syment* s = NULL;
    size_t off = -1;
    
    for (int i = 0; i < fh->f_nsyms;)
    {
        struct syment* sym = (struct syment*)(symtab + (i * SYMESZ));
        
        // Skip symbols not in the .text section
        if(sym->n_scnum  != oh->o_sntext) goto next;
        

        switch(sym->n_sclass) {
            case C_EXT:
            case C_HIDEXT:
            case C_WEAKEXT:
                break;
                
            default:
                goto next;
        }
        
        
        // If the start of the symbol is before or equal to the address,
        // it could be it so check if it's closer than the previous one
        if(sym->n_value <= address)
        {
            // If we haven't set s or sym is closer to address than s
            // set s to sym
            size_t noff = address - sym->n_value;
            
            if (!s || noff < off) {
                s = sym;
                off = noff;
            }
        }
        
        // Symbols always need an _AUX_CSECT entry, but _AUX_FCN
        // entries are optional and typically only found if -g
        // was specified to the compiler. The _AUX_FCN entries
        // 
        if (sym->n_numaux < 2)
        {
            goto next;
        }
        
        AUXENT* aux;
#ifdef __powerpc64__
        int j;
        for(j = 1; j <= sym->n_numaux; ++j)
        {
            aux = (AUXENT*)(symtab + ((i + ++j) * SYMESZ));
            if(aux->x_auxtype.x_auxtype == _AUX_FCN) break;
        }
        
        // _AUX_FCN entry not found
        if(j > sym->n_numaux) goto next;
        
#else
        // 32-bit doesn't have a field for type, _AUX_CSECT must be last
        // so _AUX_FCN must be second from last I guess
        aux = (AUXENT*)(symtab + ((i + sym->n_numaux - 1) * SYMESZ));
#endif
        
        // fn start <= address <= fn end
        if(address < sym->n_value + aux->x_fcn.x_fsize)
        {
            break;
        }
        else
        {
            i = aux->x_fcn.x_endndx;
            continue;
        }
        
    next:
        i += sym->n_numaux + 1;
    }
    
    const char* name;
#ifndef __powerpc64__
    if (s->n_zeroes != 0)
    {
        name = s->n_name;
    }
    else
#endif
    {
        name = string_tbl + s->n_offset;
    }
    
    if(name[0] == '.') name++;
    
    *offset = off;
    return name;
}


char** backtrace_symbols(void* /*const*/* frames, size_t count)
{
    char ldbuf[sizeof(struct ld_xinfo) * 20];
    int rc = loadquery(L_GETINFO, ldbuf, sizeof(ldbuf));
    
    void* stack = stack_init();
    
    char* line = NULL;
    size_t size = 0;
    
    for(size_t i = 0; i < count; ++i)
    {
        uintptr_t iar = (uintptr_t)frames[i];
        
//         printf("looking for %lx\n", iar);
        
        size_t offset = 0;
        while(1)
        {
            struct ld_xinfo* ld = (struct ld_xinfo*) &ldbuf[offset];
    //         printf("next = %d\n", ld->ldinfo_next);
    #ifdef __powerpc64__
//             printf("flags = %d\n", ld->ldinfo_flags);
    #endif
    //         printf("fd = %d\n", ld->ldinfo_fd);
    //         printf("core = %d\n", ld->ldinfo_core);
    //         printf("textorg = 0x%p\n", ld->ldinfo_textorg);
    //         printf("textsize = %ld\n", ld->ldinfo_textsize);
    //         printf("dataorg = 0x%p\n", ld->ldinfo_dataorg);
    //         printf("datasize = %ld\n", ld->ldinfo_datasize);
    //         printf("filename = %s", ld->ldinfo_filename);
            
    //         if(strcmp(ld->ldinfo_filename, "a.out") == 0) {
    //             printf("textorg = 0x%p\n", ld->ldinfo_textorg);
    //             printf("textsize = %ld\n", ld->ldinfo_textsize);
    //             getsyms((uintptr_t)ld->ldinfo_textorg, ld->ldinfo_textsize, frames, size);
    //         }
            
            
//             printf("%s", ld->ldinfo_filename);
//             if (*member)
//                 printf("(%s)\n", member);
//             else
//                 printf("\n");
            
            
            uintptr_t textorg = (uintptr_t) ld->ldinfo_textorg;
            if(iar < textorg || iar > textorg + ld->ldinfo_textsize)
            {
//                 printf("skipping\n");
                goto next;
            }
            
            if(ld->ldinfo_fd < 0)
            {
//                 printf("skipping\n");
                stack_push(&stack, "<unknown>", 9);
                goto next;
            }
            
            if(!ld->ldinfo_fd)
            {
                ld->ldinfo_fd = open(ld->ldinfo_filename, O_RDONLY);
                
                struct stat64 st;
                fstat64(ld->ldinfo_fd, &st);
                
                ld->ldinfo_dataorg = mmap(NULL, st.st_size, PROT_READ, 0, ld->ldinfo_fd, 0);
                ld->ldinfo_datasize = st.st_size;
            }
            
            size_t off;
            const char* name = get_symbol_name(ld, (void*)iar, &off);
            if(!name)
            {
                // The file did not pass sanity checks. Unmap it to prevent wasting time
                // checking it further on.
                munmap(ld->ldinfo_dataorg, ld->ldinfo_datasize);
                ld->ldinfo_dataorg = NULL;
                ld->ldinfo_datasize = 0;
                ld->ldinfo_fd = -1; // already closed
                stack_push(&stack, "<unknown>", 9);
            }
//             char line[1024];
            //char buf[255];
            //const char* name = getname_from_traceback_table(buf, sizeof(buf), iar);
            
            
            // output format:
            //
            // linux
            // ./a.out(myfunc+0x1e) [0x400a4d]
            //
            // solaris
            // /tmp/q:foo+0x8
            //
            // freebsd
            // 0x400ac5 <myfunc3+0x1f> at /usr/home/kadler/a.out
            
            char iar_str[20];
            size_t iar_len = sprintf(iar_str, "0x%p ", iar);
            
            char offset_str[22];
            size_t name_len = 0;
            size_t offset_len = 0;
            size_t symbol_len = 0;
            if(name) {
                name_len = strlen(name);
                offset_len = sprintf(offset_str, "+0x%lx> ", off);
                symbol_len = 1 + name_len + offset_len;
            }
            
            size_t filename_len = strlen(ld->ldinfo_filename);
            char* member = ld->ldinfo_filename + filename_len + 1;
            size_t member_len = strlen(member);
            
            size_t total = iar_len + symbol_len + 3 + member_len + 2 + 1;
            if(total > size) {
                line = realloc(line, total);
                size = total;
            }
            
            char* ptr = line;
            
            memcpy(ptr, iar_str, iar_len);
            ptr += iar_len;
            
            if(symbol_len) {
                *ptr++ = '<';
                
                memcpy(ptr, name, name_len);
                ptr += name_len;
                
                
                memcpy(ptr, offset_str, offset_len);
                ptr += offset_len;
            }
            
            strcpy(ptr, "in ");
            ptr += 3;
            
            memcpy(ptr, ld->ldinfo_filename, filename_len);
            ptr += filename_len;
            
            if(member_len) {
                ptr += sprintf(ptr, "(%s)", member);
            }
            
            *ptr = 0;
            
            stack_push(&stack, line, strlen(line));
            
        next:
            if (ld->ldinfo_next == 0) break;
            offset += ld->ldinfo_next;
        }
    }
    
    // Free up any opened file descriptors
    size_t offset = 0;
    while(1)
    {
        struct ld_xinfo* ld = (struct ld_xinfo*) &ldbuf[offset];
        
        if(ld->ldinfo_fd > 0)
        {
            close(ld->ldinfo_fd);
        }
        
        if (ld->ldinfo_next == 0) break;
        offset += ld->ldinfo_next;
    }
    
    return stack_finalize(&stack);
}
