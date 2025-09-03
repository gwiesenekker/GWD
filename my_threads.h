//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef ThreadsH
#define ThreadsH

#define NTHREADS_MAX     64

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

