//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

/*
int my_sscanf_s(const char *arg_src, size_t arg_limit, char *arg_dst)
{
  if ((arg_src == NULL) or (arg_dst == NULL) or (arg_limit == 0))
  {
    if ((arg_dst != NULL) and (arg_limit > 0)) arg_dst[0] = '\0';

    return 0;
  }

  while (my_isspace(*arg_src)) arg_src++;

  if (*arg_src == '\0')
  {
    arg_dst[0] = '\0';

    return 0;
  }

  size_t i = 0;

  for (i = 0; ((i + 1) < arg_limit) and (arg_src[i] != '\0') and
              !my_isspace(arg_src[i]); i++)
  {
    arg_dst[i] = arg_src[i];
  }
  arg_dst[i] = '\0';

  if ((arg_src[i] == '\0') or my_isspace(arg_src[i])) 
    return(1);
  else
    return(-1);
}
*/

#undef sscanf

int my_sscanf(const char *str, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);

  int rc = vsscanf(str, format, ap);

  va_end(ap);

  return(rc);
}

