// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "globals.h"
#include <float.h>

void clear_stats(stats_t *arg_with_stats)
{
  arg_with_stats->S_nsamples = 0;

  arg_with_stats->S_min = DBL_MAX;
  arg_with_stats->S_max = -DBL_MAX;

  arg_with_stats->S_sum = 0.0;
  arg_with_stats->S_sum2 = 0.0;

  arg_with_stats->S_mean = 0.0;
  arg_with_stats->S_sigma = 0.0;
}

void construct_stats(stats_t *arg_with_stats, const char *arg_name)
{
  HARDBUG((arg_with_stats->S_name = bfromcstr(arg_name)) == NULL)

  clear_stats(arg_with_stats);
}

void mean_sigma(stats_t *arg_with_stats)
{
  if (arg_with_stats->S_nsamples == 0)
    return;

  arg_with_stats->S_mean = arg_with_stats->S_sum / arg_with_stats->S_nsamples;

  double variance = sqrt(arg_with_stats->S_sum2 / arg_with_stats->S_nsamples -
                         arg_with_stats->S_mean * arg_with_stats->S_mean);

  if (variance < 0.0)
    variance = 0.0;

  arg_with_stats->S_sigma = sqrt(variance);
}

void printf_stats(const stats_t *arg_with_stats)
{
  PRINTF("name=%s\n", bdata(arg_with_stats->S_name));
  PRINTF("nsamples=%lld\n", arg_with_stats->S_nsamples);
  PRINTF("min=%.6f\n", arg_with_stats->S_min);
  PRINTF("max=%.6f\n", arg_with_stats->S_max);

  PRINTF("mean=%.6f\n", arg_with_stats->S_mean);
  PRINTF("sigma=%.6f\n", arg_with_stats->S_sigma);
}

void my_stats_aggregate(stats_t *arg_with_stats, MPI_Comm arg_comm)
{
  if (arg_comm == MPI_COMM_NULL)
    return;

  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_stats->S_nsamples),
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_stats->S_min),
                   1,
                   MPI_DOUBLE,
                   MPI_MIN,
                   arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_stats->S_max),
                   1,
                   MPI_DOUBLE,
                   MPI_MAX,
                   arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_stats->S_sum),
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_stats->S_sum2),
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   arg_comm);
}

enum
{
  N = 100000
};

void test_stats(void)
{
  stats_t stats;

  construct_stats(&stats, "a");

  update_stats(&stats, 1.0);
  mean_sigma(&stats);
  printf_stats(&stats);

  clear_stats(&stats);
  update_stats(&stats, 1.0);
  update_stats(&stats, 2.0);
  mean_sigma(&stats);
  printf_stats(&stats);

  clear_stats(&stats);
  update_stats(&stats, 1.0);
  update_stats(&stats, 2.0);
  update_stats(&stats, 3.0);
  mean_sigma(&stats);
  printf_stats(&stats);

  clear_stats(&stats);

  for (i64_t isample = 0; isample < N; isample++)
    update_stats(&stats, isample);

  mean_sigma(&stats);
  printf_stats(&stats);
}
