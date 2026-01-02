//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef FBUFFER_H
#define FBUFFER_H

typedef struct
{
  size_t FB_nfbuffer;
  void *FB_fbuffer;
  bstring FB_bname;
  bstring FB_bstring;
} fbuffer_t;

void flush_fbuffer(fbuffer_t *, int);

void append_fbuffer_fmt(fbuffer_t *, const char *, ...);

void append_fbuffer_bin(fbuffer_t *, void *, size_t);

bstring return_fbuffer_bname(fbuffer_t *);

void construct_fbuffer(fbuffer_t *, bstring, char *, int);

void test_fbuffer(void);

#endif

