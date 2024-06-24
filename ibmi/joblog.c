#include <stdarg.h>
#include "joblog.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#include "ibmierrinfo.h"
#include "ebcutils.c"

int joblog_write(const char *_msg_type, const char * _data) {
  return joblog_printf(_msg_type, "%s\n", _data);
}
int joblog_printf(const char *_msg_type, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  char *str = NULL;
  int rc = vasprintf(&str, format, args);
  va_end(args);
  if (NULL == str)
  {
    return -1;
  }

  char *line = str;
  for (char *linePtr = line; *linePtr; linePtr++)
  {
    int pos = linePtr - line;
    if (*linePtr == '\n' || pos >= 512)
    {
      *linePtr = '\0';
      rc = qmhsndpm(_msg_type, (const char*) line);
      if (0 != rc)
      {
        free(str);
        return rc;
      }
      line = linePtr + 1;
    }
  }
  if (0 < strlen(line))
  {
    rc = qmhsndpm(_msg_type, (const char*) line);
    if (0 != rc)
    {
      free(str);
      return rc;
    }
  }
  free(str);
  return rc;
}

int qmhsndpm(const char *_msg_type, const char *_msg )
{
  if(0 == *_msg) {
    return 0;
  }
  static ILEpointer qmhsndpm_pgm __attribute__((aligned(16)));
  int rc = _RSLOBJ2(&qmhsndpm_pgm,
                    RSLOBJ_TS_PGM,
                    "QMHSNDPM",
                    "QSYS");

  //   Message identifier 	Input 	Char(7)
  char msgid[7];
  memset(msgid, 0x40, sizeof(msgid));
  // 2 	Qualified message file name 	Input 	Char(20)
  char msgfile[20];
  memset(msgfile, 0x40, sizeof(msgfile));
  // 3 	Message data or immediate text 	Input 	Char(*)
  char msgdata[1 + strlen(_msg)];
  to_job_ccsid(msgdata, sizeof(msgdata), _msg);
  // 4 	Length of message data or immediate text 	Input 	Binary(4)
  int msglen = sizeof(msgdata);
  msglen = strlen(_msg);
  // 5 	Message type 	Input 	Char(10)
  char msgtype[11];
  to_37_spacepadded_nts(msgtype, sizeof(msgtype), _msg_type);
  // 6 	Call stack entry 	Input 	Char(*) or Pointer
  char callstack[11];
  to_37_spacepadded_nts(callstack, sizeof(callstack), "*");
  // 7 	Call stack counter 	Input 	Binary(4)
  int callstack_cnt = 0;
  // 8 	Message key 	Output 	Char(4)
  char msgkey[4];
  // 9 	Error code 	I/O 	Char(*)
  error_info_t errinfo __attribute__((aligned(16)));
  memset(&errinfo, 0, sizeof(errinfo));
  errinfo.bytes_provided = sizeof(errinfo);

  void *pgm_argv[] __attribute__((aligned(16))) = {
      &msgid,
      &msgfile,
      &msgdata,
      &msglen,
      &msgtype,
      &callstack,
      &callstack_cnt,
      &msgkey,
      &errinfo,
      NULL};
  rc = _PGMCALL(&qmhsndpm_pgm,
                pgm_argv,
                PGMCALL_EXCP_NOSIGNAL);
  if (0 == rc && 0 == errinfo.bytes_available)
  {
    return 0;
  }
  return -1;
}
