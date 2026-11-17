// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
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
  bstring P_bname;
  my_timer_t P_timer;
  i64_t P_ntotal;
  i64_t P_ntodo;
  double P_seconds;
  i64_t P_ndone;
  i64_t P_ndone_previous;
  double P_seconds_done;
  double P_seconds_previous;
  i64_t P_ndelta;
} progress_t;

typedef enum
{
  TIME_HORIZON_EXACT,
  TIME_HORIZON_PADDED
} time_horizon_t;

typedef struct
{
  double TC_game_time;
  double TC_increment;
  int TC_rolling;
  int TC_ngame_moves;
  int TC_nslots;
  int TC_ntrouble;
  int TC_nmoves_done;
  double TC_time_used;
  double TC_game_time_per_move[NODE_MAX];
} time_control_t;

void construct_my_timer(my_timer_t *, const char *, my_printf_t *, int);
void reset_my_timer(my_timer_t *);
double return_my_timer(my_timer_t *, int);
void stop_my_timer(my_timer_t *);
void start_my_timer(my_timer_t *);
void destroy_my_timer(my_timer_t *);

void construct_progress(progress_t *, const char *, i64_t, double);
int make_progress(progress_t *, i64_t);
void finalize_progress(progress_t *);
i64_t return_ndone(const progress_t *);
i64_t return_ntodo(const progress_t *);
void destroy_progress(progress_t *);

void construct_time_control(time_control_t *);
void configure_time_control(double,
                            int,
                            double,
                            time_horizon_t,
                            time_control_t *);
void sync_time_control(double, int, double, int, time_control_t *);
void update_time_control(double, time_control_t *);
void set_time_limit(const time_control_t *, double);
double return_time_used(const time_control_t *);
double return_time_left(const time_control_t *);
int is_configured_time_control(const time_control_t *);
int is_rolling_time_control(const time_control_t *);
int return_moves_done(const time_control_t *);
int return_moves_left(const time_control_t *);

void test_my_timers(void);

#endif
