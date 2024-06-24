#include "ebcutils.h"
#include "ibmierrinfo.h"
#include <string.h>
#include <ctype.h>
#include <as400_protos.h>

#include <ctype.h>
int to_37_spacepadded_nts(char *out, size_t out_len, const char *in)
{
  size_t in_len = strlen(in);
  char in_buf[out_len];
  in_buf[-1 + out_len] = 0;
  memset(in_buf, ' ', -1 + out_len);

  for (int i = 0; i < in_len; i++)
  {
    if (0 == in[i])
      break;
    in_buf[i] = toupper(in[i]);
  }
  iconv_t cd = iconv_open("IBM-037", "ISO8859-1");
  if ((iconv_t)-1 == cd)
  {
    fprintf(stderr, "Error in opening conversion descriptors\n");
    return 8;
  }

  size_t inleft = strlen(in_buf);
  size_t outleft = out_len;
  char *input = in_buf;
  char *output = out;

  int rc = iconv(cd, &input, &inleft, &output, &outleft);
  if (rc == -1)
  {
    fprintf(stderr, "Error in converting characters\n");
    return 9;
  }
  return iconv_close(cd);
}

int from_job_ccsid(char *out, size_t out_len, const char *in)
{
  char encoding[16];
  snprintf(encoding, sizeof(encoding), "IBM-%03d", Qp2jobCCSID());

  char pase_encoding[16];
  int pase_ccsid = _SETCCSID(-1);
  snprintf(pase_encoding, sizeof(pase_encoding), (1208 == pase_ccsid) ? "UTF-8" : "Cp%03d", pase_ccsid);
  iconv_t cd = iconv_open(pase_encoding, encoding);
  if ((iconv_t)-1 == cd)
  {
    fprintf(stderr, "Error in opening conversion descriptors\n");
    return 8;
  }

  size_t inleft = strlen(in);
  size_t outleft = out_len;
  char *input = (char *)in;
  char *output = out;

  int rc = iconv(cd, &input, &inleft, &output, &outleft);
  if (rc == -1)
  {
    fprintf(stderr, "Error in converting characters\n");
    return 9;
  }
  return iconv_close(cd);
}
int to_job_ccsid(char *out, size_t out_len, const char *in)
{
  char encoding[16];
  snprintf(encoding, sizeof(encoding), "IBM-%03d", Qp2jobCCSID());
  char pase_encoding[16];
  int pase_ccsid = _SETCCSID(-1);
  snprintf(pase_encoding, sizeof(pase_encoding), (1208 == pase_ccsid) ? "UTF-8" : "Cp%03d", pase_ccsid);
  iconv_t cd = iconv_open(encoding, pase_encoding);
  if ((iconv_t)-1 == cd)
  {
    fprintf(stderr, "Error in opening conversion descriptors\n");
    return 8;
  }

  size_t inleft = strlen(in);
  size_t outleft = out_len;
  char *input = (char *)in;
  char *output = out;

  int rc = iconv(cd, &input, &inleft, &output, &outleft);
  if (rc == -1)
  {
    fprintf(stderr, "Error in converting characters\n");
    return 9;
  }
  return iconv_close(cd);
}

// internal debug use only
void print_error(error_info_t *err)
{
  char in_buf[100];
  char buf[100];
  memset(in_buf, 0, sizeof(in_buf));
  memcpy(in_buf, err->msgid, 7);
  from_job_ccsid(buf, sizeof(buf), in_buf);
  fprintf(stderr, "msg=%s\n", buf);
  memset(in_buf, 0, sizeof(in_buf));
  memcpy(in_buf, err->exc_data, 92);
  from_job_ccsid(buf, sizeof(buf), in_buf);
  fprintf(stderr, "exc data=%s\n", buf);
}