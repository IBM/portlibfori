#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "ibmiperfstat.h"

/*
 * NAME: iperfstat_cpu_getNumber
 *
 * FUNCTION: Retrieves the fields of the iperfstat_cpu_number_t structure
 *
 * PARAMETER:
 *      Input: userbuf        : the structure to be filled
 *      Output: userbuf       : the filled structure
 *
 * RETURNS:
 * 0 is returned if success.
 * -1 is returned if fail.
 */
int iperfstat_cpu_getNumber(iperfstat_cpu_number_t * userbuf)
{

    if (userbuf == NULL)
    {
        /* parameter error */
        errno = EINVAL;
        return -1;
    }

    memset(userbuf, 0, sizeof(iperfstat_cpu_number_t));

    userbuf->ncpus_onli = sysconf(_SC_NPROCESSORS_ONLN);
    userbuf->ncpus_conf = sysconf(_SC_NPROCESSORS_CONF);

    return 0;

}
