//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef BstreamsH
#define BstreamsH

typedef struct
{
  FILE *BS_file;
  struct bStream* BS_bfile;
  bstring BS_bline;
  bstring BS_newline;
  bstring BS_empty;
} my_bstream_t;

void construct_my_bstream(my_bstream_t *, char *);
int my_bstream_readln(my_bstream_t *, int);
void destroy_my_bstream(my_bstream_t *);

#endif

