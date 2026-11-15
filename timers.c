// SCU REVISION 8.1278 zo 15 nov 2026 14:53:59 CET
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

local void return_my_timer_time_used(my_timer_t *arg_with_my_timer,
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
    return_my_timer_time_used(arg_with_my_timer, &cpu_time_used, &wall_time_used);

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

  return_my_timer_time_used(arg_with_my_timer, &cpu_time_used, &wall_time_used);

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

void construct_time_control(time_control_t *arg_with_time_control)
{
  *arg_with_time_control = (time_control_t){.TC_ngame_moves = INVALID};
}

double return_time_used(const time_control_t *arg_with_time_control)
{
  return arg_with_time_control->TC_time_used;
}

double return_time_left(const time_control_t *arg_with_time_control)
{
  HARDBUG(arg_with_time_control->TC_ngame_moves == INVALID)
  return (arg_with_time_control->TC_game_time - arg_with_time_control->TC_time_used) +
         (arg_with_time_control->TC_nmoves_done * arg_with_time_control->TC_increment);
}

int is_configured_time_control(const time_control_t *arg_with_time_control)
{
  return arg_with_time_control->TC_ngame_moves != INVALID;
}

int is_rolling_time_control(const time_control_t *arg_with_time_control)
{
  return arg_with_time_control->TC_rolling;
}

int return_moves_done(const time_control_t *arg_with_time_control)
{
  return arg_with_time_control->TC_nmoves_done;
}

// Signed so callers can count moves played beyond the agreed control.
int return_moves_left(const time_control_t *arg_with_time_control)
{
  HARDBUG(arg_with_time_control->TC_ngame_moves == INVALID)
  if (arg_with_time_control->TC_rolling)
    return arg_with_time_control->TC_ngame_moves;
  return arg_with_time_control->TC_ngame_moves - arg_with_time_control->TC_nmoves_done;
}

local double gaussian_cdf(int arg_imove)
{
  return 0.5 * (1.0 + erf((arg_imove - options.time_control_mean) /
                          (sqrt(2.0) * options.time_control_sigma)));
}

// Return the unassigned weight for redistribution over the schedule.
local double gaussian_weights(int arg_first_move, int arg_nweights, double *arg_weights)
{
  double weight_left = 1.0;
  double previous = gaussian_cdf(arg_first_move - 1);

  for (int islot = 0; islot < arg_nweights; ++islot)
  {
    double current = gaussian_cdf(arg_first_move + islot);
    arg_weights[islot] = current - previous;
    weight_left -= arg_weights[islot];
    previous = current;
  }
  return weight_left;
}

local double geometric_weights(int arg_first_move, int arg_nweights, double *arg_weights)
{
  int mean = options.time_control_mean - arg_first_move;
  double sigma = options.time_control_sigma;
  double half = 0.5;
  if (mean < 0)
  {
    half *= pow(1.0 - (1.0 / sigma), -mean);
    mean = 0;
  }
  double peak = half / sigma;
  if (mean < arg_nweights)
    arg_weights[mean] = peak;
  double weight_left = 1.0 - peak;

  for (int islot = mean + 1; islot < arg_nweights; ++islot)
  {
    half -= arg_weights[islot - 1];
    arg_weights[islot] = half / sigma;
    weight_left -= arg_weights[islot];
  }

  // Count the whole left tail even when the game ends before the peak.
  half = 0.5;
  double weight = peak;
  for (int islot = mean - 1; islot >= 0; --islot)
  {
    half -= weight;
    weight = half / sigma;
    weight_left -= weight;
    if (islot < arg_nweights)
      arg_weights[islot] = weight;
  }
  return weight_left;
}

#define ROLLING_GAME_MOVES 40

local void allocate_game_time(time_control_t *arg_with_time_control)
{
  int first = arg_with_time_control->TC_nmoves_done;
  int moves = arg_with_time_control->TC_ngame_moves;
  int nweights = arg_with_time_control->TC_rolling ? moves :
             (moves + arg_with_time_control->TC_ntrouble + 1);
  double game_time = fmax(0.0, return_time_left(arg_with_time_control));
  double *per_move = arg_with_time_control->TC_game_time_per_move;
  double weight_left;
  if (options.time_control_method == 0)
    weight_left = gaussian_weights(first, nweights, per_move);
  else
    weight_left = geometric_weights(first, nweights, per_move);

  // The rolling window contains exactly 'moves' slots; the fixed schedule
  // retains its original inclusive reserve slot and tail divisor.
  if (arg_with_time_control->TC_rolling)
  {
    weight_left = 1.0;
    for (int islot = 0; islot < nweights; ++islot)
      weight_left -= per_move[islot];
  }
  double tail_weight = weight_left / (arg_with_time_control->TC_rolling ? nweights : (nweights - 1));
  for (int islot = 0; islot < nweights; ++islot)
    per_move[islot] = game_time * (per_move[islot] + tail_weight);
}

void configure_time_control(double arg_game_time,
                            int arg_ngame_moves,
                            double arg_increment,
                            time_control_t *arg_with_time_control)
{
  int rolling = (arg_ngame_moves == INVALID);
  if (rolling)
    arg_ngame_moves = ROLLING_GAME_MOVES;

  HARDBUG((!isfinite(arg_game_time)) or (arg_game_time < 0.0) or (arg_ngame_moves < 0))
  HARDBUG((!isfinite(arg_increment)) or (arg_increment < 0.0))
  HARDBUG((options.time_control_ntrouble < 0) or
          (arg_ngame_moves >= (NODE_MAX - options.time_control_ntrouble)))
  HARDBUG((arg_ngame_moves + options.time_control_ntrouble) == 0)
  HARDBUG(options.time_control_sigma < 1)
  if (options.time_control_method != 0)
    HARDBUG((options.time_control_mean < 0) or (options.time_control_mean >= NODE_MAX))

  arg_with_time_control->TC_rolling = rolling;
  arg_with_time_control->TC_increment = arg_increment;
  arg_with_time_control->TC_game_time = arg_game_time;
  arg_with_time_control->TC_ngame_moves = arg_ngame_moves;
  arg_with_time_control->TC_ntrouble = options.time_control_ntrouble;
  arg_with_time_control->TC_nmoves_done = 0;
  arg_with_time_control->TC_time_used = 0.0;

  allocate_game_time(arg_with_time_control);
}

void update_time_control(double arg_move_time,
                         time_control_t *arg_with_time_control)
{
  HARDBUG(arg_with_time_control->TC_ngame_moves == INVALID)
  int jmove = arg_with_time_control->TC_nmoves_done;
  int last_move = arg_with_time_control->TC_ngame_moves + arg_with_time_control->TC_ntrouble;
  HARDBUG((!isfinite(arg_move_time)) or (arg_move_time < 0.0))
  ++arg_with_time_control->TC_nmoves_done;
  arg_with_time_control->TC_time_used += arg_move_time;
  if (arg_with_time_control->TC_rolling)
  {
    allocate_game_time(arg_with_time_control);
    return;
  }
  if (jmove > last_move)
    return;

  double delta =
    (arg_move_time - arg_with_time_control->TC_increment) -
    arg_with_time_control->TC_game_time_per_move[jmove];

  PRINTF("jmove=%d delta=%.2f\n", jmove, delta);

  int moves2go = last_move - (jmove + 1);

  PRINTF("moves2go=%d\n", moves2go);

  if (moves2go > 0)
  {
    for (int imove = jmove + 1;
         imove <= last_move;
         ++imove)
    {
      arg_with_time_control->TC_game_time_per_move[imove] -= delta / moves2go;

      PRINTF("imove=%d time_per_move[imove]=%.2f\n",
             imove,
             arg_with_time_control->TC_game_time_per_move[imove]);
    }
  }
}

// Reconcile an external clock report with the same schedule used by DXP.
void sync_time_control(double arg_time_left,
                       int arg_moves_left,
                       double arg_increment,
                       int arg_played,
                       time_control_t *arg_with_time_control)
{
  HARDBUG((!isfinite(arg_time_left)) or (arg_time_left < 0.0))
  HARDBUG((!isfinite(arg_increment)) or (arg_increment < 0.0))

  int rolling = (arg_moves_left == INVALID);
  if ((!is_configured_time_control(arg_with_time_control)) or
      (rolling != arg_with_time_control->TC_rolling))
  {
    configure_time_control(arg_time_left, arg_moves_left, arg_increment,
                           arg_with_time_control);
    return;
  }

  if (!rolling)
  {
    int moves_left = return_moves_left(arg_with_time_control);
    if (arg_moves_left == moves_left)
      arg_played = FALSE;
    else if (arg_moves_left == (moves_left - 1))
      arg_played = TRUE;
    else
    {
      // A new control period, or the GUI jumped to another move.
      configure_time_control(arg_time_left, arg_moves_left, arg_increment,
                             arg_with_time_control);
      return;
    }
  }

  if (arg_played)
  {
    double elapsed = return_time_left(arg_with_time_control) +
                     arg_with_time_control->TC_increment - arg_time_left;
    update_time_control(fmax(0.0, elapsed), arg_with_time_control);
  }

  // Changing the increment applies to future moves only.
  double correction = arg_time_left - return_time_left(arg_with_time_control);
  arg_with_time_control->TC_game_time += correction +
    arg_with_time_control->TC_nmoves_done *
    (arg_with_time_control->TC_increment - arg_increment);
  arg_with_time_control->TC_increment = arg_increment;

  if (correction != 0.0)
  {
    if (rolling)
      allocate_game_time(arg_with_time_control);
    else
    {
      int first = arg_with_time_control->TC_nmoves_done;
      int last = arg_with_time_control->TC_ngame_moves + arg_with_time_control->TC_ntrouble;
      for (int islot = first; islot <= last; ++islot)
        arg_with_time_control->TC_game_time_per_move[islot] +=
          correction / (last - first + 1);
    }
  }
}

void set_time_limit(const time_control_t *arg_with_time_control)
{
  HARDBUG(arg_with_time_control->TC_ngame_moves == INVALID)
  int jmove = arg_with_time_control->TC_rolling ? 0 : arg_with_time_control->TC_nmoves_done;
  int last = arg_with_time_control->TC_rolling ? (arg_with_time_control->TC_ngame_moves - 1) :
             (arg_with_time_control->TC_ngame_moves + arg_with_time_control->TC_ntrouble);
  HARDBUG(jmove > last)

  double remaining = fmax(0.0, return_time_left(arg_with_time_control));
  for (int itrouble = 0; itrouble <= arg_with_time_control->TC_ntrouble; ++itrouble)
  {
    int islot = jmove + itrouble;
    double limit = 0.1;
    if (islot <= last)
      limit = fmax(limit, arg_with_time_control->TC_game_time_per_move[islot] +
                          arg_with_time_control->TC_increment);
    else if (arg_with_time_control->TC_rolling)
      limit = 0.0;

    if ((arg_with_time_control->TC_increment > 0.0) or arg_with_time_control->TC_rolling)
    {
      limit = fmin(limit, remaining);
      remaining -= limit;
    }

    if (itrouble == 0)
      options.time_limit = limit;
    else
      options.time_trouble[itrouble - 1] = limit;
  }
  options.time_ntrouble = arg_with_time_control->TC_ntrouble;
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
