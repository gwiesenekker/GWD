// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#define sscanf do_not_use_sscanf_but_my_sscanf

int my_sscanf(const char *arg_string, const char *arg_format, ...)
  COMPAT_FORMAT(scanf, 2, 3);

#endif
