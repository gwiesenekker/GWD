// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#include "globals.h"

#define MY_TIMER_STOPPED 0
#define MY_TIMER_STARTED 1

void construct_my_timer(my_timer_t *arg_with_my_timer,
                        char *arg_name,
                        my_printf_t *arg_my_printf,
                        int arg_suppress_warnings)
{
  HARDBUG((arg_with_my_timer->MT_bname = bfromcstr(arg_name)) == NULL)

  arg_with_my_timer->MT_my_printf = arg_my_printf;

  if (arg_suppress_warnings)
  {
    arg_with_my_timer->MT_return_wall_clock_warning_given = TRUE;
  }
  else
  {
    arg_with_my_timer->MT_return_wall_clock_warning_given = FALSE;
  }

  arg_with_my_timer->MT_status = INVALID;
}

void reset_my_timer(my_timer_t *arg_with_my_timer)
{
  arg_with_my_timer->MT_cpu_time_used = 0.0;
  arg_with_my_timer->MT_wall_time_used = 0.0;

  arg_with_my_timer->MT_start_cpu_clock = return_my_clock();

  HARDBUG(arg_with_my_timer->MT_start_cpu_clock < 0.0)

  arg_with_my_timer->MT_start_wall_clock = compat_time();

  arg_with_my_timer->MT_status = MY_TIMER_STARTED;
}

local void return_time_used(my_timer_t *arg_with_my_timer,
                            double *arg_cpu_time_used,
                            double *arg_wall_time_used)
{
  double current_cpu_clock = return_my_clock();

  HARDBUG(current_cpu_clock < 0.0)

  *arg_cpu_time_used =
    current_cpu_clock - arg_with_my_timer->MT_start_cpu_clock;

  HARDBUG(*arg_cpu_time_used < 0.0)

  double current_wall_clock = compat_time();

  *arg_wall_time_used =
    current_wall_clock - arg_with_my_timer->MT_start_wall_clock;

  if (*arg_wall_time_used < 0.0)
  {
    my_printf(arg_with_my_timer->MT_my_printf,
              "WARNING: SYSTEM TIME ADJUSTMENT DETECTED!\n");

    *arg_wall_time_used = 0.0;
  }
}

double return_my_timer(my_timer_t *arg_with_my_timer, int arg_wall)
{
  double cpu_time_used = 0.0;
  double wall_time_used = 0.0;

  if (arg_with_my_timer->MT_status == MY_TIMER_STARTED)
  {
    return_time_used(arg_with_my_timer, &cpu_time_used, &wall_time_used);

    cpu_time_used += arg_with_my_timer->MT_cpu_time_used;

    wall_time_used += arg_with_my_timer->MT_wall_time_used;
  }
  else if (arg_with_my_timer->MT_status == MY_TIMER_STOPPED)
  {
    cpu_time_used = arg_with_my_timer->MT_cpu_time_used;

    wall_time_used = arg_with_my_timer->MT_wall_time_used;
  }
  else
    FATAL("arg_with_my_timer->MT_status error", EXIT_FAILURE)

  double result = cpu_time_used;

  if (arg_wall)
  {
    result = wall_time_used;
  }
  else if ((wall_time_used > options.wall_time_threshold) and
           (wall_time_used > (1.2 * cpu_time_used)))
  {
    if (!arg_with_my_timer->MT_return_wall_clock_warning_given)
    {
      my_printf(arg_with_my_timer->MT_my_printf,
                "WARNING: RETURNING WALL CLOCK FOR TIMER %s!\n",
                bdata(arg_with_my_timer->MT_bname));

      arg_with_my_timer->MT_return_wall_clock_warning_given = TRUE;
    }
    result = wall_time_used;
  }

  return (result);
}

void stop_my_timer(my_timer_t *arg_with_my_timer)
{
  HARDBUG(arg_with_my_timer->MT_status != MY_TIMER_STARTED)

  double cpu_time_used;

  double wall_time_used;

  return_time_used(arg_with_my_timer, &cpu_time_used, &wall_time_used);

  arg_with_my_timer->MT_cpu_time_used += cpu_time_used;

  arg_with_my_timer->MT_wall_time_used += wall_time_used;

  arg_with_my_timer->MT_status = MY_TIMER_STOPPED;

  my_printf(arg_with_my_timer->MT_my_printf,
            "arg_with_my_timer->MT_bname=%s"
            " arg_with_my_timer->MT_cpu_time_used=%.6f"
            " arg_with_my_timer->MT_wall_time_used=%.0f\n",
            bdata(arg_with_my_timer->MT_bname),
            arg_with_my_timer->MT_cpu_time_used,
            arg_with_my_timer->MT_wall_time_used);
}

void start_my_timer(my_timer_t *arg_with_my_timer)
{
  HARDBUG(arg_with_my_timer->MT_status != MY_TIMER_STOPPED)

  arg_with_my_timer->MT_start_cpu_clock = return_my_clock();

  HARDBUG(arg_with_my_timer->MT_start_cpu_clock < 0.0)

  arg_with_my_timer->MT_start_wall_clock = compat_time();

  arg_with_my_timer->MT_status = MY_TIMER_STARTED;
}

void destroy_my_timer(my_timer_t *arg_with_my_timer)
{
  BDESTROY(arg_with_my_timer->MT_bname)
}

void construct_progress(progress_t *arg_progress,
                        char *arg_name,
                        i64_t arg_ntotal,
                        double arg_seconds)
{
  HARDBUG((arg_progress->P_bname = bfromcstr(arg_name)) == NULL)

  construct_my_timer(&(arg_progress->P_timer), arg_name, STDOUT, FALSE);

  reset_my_timer(&(arg_progress->P_timer));

  arg_progress->P_ntotal = arg_ntotal;

  if ((arg_progress->P_ntotal == 0) or !my_mpi_globals.MMG_is_slave)
  {
    arg_progress->P_ntodo = arg_ntotal;
  }
  else
  {
    i64_t nslaves = my_mpi_globals.MMG_nslaves;
    i64_t id_slave = my_mpi_globals.MMG_id_slave;

    arg_progress->P_ntodo = (arg_ntotal + nslaves - 1 - id_slave) / nslaves;

    // arg_progress->P_ntodo = arg_ntotal / my_mpi_globals.MMG_nslaves + 1;
  }

  arg_progress->P_seconds = arg_seconds;
  arg_progress->P_ndone = 0;
  arg_progress->P_ndone_previous = 0;
  arg_progress->P_seconds_done = 0.0;
  arg_progress->P_seconds_previous = 0.0;
  arg_progress->P_ndelta = 0;

  if (arg_progress->P_ntotal > 0)
  {
    PRINTF("arg_progress->P_bname=%s"
           " arg_progress->P_ntotal=%lld"
           " arg_progress->P_ntodo=%lld\n",
           bdata(arg_progress->P_bname),
           arg_progress->P_ntotal,
           arg_progress->P_ntodo);
  }
}

int make_progress(progress_t *arg_progress, i64_t idone)
{
  if ((arg_progress->P_ntotal > 0) and (idone > 0) and
      (my_mpi_globals.MMG_is_slave) and
      ((idone % my_mpi_globals.MMG_nslaves) != my_mpi_globals.MMG_id_slave))
    return (FALSE);

  arg_progress->P_ndone++;

  if ((arg_progress->P_ntodo > 0) and
      (arg_progress->P_ndone > arg_progress->P_ntodo))
  {
    PRINTF("WARNING: arg_progress->P_ndone > arg_progress->P_ntodo"
           " arg_progress->P_bname=%s"
           " arg_progres->P_ndone=%lld"
           " arg_progres->P_ntodo=%lld\n",
           bdata(arg_progress->P_bname),
           arg_progress->P_ndone,
           arg_progress->P_ntodo);

    // FATAL("P_ndone > P_ntodo", EXIT_FAILURE)
  }

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

      PRINTF("arg_progress->P_bname=%s"
             " arg_progress->P_ndone=%lld"
             " (%.0f items/second)\n$",
             bdata(arg_progress->P_bname),
             arg_progress->P_ndone,
             speed);

      if (arg_progress->P_ntodo > 0)
      {
        i64_t nremaining = arg_progress->P_ntodo - arg_progress->P_ndone;

        PRINTF("arg_progress->P_bname=%s"
               " nremaining=%lld"
               " ETA=%.6f seconds\n$",
               bdata(arg_progress->P_bname),
               nremaining,
               (double)nremaining / speed);
      }

      arg_progress->P_ndone_previous = arg_progress->P_ndone;

      arg_progress->P_seconds_previous = arg_progress->P_seconds_done;

      arg_progress->P_ndelta = round(speed * seconds_delta);

      if (arg_progress->P_ndelta == 0)
        arg_progress->P_ndelta = 1;
    }
  }
  return (TRUE);
}

void finalize_progress(progress_t *arg_progress)
{
  arg_progress->P_seconds_done =
    return_my_timer(&(arg_progress->P_timer), FALSE);

  PRINTF("progress %s"
         " took %.6f seconds"
         " for %lld items"
         " (%.0f items/second)\n",
         bdata(arg_progress->P_bname),
         arg_progress->P_seconds_done,
         arg_progress->P_ndone,
         (double)arg_progress->P_ndone / arg_progress->P_seconds_done);

  if (arg_progress->P_ntotal > 0)
  {
    i64_t ntotal_mpi = arg_progress->P_ndone;

    my_mpi_allreduce(MPI_IN_PLACE,
                     &ntotal_mpi,
                     1,
                     MPI_LONG_LONG_INT,
                     MPI_SUM,
                     my_mpi_globals.MMG_comm_slaves);

    HARDBUG(ntotal_mpi < arg_progress->P_ntotal);

    if (ntotal_mpi > arg_progress->P_ntotal)
    {
      PRINTF("WARNING ntotal_mpi > arg_progress->P_ntotal"
             " arg_progress->P_bname=%s"
             " ntotal_mpi=%lld"
             " arg_progress->P_ntotal=%lld\n",
             bdata(arg_progress->P_bname),
             ntotal_mpi,
             arg_progress->P_ntotal);
    }
  }
}

void destroy_progress(progress_t *arg_progress)
{
  BDESTROY(arg_progress->P_bname)

  destroy_my_timer(&(arg_progress->P_timer));
}

i64_t return_ndone(progress_t *arg_progress) { return (arg_progress->P_ndone); }
i64_t return_ntodo(progress_t *arg_progress) { return (arg_progress->P_ntodo); }

local double cdf(int arg_game_time, int arg_imove)
{
  return (arg_game_time / 2.0 *
          (1.0 + erf((arg_imove - options.time_control_mean) /
                     (sqrt(2.0) * options.time_control_sigma))));
}

void configure_time_control(int arg_game_time,
                            int arg_ngame_moves,
                            time_control_t *arg_with_time_control)
{
  arg_ngame_moves += options.time_control_ntrouble;

  arg_with_time_control->TC_ngame_moves = arg_ngame_moves;

  double game_time_left = arg_game_time;

  if (options.time_control_method == 0)
  {
    // gaussian distribution

    for (int imove = 0; imove <= arg_ngame_moves; ++imove)
    {
      //(imove - 1) can be negative

      arg_with_time_control->TC_game_time_per_move[imove] =
        cdf(arg_game_time, imove) - cdf(arg_game_time, imove - 1);

      game_time_left -= arg_with_time_control->TC_game_time_per_move[imove];
    }
  }
  else
  {
    double game_time_half = (double)arg_game_time / 2.0;

    arg_with_time_control->TC_game_time_per_move[options.time_control_mean] =
      game_time_half / options.time_control_sigma;

    game_time_left -=
      arg_with_time_control->TC_game_time_per_move[options.time_control_mean];

    for (int imove = options.time_control_mean + 1; imove <= arg_ngame_moves;
         ++imove)
    {
      game_time_half -= arg_with_time_control->TC_game_time_per_move[imove - 1];

      arg_with_time_control->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= arg_with_time_control->TC_game_time_per_move[imove];
    }

    game_time_half = (double)arg_game_time / 2.0;

    for (int imove = options.time_control_mean - 1; imove >= 0; --imove)
    {
      game_time_half -= arg_with_time_control->TC_game_time_per_move[imove + 1];

      arg_with_time_control->TC_game_time_per_move[imove] =
        game_time_half / options.time_control_sigma;

      game_time_left -= arg_with_time_control->TC_game_time_per_move[imove];
    }
  }

  // distribute tails and integrate

  PRINTF("game_time_left=%.2f\n", game_time_left);

  double S_total_game_time = 0.0;

  for (int imove = 0; imove <= arg_ngame_moves; ++imove)
  {
    arg_with_time_control->TC_game_time_per_move[imove] +=
      game_time_left / arg_ngame_moves;

    PRINTF("imove=%d time_per_move[imove]=%.2f\n",
           imove,
           arg_with_time_control->TC_game_time_per_move[imove]);

    S_total_game_time += arg_with_time_control->TC_game_time_per_move[imove];
  }

  PRINTF("game_time=%d S_total_game_time=%.2f\n",
         arg_game_time,
         S_total_game_time);
}

void update_time_control(int arg_jmove,
                         double arg_move_time,
                         time_control_t *arg_with_time_control)
{
  double delta =
    (arg_move_time - arg_with_time_control->TC_game_time_per_move[arg_jmove]);

  PRINTF("jmove=%d delta=%.2f\n", arg_jmove, delta);

  int moves2go = arg_with_time_control->TC_ngame_moves - (arg_jmove + 1);

  PRINTF("moves2go=%d\n", moves2go);

  if (moves2go > 0)
  {
    for (int imove = arg_jmove + 1;
         imove <= arg_with_time_control->TC_ngame_moves;
         ++imove)
    {
      arg_with_time_control->TC_game_time_per_move[imove] -= delta / moves2go;

      PRINTF("imove=%d time_per_move[imove]=%.2f\n",
             imove,
             arg_with_time_control->TC_game_time_per_move[imove]);
    }
  }
}

void set_time_limit(int arg_jmove, time_control_t *arg_with_time_control)
{
  HARDBUG(arg_jmove > arg_with_time_control->TC_ngame_moves);

  options.time_limit = arg_with_time_control->TC_game_time_per_move[arg_jmove];

  if (options.time_limit < 0.1)
    options.time_limit = 0.1;

  for (int itrouble = 0; itrouble < options.time_control_ntrouble; itrouble++)
  {
    options.time_trouble[itrouble] = 0.1;

    if ((arg_jmove + itrouble + 1) <= arg_with_time_control->TC_ngame_moves)
    {
      options.time_trouble[itrouble] =
        arg_with_time_control->TC_game_time_per_move[arg_jmove + itrouble + 1];
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

  while (TRUE)
  {
    double t2 = time(NULL);
    double d = t2 - t1;
    s += d;

    if (d >= t)
      break;
  }
  return (s);
}

#define NTEST 4
#define NSECONDS 5

void test_my_timers(void)
{
  my_timer_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
  {
    BSTRING(bname)

    HARDBUG(bformata(bname, "-%d", itest) == BSTR_ERR)

    construct_my_timer(test + itest, bdata(bname), STDOUT, FALSE);

    BDESTROY(bname)
  }

  PRINTF("resetting and starting timers..\n");

  for (int itest = 0; itest < NTEST; itest++)
    reset_my_timer(test + itest);

  for (int isecond = 1; isecond <= NSECONDS; ++isecond)
  {
    PRINTF("sleeping for %d seconds..\n", isecond);

    HARDBUG(compat_sleep(isecond) != 0)

    for (int itest = 0; itest < NTEST; itest++)
    {
      double cpu = return_my_timer(test + itest, FALSE);
      double wall = return_my_timer(test + itest, TRUE);

      PRINTF("itimer=%d cpu=%.6f wall=%.0f\n", itest, cpu, wall);
    }

    PRINTF("burning CPU for %d +/- 0.5 seconds..\n", isecond);

    burn_cpu(isecond);

    for (int itest = 0; itest < NTEST; itest++)
    {
      double cpu = return_my_timer(test + itest, FALSE);
      double wall = return_my_timer(test + itest, TRUE);

      PRINTF("itimer=%d cpu=%.6f wall=%.0f\n", itest, cpu, wall);
    }
  }
}
