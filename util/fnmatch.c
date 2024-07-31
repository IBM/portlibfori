#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

#define FNM_CASEFOLD  0x4000000 // Some large power of 2 to avoid collisions

void str_to_lower(char *str) {
  while (*str) {
    *str = tolower((unsigned char) *str);
    str++;
  }
}

int libutil_fnmatch(const char *pattern, const char *string, int flags)
{
  int result;

  if (flags & FNM_CASEFOLD)
  {
    // Create copies of the pattern and the string
    char *lower_pattern = strdup(pattern);
    char *lower_string = strdup(string);
    if (!lower_pattern || !lower_string) {
        free(lower_pattern);
        free(lower_string);
        return FNM_NOMATCH;
    }

    // Convert copies to lowercase
    str_to_lower(lower_pattern);
    str_to_lower(lower_string);


    // Use fnmatch to compare the lowercase strings
    result = fnmatch(lower_pattern, lower_string, flags ^ FNM_CASEFOLD);

    // Clean up
    free(lower_pattern);
    free(lower_string);
  } else {
    result = fnmatch(pattern, string, flags);
  }

  return result;
}
