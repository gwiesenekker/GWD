//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef ThreadsH
#define ThreadsH

#define NTHREADS_MAX     64

//threads.c

#define THREAD_ID_OFFSET 2048

#define THREAD_ALPHA_BETA_MASTER 0
#define THREAD_ALPHA_BETA_SLAVE  1

typedef struct
{
  int thread_id;

  int thread_role;

  my_printf_t *thread_my_printf;

  queue_t thread_queue;

  my_thread_t thread;

  my_timer_t thread_idle_timer;
  my_timer_t thread_busy_timer;

  int thread_board_id;

  double thread_idle;

  ui64_t thread_y[55];
  int thread_j;
  int thread_k;
} thread_t;

extern int threads[NTHREADS_MAX];

extern int thread_alpha_beta_master_id;

thread_t *return_with_thread(int);
queue_t *return_thread_queue(int);
ui64_t randull_thread(int, int);
void init_threads(void);
void start_threads(void);
void join_threads(void);
void test_threads(void);

#endif

