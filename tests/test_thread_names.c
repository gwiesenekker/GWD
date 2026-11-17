/* Exercise the actual TLS stack/registry while other threads dump it. */
#include "../my_malloc.c"
#include <assert.h>

void *STDOUT;
int zzzzzz_invocation;
void my_printf(my_printf_t *arg_output, const char *arg_format, ...)
{
  (void)arg_output;
  (void)arg_format;
}
_Noreturn void zzzzzz(const char *arg_file,
                      const char *arg_func,
                      long arg_line,
                      const char *arg_message,
                      int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

static void *worker(void *arg_argument)
{
  (void)arg_argument;

  assert(name_stack == NULL);

  for (int iiteration = 0; iiteration < 10000; ++iiteration)
  {
    push_name("outer", __FILE__, __func__, __LINE__);
    push_name("inner", __FILE__, __func__, __LINE__);

    assert(name_stack->nnames == 2);

    pop_name("inner", __FILE__, __func__, __LINE__);
    pop_name("outer", __FILE__, __func__, __LINE__);

    assert(name_stack->nnames == 0);
  }

  return NULL;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  assert(compat_mutex_init(&my_names_mutex) == 0);
  /* More than the old global limit, but below each thread's limit. */
  for (int iname = 0; iname < NMY_NAMES_MAX; ++iname)
    push_name("main", __FILE__, __func__, __LINE__);

  my_thread_t threads[4];

  for (int ithread = 0; ithread < 4; ++ithread)
    assert(compat_thread_create(&threads[ithread], worker, NULL) == 0);

  for (int isnapshot = 0; isnapshot < 1000; ++isnapshot)
    print_my_names(TRUE);

  for (int ithread = 0; ithread < 4; ++ithread)
    assert(compat_thread_join(threads[ithread]) == 0);
  assert(print_all_name_stacks() == NMY_NAMES_MAX);

  for (int iname = 0; iname < NMY_NAMES_MAX; ++iname)
    pop_name("main", __FILE__, __func__, __LINE__);

  assert(print_all_name_stacks() == 0);

  puts("thread-local name stack tests passed");

  return 0;
}
