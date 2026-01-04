//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef ThreadsH
#define ThreadsH

#define NTHREADS_MAX 64

typedef struct
{
  int thread_role;

  my_printf_t T_my_printf;

  my_random_t T_random;

  queue_t T_queue;

  my_thread_t T_thread;

  my_timer_t T_idle_timer;
  my_timer_t T_busy_timer;

  search_t T_search;

  double T_idle;
} thread_t;

extern thread_t threads[NTHREADS_MAX];

extern thread_t *thread_alpha_beta_master;

queue_t *return_thread_queue(void *);

void start_threads(void);
void join_threads(void);
void test_threads(void);

#endif
