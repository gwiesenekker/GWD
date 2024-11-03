//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#include "globals.h"

/*
uint64_t splitmix64_next(uint64_t* state) {
    uint64_t result = (*state += 0x9e3779b97f4a7c15);
    result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
    result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
    return result ^ (result >> 31);
}
*/

local ui64_t splitmix64(ui64_t *seed)
{
  *seed += 0x9e3779b97f4a7c15ULL;

  ui64_t result = *seed;

  result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9ULL;

  result = (result ^ (result >> 27)) * 0x94d049bb133111ebULL;

  result ^= result >> 31;
  
  return(result);
}

/*
uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}
*/

local ui64_t rotl(ui64_t x, int k)
{
  HARDBUG((k < 0) or (k > 63))

  return (x << k) | (x >> (64 - k));
}

void construct_my_random(void *self, i64_t arg_seed)
{
  my_random_t *object = self;

  ui64_t seed;

  if (arg_seed == INVALID)
  {
#ifdef USE_HARDWARE_RAND
    HARDBUG(!_rdrand64_step(&seed))
#else
#error NOT IMPLEMENTED
#endif
  }
  else
  {
    seed = arg_seed;
  }

  for (int i = 0; i < 4; i++)
    object->MR_state[i] = splitmix64(&seed);
}

/*
uint64_t xoshiro256pp(void) {
    const uint64_t result = rotl(s[0] + s[3], 23) + s[0];
    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}
*/

ui64_t return_my_random(void *self)
{
  my_random_t *object = self;

  ui64_t result = rotl(object->MR_state[0] + object->MR_state[3], 23) +
                  object->MR_state[0];

  ui64_t t = object->MR_state[1] << 17;

  object->MR_state[2] ^= object->MR_state[0];
  object->MR_state[3] ^= object->MR_state[1];
  object->MR_state[1] ^= object->MR_state[2];
  object->MR_state[0] ^= object->MR_state[3];

  object->MR_state[2] ^= t;
  object->MR_state[3] = rotl(object->MR_state[3], 45);

  return result;
}

#define NBITS    16
#define MASK     (~(~0ULL << NBITS))
#define NBUCKETS (MASK + 1)
#define NFILL    100

local i64_t buckets[NBUCKETS];

void test_my_random(void)
{ 
  double mean_min, mean_max;
  double sigma_min, sigma_max;

  for (int ishift = 0; ishift <= (64 - NBITS); ++ishift)
  {
    for (i64_t ibucket = 0; ibucket < NBUCKETS; ibucket++)
      buckets[ibucket] = 0;
  
    my_random_t test_random;
  
    construct_my_random(&test_random, 0);
  
    stats_t stats;
    construct_stats(&stats);
  
    for (i64_t isample = 0; isample < (NBUCKETS * NFILL); isample++)
    {
      int r = (return_my_random(&test_random) >> ishift) & MASK;
  
      buckets[r]++;
  
      update_stats(&stats, r);
    }
  
    i64_t min = buckets[0];
    i64_t max = buckets[0];
  
    for (i64_t ibucket = 1; ibucket < NBUCKETS; ibucket++)
    {
      if (buckets[ibucket] < min) min = buckets[ibucket];
      if (buckets[ibucket] > max) max = buckets[ibucket];
    }
    mean_sigma(&stats);
  
    //PRINTF("ishift=%d\n", ishift);
    //PRINTF("min=%lld max=%lld\n", min, max);
    //PRINTF("mean=%.6f(%.6f)\n", stats.S_mean, stats.S_mean_sum);
    //PRINTF("sigma=%.6f(%.6f)\n", stats.S_sigma, stats.S_sigma_sum2);

    if (ishift == 0)
    {
      mean_min = mean_max = stats.S_mean;
      sigma_min = sigma_max = stats.S_sigma;
    }
    else
    {
       if (stats.S_mean < mean_min) mean_min = stats.S_mean;
       if (stats.S_mean > mean_max) mean_max = stats.S_mean;
       if (stats.S_sigma < sigma_min) sigma_min = stats.S_sigma;
       if (stats.S_sigma > sigma_max) sigma_max = stats.S_sigma;
    }
  }

  PRINTF("exact mean=%.6f mean_min=%.6f mean_max=%0.6f\n",
         MASK / 2.0, mean_min, mean_max);
  PRINTF("exact sigma=%.6f sigma_min=%.6f sigma_max=%0.6f\n",
         MASK / sqrt(12.0), sigma_min, sigma_max);
}

