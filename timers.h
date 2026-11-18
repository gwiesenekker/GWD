// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
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
  my_int64_t P_ntotal;
  my_int64_t P_ntodo;
  double P_seconds;
  my_int64_t P_ndone;
  my_int64_t P_ndone_previous;
  double P_seconds_done;
  double P_seconds_previous;
  my_int64_t P_ndelta;
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

void construct_my_timer(my_timer_t *arg_with_my_timer,
                        const char *arg_name,
                        my_printf_t *arg_my_printf,
                        int arg_suppress_warnings);
void reset_my_timer(my_timer_t *arg_with_my_timer);
double return_my_timer(my_timer_t *arg_with_my_timer, int arg_wall);
void stop_my_timer(my_timer_t *arg_with_my_timer);
void start_my_timer(my_timer_t *arg_with_my_timer);
void destroy_my_timer(my_timer_t *arg_with_my_timer);

void construct_progress(progress_t *arg_progress,
                        const char *arg_name,
                        my_int64_t arg_ntotal,
                        double arg_seconds);
int make_progress(progress_t *arg_progress, my_int64_t arg_idone);
void finalize_progress(progress_t *arg_progress);
my_int64_t return_ndone(const progress_t *arg_progress);
my_int64_t return_ntodo(const progress_t *arg_progress);
void destroy_progress(progress_t *arg_progress);

void construct_time_control(time_control_t *arg_with_time_control);
void configure_time_control(double arg_game_time,
                            int arg_ngame_moves,
                            double arg_increment,
                            time_horizon_t arg_horizon,
                            time_control_t *arg_with_time_control);
void sync_time_control(double arg_time_left,
                       int arg_moves_left,
                       double arg_increment,
                       int arg_played,
                       time_control_t *arg_with_time_control);
void update_time_control(double arg_move_time,
                         time_control_t *arg_with_time_control);
void set_time_limit(const time_control_t *arg_with_time_control,
                    double arg_reserve);
double return_time_used(const time_control_t *arg_with_time_control);
double return_time_left(const time_control_t *arg_with_time_control);
int is_configured_time_control(const time_control_t *arg_with_time_control);
int is_rolling_time_control(const time_control_t *arg_with_time_control);
int return_moves_done(const time_control_t *arg_with_time_control);
int return_moves_left(const time_control_t *arg_with_time_control);

void test_my_timers(void);

#endif
