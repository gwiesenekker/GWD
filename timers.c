//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

#define MY_TIMER_STOPPED 0
#define MY_TIMER_STARTED 1

void construct_my_timer(my_timer_t *self, char *arg_name,
  my_printf_t *arg_my_printf, int arg_suppress_warnings)
{
  my_timer_t *object = self;

  HARDBUG((object->MT_bname = bfromcstr(arg_name)) == NULL)

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

void reset_my_timer(my_timer_t *self)
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

double return_my_timer(my_timer_t *self, int arg_wall)
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
        bdata(object->MT_bname));

      object->MT_return_wall_clock_warning_given = TRUE;
    }
    result = wall_time_used;
  }

  return(result);
}

void stop_my_timer(my_timer_t *self)
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
    bdata(object->MT_bname),
    object->MT_cpu_time_used, object->MT_wall_time_used);
}

void start_my_timer(my_timer_t *self)
{
  my_timer_t *object = self;

  HARDBUG(object->MT_status != MY_TIMER_STOPPED)

  object->MT_start_cpu_clock = return_my_clock();

  object->MT_start_wall_clock = compat_time();

  object->MT_status = MY_TIMER_STARTED;
}

void construct_progress(progress_t *arg_progress, i64_t arg_ntodo,
  double arg_seconds)
{
  construct_my_timer(&(arg_progress->P_timer), "progress", STDOUT, FALSE);

  reset_my_timer(&(arg_progress->P_timer));

  arg_progress->P_ntodo = arg_ntodo;
  arg_progress->P_seconds = arg_seconds; 
  arg_progress->P_ndone = 0;
  arg_progress->P_ndone_previous = 0;
  arg_progress->P_seconds_done = 0.0;
  arg_progress->P_seconds_previous = 0.0;
  arg_progress->P_ndelta = 0;
}

void update_progress(progress_t *arg_progress)
{
  if ((arg_progress->P_ntodo > 0) and
      (arg_progress->P_ndone > arg_progress->P_ntodo))
  {
    PRINTF("P_ndone=%lld P_ntodo=%lld\n", 
           arg_progress->P_ndone, arg_progress->P_ntodo);

    //FATAL("P_ndone > P_ntodo", EXIT_FAILURE)
  }

  arg_progress->P_ndone++;

  if ((arg_progress->P_ndone - arg_progress->P_ndone_previous) >=
      arg_progress->P_ndelta)
  {
    arg_progress->P_seconds_done =
      return_my_timer(&(arg_progress->P_timer), FALSE);

    double seconds_delta = 
      arg_progress->P_seconds_done - arg_progress->P_seconds_previous;

    if (seconds_delta >= arg_progress->P_seconds)
    {
      double speed = arg_progress->P_ndone / arg_progress->P_seconds_done;
  
      PRINTF("ndone=%lld speed=%.2f/second\n$", arg_progress->P_ndone, speed);

      if (arg_progress->P_ntodo > 0)
      {
        i64_t nremaining = arg_progress->P_ntodo - arg_progress->P_ndone;
  
        PRINTF("nremaining=%lld ETA=%.2f seconds\n$",
               nremaining, (double) nremaining / speed);
      }

      arg_progress->P_ndone_previous = arg_progress->P_ndone;
  
      arg_progress->P_seconds_previous = arg_progress->P_seconds_done;

      arg_progress->P_ndelta = round(speed * seconds_delta);

      if (arg_progress->P_ndelta == 0) arg_progress->P_ndelta = 1;
    }
  }
}

void finalize_progress(progress_t *arg_progress)
{
  arg_progress->P_seconds_done =
    return_my_timer(&(arg_progress->P_timer), FALSE);

  PRINTF("progress took %.0f seconds for %lld entries %.0f entries/second\n",
         arg_progress->P_seconds_done, arg_progress->P_ndone,
         (double) arg_progress->P_ndone / arg_progress->P_seconds_done);
}

i64_t return_ndone(progress_t *arg_progress)
{
  return(arg_progress->P_ndone);
}

local double cdf(int arg_game_time, int arg_imove)
{
  return(arg_game_time / 2.0 *
         (1.0 + erf((arg_imove - options.time_control_mean)/
                    (sqrt(2.0) * options.time_control_sigma))));
}

void configure_time_control(int arg_game_time, int arg_ngame_moves,
  time_control_t *object)
{
  object->TC_game_time = arg_game_time;

  arg_ngame_moves += options.time_control_ntrouble;

  object->TC_ngame_moves = arg_ngame_moves;

  double game_time_left = arg_game_time;

  if (options.time_control_method == 0)
  {
    //gaussian distribution
  
    for (int imove = 0; imove <= arg_ngame_moves; ++imove)
    {
      //(imove - 1) can be negative

      object->TC_game_time_per_move[imove] =
        cdf(arg_game_time, imove) - cdf(arg_game_time, imove - 1);
  
      game_time_left -= object->TC_game_time_per_move[imove];
    }
  }
  else
  {
    double game_time_half = (double) arg_game_time / 2.0;

    object->TC_game_time_per_move[options.time_control_mean] =
      game_time_half / options.time_control_sigma;

    game_time_left -=
      object->TC_game_time_per_move[options.time_control_mean];

    for (int imove = options.time_control_mean + 1; imove <= arg_ngame_moves;
         ++imove)
    {
      game_time_half -= object->TC_game_time_per_move[imove - 1];

      object->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= object->TC_game_time_per_move[imove];
    }

    game_time_half = (double) arg_game_time / 2.0;

    for (int imove = options.time_control_mean - 1; imove >= 0; --imove)
    {
      game_time_half -= object->TC_game_time_per_move[imove + 1];

      object->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= object->TC_game_time_per_move[imove];
    }
  }
  
  //distribute tails and integrate
  
  PRINTF("game_time_left=%.2f\n", game_time_left);
  
  double S_total_game_time = 0.0;
  
  for (int imove = 0; imove <= arg_ngame_moves; ++imove)
  {
    object->TC_game_time_per_move[imove] +=
      game_time_left / arg_ngame_moves;
     
    PRINTF("imove=%d time_per_move[imove]=%.2f\n",
      imove, object->TC_game_time_per_move[imove]);
  
    S_total_game_time += object->TC_game_time_per_move[imove];
  }

  PRINTF("game_time=%d S_total_game_time=%.2f\n", arg_game_time, S_total_game_time);
}

void update_time_control(int arg_jmove, double arg_move_time,
  time_control_t *object)
{
  double delta = (arg_move_time - object->TC_game_time_per_move[arg_jmove]);

  PRINTF("jmove=%d delta=%.2f\n", arg_jmove, delta);

  int moves2go = object->TC_ngame_moves - (arg_jmove + 1);

  PRINTF("moves2go=%d\n", moves2go);
      
  if (moves2go > 0)
  {
    for (int imove = arg_jmove + 1; imove <= object->TC_ngame_moves;
         ++imove)
    {
      object->TC_game_time_per_move[imove] -= delta / moves2go;

      PRINTF("imove=%d time_per_move[imove]=%.2f\n",
        imove, object->TC_game_time_per_move[imove]);
    }
  }
}

void set_time_limit(int arg_jmove, time_control_t *object)
{
  HARDBUG(arg_jmove > object->TC_ngame_moves);

  options.time_limit = object->TC_game_time_per_move[arg_jmove];

  if (options.time_limit < 0.1) options.time_limit = 0.1;

  for (int itrouble = 0; itrouble < options.time_control_ntrouble;
       itrouble++)
  {
    options.time_trouble[itrouble] = 0.1;

    if ((arg_jmove + itrouble + 1) <= object->TC_ngame_moves)
    {
      options.time_trouble[itrouble] =
        object->TC_game_time_per_move[arg_jmove + itrouble + 1];
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
    BSTRING(bname)

    HARDBUG(bformata(bname, "-%d", itest) == BSTR_ERR)

    construct_my_timer(test + itest, bdata(bname),  STDOUT, FALSE);

    BDESTROY(bname)
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
             itest, cpu, wall);
    }

    PRINTF("burning CPU for %d +/- 0.5 seconds..\n", isecond);
  
    burn_cpu(isecond);

    for (int itest = 0; itest < NTEST; itest++)
    {
      double cpu = return_my_timer(test + itest, FALSE);
      double wall = return_my_timer(test + itest, TRUE);

      PRINTF("itimer=%d cpu=%.2f wall=%.2f\n",
             itest, cpu, wall);
    }
  }
}

