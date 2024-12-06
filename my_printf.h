//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef MyPrintfH
#define MyPrintfH

typedef struct 
{
  int my_printf2stdout;

  bstring my_printf_bname;

  FILE *my_printf_flog;

  long my_printf_fsize;

  bstring my_printf_bbuffer;

  my_mutex_t my_printf_mutex;
} my_printf_t;

//my_printf.c

void construct_my_printf(void *, char *, int);
void my_printf(void *, char *, ...);
void init_my_printf(void);
void test_my_printf(void);

#endif

