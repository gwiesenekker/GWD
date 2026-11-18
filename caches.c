// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "globals.h"

#undef ALIGN8

#define CACHE_ENTRY_SIZE(P)                                              \
  (P->C_entry_key_size + P->C_entry_key_nalign + P->C_entry_value_size + \
   P->C_entry_value_nalign)

local void printf_cache(const cache_t *arg_with_cache)
{
  PRINTF("C_name=%s\n", bdata(arg_with_cache->C_name));

  PRINTF("%s_entry_key_type=%d\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_entry_key_type);

  PRINTF("%s_entry_key_size=%d\n",
         bdata(arg_with_cache->C_name),
         (int)arg_with_cache->C_entry_key_size);

  PRINTF("%s_entry_key_nalign=%d\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_entry_key_nalign);

  PRINTF("%s_entry_value_size=%d\n",
         bdata(arg_with_cache->C_name),
         (int)arg_with_cache->C_entry_value_size);

  PRINTF("%s_entry_value_nalign=%d\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_entry_value_nalign);

  PRINTF("%s_nentries=%lld\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_nentries);

  PRINTF("%s_nhits=%lld\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_nhits);

  PRINTF("%s_nstored=%lld\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_nstored);

  PRINTF("%s_nupdates=%lld\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_nupdates);

  PRINTF("%s_noverwrites=%lld\n",
         bdata(arg_with_cache->C_name),
         arg_with_cache->C_noverwrites);

  i64_t nunused = 0;

  for (i64_t icache_entry = 0; icache_entry < arg_with_cache->C_nentries;
       icache_entry++)
  {
    void *cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                        CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;

    if (memcmp(cache_entry,
               arg_with_cache->C_entry_key_default,
               arg_with_cache->C_entry_key_size) == 0)

      nunused++;
  }

  PRINTF("%s_nunused=%lld or %.2f percent\n",
         bdata(arg_with_cache->C_name),
         nunused,
         nunused * 100.0 / arg_with_cache->C_nentries);
}

int check_entry_in_cache(cache_t *arg_with_cache,
                         const void *arg_entry_key,
                         void *arg_entry_value)
{
  int result = FALSE;

  void *cache_entry = NULL;

  if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else
    FATAL("unknown C_entry_key_type", EXIT_FAILURE)

  if (memcmp(cache_entry, arg_entry_key, arg_with_cache->C_entry_key_size) == 0)
  {
    arg_with_cache->C_nhits++;

    if (arg_entry_value != NULL)
    {
      memcpy(arg_entry_value,
             (i8_t *)cache_entry + arg_with_cache->C_entry_key_size,
             arg_with_cache->C_entry_value_size);
    }

    result = TRUE;
  }

  return (result);
}

int store_entry_in_cache(cache_t *arg_with_cache,
                         const void *arg_entry_key,
                         const void *arg_entry_value)
{
  int result = FALSE;

  void *cache_entry = NULL;

  if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (i8_t *)arg_with_cache->C_entry_values +
                  CACHE_ENTRY_SIZE(arg_with_cache) * icache_entry;
  }
  else
    FATAL("unknown C_entry_key_type", EXIT_FAILURE)

  arg_with_cache->C_nstored++;

  if (memcmp(cache_entry, arg_entry_key, arg_with_cache->C_entry_key_size) == 0)
  {
    arg_with_cache->C_nupdates++;
  }
  else if (memcmp(cache_entry,
                  arg_with_cache->C_entry_key_default,
                  arg_with_cache->C_entry_key_size) != 0)
  {
    arg_with_cache->C_noverwrites++;
    result = TRUE;
  }

  memcpy(cache_entry, arg_entry_key, arg_with_cache->C_entry_key_size);

  memcpy((i8_t *)cache_entry + arg_with_cache->C_entry_key_size,
         arg_entry_value,
         arg_with_cache->C_entry_value_size);

  return (result);
}

void clear_cache(cache_t *arg_with_cache)
{
  memcpy(arg_with_cache->C_entry_values,
         arg_with_cache->C_entry_key_default,
         arg_with_cache->C_entry_key_size);

  void *source = arg_with_cache->C_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while (itarget < arg_with_cache->C_nentries)
  {
    void *target = (i8_t *)arg_with_cache->C_entry_values +
                   CACHE_ENTRY_SIZE(arg_with_cache) * itarget;

    while ((itarget + nsource) > arg_with_cache->C_nentries)
      nsource--;

    memcpy(target, source, arg_with_cache->C_entry_key_size * nsource);

    itarget += nsource;

    nsource *= 2;
  }
}

void construct_cache(cache_t *arg_with_cache,
                     const char *arg_cache_name,
                     i64_t arg_cache_size,
                     int arg_entry_key_type,
                     const void *arg_entry_key_default,
                     size_t arg_entry_value_size,
                     const void *arg_entry_value_default)
{
  HARDBUG((arg_with_cache->C_name = bfromcstr(arg_cache_name)) == NULL)

  arg_with_cache->C_entry_key_type = arg_entry_key_type;

  if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
    arg_with_cache->C_entry_key_size = sizeof(ui32_t);
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
    arg_with_cache->C_entry_key_size = sizeof(i64_t);
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
    arg_with_cache->C_entry_key_size = sizeof(ui64_t);
  else
    FATAL("unknown cache_key_type", EXIT_FAILURE)

  arg_with_cache->C_entry_key_nalign = 0;
#ifdef ALIGN8
  while (
    ((arg_with_cache->C_entry_key_size + arg_with_cache->C_entry_key_nalign) %
     8) != 0)
    arg_with_cache->C_entry_key_nalign++;
#endif

  // copy the default key value

  MY_MALLOC_BY_TYPE(arg_with_cache->C_entry_key_default,
                    i8_t,
                    arg_with_cache->C_entry_key_size)

  memcpy(arg_with_cache->C_entry_key_default,
         arg_entry_key_default,
         arg_with_cache->C_entry_key_size);

  HARDBUG(arg_entry_value_size < 1)

  arg_with_cache->C_entry_value_size = arg_entry_value_size;

  arg_with_cache->C_entry_value_nalign = 0;
#ifdef ALIGN8
  while (((arg_with_cache->C_entry_value_size +
           arg_with_cache->C_entry_value_nalign) %
          8) != 0)
    arg_with_cache->C_entry_value_nalign++;
#endif

  arg_with_cache->C_nentries =
    first_prime_below(arg_cache_size / CACHE_ENTRY_SIZE(arg_with_cache));

  HARDBUG(arg_with_cache->C_nentries < 3)

  MY_MALLOC_BY_TYPE(arg_with_cache->C_entry_values,
                    i8_t,
                    CACHE_ENTRY_SIZE(arg_with_cache) *
                      arg_with_cache->C_nentries)

  memcpy(arg_with_cache->C_entry_values,
         arg_with_cache->C_entry_key_default,
         arg_with_cache->C_entry_key_size);

  memcpy((i8_t *)arg_with_cache->C_entry_values +
           arg_with_cache->C_entry_key_size,
         arg_entry_value_default,
         arg_with_cache->C_entry_value_size);

  void *source = arg_with_cache->C_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while (itarget < arg_with_cache->C_nentries)
  {
    void *target = (i8_t *)arg_with_cache->C_entry_values +
                   CACHE_ENTRY_SIZE(arg_with_cache) * itarget;

    while ((itarget + nsource) > arg_with_cache->C_nentries)
      nsource--;

    memcpy(target, source, CACHE_ENTRY_SIZE(arg_with_cache) * nsource);

    itarget += nsource;

    nsource *= 2;
  }

  arg_with_cache->C_nhits = 0;
  arg_with_cache->C_nstored = 0;
  arg_with_cache->C_nupdates = 0;
  arg_with_cache->C_noverwrites = 0;
}

void test_caches(void)
{
  my_random_t test_random;

  construct_my_random(&test_random, NULL);

  i64_t signed_empty = -1;

  cache_t signed_cache;

  construct_cache(&signed_cache,
                  "a",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &signed_empty,
                  sizeof(i64_t),
                  &signed_empty);

  for (i64_t ientry = 0; ientry < 100000; ientry++)
  {
    i64_t missing_value;

    HARDBUG(check_entry_in_cache(&signed_cache, &ientry, &missing_value))

    i64_t expected_value = 2 * ientry;

    if (store_entry_in_cache(&signed_cache, &ientry, &expected_value))
    {
      PRINTF("overwrite i=%lld\n", ientry);

      break;
    }

    i64_t stored_value;

    HARDBUG(!check_entry_in_cache(&signed_cache, &ientry, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&signed_cache);

  for (i64_t ientry = 0; ientry < 100000; ientry++)
  {
    i64_t entry_key = return_my_random(&test_random) % 100000000;

    i64_t expected_value = 2 * entry_key;

    store_entry_in_cache(&signed_cache, &entry_key, &expected_value);

    i64_t stored_value;

    HARDBUG(!check_entry_in_cache(&signed_cache, &entry_key, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&signed_cache);

  ui64_t unsigned_empty = 0;

  cache_t unsigned_cache;

  construct_cache(&unsigned_cache,
                  "b",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_UI64_T,
                  &unsigned_empty,
                  sizeof(ui64_t),
                  &unsigned_empty);

  // for ui64_t the key 0 is not allowed

  for (ui64_t ientry = 1; ientry < 100000; ientry++)
  {
    ui64_t missing_value;

    HARDBUG(check_entry_in_cache(&unsigned_cache, &ientry, &missing_value))

    ui64_t expected_value = 2 * ientry;

    if (store_entry_in_cache(&unsigned_cache, &ientry, &expected_value))
    {
      PRINTF("overwrite i=%lld\n", ientry);

      break;
    }

    ui64_t stored_value;

    HARDBUG(!check_entry_in_cache(&unsigned_cache, &ientry, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&unsigned_cache);

  for (i64_t ientry = 0; ientry < 100000; ientry++)
  {
    ui64_t entry_key = return_my_random(&test_random) % 100000000;

    ui64_t expected_value = 2 * entry_key;

    store_entry_in_cache(&unsigned_cache, &entry_key, &expected_value);

    ui64_t stored_value;

    HARDBUG(!check_entry_in_cache(&unsigned_cache, &entry_key, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&unsigned_cache);
}
