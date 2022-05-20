

#include <fcntl.h>

// Modeled after doc at https://man7.org/linux/man-pages/man3/daemon.3.html
static int libutil_daemon(int nochdir, int noclose)
{
  switch (fork())
  {
    case -1:
      return -1;
    case 0:
      break;
    default:
      exit(0);
  }

  if (-1 == setsid())
  {
    return -1;
  }

  if (-1 == umask(0))
  {
    return -1;
  }
  int devnull = open("/dev/null", O_RDWR);
  if (-1 == devnull)
  {
    return -1;
  }

  // "If nochdir is zero, daemon() changes the process's current
  // working directory to the root directory ("/"); otherwise, the
  // current working directory is left unchanged.""
  if (0 == nochdir)
  {
    if (chdir("/") == -1)
    {
      return -1;
    }
  }
  // "If noclose is zero, daemon() redirects standard input, standard
  // output, and standard error to /dev/null; otherwise, no changes
  // are made to these file descriptors."
  if (0 == noclose)
  {
    for (int i = 0; i <= 2; ++i)
    {
      if (-1 == dup2(devnull, i))
      {
        return -1;
      }
    }
  }
  return 0;
}