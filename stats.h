// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#ifndef StatsH
#define StatsH

#include "stats.h"

typedef struct
{
  bstring S_name;
  i64_t S_n;

  double S_min;
  double S_max;

  double S_mean_welford;
  double S_sigma_welford;

  double S_sum;
  double S_sum2;

  double S_mean;
  double S_sigma;

  double S_mean_sum;
  double S_sigma_sum2;
} stats_t;

void clear_stats(stats_t *);

inline local void update_stats(stats_t *arg_with_stats, double x)
{
  arg_with_stats->S_n++;

  if (arg_with_stats->S_n == 1)
  {
    arg_with_stats->S_min = x;
    arg_with_stats->S_max = x;
  }
  else
  {
    if (x > arg_with_stats->S_max)
      arg_with_stats->S_max = x;
    if (x < arg_with_stats->S_min)
      arg_with_stats->S_min = x;
  }

  double mnm1 = arg_with_stats->S_mean_welford;
  double snm1 = arg_with_stats->S_sigma_welford;

  arg_with_stats->S_mean_welford =
    mnm1 + (x - mnm1) / (double)arg_with_stats->S_n;

  arg_with_stats->S_sigma_welford =
    snm1 + (x - mnm1) * (x - arg_with_stats->S_mean_welford);

  arg_with_stats->S_sum += x;
  arg_with_stats->S_sum2 += x * x;
}

void construct_stats(stats_t *, char *);
void mean_sigma(stats_t *);
void printf_stats(stats_t *);
void my_stats_aggregate(stats_t *, MPI_Comm);

void test_stats();

#endif
