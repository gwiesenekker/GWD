/* Exercise the actual TLS stack/registry while other threads dump it. */
#include "../my_malloc.c"
#include <assert.h>

void *STDOUT;
int zzzzzz_invocation;
void my_printf(my_printf_t *output, char *format, ...)
{
  (void)output;
  (void)format;
}
_Noreturn void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

static void *worker(void *argument)
{
  (void)argument;
  assert(name_stack == NULL);
  for (int i = 0; i < 10000; ++i)
  {
    push_name("outer", __FILE__, __func__, __LINE__);
    push_name("inner", __FILE__, __func__, __LINE__);
    assert(name_stack->count == 2);
    pop_name("inner", __FILE__, __func__, __LINE__);
    pop_name("outer", __FILE__, __func__, __LINE__);
    assert(name_stack->count == 0);
  }
  return NULL;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  assert(compat_mutex_init(&my_names_mutex) == 0);
  /* More than the old global limit, but below each thread's limit. */
  for (int i = 0; i < NMY_NAMES_MAX; ++i)
    push_name("main", __FILE__, __func__, __LINE__);
  my_thread_t threads[4];
  for (int i = 0; i < 4; ++i)
    assert(compat_thread_create(&threads[i], worker, NULL) == 0);
  for (int i = 0; i < 1000; ++i)
    print_my_names(TRUE);
  for (int i = 0; i < 4; ++i)
    assert(compat_thread_join(threads[i]) == 0);
  assert(print_all_name_stacks() == NMY_NAMES_MAX);
  for (int i = 0; i < NMY_NAMES_MAX; ++i)
    pop_name("main", __FILE__, __func__, __LINE__);
  assert(print_all_name_stacks() == 0);
  puts("thread-local name stack tests passed");
  return 0;
}
