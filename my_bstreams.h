//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef BstreamsH
#define BstreamsH

typedef struct
{
  FILE *BS_file;
  struct bStream *BS_bfile;
  bstring BS_bline;
  bstring BS_newline;
  bstring BS_empty;
} my_bstream_t;

void construct_my_bstream(my_bstream_t *, char *);
int my_bstream_readln(my_bstream_t *, int);
void destroy_my_bstream(my_bstream_t *);

#endif
