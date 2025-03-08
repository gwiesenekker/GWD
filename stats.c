//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
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

void construct_stats(void *self)
{
  clear_stats(self);
}

void mean_sigma(void *self)
{
  stats_t *object = self;

  object->S_sigma = sqrt(object->S_sigma / object->S_n);

  object->S_mean_sum = object->S_sum / object->S_n;

  object->S_sigma_sum2 = sqrt(object->S_sum2 / object->S_n -
                              object->S_mean_sum * object->S_mean_sum);
}

local void printf_stats(void *self)
{
  stats_t *object = self;

  PRINTF("n=%lld\n", object->S_n);
  PRINTF("min=%.6f\n", object->S_min);
  PRINTF("max=%.6f\n", object->S_max);

  PRINTF("mean=%.6f(%.6f)\n", object->S_mean, object->S_mean_sum);
  PRINTF("sigma=%.6f(%.6f)\n", object->S_sigma, object->S_sigma_sum2);
}

#define N 100000

void test_stats(void)
{
  stats_t a;

  construct_stats(&a);

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
