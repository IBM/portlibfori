
#ifndef LIBIBMI_IBMIERRINFO_H
#define LIBIBMI_IBMIERRINFO_H
#include <as400_protos.h>
#include <as400_types.h>

typedef struct
{
  int bytes_provided;
  int bytes_available;
  char msgid[7];
  char reserved[1];
  char exc_data[100];
} error_info_t;

#endif