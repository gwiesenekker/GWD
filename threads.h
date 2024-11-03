//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#ifndef ThreadsH
#define ThreadsH

#define NTHREADS_MAX     64

typedef struct
{
  int thread_role;

  my_printf_t thread_my_printf;

  my_random_t thread_random;

  queue_t thread_queue;

  my_thread_t thread;

  my_timer_t thread_idle_timer;
  my_timer_t thread_busy_timer;

  int thread_board_id;

  double thread_idle;
} thread_t;

extern thread_t threads[NTHREADS_MAX];

extern thread_t *thread_alpha_beta_master;

queue_t *return_thread_queue(void *);

void start_threads(void);
void join_threads(void);
void test_threads(void);

#endif

