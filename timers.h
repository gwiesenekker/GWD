//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef TimersH
#define TimersH

typedef struct
{
  bstring MT_name;
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

void construct_my_timer(void *, char *, my_printf_t *, int);
void reset_my_timer(void *);
double return_my_timer(void *, int);
void stop_my_timer(void *);
void start_my_timer(void *);

void configure_time_control(int, int, time_control_t *);
void update_time_control(int, double, time_control_t *);
void set_time_limit(int, time_control_t *);

void test_my_timers(void);

#endif

