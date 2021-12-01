#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
/* On IBM i PASE, for better compatibility with running interactive programs in
 * a 5250 environment, isatty() will return true for the stdin/stdout/stderr
 * streams created by QSH/QP2TERM.
 *
 * For more, see docs on PASE_STDIO_ISATTY in
 * https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_74/apis/pase_environ.htm
 *
 * This behavior causes problems for Node / Python as it expects that if isatty() returns
 * true that TTY ioctls will be supported by that fd (which is not an
 * unreasonable expectation) and when they don't it crashes with assertion
 * errors.
 *
 * Here, we create our own version of isatty() that uses ioctl() to identify
 * whether the fd is *really* a TTY or not.
 */
int libutil_isatty(int fd) {
  int rc;

  rc = ioctl(fd, TXISATTY + 0x81, NULL);
  if (rc && errno != EBADF) {
      errno = ENOTTY;
  }

  return rc == 0;
}
