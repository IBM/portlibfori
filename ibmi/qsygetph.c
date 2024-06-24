#include "qsygetph.h"
#include "ebcutils.h"
#include <stdlib.h>
#include <string.h>

#include "ibmierrinfo.h"

int validate_pw(char *_username, char *_pw)
{
  if (0 == _pw)
  {
    return -1;
  }
  char handle[12];
  int rc = qsygetph(_username, _pw, handle);
  if (0 == rc)
  {
    int rc2 = qsyrlsph(handle);
  }
  return rc;
}
int qsygetph(char *_username, char *_pw, char *_handle_buf)
{
  if (0 == _pw)
  {
    return qsygetph_np(_username, _handle_buf);
  }
  char usrprf_ebcdic[11] __attribute__((aligned(16)));
  to_37_spacepadded_nts(usrprf_ebcdic, 11, _username);
  char handle[12] __attribute__((aligned(16)));
  memset(handle, 0, sizeof(handle));

  static ILEpointer qsygetph_pgm __attribute__((aligned(16)));
  static int qsygetph_pgm_loaded = 0;
  int rc = -1;
  if (0 == qsygetph_pgm_loaded)
  {
    rc = _RSLOBJ2(&qsygetph_pgm,
                  RSLOBJ_TS_PGM,
                  "QSYGETPH",
                  "QSYS");
    if (0 != rc)
    {
      return rc;
    }
    qsygetph_pgm_loaded = 1;
  }

  error_info_t errinfo __attribute__((aligned(16)));
  memset(&errinfo, 0, sizeof(errinfo));
  errinfo.bytes_provided = sizeof(errinfo);

  int ccsid = _SETCCSID(-1);
  int pwlen = strlen(_pw);
  void *pgm_argv[] __attribute__((aligned(16), packed)) = {
      &usrprf_ebcdic,
      _pw,
      &handle,
      &errinfo,
      &pwlen,
      &ccsid,
      NULL};
  rc = _PGMCALL(&qsygetph_pgm,
                pgm_argv,
                PGMCALL_EXCP_NOSIGNAL);
  if (0 == rc && 0 == errinfo.bytes_available)
  {
    if (0 != _handle_buf)
    {
      memcpy(_handle_buf, handle, 12);
    }
    return 0;
  }
  else
  {
    if (0 != _handle_buf)
    {
      *_handle_buf = 0;
    }
    return -1;
  }
}
int qsyrlsph(char *_handle_buf)
{
  static ILEpointer qsyrlsph_pgm __attribute__((aligned(16)));
  static int qsyrlsph_pgm_loaded = 0;
  int rc = -1;
  if (0 == qsyrlsph_pgm_loaded)
  {
    rc = _RSLOBJ2(&qsyrlsph_pgm,
                  RSLOBJ_TS_PGM,
                  "QSYRLSPH",
                  "QSYS");
    if (0 != rc)
    {
      return rc;
    }
    qsyrlsph_pgm_loaded = 1;
  }
  error_info_t errinfo __attribute__((aligned(16)));
  memset(&errinfo, 0, sizeof(errinfo));
  errinfo.bytes_provided = sizeof(errinfo);
  char *handle = _handle_buf;
  void *pgm_argv[] __attribute__((aligned(16))) = {
      &handle,
      &errinfo,
      NULL};
  rc = _PGMCALL(&qsyrlsph_pgm,
                pgm_argv,
                PGMCALL_EXCP_NOSIGNAL);
  if (0 == rc && 0 == errinfo.bytes_available)
  {
    return 0;
  }
  return -1;
}

int qsygetph_np(char *_username, char *_handle_buf)
{
  // ILEpointer target __attribute__((aligned(16)));
  unsigned long long actmark;

  /* _ILELOADX() loads the service program */
  actmark = _ILELOADX("QSYS/QSYPHANDLE", ILELOAD_LIBOBJ);
  if (-1 == actmark)
  {
    return -1;
  }
  ILEpointer target __attribute__((aligned(16)));
  int res = _ILESYMX(&target, actmark, "QsyGetProfileHandleNoPwd");

  if (-1 == res)
  {
    return -1;
  }
  static result_type_t result_type = RESULT_VOID;
  arg_type_t signature[] __attribute__((aligned(16))) =
      {
          ARG_MEMPTR,
          ARG_MEMPTR,
          ARG_MEMPTR,
          ARG_MEMPTR,
          ARG_END};
  // (unsigned char  *Profile_handle,
  char *handle __attribute__((aligned(16))) = _handle_buf;
  //  char           *User_ID,
  char username[11] __attribute__((aligned(16)));
  to_37_spacepadded_nts(username, sizeof(username), _username);
  //  char           *Password_value,
  char pw[11] __attribute__((aligned(16)));
  to_37_spacepadded_nts(pw, sizeof(pw), "*NOPWD");
  //  void           *Error_code);
  error_info_t errinfo __attribute__((aligned(16)));
  memset(&errinfo, 0, sizeof(errinfo));
  errinfo.bytes_provided = sizeof(errinfo);

  struct
  {
    ILEarglist_base base __attribute__((aligned(16)));
    ILEpointer _outhandle __attribute__((aligned(16)));
    ILEpointer _inuserid __attribute__((aligned(16)));
    ILEpointer _inpw __attribute__((aligned(16)));
    ILEpointer _error __attribute__((aligned(16)));
  } arglist __attribute__((aligned(16)));
  arglist._outhandle.s.addr = &handle;
  arglist._inuserid.s.addr = &username;
  arglist._inpw.s.addr = &pw;
  arglist._error.s.addr = &errinfo;
  int rc = _ILECALLX(&target,
                     &arglist.base,
                     signature,
                     result_type,
                     ILECALL_EXCP_NOSIGNAL);
  if (0 == rc && 0 == errinfo.bytes_available)
  {
    return 0;
  }
  return 5;
}