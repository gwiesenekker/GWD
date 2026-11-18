// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "caches.h"

#include "bstrlib.h"
#include "common.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "my_random.h"
#include "utils.h"

#undef ALIGN8

local inline size_t return_cache_entry_size(const cache_t *arg_with_cache)
{
  return (
    arg_with_cache->C_entry_key_size + arg_with_cache->C_entry_key_nalign +
    arg_with_cache->C_entry_value_size + arg_with_cache->C_entry_value_nalign);
}

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

  my_int64_t nunused = 0;

  for (my_int64_t icache_entry = 0; icache_entry < arg_with_cache->C_nentries;
       icache_entry++)
  {
    void *cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                        return_cache_entry_size(arg_with_cache) * icache_entry;

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
    uint32_t key;

    memcpy(&key, arg_entry_key, sizeof(uint32_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    my_int64_t key;

    memcpy(&key, arg_entry_key, sizeof(my_int64_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    my_uint64_t key;

    memcpy(&key, arg_entry_key, sizeof(my_uint64_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
  }
  else
    FATAL("unknown C_entry_key_type", EXIT_FAILURE)

  if (memcmp(cache_entry, arg_entry_key, arg_with_cache->C_entry_key_size) == 0)
  {
    arg_with_cache->C_nhits++;

    if (arg_entry_value != NULL)
    {
      memcpy(arg_entry_value,
             (int8_t *)cache_entry + arg_with_cache->C_entry_key_size,
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
    uint32_t key;

    memcpy(&key, arg_entry_key, sizeof(uint32_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    my_int64_t key;

    memcpy(&key, arg_entry_key, sizeof(my_int64_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
  }
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    my_uint64_t key;

    memcpy(&key, arg_entry_key, sizeof(my_uint64_t));

    my_int64_t icache_entry = key % arg_with_cache->C_nentries;

    cache_entry = (int8_t *)arg_with_cache->C_entry_values +
                  return_cache_entry_size(arg_with_cache) * icache_entry;
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

  memcpy((int8_t *)cache_entry + arg_with_cache->C_entry_key_size,
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
  my_int64_t itarget = 1;
  my_int64_t nsource = 1;

  while (itarget < arg_with_cache->C_nentries)
  {
    void *target = (int8_t *)arg_with_cache->C_entry_values +
                   return_cache_entry_size(arg_with_cache) * itarget;

    while ((itarget + nsource) > arg_with_cache->C_nentries)
      nsource--;

    memcpy(target, source, arg_with_cache->C_entry_key_size * nsource);

    itarget += nsource;

    nsource *= 2;
  }
}

void construct_cache(cache_t *arg_with_cache,
                     const char *arg_cache_name,
                     my_int64_t arg_cache_size,
                     int arg_entry_key_type,
                     const void *arg_entry_key_default,
                     size_t arg_entry_value_size,
                     const void *arg_entry_value_default)
{
  HARDBUG((arg_with_cache->C_name = bfromcstr(arg_cache_name)) == NULL)

  arg_with_cache->C_entry_key_type = arg_entry_key_type;

  if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
    arg_with_cache->C_entry_key_size = sizeof(uint32_t);
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
    arg_with_cache->C_entry_key_size = sizeof(my_int64_t);
  else if (arg_with_cache->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
    arg_with_cache->C_entry_key_size = sizeof(my_uint64_t);
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
                    int8_t,
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
    first_prime_below(arg_cache_size / return_cache_entry_size(arg_with_cache));

  HARDBUG(arg_with_cache->C_nentries < 3)

  MY_MALLOC_BY_TYPE(arg_with_cache->C_entry_values,
                    int8_t,
                    return_cache_entry_size(arg_with_cache) *
                      arg_with_cache->C_nentries)

  memcpy(arg_with_cache->C_entry_values,
         arg_with_cache->C_entry_key_default,
         arg_with_cache->C_entry_key_size);

  memcpy((int8_t *)arg_with_cache->C_entry_values +
           arg_with_cache->C_entry_key_size,
         arg_entry_value_default,
         arg_with_cache->C_entry_value_size);

  void *source = arg_with_cache->C_entry_values;
  my_int64_t itarget = 1;
  my_int64_t nsource = 1;

  while (itarget < arg_with_cache->C_nentries)
  {
    void *target = (int8_t *)arg_with_cache->C_entry_values +
                   return_cache_entry_size(arg_with_cache) * itarget;

    while ((itarget + nsource) > arg_with_cache->C_nentries)
      nsource--;

    memcpy(target, source, return_cache_entry_size(arg_with_cache) * nsource);

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

  my_int64_t signed_empty = -1;

  cache_t signed_cache;

  construct_cache(&signed_cache,
                  "a",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &signed_empty,
                  sizeof(my_int64_t),
                  &signed_empty);

  for (my_int64_t ientry = 0; ientry < 100000; ientry++)
  {
    my_int64_t missing_value;

    HARDBUG(check_entry_in_cache(&signed_cache, &ientry, &missing_value))

    my_int64_t expected_value = 2 * ientry;

    if (store_entry_in_cache(&signed_cache, &ientry, &expected_value))
    {
      PRINTF("overwrite i=%lld\n", ientry);

      break;
    }

    my_int64_t stored_value;

    HARDBUG(!check_entry_in_cache(&signed_cache, &ientry, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&signed_cache);

  for (my_int64_t ientry = 0; ientry < 100000; ientry++)
  {
    my_int64_t entry_key = return_my_random(&test_random) % 100000000;

    my_int64_t expected_value = 2 * entry_key;

    store_entry_in_cache(&signed_cache, &entry_key, &expected_value);

    my_int64_t stored_value;

    HARDBUG(!check_entry_in_cache(&signed_cache, &entry_key, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&signed_cache);

  my_uint64_t unsigned_empty = 0;

  cache_t unsigned_cache;

  construct_cache(&unsigned_cache,
                  "b",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_UI64_T,
                  &unsigned_empty,
                  sizeof(my_uint64_t),
                  &unsigned_empty);

  // for my_uint64_t the key 0 is not allowed

  for (my_uint64_t ientry = 1; ientry < 100000; ientry++)
  {
    my_uint64_t missing_value;

    HARDBUG(check_entry_in_cache(&unsigned_cache, &ientry, &missing_value))

    my_uint64_t expected_value = 2 * ientry;

    if (store_entry_in_cache(&unsigned_cache, &ientry, &expected_value))
    {
      PRINTF("overwrite i=%lld\n", ientry);

      break;
    }

    my_uint64_t stored_value;

    HARDBUG(!check_entry_in_cache(&unsigned_cache, &ientry, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&unsigned_cache);

  for (my_int64_t ientry = 0; ientry < 100000; ientry++)
  {
    my_uint64_t entry_key = return_my_random(&test_random) % 100000000;

    my_uint64_t expected_value = 2 * entry_key;

    store_entry_in_cache(&unsigned_cache, &entry_key, &expected_value);

    my_uint64_t stored_value;

    HARDBUG(!check_entry_in_cache(&unsigned_cache, &entry_key, &stored_value))

    HARDBUG(stored_value != expected_value)
  }

  printf_cache(&unsigned_cache);
}
