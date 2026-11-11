// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#ifndef StatsH
#define StatsH

#include "stats.h"

typedef struct
{
  bstring S_name;
  i64_t S_n;

  double S_min;
  double S_max;

  double S_sum;
  double S_sum2;

  double S_mean;
  double S_sigma;
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

  arg_with_stats->S_sum += x;
  arg_with_stats->S_sum2 += x * x;
}

void construct_stats(stats_t *, char *);
void mean_sigma(stats_t *);
void printf_stats(stats_t *);
void my_stats_aggregate(stats_t *, MPI_Comm);

void test_stats();

#endif
