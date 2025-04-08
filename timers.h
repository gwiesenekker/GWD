//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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
  int TC_game_time;
  int TC_ngame_moves;
  double TC_game_time_per_move[NODE_MAX];
} time_control_t;

void construct_my_timer(my_timer_t *, char *, my_printf_t *, int);
void reset_my_timer(my_timer_t *);
double return_my_timer(my_timer_t *, int);
void stop_my_timer(my_timer_t *);
void start_my_timer(my_timer_t *);

void configure_time_control(int, int, time_control_t *);
void update_time_control(int, double, time_control_t *);
void set_time_limit(int, time_control_t *);

void test_my_timers(void);

#endif

