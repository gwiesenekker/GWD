// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef BstreamsH
#define BstreamsH

#include "common.h"
#include "bstrlib.h"

typedef struct
{
  FILE *BS_file;
  struct bStream *BS_bfile;
  bstring BS_bline;
  bstring BS_newline;
  bstring BS_empty;
} my_bstream_t;

void construct_my_bstream(my_bstream_t *arg_with_my_bstream,
                          const char *arg_name);
int my_bstream_readln(my_bstream_t *arg_with_my_bstream,
                      int arg_remove_newline);
void destroy_my_bstream(my_bstream_t *arg_with_my_bstream);

size_t
my_bread(void *arg_buffer, size_t arg_size, size_t arg_count, void *arg_file);
void my_bstrftime(bstring arg_string, const char *arg_format, time_t arg_time);
int my_breadline(bstring arg_line, FILE *arg_file);
void my_bcpu_flags(bstring arg_flags);

#endif
