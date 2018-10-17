#include <errno.h>
#include <string.h>
#include "as400_protos.h"
#include "ibmiperfstat.h"

/*
 * NAME: iperfstat_memory_get_main_storage_size
 *
 * FUNCTION: Get main storage size in kilobytes
 *
 * PARAMETER:void
 *
 * RETURNS:
 * main storage size is returned if success.
 * -1 is returned if fail.
 */
int iperfstat_memory_get_main_storage_size()
{
   ILEpointer qsyrusri_pointer __attribute__((aligned(16)));

   char rcvr[148];
   int rcvrlen = sizeof(rcvr);

   /*Text 'SSTS0200' in EBCDIC*/
   char format[] = {0xe2, 0xe2, 0xe3, 0xe2, 0xf0, 0xf2, 0xf0, 0xf0};

   /*Text '*YES' in EBCDIC*/
   char reset_status[] = {0x5c, 0xe8, 0xc5, 0xe2, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
   struct {
 	int bytes_provided;
 	int bytes_available;
 	char msgid[7];
 	char reserved;
 	char exception_data[64];
   } errcode = { sizeof(errcode) };
   /* initialize the array of argumnet pointers for the API */
   void *qsyrusri_argv[]={
        &rcvr,
        &rcvrlen,
        &format,
        &reset_status,
        &errcode,
        NULL
   };

   /*Set the IBM i pointer to the QSYS/QWCRSSTS *PGM object*/
   if(0 != _RSLOBJ2(&qsyrusri_pointer, RSLOBJ_TS_PGM, "QWCRSSTS", "QSYS")) {
         return -1;
   }

   /* Call the IBM i QWCRSSTS API from PASE for i */
   if(0 != _PGMCALL(&qsyrusri_pointer, &qsyrusri_argv, 0)) {
 	return -1;
   }

   if(0 != errcode.bytes_available)
   {
       return -1;
   }

   return (*((int *)(&rcvr[72]))); /* Main storage size, in kilobytes*/

}
