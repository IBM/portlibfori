#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <procinfo.h>
extern int getargs(void *, int, char *, int);

static const char* __progname = NULL;

const char* libutil_getprogname (void) {
    return __progname;
}

void libutil_setprogname (const char* progname) {
    const char *p;
    p = strrchr (progname, '/');
    if (p != NULL) {
        __progname = p + 1;
    } else {
        __progname = progname;
    }
    return;
}

__attribute__((constructor))
static void init_progname() {
    static char buffer[PATH_MAX];
    struct procentry64 entry;
    pid_t pid = getpid();

    if (getprocs64(&entry, sizeof(entry), NULL, 0, &pid, 1) != 1)
        return;

    if (getargs(&entry, sizeof(entry), buffer, sizeof(buffer)) != 0)
        return;

    libutil_setprogname(buffer);
}
