#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <as400_protos.h>
#include <libiperf.h>

/*
 * NAME: iperfstat_memory_get_main_storage_size
 *
 * FUNCTION: Get main storage size in kilobytes
 *
 * PARAMETER: uint64_t* out
 *
 * RETURNS:
 *  0 if successful
 * -1 if failure
 * 
 * ERRNO:
 *  EINVAL if out is NULL
 * -1 is returned if fail.
 */
int iperfstat_memory_get_main_storage_size(uint64_t* out)
{
    ILEpointer qsyrusri_pointer __attribute__((aligned(16)));

    char rcvr[148];
    int rcvrlen = sizeof(rcvr);

    /*Text 'SSTS0200' in EBCDIC*/
    static char format[] = {0xe2, 0xe2, 0xe3, 0xe2, 0xf0, 0xf2, 0xf0, 0xf0};

    /*Text '*YES' in EBCDIC*/
    static char reset_status[] = {0x5c, 0xe8, 0xc5, 0xe2, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
    struct {
        int bytes_provided;
        int bytes_available;
        char msgid[7];
        char reserved;
    } errcode = { sizeof(errcode) };

    /* initialize the array of argument pointers for the API */
    void *qsyrusri_argv[]={
        &rcvr,
        &rcvrlen,
        &format,
        &reset_status,
        &errcode,
        NULL
    };

    if (out == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Set the IBM i pointer to the QSYS/QWCRSSTS *PGM object */
    if(0 != _RSLOBJ2(&qsyrusri_pointer, RSLOBJ_TS_PGM, "QWCRSSTS", "QSYS")) {
        return -1;
    }

    /* Call the IBM i QWCRSSTS API from PASE for i */
    if(0 != _PGMCALL(&qsyrusri_pointer, qsyrusri_argv, 0)) {
        return -1;
    }

    if(0 != errcode.bytes_available)
    {
        // TODO: We need to determine the correct errno to set
        // based on the msgid returned
        errno = EFAULT;
        return -1;
    }

    memcpy(out, rcvr + 140, sizeof(*out));
    return 0;
}
