// SCU REVISION 8.1263 di 10 nov 2026 16:08:19 CET
#include "globals.h"
#include <float.h>

void clear_stats(stats_t *arg_with_stats)
{
  arg_with_stats->S_n = 0;

  arg_with_stats->S_min = DBL_MAX;
  arg_with_stats->S_max = -DBL_MAX;

  arg_with_stats->S_sum = 0.0;
  arg_with_stats->S_sum2 = 0.0;

  arg_with_stats->S_mean = 0.0;
  arg_with_stats->S_sigma = 0.0;
}

void construct_stats(stats_t *arg_with_stats, char *arg_name)
{
  HARDBUG((arg_with_stats->S_name = bfromcstr(arg_name)) == NULL)

  clear_stats(arg_with_stats);
}

void mean_sigma(stats_t *arg_with_stats)
{
  if (arg_with_stats->S_n == 0)
    return;

  arg_with_stats->S_mean = arg_with_stats->S_sum / arg_with_stats->S_n;

  double variance = sqrt(arg_with_stats->S_sum2 / arg_with_stats->S_n -
                         arg_with_stats->S_mean * arg_with_stats->S_mean);

  if (variance < 0.0)
    variance = 0.0;

  arg_with_stats->S_sigma = sqrt(variance);
}

void printf_stats(stats_t *arg_with_stats)
{
  PRINTF("name=%s\n", bdata(arg_with_stats->S_name));
  PRINTF("n=%lld\n", arg_with_stats->S_n);
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
                   &(arg_with_stats->S_n),
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

#define N 100000

void test_stats(void)
{
  stats_t a;

  construct_stats(&a, "a");

  update_stats(&a, 1.0);
  mean_sigma(&a);
  printf_stats(&a);

  clear_stats(&a);
  update_stats(&a, 1.0);
  update_stats(&a, 2.0);
  mean_sigma(&a);
  printf_stats(&a);

  clear_stats(&a);
  update_stats(&a, 1.0);
  update_stats(&a, 2.0);
  update_stats(&a, 3.0);
  mean_sigma(&a);
  printf_stats(&a);

  clear_stats(&a);

  for (i64_t i = 0; i < N; i++)
    update_stats(&a, i);

  mean_sigma(&a);
  printf_stats(&a);
}
