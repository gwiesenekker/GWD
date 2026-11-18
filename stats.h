// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef StatsH
#define StatsH

#include "stats.h"

typedef struct
{
  bstring S_name;
  my_int64_t S_nsamples;

  double S_min;
  double S_max;

  double S_sum;
  double S_sum2;

  double S_mean;
  double S_sigma;
} stats_t;

void clear_stats(stats_t *);

inline local void update_stats(stats_t *arg_with_stats, double arg_sample)
{
  arg_with_stats->S_nsamples++;

  if (arg_with_stats->S_nsamples == 1)
  {
    arg_with_stats->S_min = arg_sample;
    arg_with_stats->S_max = arg_sample;
  }
  else
  {
    if (arg_sample > arg_with_stats->S_max)
      arg_with_stats->S_max = arg_sample;

    if (arg_sample < arg_with_stats->S_min)
      arg_with_stats->S_min = arg_sample;
  }

  arg_with_stats->S_sum += arg_sample;
  arg_with_stats->S_sum2 += arg_sample * arg_sample;
}

void construct_stats(stats_t *, const char *);
void mean_sigma(stats_t *);
void printf_stats(const stats_t *);
void my_stats_aggregate(stats_t *, MPI_Comm);

void test_stats();

#endif
