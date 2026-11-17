// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
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

void append_fbuffer_bin(fbuffer_t *, const void *, size_t);

bstring return_fbuffer_bname(const fbuffer_t *);

void construct_fbuffer(fbuffer_t *, const_bstring, const char *, int);

void test_fbuffer(void);

#endif
