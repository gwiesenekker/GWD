//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

void clear_bucket(void *self)
{
  bucket_t *object = self;

  for (i64_t ibucket = 0; ibucket < object->nbuckets; ibucket++)
   object->buckets[ibucket] = 0;
}

void update_bucket(void *self, double value)
{
  bucket_t *object = self;

  if (value < object->bucket_min)
    object->bucket_nlt_min++;
  else if (value >= object->bucket_max)
    object->bucket_nge_max++;
  else
  {
    if (object->bucket_scale == BUCKET_LINEAR)
    {
      i64_t ibucket = floor((value - object->bucket_min) /
                            object->bucket_size);

      if (ibucket >= object->nbuckets) ibucket = object->nbuckets - 1;
           
      object->buckets[ibucket]++;
    }
    else if (object->bucket_scale == BUCKET_LOG)
    {
      i64_t ibucket = 0;
 
      double bucket_min = object->bucket_min;
      double bucket_max = object->bucket_min + object->bucket_size;

      while(TRUE)
      {
        if ((value >= bucket_min) and (value < bucket_max)) break;

        ibucket++;

        bucket_min = object->bucket_min + pow(object->bucket_size, ibucket);
        bucket_max = object->bucket_min + pow(object->bucket_size, ibucket + 1);
      }
      if (ibucket >= object->nbuckets) ibucket = object->nbuckets - 1;

      object->buckets[ibucket]++;
    }
    else
      FATAL("unknown bucket_scale", EXIT_FAILURE)
  }
}

void printf_bucket(void *self)
{
  bucket_t *object = self;

  PRINTF("bucket_size=%.2f\n", object->bucket_size);
  PRINTF("bucket_min=%.2f\n", object->bucket_min);
  PRINTF("bucket_max=%.2f\n", object->bucket_max);
  PRINTF("nbuckets=%lld\n", (i64_t) object->nbuckets);

  i64_t bucket_total_sum = 0;

  for (int ibucket = 0; ibucket < object->nbuckets; ibucket++)
    bucket_total_sum += object->buckets[ibucket];

  PRINTF("bucket_total_sum=%lld\n", bucket_total_sum);

  i64_t bucket_partial_sum = object->bucket_nlt_min;

  PRINTF("< o-o, %.2f>: %10lld %5.2f %10lld %5.2f\n", object->bucket_min,
   object->bucket_nlt_min, 
   100.0 * object->bucket_nlt_min / bucket_total_sum,
   bucket_partial_sum,
   100.0 * bucket_partial_sum / bucket_total_sum);

  int nzero = 0;

  for (int ibucket = 0; ibucket < object->nbuckets; ibucket++)
  {
    if (object->buckets[ibucket] == 0)
    {
      ++nzero;

      if (nzero > 1)
      {
        if (nzero == 2) PRINTF("[.., ..>\n");
        continue;
      }
    }
    else
    {
      nzero = 0;
    }

    double bucket_min = 0.0;
    double bucket_max = 0.0;

    if (object->bucket_scale == BUCKET_LINEAR)
    {
      bucket_min = object->bucket_min + ibucket * object->bucket_size;
      bucket_max = bucket_min + object->bucket_size;
    }
    else if (object->bucket_scale == BUCKET_LOG)
    {
      if (ibucket == 0)
      {
        bucket_min = object->bucket_min;
        bucket_max = object->bucket_min + object->bucket_size;
      }
      else
      {
        bucket_min = object->bucket_min + pow(object->bucket_size, ibucket);
        bucket_max = object->bucket_min + pow(object->bucket_size, ibucket + 1);
      }
    } 
    else
      FATAL("unknown bucket_scale", EXIT_FAILURE)

    bucket_partial_sum += object->buckets[ibucket];

    PRINTF("[%.2f, %.2f>: %10lld %5.2f %10lld %5.2f\n", bucket_min, bucket_max,
      object->buckets[ibucket],
      100.0 * object->buckets[ibucket] / bucket_total_sum,
      bucket_partial_sum,
      100.0 * bucket_partial_sum / bucket_total_sum);
  }

  bucket_partial_sum += object->bucket_nge_max;

  PRINTF("[%.2f, o-o>: %10lld %5.2f %10lld %5.2f\n", object->bucket_max,
   object->bucket_nge_max, 
   100.0 * object->bucket_nge_max / bucket_total_sum,
   bucket_partial_sum,
   100.0 * bucket_partial_sum / bucket_total_sum);
}

void construct_bucket(void *self, double bucket_size,
  double bucket_min, double bucket_max, int bucket_scale)
{
  bucket_t *object = self;

  HARDBUG(bucket_size <= 0.0)

  HARDBUG(bucket_min >= bucket_max)

  object->bucket_size = bucket_size;
  object->bucket_min = bucket_min;
  object->bucket_max = bucket_max;
  object->bucket_scale = bucket_scale;

  object->bucket_nlt_min = 0;
  object->bucket_nge_max = 0;

  if (object->bucket_scale == BUCKET_LINEAR)
    object->nbuckets = (bucket_max - bucket_min) / bucket_size;
  else if (object->bucket_scale == BUCKET_LOG)
  {
    object->nbuckets = 1;

    while((bucket_min + pow(bucket_size, object->nbuckets)) < bucket_max)
      object->nbuckets++;
  }
  else
    FATAL("unknown bucket_scale", EXIT_FAILURE)

  MY_MALLOC(object->buckets, i64_t, object->nbuckets)

  for (int ibucket = 0; ibucket < object->nbuckets; ibucket++)
   object->buckets[ibucket] = 0;
}

#define NTEST 1000000

void test_buckets(void)
{
  my_random_t test_random;

  construct_my_random(&test_random, 0);

  PRINTF("test_bucket_class\n");

  bucket_t a;

  construct_bucket(&a, 0.1, -1.0, 1.0, BUCKET_LINEAR);

  for (int i = 0; i < NTEST; i++)
  {
    double value = (2.0 * (return_my_random(&test_random) % NTEST)) /
                   NTEST - 1.0;

    update_bucket(&a, value);
  }

  printf_bucket(&a);

  bucket_t b;

  construct_bucket(&b, 10, 0, NTEST, BUCKET_LOG);

  for (int i = 0; i < NTEST; i++)
  {
    double value = (int) (return_my_random(&test_random) % (2 * NTEST)) -
                         NTEST;

    update_bucket(&b, value);
  }

  printf_bucket(&b);
}

