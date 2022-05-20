

#include <fcntl.h>

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
  if (0 == nochdir)
  {
    if (chdir("/") == -1)
    {
      return -1;
    }
  }
  if (0 != noclose)
  {
    return 0;
  }
  for (int i = 0; i <= 3; ++i)
  {
    if (-1 == dup2(devnull, i))
    {
      return -1;
    }
  }
  return 0;
}