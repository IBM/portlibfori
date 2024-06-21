// #define __XCOFF_HYBRID__
#ifdef __powerpc64__
#define __LDINFO_PTRACE64__
#define __XCOFF64__
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

#include <sys/ldr.h>
#include <xcoff.h>

#include <assert.h>

#if 1
///////////////////////////////////////////////////
#include <ctype.h>

static void printHex(const void* data, size_t size)
{
    unsigned char* bytes = (unsigned char*) data;
    
    size_t full = size & ~0xf;
    
    size_t i;
    for(i = 0; i < full; i += 16)
    {
        printf("%04x %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X " \
               "<%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c>\n",
               i,
               
               bytes[i+0x0], bytes[i+0x1], bytes[i+0x2], bytes[i+0x3],
               bytes[i+0x4], bytes[i+0x5], bytes[i+0x6], bytes[i+0x7],
               bytes[i+0x8], bytes[i+0x9], bytes[i+0xa], bytes[i+0xb],
               bytes[i+0xc], bytes[i+0xd], bytes[i+0xe], bytes[i+0xf],
               
               isprint(bytes[i+0x0]) ? bytes[i+0x0] : '.', isprint(bytes[i+0x1]) ? bytes[i+0x1] : '.', isprint(bytes[i+0x2]) ? bytes[i+0x2] : '.', isprint(bytes[i+0x3]) ? bytes[i+0x3] : '.',
               isprint(bytes[i+0x4]) ? bytes[i+0x4] : '.', isprint(bytes[i+0x5]) ? bytes[i+0x5] : '.', isprint(bytes[i+0x6]) ? bytes[i+0x6] : '.', isprint(bytes[i+0x7]) ? bytes[i+0x7] : '.',
               isprint(bytes[i+0x8]) ? bytes[i+0x8] : '.', isprint(bytes[i+0x9]) ? bytes[i+0x9] : '.', isprint(bytes[i+0xa]) ? bytes[i+0xa] : '.', isprint(bytes[i+0xb]) ? bytes[i+0xb] : '.',
               isprint(bytes[i+0xc]) ? bytes[i+0xc] : '.', isprint(bytes[i+0xd]) ? bytes[i+0xd] : '.', isprint(bytes[i+0xe]) ? bytes[i+0xe] : '.', isprint(bytes[i+0xf]) ? bytes[i+0xf] : '.'
              );
    }
    
    if(i < size)
    {
        size_t remain = size - i;
        
        printf("%04x ", i);
        
        for(size_t j = i; j < size; ++j)
        {
            printf("%02X", bytes[i]);
            if((j+1) % 4 == 0) printf(" ");
        }
        
        for(size_t j = i+remain; j < i+16; ++j)
        {
            printf("  ");
            if((j+1) % 4 == 0) printf(" ");
        }
        
        printf("<");
        
        for(size_t j = i; j < size; ++j)
        {
            printf("%c", isprint(bytes[i+0x0]) ? bytes[i] : '.');
        }
        
        printf(">\n");
    }
    
    printf("\n");
}

///////////////////////////////////////////////////
#endif

static void** getsp()
{
    // POWER reserves r1 for the stack pointer
    register void** sp asm ("r1");
    return (void**)sp[0];
    
}

size_t backtrace(void** buffer, size_t size)
{
    size_t i;
    
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
    // We can walk the stack using the back chain reference at SP[0]
    // and the return address (saved link register) at SP[2]
    
    // We ignore the count stack frame (this function)
    void** sp = getsp()[0];
    for(i = 0; i < size && sp; ++i, sp = (void**) sp[0]) {
        buffer[i] = sp[2];
    }
    
    return i;
}

#if 0
void getsyms(uintptr_t real_start, size_t real_size, void* /*const*/* frames, size_t size)
{
    struct stat64 st;
    int fd = open("a.out", O_RDONLY);
    
    fstat64(fd, &st);
    
    char* loadmodule = mmap(NULL, st.st_size, PROT_READ, 0, fd, 0);
    
    printf("loadmodule = %p\n", loadmodule);
    
    struct xcoffhdr* xcoff = (struct xcoffhdr*) loadmodule;
    struct filehdr* fh = &xcoff->filehdr;
    struct aouthdr* oh = &xcoff->aouthdr;
    
    printf("magic = 0x%04hX, sections = %d\n", fh->f_magic, (int)fh->f_nscns);
    
    printf("filehdr: 0x%p\n", fh);
    printf("aouthdr: 0x%p\n", oh);
    
    int text_section = -5;
    if(fh->f_opthdr) {
        printf("o_text_start: 0x%p\n", oh->o_text_start);
        text_section = oh->o_sntext;
    }
    
    char* stringtab = NULL;
    
    struct scnhdr* sh = (struct scnhdr*)(((char*)oh) + fh->f_opthdr);
    for(int i = 0; i < (int)fh->f_nscns; ++i) {
        printf("header[%d]: 0x%p %s\n", i, &sh[i], sh[i].s_name);
        printf("section[%d]: 0x%p - 0x%p\n", i, (loadmodule + sh[i].s_scnptr), (loadmodule + sh[i].s_scnptr + sh[i].s_size));
    }
    
    // 72
    
//     if(fh->f_opthdr) {
//         printf("loader section: %d\n", (int) oh->o_snloader);
        
//         struct scnhdr* lh = &sh[oh->o_snloader-1];
//         
//         printf("%s\n", lh->s_name);
//         struct ldhdr* ld = (struct ldhdr*)(loadmodule + lh->s_scnptr);
//         
//         printf("ldhdr: 0x%p\n", ld);
//         
//         
//         
//         printf("l_version = %d\n", ld->l_version);
//         printf("l_nsyms = %d\n", ld->l_nsyms);
//         printf("l_nreloc = %d\n", ld->l_nreloc);
//         printf("l_istlen = %d\n", ld->l_istlen);
//         printf("l_nimpid = %d\n", ld->l_nimpid);
//         printf("l_version = %d\n", ld->l_version);
//         printf("l_impoff = %ld\n", (long) ld->l_impoff);
//         printf("l_stlen = %d\n", ld->l_stlen);
//         printf("l_stoff = %ld\n", (long) ld->l_stoff);
// #ifdef __powerpc64__
//         printf("l_symoff = %ld\n", (long) ld->l_symoff);
//         printf("l_rldoff = %d\n", (int) ld->l_rldoff);
//         
//         struct ldsym* symtab = (struct ldsym*)(((char*)ld) + ld->l_symoff);
// #else
//         struct ldsym* symtab = (struct ldsym*)(((char*)ld) + ld->l_impoff + ld->l_istlen);
// #endif
//         printf("symtab: 0x%p\n", symtab);
//         stringtab = ((char*)ld) + ld->l_stoff;
//         
//         if(symtab) {
//             for(int i = 0; i < ld->l_nsyms; ++i) {
//                 struct ldsym* syment = &symtab[i];
//                 
//                 char* name = stringtab + syment->l_offset;
//                 printf("%d: %s\n", i, name);
// //                 printf("l_offset: %d\n", syment->l_offset);
//                 printf("l_value: %p\n", syment->l_value);
// //                 printf("l_scnum: %hd\n", syment->l_scnum);
// //                 printf("l_smtype: %hhd\n", syment->l_smtype);
// //                 printf("l_smclas: %hhd\n", syment->l_smclas);
// //                 printf("l_ifile: %d\n", syment->l_ifile);
// //                 printf("l_parm: %d\n", syment->l_parm);
//                 
//                 break;
// //                 printf("%s\n", name);
//             }
//         }
//     }
    
//     return;
    
    if(fh->f_nsyms) {
        char* symtab = loadmodule + fh->f_symptr;
        printf("nsyms: %d, symtab: %p\n", fh->f_nsyms, symtab);
        
        char* st = symtab + (fh->f_nsyms * SYMESZ);
        printf("st: 0x%p\n", st);
        
        printf("string table len: %d\n", *(int*) st);
        
        char* name = NULL;
        
        int ent = 0;
        for(int i = 0; i < fh->f_nsyms; ++i, ++ent) {
            struct syment* sym = (struct syment*)(symtab + (i * SYMESZ));
            
//             printf("%d %d\n", i, fh->f_nsyms);
//             printf("syment %d: 0x%p ", ent, sym);
            
            if(sym->n_scnum  == text_section) {
//             if(sym->n_type & 0x0020) {
            if(1) {
#ifndef __powerpc64__
            if (sym->n_zeroes != 0)
            {
//                 printf("%8s\n", sym->n_name);
            }
            else
#endif
            {
//                 printf("name offset = %d\n", sym->n_offset);
//                 printf("%s\n", st + sym->n_offset);
                
                printf("%s\n", st + sym->n_offset);
                
            }
            
//             printf("n_scnum: %hd\n", sym->n_scnum);
//             printf("n_lang: %hhd\n", sym->n_lang);
//             printf("n_cputype: %hhd\n", sym->n_cputype);
            printf("n_type: %hd\n", sym->n_type);
            printf("n_sclass: %hhd\n", sym->n_sclass);
            
            
//             switch(sym->n_sclass) {
//                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
//                 case C_HIDEXT: printf("n_sclass: C_HIDEXT\n"); break;
//                 case C_BLOCK: printf("n_sclass: C_BLOCK\n"); break;
//                 case C_FCN: printf("n_sclass: C_FCN\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
//                 
//                 default:
//                     printf("n_sclass: %hhd\n", sym->n_sclass);
//                     break;
//             }
//             printf("n_numaux: %hhd\n", sym->n_numaux);
            
//             printf("%lX %hX\n", sym->n_value, sym->n_type);
//                 printf("%lX\n", sym->n_value);
                
                uintptr_t real = sym->n_value - oh->o_text_start + real_start;
                printf("%lx 0x%p\n", sym->n_value, real);
//                 
//                 for(size_t i = 0; i < size; ++i) {
//                     if ((uintptr_t)frames[i] < real) {
//                         printf("%s %p\n", name, frames[i]);
//                         frames[i] = (void*)(uintptr_t)-1;
//                     }
//                 }
                
                name = st + sym->n_offset;
//                 printf("\n");
            }
            }
            
            i += sym->n_numaux;
            
//             break;
            
        }
        
    }
    
    


//     if(fh->f_magic == U802TOCMAGIC) {
//     } else {
//     }
}
#endif

#if 0

static const char* getname_from_traceback_table(char* buf, size_t size, uintptr_t iar)
{
    uint32_t* instruction = (uint32_t*)iar;
    while(*instruction) instruction++;
    
    // skip over eye-catcher
    instruction++;
    
    uint32_t* flags = instruction;
    if(!(flags[0] & 0x00000040)) {
        return NULL;
    }
    // skip over flags
    instruction += 2;
    
//     // skip over parminfo
//     if(flags[1] & 0x0000FFFE) {
//         instruction++;
//     }
    
    // skip over tb_offset
    if(flags[0] & 0x00002000) {
        instruction++;
    }
    
    uint16_t* name_len = (uint16_t*) instruction;
    char* name = (char*)(name_len+1);
    
    if(*name_len-1 > size) {
        return NULL;
    }
    
    memcpy(buf, name, *name_len);
    buf[*name_len] = 0;
    
    return buf;
}

#endif

// The memory is allocated like a stack, with the char pointers at the count
// growing down and the char data they point to at the bottom, growing up.
// The pointers are initially stored as offsets from the end of the buffer.
// eg.
//  push
// This allows us to grow the buffer without having to re-adjust the pointers.
// Once the buffer is completely built

// return ptr -> | char* |
//               | char* |
//               | char* |
//               | char* |
//                         <- count
//                 
//                 ...
//               
//                         <- bottom
//               [ char  ]
//               [ data  ]
//               [ here  ]
//               [ ...   ]
//
// When we re-alloc, we need to shift the char data to the end of the new
// buffer and update count. 
// Intially, all the char pointers are offsets from the end of the buffer
// 

typedef struct {
    uintptr_t eyecatcher;
    uintptr_t length;
    uintptr_t count;
    uintptr_t bottom;
} stack_header_t;


static void stack_dump(void** stack) {
//     stack_header_t header;
//     memcpy(&header, *stack, sizeof(header));
//     
//     printHex(*stack, header.length);
}

static int _v = '#';

#define STACK_MAGIC (0x5374436b)

static void* stack_init()
{
    const uintptr_t initial_size = (10 * (sizeof("/QOpenSys/usr/lib/libxlfpthrds_compat.a") + sizeof(char*)));
//     const uintptr_t initial_size = sizeof(stack_header_t)*2 + 10;
    stack_header_t header = {
        STACK_MAGIC,
        initial_size,
        0,
        sizeof(header)
    };
    
    void* stack = malloc(header.length);
    memset(stack, 0, header.length);
    
    memcpy(stack, &header, sizeof(header));
    
    stack_dump(&stack);
    
    return stack;
}

static int stack_grow(void** stack)
{
    assert(*(uintptr_t*)*stack == STACK_MAGIC);
    stack_header_t header;
    memcpy(&header, *stack, sizeof(header));
    
    uintptr_t newlen = header.length * 2;
    
    void* newstack = realloc(*stack, newlen);
//     void* newstack = malloc(newlen);
//     memset(newstack, ++_v, newlen);
//     memcpy(newstack, *stack, header.length);
//     free(*stack);
//     printHex(newstack, newlen);
    if(!newstack) return -1;
    
//     printf("growing stack, %lu -> %lu, %p -> %p\n", header.length, newlen, *stack, newstack);
    
    void* oldbottom = ((char*)newstack) + (header.length - header.bottom);
    void* newbottom = ((char*)newstack) + (newlen - header.bottom);
    
//     printf("bottom %p -> %p\n", oldbottom, newbottom);
    
    memmove(newbottom, oldbottom, header.bottom);
//     printf("%s\n", oldbottom);
    
    header.length = newlen;
    memcpy(newstack, &header, sizeof(header));
    
    
    *stack = newstack;
    stack_dump(stack);
    
    return 0;
}

static int stack_push(void** stack, const char* str, size_t str_len)
{
    assert(*(uintptr_t*)*stack == STACK_MAGIC);
    stack_header_t header;
    memcpy(&header, *stack, sizeof(header));
    
    
//     printf("pushing %s %d\n", str, str_len+1);
    
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
    
    stack_dump(stack);
    
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
    
//     printf("%p %lu %lu %lu\n", stack_buf, header.length, header.count, header.bottom);
    
    // loop through each entry and convert the offset to a real pointer
    char** arr = (char**)stack_buf;
    for(size_t i = 0; i < (size_t) header.count; ++i)
    {
//         printf("arr[%d] = %lu", i, (unsigned long) arr[i]);
        arr[i] = stack_buf + (header.length - (uintptr_t) arr[i]);
//         printf(" -> %p\n", arr[i]);
    }
    
    return arr;
}

const char* get_syment(const struct ld_info* ld, const void* addr, size_t* offset)
{
    const char* loadmodule = (const char*) ld->ldinfo_dataorg;
    
    struct xcoffhdr* xcoff = (struct xcoffhdr*) loadmodule;
    struct filehdr* fh = &xcoff->filehdr;
    struct aouthdr* oh = &xcoff->aouthdr;
    struct scnhdr* sh = (struct scnhdr*)(((char*)oh) + fh->f_opthdr);
    struct scnhdr* th = sh + (oh->o_sntext-1);
    

    uintptr_t text_start = th->s_paddr - th->s_scnptr;
//     printf("%p %lx\n", sh->s_paddr, (unsigned long) sh->s_scnptr);
    
    uintptr_t address = text_start + (uint32_t)(addr - ld->ldinfo_textorg);
//     printf("%p %p %p\n", text_start, ld->ldinfo_textorg, (uint32_t)(addr - ld->ldinfo_textorg));
//     printf("%p\n", address);
    
    if(fh->f_nsyms)
    {
        const char* symtab = loadmodule + fh->f_symptr;
        const char* string_tbl = symtab + (fh->f_nsyms * SYMESZ);
        
        struct syment* s = NULL;
        size_t off = -1;
        
        for (int i = 0; i < fh->f_nsyms; ++i)
        {
            struct syment* sym = (struct syment*)(symtab + (i * SYMESZ));
            
            // Skip symbols not in the .text section
            if(sym->n_scnum  != oh->o_sntext) goto next;
            
            // If the start of the symbol is after this address, it must
            // not be this function so skip it
            if(sym->n_value > address) goto next;
            
            switch(sym->n_sclass) {
                case C_EXT:
                case C_HIDEXT:
                    break;
                    
                default:
                    goto next;
            }
            
            // If we haven't set s or sym is closer to address than s
            // set s to sym
            size_t noff = address - sym->n_value;
            
            if (!s || noff < off) {
                
//                 printf("%p %lu %s\n", sym->n_value, noff, string_tbl + sym->n_offset);
//                 if(s)
//                     printf("%p %lu %s\n\n", s->n_value, off, string_tbl + s->n_offset);
                s = sym;
                off = noff;
            }/* else if(
                0 || address - sym->n_value < address - s->n_value) {
            
            
                s = sym;
            }*/
            
        next:
            i += sym->n_numaux;
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
            
            
//             switch(sym->n_sclass) {
//                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
//                 case C_HIDEXT: printf("n_sclass: C_HIDEXT\n"); break;
//                 case C_BLOCK: printf("n_sclass: C_BLOCK\n"); break;
//                 case C_FCN: printf("n_sclass: C_FCN\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
// //                 case C_EXT: printf("n_sclass: C_EXT\n"); break;
//                 
//                 default:
//                     printf("n_sclass: %hhd\n", sym->n_sclass);
//                     break;
//             }
                
//                 printf("%lx 0x%p\n", sym->n_value, real);
//                 
//                 for(size_t i = 0; i < size; ++i) {
//                     if ((uintptr_t)frames[i] < real) {
//                         printf("%s %p\n", name, frames[i]);
//                         frames[i] = (void*)(uintptr_t)-1;
//                     }
//                 }
                
//                 printf("\n");
    }
    
    


//     if(fh->f_magic == U802TOCMAGIC) {
//     } else {
//     }
}


char** backtrace_symbols(void* /*const*/* frames, size_t count)
{
    char ldbuf[sizeof(struct ld_info) * 20];
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
            struct ld_info* ld = (struct ld_info*) &ldbuf[offset];
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
            const char* name = get_syment(ld, (void*)iar, &off);
            
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
        struct ld_info* ld = (struct ld_info*) &ldbuf[offset];
        
        if(ld->ldinfo_fd > 0)
        {
            close(ld->ldinfo_fd);
        }
        
        if (ld->ldinfo_next == 0) break;
        offset += ld->ldinfo_next;
    }
    
    return stack_finalize(&stack);
}
