// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
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
