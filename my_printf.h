//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef MyPrintfH
#define MyPrintfH

typedef struct my_printf_s
{
  int object_id;

  char my_printf_fname[MY_LINE_MAX];

  FILE *my_printf_flog;

  long my_printf_fsize;

  char my_printf_buffer[MY_LINE_MAX];

  int my_printf_ibuffer;

  my_mutex_t my_printf_mutex;
} my_printf_t;

//my_printf.c

extern char log_infix[MY_LINE_MAX];
extern class_t *my_printf_class;

my_printf_t *construct_my_printf(int);
void my_printf(my_printf_t *, char *, ...);
void init_my_printf_class(void);
void fin_my_printf_class(void);
void test_my_printf_class(void);

#endif

