//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#undef ALIGN8

#define CACHE_ENTRY_SIZE(P)\
  (P->C_entry_key_size + P->C_entry_key_nalign +\
   P->C_entry_value_size + P->C_entry_value_nalign)

local void printf_cache(void *self)
{
  cache_t *object = self;

  PRINTF("C_name=%s\n", bdata(object->C_name));

  PRINTF("%s_entry_key_type=%d\n", bdata(object->C_name),
    object->C_entry_key_type);

  PRINTF("%s_entry_key_size=%d\n", bdata(object->C_name),
    (int) object->C_entry_key_size);

  PRINTF("%s_entry_key_nalign=%d\n", bdata(object->C_name),
    object->C_entry_key_nalign);

  PRINTF("%s_entry_value_size=%d\n", bdata(object->C_name),
    (int) object->C_entry_value_size);

  PRINTF("%s_entry_value_nalign=%d\n", bdata(object->C_name),
    object->C_entry_value_nalign);

  PRINTF("%s_nentries=%lld\n", bdata(object->C_name),
    object->C_nentries);

  PRINTF("%s_nhits=%lld\n", bdata(object->C_name),
    object->C_nhits);

  PRINTF("%s_nstored=%lld\n", bdata(object->C_name),
    object->C_nstored);

  PRINTF("%s_nupdates=%lld\n", bdata(object->C_name),
    object->C_nupdates);

  PRINTF("%s_noverwrites=%lld\n", bdata(object->C_name),
    object->C_noverwrites);

  i64_t nunused = 0;

  for (i64_t icache_entry = 0; icache_entry < object->C_nentries;
       icache_entry++)
  {
    void *cache_entry = (i8_t *) object->C_entry_values + 
                        CACHE_ENTRY_SIZE(object) * icache_entry;

    if (memcmp(cache_entry, object->C_entry_key_default,
               object->C_entry_key_size) == 0)

      nunused++;
  }

  PRINTF("%s_nunused=%lld or %.2f percent\n", bdata(object->C_name),
    nunused, nunused * 100.0 / object->C_nentries);
}

int check_entry_in_cache(void *self,
  void *arg_entry_key, void *arg_entry_value)
{
  cache_t *object = self;

  int result = FALSE;

  void *cache_entry = NULL;

  if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  } 
  else
    FATAL("unknown C_entry_key_type", EXIT_FAILURE)

  if (memcmp(cache_entry, arg_entry_key, object->C_entry_key_size) == 0)
  {
    object->C_nhits++;

    if (arg_entry_value != NULL)
    {
      memcpy(arg_entry_value,
             (i8_t *) cache_entry + object->C_entry_key_size,
             object->C_entry_value_size);
    }

    result = TRUE;
  }

  return(result);
}

int store_entry_in_cache(void *self,
  void *arg_entry_key, void *arg_entry_value)
{
  cache_t *object = self;

  int result = FALSE;

  void *cache_entry = NULL;

  if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values + 
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % object->C_nentries;

    cache_entry = (i8_t *) object->C_entry_values + 
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  } 
  else
    FATAL("unknown C_entry_key_type", EXIT_FAILURE)

  object->C_nstored++;

  if (memcmp(cache_entry, arg_entry_key, object->C_entry_key_size) == 0)
  {
    object->C_nupdates++;
  }
  else if (memcmp(cache_entry, object->C_entry_key_default,
                  object->C_entry_key_size) != 0)
  {
    object->C_noverwrites++;
    result = TRUE;
  }

  memcpy(cache_entry, arg_entry_key, object->C_entry_key_size);

  memcpy((i8_t *) cache_entry + object->C_entry_key_size,
         arg_entry_value, object->C_entry_value_size);

  return(result);
}

void clear_cache(void *self)
{
  cache_t *object = self;

  memcpy(object->C_entry_values, object->C_entry_key_default,
         object->C_entry_key_size);

  void *source = object->C_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while(itarget < object->C_nentries)
  {
    void *target = (i8_t *) object->C_entry_values + 
                   CACHE_ENTRY_SIZE(object) * itarget;

    while((itarget + nsource) > object->C_nentries) nsource--;

    memcpy(target, source, object->C_entry_key_size * nsource);

    itarget += nsource;

    nsource *= 2;
  }
}

void construct_cache(void *self, char *arg_cache_name, i64_t arg_cache_size,
  int arg_entry_key_type, void *arg_entry_key_default,
  size_t arg_entry_value_size, void *arg_entry_value_default)
{
  cache_t *object = self;
  
  object->C_name = bfromcstr(arg_cache_name);

  object->C_entry_key_type = arg_entry_key_type;

  if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
    object->C_entry_key_size = sizeof(ui32_t);
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
    object->C_entry_key_size = sizeof(i64_t);
  else if (object->C_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
    object->C_entry_key_size = sizeof(ui64_t);
  else
    FATAL("unknown cache_key_type", EXIT_FAILURE)

  object->C_entry_key_nalign = 0;
#ifdef ALIGN8
  while(((object->C_entry_key_size +
          object->C_entry_key_nalign) % 8) != 0)
    object->C_entry_key_nalign++;
#endif

  //copy the default key value

  MY_MALLOC_BY_TYPE(object->C_entry_key_default, i8_t,
    object->C_entry_key_size)

  memcpy(object->C_entry_key_default, arg_entry_key_default,
    object->C_entry_key_size);

  HARDBUG(arg_entry_value_size < 1)

  object->C_entry_value_size = arg_entry_value_size;

  object->C_entry_value_nalign = 0;
#ifdef ALIGN8
  while(((object->C_entry_value_size +
          object->C_entry_value_nalign) % 8) != 0)
    object->C_entry_value_nalign++;
#endif

  object->C_nentries = first_prime_below(arg_cache_size /
                                           CACHE_ENTRY_SIZE(object));

  HARDBUG(object->C_nentries < 3)

  MY_MALLOC_BY_TYPE(object->C_entry_values, i8_t,
    CACHE_ENTRY_SIZE(object) * object->C_nentries)

  memcpy(object->C_entry_values, object->C_entry_key_default,
         object->C_entry_key_size);

  memcpy((i8_t *) object->C_entry_values + object->C_entry_key_size,
         arg_entry_value_default, object->C_entry_value_size);

  void *source = object->C_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while(itarget < object->C_nentries)
  {
    void *target = (i8_t *) object->C_entry_values + 
                   CACHE_ENTRY_SIZE(object) * itarget;

    while((itarget + nsource) > object->C_nentries) nsource--;

    memcpy(target, source, CACHE_ENTRY_SIZE(object) * nsource);

    itarget += nsource;

    nsource *= 2;
  }

  object->C_nhits = 0;
  object->C_nstored = 0;
  object->C_nupdates = 0;
  object->C_noverwrites = 0;
}

void test_caches(void)
{
  my_random_t test_random;

  construct_my_random(&test_random, 0);

  i64_t d = -1;

  cache_t a;

  construct_cache(&a, "a", MBYTE, CACHE_ENTRY_KEY_TYPE_I64_T,
    &d, sizeof(i64_t), &d);

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

  construct_cache(&b, "b", MBYTE, CACHE_ENTRY_KEY_TYPE_UI64_T,
    &e, sizeof(ui64_t), &e);

  //for ui64_t the key 0 is not allowed

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
