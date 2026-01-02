//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef TimersH
#define TimersH

typedef struct
{
  bstring MT_bname;

  my_printf_t *MT_my_printf;

  int MT_return_wall_clock_warning_given;

  double MT_cpu_time_used;
  double MT_wall_time_used;

  double MT_start_cpu_clock;
  double MT_start_wall_clock;

  int MT_status;
} my_timer_t;

typedef struct
{
  my_timer_t P_timer;
  i64_t P_ntodo;
  double P_seconds;
  i64_t P_ndone;
  i64_t P_ndone_previous;
  double P_seconds_done;
  double P_seconds_previous;
  i64_t P_ndelta;
} progress_t;

typedef struct
{
  int TC_game_time;
  int TC_ngame_moves;
  double TC_game_time_per_move[NODE_MAX];
} time_control_t;

void construct_my_timer(my_timer_t *, char *, my_printf_t *, int);
void reset_my_timer(my_timer_t *);
double return_my_timer(my_timer_t *, int);
void stop_my_timer(my_timer_t *);
void start_my_timer(my_timer_t *);

void construct_progress(progress_t *, i64_t, double);
void update_progress(progress_t *);
void finalize_progress(progress_t *);
i64_t return_ndone(progress_t *);

void configure_time_control(int, int, time_control_t *);
void update_time_control(int, double, time_control_t *);
void set_time_limit(int, time_control_t *);

void test_my_timers(void);

#endif

