#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <procinfo.h>
extern int getargs(void *, int, char *, int);

const char* getprogname (void) {
    static const char* name = NULL;
    static char buffer[PATH_MAX];

    if (!name) {
        struct procentry64 entry;
        pid_t pid = getpid();

        if(getprocs64(&entry, sizeof(entry), NULL, 0, &pid, 1) > 0)
        {
            if(getargs(&entry, sizeof(entry), buffer, sizeof(buffer)) == 0)
            {
                const char *p;
                name = buffer;
                p = strrchr (name, '/');
                if (p != NULL)
                    name = p + 1;
            }
        }
    }

    return name;
}
