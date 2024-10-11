//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
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

local uint64_t rotl(uint64_t x, int k)
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

  uint64_t t = object->MR_state[1] << 17;

  object->MR_state[2] ^= object->MR_state[0];
  object->MR_state[3] ^= object->MR_state[1];
  object->MR_state[1] ^= object->MR_state[2];
  object->MR_state[0] ^= object->MR_state[3];

  object->MR_state[2] ^= t;
  object->MR_state[3] = rotl(object->MR_state[3], 45);

  return result;
}

#define NBUCKETS 100000ULL
#define NFILL    1000

local i64_t buckets[NBUCKETS];

void test_my_random(void)
{
  for (i64_t ibucket = 0; ibucket < NBUCKETS; ibucket++)
    buckets[ibucket] = 0;

  my_random_t test_random;

  construct_my_random(&test_random, 0);

  for (i64_t isample = 0; isample < (NBUCKETS * NFILL); isample++)
  {
    buckets[return_my_random(&test_random) % NBUCKETS]++;
  }

  i64_t min = buckets[0];
  i64_t max = buckets[0];

  for (i64_t ibucket = 1; ibucket < NBUCKETS; ibucket++)
  {
    if (buckets[ibucket] < min) min = buckets[ibucket];
    if (buckets[ibucket] > max) max = buckets[ibucket];
  }
  PRINTF("min=%lld max=%lld\n", min, max);
}

