// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#include "my_safec.h"

#include <stdarg.h>
#include <stdio.h>

#undef sscanf

int my_sscanf(const char *arg_string, const char *arg_format, ...)
{
  va_list arguments;

  va_start(arguments, arg_format);

  int result = vsscanf(arg_string, arg_format, arguments);

  va_end(arguments);

  return (result);
}
