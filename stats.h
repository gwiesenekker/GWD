//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
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

void clear_stats(void *);

inline local void update_stats(void *self, double x)
{
  stats_t *object = self;

  object->S_n++;

  if (object->S_n == 1)
  { 
    object->S_min = x;
    object->S_max = x;
  }
  else
  {
    if (x > object->S_max) object->S_max = x;
    if (x < object->S_min) object->S_min = x;
  }
  
  double mnm1 = object->S_mean_welford;
  double snm1 = object->S_sigma_welford;

  object->S_mean_welford = mnm1 + (x - mnm1) / object->S_n;
   
  object->S_sigma_welford = snm1 + (x - mnm1) * (x - object->S_mean_welford);

  object->S_sum += x;
  object->S_sum2 += x * x ;
}

void construct_stats(void *, char *);
void mean_sigma(void *);
void printf_stats(void *);
void my_stats_aggregate(void *, MPI_Comm);

void test_stats();

#endif
