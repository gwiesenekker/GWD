//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#define sscanf do_not_use_sscanf_but_my_sscanf

#define my_isspace(C) isspace((unsigned char) (C))

int my_sscanf(const char *, const char *, ...) __attribute__((format(scanf, 2, 3)));

#endif

