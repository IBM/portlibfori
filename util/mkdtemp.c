#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>


static char pathchars[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '+'
};

#define _swap4( n )   (unsigned int  ) ( ( (unsigned int  )(n) >> 24 )                |   \
                                       ( ( (unsigned int  )(n) >> 8  ) & 0x0000ff00 ) |   \
                                       ( ( (unsigned int  )(n) & 0x0000ff00 ) << 8  ) |   \
                                       (   (unsigned int  )(n) << 24 )                    )

char* mkdtemp(char* template)
{
  size_t tlen = strlen(template);
  size_t xlen;
  size_t xind = tlen;
  int found = 0;
  
  for(; xind; --xind)
  {
      if(template[xind-1] != 'X') break;
      else found = 1;
  }
  
  xlen = tlen - xind;
  
  if(!found || xlen < 6)
  {
      errno = EINVAL;
      return NULL;
  }
  
  unsigned int pid = (unsigned int) getpid();
  unsigned int count = 1;
  
  char x[6];
  
  int rc;
  do
  {
      unsigned int value = pid | _swap4(count);
      
      x[5] = pathchars[value & 0x3F];
      value = value >> 6;
      x[4] = pathchars[value & 0x3F];
      value = value >> 6;
      x[3] = pathchars[value & 0x3F];
      value = value >> 6;
      x[2] = pathchars[value & 0x3F];
      value = value >> 6;
      x[1] = pathchars[value & 0x3F];
      value = value >> 6;
      x[0] = pathchars[value & 0x3F];
      
      memcpy(&template[xind], x, sizeof(x));
      if(xlen > sizeof(x))
      {
          memcpy(&template[xind+sizeof(x)], x, xlen-sizeof(x) > sizeof(x) ? sizeof(x) : xlen-sizeof(x));
      }
      
      errno = 0;
      rc = mkdir(template, S_IRWXU);
      if(rc == 0 || errno != EEXIST) break;
      
      ++count;
  } while(count);
  
  return rc ? NULL : template;
}

