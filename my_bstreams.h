// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
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

void construct_my_bstream(my_bstream_t *, const char *);
int my_bstream_readln(my_bstream_t *, int);
void destroy_my_bstream(my_bstream_t *);

size_t my_bread(void *, size_t, size_t, void *);
void my_bstrftime(bstring, const char *, time_t);
int my_breadline(bstring, FILE *);
void my_bcpu_flags(bstring);

#endif
