
#ifndef LIBIBMI_EBCUTILS_H
#define LIBIBMI_EBCUTILS_H

#include <stdio.h>
#include <iconv.h>
int to_37_spacepadded_nts(char *out, size_t out_len, const char *in);
int from_job_ccsid(char *out, size_t out_len, const char *in);
int to_job_ccsid(char *out, size_t out_len, const char *in);

#endif