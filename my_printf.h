//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef MyPrintfH
#define MyPrintfH

typedef struct 
{
  int my_printf2stdout;

  char my_printf_fname[MY_LINE_MAX];

  FILE *my_printf_flog;

  long my_printf_fsize;

  char my_printf_buffer[MY_LINE_MAX];

  int my_printf_ibuffer;

  my_mutex_t my_printf_mutex;
} my_printf_t;

//my_printf.c

void construct_my_printf(void *, char *, int);
void my_printf(void *, char *, ...);
void init_my_printf(void);
void test_my_printf(void);

#endif

