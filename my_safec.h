//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#define sscanf do_not_use_sscanf_but_my_sscanf

#define my_isspace(C) isspace((unsigned char)(C))

int my_sscanf(const char *, const char *, ...)
    __attribute__((format(scanf, 2, 3)));

#endif
