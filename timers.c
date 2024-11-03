//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

#define MY_TIMER_STOPPED 0
#define MY_TIMER_STARTED 1

void construct_my_timer(void *self, char *arg_name,
  my_printf_t *arg_my_printf, int arg_suppress_warnings)
{
  my_timer_t *object = self;

  HARDBUG((object->MT_name = bfromcstr(arg_name)) == NULL)

  object->MT_my_printf = arg_my_printf;

  if (arg_suppress_warnings)
  {
    object->MT_return_wall_clock_warning_given = TRUE;
  }
  else
  {
    object->MT_return_wall_clock_warning_given = FALSE;
  }
  
  object->MT_status = INVALID;
}

void reset_my_timer(void *self)
{
  my_timer_t *object = self;

  object->MT_cpu_time_used = 0.0;
  object->MT_wall_time_used = 0.0;

  object->MT_start_cpu_clock = return_my_clock();

  object->MT_start_wall_clock = compat_time();

  object->MT_status = MY_TIMER_STARTED;
}

local void return_time_used(my_timer_t *object,
  double *arg_cpu_time_used, double *arg_wall_time_used)
{
  double current_cpu_clock = return_my_clock();

  *arg_cpu_time_used = current_cpu_clock - object->MT_start_cpu_clock;

  HARDBUG(*arg_cpu_time_used < 0.0)

  double current_wall_clock = compat_time();

  *arg_wall_time_used =
    current_wall_clock - object->MT_start_wall_clock;

  if (*arg_wall_time_used < 0.0)
  {
    my_printf(object->MT_my_printf,
      "WARNING: SYSTEM TIME ADJUSTMENT DETECTED!\n");

    *arg_wall_time_used = 0.0;
  }
}

double return_my_timer(void *self, int arg_wall)
{
  my_timer_t *object = self;

  double cpu_time_used = 0.0;
  double wall_time_used = 0.0;

  if (object->MT_status == MY_TIMER_STARTED)
  {
    return_time_used(object, &cpu_time_used, &wall_time_used);

    cpu_time_used += object->MT_cpu_time_used;

    wall_time_used += object->MT_wall_time_used;
  }
  else if (object->MT_status == MY_TIMER_STOPPED)
  {
    cpu_time_used = object->MT_cpu_time_used;

    wall_time_used = object->MT_wall_time_used;
  }
  else
    FATAL("object->MT_status error", EXIT_FAILURE)

  double result = cpu_time_used;

  if (arg_wall)
  {
    result = wall_time_used;
  }
  else if ((wall_time_used > options.wall_time_threshold) and
           (wall_time_used > (1.2 * cpu_time_used)))
  {
    if (!object->MT_return_wall_clock_warning_given)
    {
      my_printf(object->MT_my_printf,
        "WARNING: RETURNING WALL CLOCK FOR TIMER %s!\n",
        bdata(object->MT_name));

      object->MT_return_wall_clock_warning_given = TRUE;
    }
    result = wall_time_used;
  }

  END_BLOCK

  return(result);
}

void stop_my_timer(void *self)
{
  my_timer_t *object = self;

  HARDBUG(object->MT_status != MY_TIMER_STARTED)

  double cpu_time_used;

  double wall_time_used;

  return_time_used(object, &cpu_time_used, &wall_time_used);

  object->MT_cpu_time_used += cpu_time_used;

  object->MT_wall_time_used += wall_time_used;

  object->MT_status = MY_TIMER_STOPPED;

  my_printf(object->MT_my_printf, "TIMER=%s CPU=%.2f WALL=%.2f\n",
    bdata(object->MT_name),
    object->MT_cpu_time_used, object->MT_wall_time_used);
}

void start_my_timer(void *self)
{
  my_timer_t *object = self;

  HARDBUG(object->MT_status != MY_TIMER_STOPPED)

  object->MT_start_cpu_clock = return_my_clock();

  object->MT_start_wall_clock = compat_time();

  object->MT_status = MY_TIMER_STARTED;
}

local double cdf(int game_time, int imove)
{
  return(game_time / 2.0 *
         (1.0 + erf((imove - options.time_control_mean)/
                    (sqrt(2.0) * options.time_control_sigma))));
}

void configure_time_control(int game_time, int ngame_moves,
  time_control_t *time_control)
{
  time_control->TC_game_time = game_time;

  ngame_moves += options.time_control_ntrouble;

  time_control->TC_ngame_moves = ngame_moves;

  double game_time_left = game_time;

  if (options.time_control_method == 0)
  {
    //gaussian distribution
  
    for (int imove = 0; imove <= ngame_moves; ++imove)
    {
      //(imove - 1) can be negative

      time_control->TC_game_time_per_move[imove] =
        cdf(game_time, imove) - cdf(game_time, imove - 1);
  
      game_time_left -= time_control->TC_game_time_per_move[imove];
    }
  }
  else
  {
    double game_time_half = (double) game_time / 2.0;

    time_control->TC_game_time_per_move[options.time_control_mean] =
      game_time_half / options.time_control_sigma;

    game_time_left -=
      time_control->TC_game_time_per_move[options.time_control_mean];

    for (int imove = options.time_control_mean + 1; imove <= ngame_moves;
         ++imove)
    {
      game_time_half -= time_control->TC_game_time_per_move[imove - 1];

      time_control->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= time_control->TC_game_time_per_move[imove];
    }

    game_time_half = (double) game_time / 2.0;

    for (int imove = options.time_control_mean - 1; imove >= 0; --imove)
    {
      game_time_half -= time_control->TC_game_time_per_move[imove + 1];

      time_control->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= time_control->TC_game_time_per_move[imove];
    }
  }
  
  //distribute tails and integrate
  
  PRINTF("game_time_left=%.2f\n", game_time_left);
  
  double total_game_time = 0.0;
  
  for (int imove = 0; imove <= ngame_moves; ++imove)
  {
    time_control->TC_game_time_per_move[imove] +=
      game_time_left / ngame_moves;
     
    PRINTF("imove=%d time_per_move[imove]=%.2f\n",
      imove, time_control->TC_game_time_per_move[imove]);
  
    total_game_time += time_control->TC_game_time_per_move[imove];
  }

  PRINTF("game_time=%d total_game_time=%.2f\n", game_time, total_game_time);
}

void update_time_control(int jmove, double move_time,
  time_control_t *time_control)
{
  double delta = (move_time - time_control->TC_game_time_per_move[jmove]);

  PRINTF("jmove=%d delta=%.2f\n", jmove, delta);

  int moves2go = time_control->TC_ngame_moves - (jmove + 1);

  PRINTF("moves2go=%d\n", moves2go);
      
  if (moves2go > 0)
  {
    for (int imove = jmove + 1; imove <= time_control->TC_ngame_moves;
         ++imove)
    {
      time_control->TC_game_time_per_move[imove] -= delta / moves2go;

      PRINTF("imove=%d time_per_move[imove]=%.2f\n",
        imove, time_control->TC_game_time_per_move[imove]);
    }
  }
}

void set_time_limit(int jmove, time_control_t *time_control)
{
  HARDBUG(jmove > time_control->TC_ngame_moves);

  options.time_limit = time_control->TC_game_time_per_move[jmove];

  if (options.time_limit < 0.1) options.time_limit = 0.1;

  for (int itrouble = 0; itrouble < options.time_control_ntrouble;
       itrouble++)
  {
    options.time_trouble[itrouble] = 0.1;

    if ((jmove + itrouble + 1) <= time_control->TC_ngame_moves)
    {
      options.time_trouble[itrouble] =
        time_control->TC_game_time_per_move[jmove + itrouble + 1];
      if (options.time_trouble[itrouble] < 0.1)
        options.time_trouble[itrouble] = 0.1;
    }
  }
  options.time_ntrouble = options.time_control_ntrouble;
}

local double burn_cpu(double t)
{
  double t1 = time(NULL);

  double s = 0.0;

  while(TRUE)
  {
    double t2 = time(NULL);
    double d = t2 - t1;
    s += d;

    if (d >= t) break;
  }
  return(s);
}

#define NTEST    4
#define NSECONDS 5

void test_my_timers(void)
{
  my_timer_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
  {
    bstring bname = bfromcstr("test");

    HARDBUG(bname == NULL)

    HARDBUG(bformata(bname, "%d", itest) != BSTR_OK)

    construct_my_timer(test + itest, bdata(bname),  STDOUT, FALSE);
  }

  PRINTF("resetting and starting timers..\n");

  for (int itest = 0; itest < NTEST; itest++)
    reset_my_timer(test + itest);
  
  for (int isecond = 1; isecond <= NSECONDS; ++isecond)
  {
    PRINTF("sleeping for %d seconds..\n", isecond);

    compat_sleep(isecond);

    for (int itest = 0; itest < NTEST; itest++)
    {
      double cpu = return_my_timer(test + itest, FALSE);
      double wall = return_my_timer(test + itest, TRUE);

      PRINTF("itimer=%d cpu=%.2f wall=%.2f\n",
        test + itest, cpu, wall);
    }

    PRINTF("burning CPU for %d +/- 0.5 seconds..\n", isecond);
  
    burn_cpu(isecond);

    for (int itest = 0; itest < NTEST; itest++)
    {
      double cpu = return_my_timer(test + itest, FALSE);
      double wall = return_my_timer(test + itest, TRUE);

      PRINTF("itimer=%d cpu=%.2f wall=%.2f\n",
        test + itest, cpu, wall);
    }
  }
}

