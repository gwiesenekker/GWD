// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef FBUFFER_H
#define FBUFFER_H

typedef struct
{
  size_t FB_nfbuffer;
  void *FB_fbuffer;
  bstring FB_bname;
  bstring FB_bstring;
} fbuffer_t;

void flush_fbuffer(fbuffer_t *arg_with_fbuffer, int arg_nfbuffer);

void append_fbuffer_fmt(fbuffer_t *arg_with_fbuffer, const char *arg_fmt, ...);

void append_fbuffer_bin(fbuffer_t *arg_with_fbuffer,
                        const void *arg_data,
                        size_t arg_nbytes);

bstring return_fbuffer_bname(const fbuffer_t *arg_with_fbuffer);

void construct_fbuffer(fbuffer_t *arg_with_fbuffer,
                       const_bstring arg_bname,
                       const char *arg_suffix,
                       int arg_remove);

void test_fbuffer(void);

#endif
