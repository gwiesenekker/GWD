// SCU REVISION 8.1263 di 10 nov 2026 16:08:19 CET
#include "globals.h"

#undef WEIBULL

void clear_bucket(bucket_t *arg_with_bucket)
{
  for (i64_t ibucket = 0; ibucket < arg_with_bucket->nbuckets; ibucket++)
    arg_with_bucket->buckets[ibucket] = 0;
}

void update_bucket(bucket_t *arg_with_bucket, double arg_value)
{
  if (arg_value < arg_with_bucket->bucket_min)
    arg_with_bucket->bucket_nlt_min++;
  else if (arg_value >= arg_with_bucket->bucket_max)
    arg_with_bucket->bucket_nge_max++;
  else
  {
    if (arg_with_bucket->bucket_scale == BUCKET_LINEAR)
    {
      i64_t ibucket = floor((arg_value - arg_with_bucket->bucket_min) /
                            arg_with_bucket->bucket_size);

      if (ibucket >= arg_with_bucket->nbuckets)
        ibucket = arg_with_bucket->nbuckets - 1;

      arg_with_bucket->buckets[ibucket]++;
    }
    else if (arg_with_bucket->bucket_scale == BUCKET_LOG)
    {
      i64_t ibucket = 0;

      double bucket_min = arg_with_bucket->bucket_min;
      double bucket_max =
        arg_with_bucket->bucket_min + arg_with_bucket->bucket_size;

      while (TRUE)
      {
        if ((arg_value >= bucket_min) and (arg_value < bucket_max))
          break;

        ibucket++;

        bucket_min = arg_with_bucket->bucket_min +
                     pow(arg_with_bucket->bucket_size, ibucket);
        bucket_max = arg_with_bucket->bucket_min +
                     pow(arg_with_bucket->bucket_size, ibucket + 1);
      }
      if (ibucket >= arg_with_bucket->nbuckets)
        ibucket = arg_with_bucket->nbuckets - 1;

      arg_with_bucket->buckets[ibucket]++;
    }
    else
      FATAL("unknown bucket_scale", EXIT_FAILURE)
  }
}

void printf_bucket(bucket_t *arg_with_bucket)
{
  PRINTF("bucket_name=%s\n", bdata(arg_with_bucket->bucket_name));
  PRINTF("bucket_size=%.2f\n", arg_with_bucket->bucket_size);
  PRINTF("bucket_min=%.2f\n", arg_with_bucket->bucket_min);
  PRINTF("bucket_max=%.2f\n", arg_with_bucket->bucket_max);
  PRINTF("nbuckets=%lld\n", (i64_t)arg_with_bucket->nbuckets);

  i64_t bucket_total_sum = arg_with_bucket->bucket_nlt_min;

  for (int ibucket = 0; ibucket < arg_with_bucket->nbuckets; ibucket++)
    bucket_total_sum += arg_with_bucket->buckets[ibucket];

  bucket_total_sum += arg_with_bucket->bucket_nge_max;

  PRINTF("bucket_total_sum=%lld\n", bucket_total_sum);

  i64_t bucket_partial_sum = arg_with_bucket->bucket_nlt_min;

#ifndef WEIBULL
  PRINTF("< o-o, %.2f>: %10lld %5.2f %10lld %5.2f\n",
         arg_with_bucket->bucket_min,
         arg_with_bucket->bucket_nlt_min,
         100.0 * arg_with_bucket->bucket_nlt_min / bucket_total_sum,
         bucket_partial_sum,
         100.0 * bucket_partial_sum / bucket_total_sum);
#endif

  int nzero = 0;

  for (int ibucket = 0; ibucket < arg_with_bucket->nbuckets; ibucket++)
  {
    if (arg_with_bucket->buckets[ibucket] == 0)
    {
      ++nzero;

      if (nzero > 1)
      {
        if (nzero == 2)
          PRINTF("[.., ..>\n");
        continue;
      }
    }
    else
    {
      nzero = 0;
    }

    double bucket_min = 0.0;
    double bucket_max = 0.0;

    if (arg_with_bucket->bucket_scale == BUCKET_LINEAR)
    {
      bucket_min =
        arg_with_bucket->bucket_min + ibucket * arg_with_bucket->bucket_size;
      bucket_max = bucket_min + arg_with_bucket->bucket_size;
    }
    else if (arg_with_bucket->bucket_scale == BUCKET_LOG)
    {
      if (ibucket == 0)
      {
        bucket_min = arg_with_bucket->bucket_min;
        bucket_max = arg_with_bucket->bucket_min + arg_with_bucket->bucket_size;
      }
      else
      {
        bucket_min = arg_with_bucket->bucket_min +
                     pow(arg_with_bucket->bucket_size, ibucket);
        bucket_max = arg_with_bucket->bucket_min +
                     pow(arg_with_bucket->bucket_size, ibucket + 1);
      }
    }
    else
      FATAL("unknown bucket_scale", EXIT_FAILURE)

    bucket_partial_sum += arg_with_bucket->buckets[ibucket];

#ifdef WEIBULL
    PRINTF("%.2f %.4f\n",
           bucket_min,
           1.0 * arg_with_bucket->buckets[ibucket] / bucket_total_sum);
#else
    PRINTF("[%.2f, %.2f>: %10lld %5.2f %10lld %5.2f\n",
           bucket_min,
           bucket_max,
           arg_with_bucket->buckets[ibucket],
           100.0 * arg_with_bucket->buckets[ibucket] / bucket_total_sum,
           bucket_partial_sum,
           100.0 * bucket_partial_sum / bucket_total_sum);
#endif
  }

  bucket_partial_sum += arg_with_bucket->bucket_nge_max;

#ifndef WEIBULL
  PRINTF("[%.2f, o-o>: %10lld %5.2f %10lld %5.2f\n",
         arg_with_bucket->bucket_max,
         arg_with_bucket->bucket_nge_max,
         100.0 * arg_with_bucket->bucket_nge_max / bucket_total_sum,
         bucket_partial_sum,
         100.0 * bucket_partial_sum / bucket_total_sum);
#endif
}

void construct_bucket(bucket_t *arg_with_bucket,
                      char *arg_bucket_name,
                      double arg_bucket_size,
                      double arg_bucket_min,
                      double arg_bucket_max,
                      int arg_bucket_scale)
{
  HARDBUG(arg_bucket_size <= 0.0)

  HARDBUG(arg_bucket_min >= arg_bucket_max)

  HARDBUG((arg_with_bucket->bucket_name = bfromcstr(arg_bucket_name)) == NULL)

  arg_with_bucket->bucket_size = arg_bucket_size;
  arg_with_bucket->bucket_min = arg_bucket_min;
  arg_with_bucket->bucket_max = arg_bucket_max;
  arg_with_bucket->bucket_scale = arg_bucket_scale;

  arg_with_bucket->bucket_nlt_min = 0;
  arg_with_bucket->bucket_nge_max = 0;

  if (arg_with_bucket->bucket_scale == BUCKET_LINEAR)
    arg_with_bucket->nbuckets =
      (arg_bucket_max - arg_bucket_min) / arg_bucket_size;
  else if (arg_with_bucket->bucket_scale == BUCKET_LOG)
  {
    arg_with_bucket->nbuckets = 1;

    while ((arg_bucket_min + pow(arg_bucket_size, arg_with_bucket->nbuckets)) <
           arg_bucket_max)
      arg_with_bucket->nbuckets++;
  }
  else
    FATAL("unknown bucket_scale", EXIT_FAILURE)

  MY_MALLOC_BY_TYPE(arg_with_bucket->buckets, i64_t, arg_with_bucket->nbuckets)

  for (int ibucket = 0; ibucket < arg_with_bucket->nbuckets; ibucket++)
    arg_with_bucket->buckets[ibucket] = 0;
}

void my_mpi_bucket_aggregate(bucket_t *arg_with_bucket, MPI_Comm arg_comm)
{
  if (arg_comm == MPI_COMM_NULL)
    return;

  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_bucket->bucket_nlt_min),
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   arg_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &(arg_with_bucket->bucket_nge_max),
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   arg_with_bucket->buckets,
                   arg_with_bucket->nbuckets,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   arg_comm);
}

#define NTEST 1000000

void test_buckets(void)
{
  int ntest = NTEST;

  if (RUNNING_ON_VALGRIND)
    ntest = 1000;

  my_random_t test_random;

  construct_my_random(&test_random, NULL);

  PRINTF("test_bucket_class\n");

  bucket_t a;

  construct_bucket(&a, "a", 0.1, -1.0, 1.0, BUCKET_LINEAR);

  for (int i = 0; i < ntest; i++)
  {
    double value =
      (2.0 * (return_my_random(&test_random) % ntest)) / ntest - 1.0;

    update_bucket(&a, value);
  }

  printf_bucket(&a);

  bucket_t b;

  construct_bucket(&b, "b", 10, 0, ntest, BUCKET_LOG);

  for (int i = 0; i < ntest; i++)
  {
    double value = (int)(return_my_random(&test_random) % (2 * ntest)) - ntest;

    update_bucket(&b, value);
  }

  printf_bucket(&b);
}
