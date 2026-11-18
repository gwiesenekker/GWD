// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#include "compat.h"

#define sscanf do_not_use_sscanf_but_my_sscanf

int my_sscanf(const char *arg_string, const char *arg_format, ...)
  COMPAT_FORMAT(scanf, 2, 3);

#endif
