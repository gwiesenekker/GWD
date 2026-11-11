// SCU REVISION 8.1270 wo 11 nov 2026  9:25:12 CET
#ifndef MY_SAFEC_H
#define MY_SAFEC_H

#define sscanf do_not_use_sscanf_but_my_sscanf

#define my_isspace(C) isspace((unsigned char)(C))

int my_sscanf(const char *, const char *, ...) COMPAT_FORMAT(scanf, 2, 3);

#endif
