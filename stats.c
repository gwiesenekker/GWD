//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

void clear_stats(void *self)
{
  stats_t *object = self;
 
  object->S_n = 0;

  object->S_min = 0.0;
  object->S_max = 0.0;
  object->S_mean = 0.0;
  object->S_sigma = 0.0;

  object->S_sum = 0.0;
  object->S_sum2 = 0.0;
  object->S_mean_sum = 0.0;
  object->S_sigma_sum2 = 0.0;
}

void construct_stats(void *self, char *arg_name)
{
  stats_t *object = self;

  HARDBUG((object->S_name = bfromcstr(arg_name)) == NULL)

  clear_stats(object);
}

void mean_sigma(void *self)
{
  stats_t *object = self;

  object->S_sigma = sqrt(object->S_sigma / object->S_n);

  object->S_mean_sum = object->S_sum / object->S_n;

  object->S_sigma_sum2 = sqrt(object->S_sum2 / object->S_n -
                              object->S_mean_sum * object->S_mean_sum);
}

void printf_stats(void *self)
{
  stats_t *object = self;

  PRINTF("name=%s\n", bdata(object->S_name));
  PRINTF("n=%lld\n", object->S_n);
  PRINTF("min=%.6f\n", object->S_min);
  PRINTF("max=%.6f\n", object->S_max);

  PRINTF("mean=%.6f(%.6f)\n", object->S_mean, object->S_mean_sum);
  PRINTF("sigma=%.6f(%.6f)\n", object->S_sigma, object->S_sigma_sum2);
}

void my_stats_aggregate(void *self, MPI_Comm arg_comm)
{
  if (arg_comm == MPI_COMM_NULL) return;

  stats_t *object = self;

  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_n), 1,
    MPI_LONG_LONG_INT, MPI_SUM, arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_min), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_max), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_mean), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_sigma), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_sum), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_sum2), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_mean_sum), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE, &(object->S_sigma_sum2), 1,
    MPI_DOUBLE, MPI_SUM, arg_comm);
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
