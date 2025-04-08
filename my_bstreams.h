//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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

