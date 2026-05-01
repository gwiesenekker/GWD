// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#ifndef MyPrintfH
#define MyPrintfH

typedef struct
{
  int MP2stdout;

  bstring MP_bname;

  FILE *MP_flog;

  long MP_fsize;

  bstring MP_bformat;
  bstring MP_bbuffer;

  bstring MP_bline;
  bstring MP_bstamp;

  my_mutex_t MP_mutex;

  int MP_paused;
} my_printf_t;

extern bstring my_printf_logs_dir;

// my_printf.c

void construct_my_printf(my_printf_t *, char *, int);
void my_printf(my_printf_t *, char *, ...)
  __attribute__((format(printf, 2, 3)));
void pause_my_printf(my_printf_t *);
void resume_my_printf(my_printf_t *);
void init_my_printf(void);
void test_my_printf(void);

#endif
