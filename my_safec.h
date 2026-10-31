// SCU REVISION 8.1255 za 31 okt 2026  9:15:42 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#define sscanf do_not_use_sscanf_but_my_sscanf

#define my_isspace(C) isspace((unsigned char)(C))

int my_sscanf(const char *, const char *, ...) COMPAT_FORMAT(scanf, 2, 3);

#endif
