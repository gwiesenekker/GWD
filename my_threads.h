// SCU REVISION 8.1270 wo 11 nov 2026  9:25:12 CET
#ifndef ThreadsH
#define ThreadsH
#include <stdatomic.h>

#define NTHREADS_MAX 64

typedef struct
{
  int thread_role;

  my_printf_t T_my_printf;

  queue_t T_queue;

  my_thread_t T_thread;

  my_timer_t T_idle_timer;
  my_timer_t T_busy_timer;

  search_t T_search;

  double T_idle;

  /* Keep publication traffic away from the worker's hot private counters. */
  _Alignas(64) _Atomic(i64_t) T_published_nodes;
  _Atomic(int) T_nodes_ready;
} thread_t;

extern thread_t threads[NTHREADS_MAX];

extern thread_t *thread_alpha_beta_master;

queue_t *return_thread_queue(void *);

void start_threads(void);
void join_threads(void);
void test_threads(void);

#endif
