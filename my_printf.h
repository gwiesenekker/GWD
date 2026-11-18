// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef MyPrintfH
#define MyPrintfH

#include "common.h"
#include "bstrlib.h"

typedef struct
{
  int MP2stdout;

  bstring MP_bname;

  FILE *MP_flog;

  long MP_fsize;

  bstring MP_bformat;
  bstring MP_bbuffer;

  bstring MP_bline;
  bstring MP_btime_stamp;

  my_mutex_t MP_mutex;

  int MP_paused;
} my_printf_t;

extern void *STDOUT;

#define PRINTF(...) my_printf(STDOUT, __VA_ARGS__)

extern bstring my_printf_logs_dir;

// my_printf.c

void construct_my_printf(my_printf_t *arg_with_my_printf,
                         const char *arg_prefix,
                         int arg_to_stdout);
void my_printf(my_printf_t *arg_with_my_printf, const char *arg_format, ...)
  COMPAT_FORMAT(printf, 2, 3);
void pause_my_printf(my_printf_t *arg_with_my_printf);
void resume_my_printf(my_printf_t *arg_with_my_printf);
void init_my_printf(void);
void test_my_printf(void);

#endif
