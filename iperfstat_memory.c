#include <errno.h>
#include <string.h>
#include "as400_protos.h"
#include "ibmiperfstat.h"

/*
 * NAME: iperfstat_memory_getMSsize
 *
 * FUNCTION: Retrieves Main Storage Size in kilobytes
 *
 * PARAMETER:NULL
 *
 * RETURNS:
 * main storage size is returned if success.
 * -1 is returned if fail.
 */
int iperfstat_memory_getMSsize()
{
   const char* objname = "QWCRSSTS";
   const char* libname = "QSYS";

   char ilepContainer[sizeof(ILEpointer) + 16];
   ILEpointer* qsyrusri_pointer= (ILEpointer*)(((size_t)(ilepContainer) + 0xf) & ~0xf);

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
   } errcode;
   void *qsyrusri_argv[6];

   memset(ilepContainer, 0 , sizeof(ilepContainer));

   /*Set the IBM i pointer to the QSYS/QWCRSSTS *PGM object*/
   if(0 != _RSLOBJ2(qsyrusri_pointer, RSLOBJ_TS_PGM, objname, libname)) {
         return -1;
   }
   /* initialize the QWCRSSTS returned info structure and error code structure  */
   memset(rcvr, 0, sizeof(rcvr));
   memset(&errcode, 0, sizeof(errcode));
   errcode.bytes_provided = sizeof(errcode);

   /* initialize the array of argument pointers for the API */
   qsyrusri_argv[0] = &rcvr;
   qsyrusri_argv[1] = &rcvrlen;
   qsyrusri_argv[2] = &format;
   qsyrusri_argv[3] = &reset_status;
   qsyrusri_argv[4] = &errcode;
   qsyrusri_argv[5] = NULL;

   /* Call the IBM i QWCRSSTS API from PASE for i */
   if(0 != _PGMCALL((const ILEpointer*)qsyrusri_pointer, (void*)&qsyrusri_argv, 0)) {
 	return -1;
   }

   if(0 != errcode.bytes_available)
   {
       return -1;
   }

   return (*((int *)(&rcvr[72]))); /* Main storage size, in kilobytes*/

}

