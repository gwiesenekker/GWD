//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#ifndef FBUFFER_H
#define FBUFFER_H

typedef struct
{
  bstring FB_bname;
  bstring FB_bbuffer;
} fbuffer_t;

void flush_fbuffer(fbuffer_t *, int);

void append_fbuffer(fbuffer_t *, const char *fmt, ...);

bstring return_fbuffer_bname(fbuffer_t *);

void construct_fbuffer(fbuffer_t *, bstring, char *, int);

void test_fbuffer(void);

#endif

