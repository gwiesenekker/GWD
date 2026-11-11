// SCU REVISION 8.1265 wo 11 nov 2026  2:19:30 CET
#include "globals.h"

#undef ALIGN8

#define CACHE_ENTRY_SIZE(P)                                              \
  (P->C_entry_key_size + P->C_entry_key_nalign + P->C_entry_value_size + \
   P->C_entry_value_nalign)

local void printf_cache(cache_t *arg_with_cache)
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
                         void *arg_entry_key,
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
                         void *arg_entry_key,
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
                     char *arg_cache_name,
                     i64_t arg_cache_size,
                     int arg_entry_key_type,
                     void *arg_entry_key_default,
                     size_t arg_entry_value_size,
                     void *arg_entry_value_default)
{
  arg_with_cache->C_name = bfromcstr(arg_cache_name);

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

  i64_t d = -1;

  cache_t a;

  construct_cache(&a,
                  "a",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &d,
                  sizeof(i64_t),
                  &d);

  for (i64_t i = 0; i < 100000; i++)
  {
    i64_t j;

    HARDBUG(check_entry_in_cache(&a, &i, &j))

    i64_t v = 2 * i;

    if (store_entry_in_cache(&a, &i, &v))
    {
      PRINTF("overwrite i=%lld\n", i);

      break;
    }

    i64_t w;

    HARDBUG(!check_entry_in_cache(&a, &i, &w))

    HARDBUG(w != v)
  }

  printf_cache(&a);

  for (i64_t i = 0; i < 100000; i++)
  {
    i64_t j = return_my_random(&test_random) % 100000000;

    i64_t v = 2 * j;

    store_entry_in_cache(&a, &j, &v);

    i64_t w;

    HARDBUG(!check_entry_in_cache(&a, &j, &w))

    HARDBUG(w != v)
  }

  printf_cache(&a);

  ui64_t e = 0;

  cache_t b;

  construct_cache(&b,
                  "b",
                  MBYTE,
                  CACHE_ENTRY_KEY_TYPE_UI64_T,
                  &e,
                  sizeof(ui64_t),
                  &e);

  // for ui64_t the key 0 is not allowed

  for (ui64_t i = 1; i < 100000; i++)
  {
    ui64_t j;

    HARDBUG(check_entry_in_cache(&b, &i, &j))

    ui64_t v = 2 * i;

    if (store_entry_in_cache(&b, &i, &v))
    {
      PRINTF("overwrite i=%lld\n", i);

      break;
    }

    ui64_t w;

    HARDBUG(!check_entry_in_cache(&b, &i, &w))

    HARDBUG(w != v)
  }

  printf_cache(&b);

  for (i64_t i = 0; i < 100000; i++)
  {
    ui64_t j = return_my_random(&test_random) % 100000000;

    ui64_t v = 2 * j;

    store_entry_in_cache(&b, &j, &v);

    ui64_t w;

    HARDBUG(!check_entry_in_cache(&b, &j, &w))

    HARDBUG(w != v)
  }

  printf_cache(&b);
}
