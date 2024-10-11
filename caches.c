//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#undef ALIGN8

#define CACHE_ENTRY_SIZE(P)\
  (P->cache_entry_key_size + P->cache_entry_key_nalign +\
   P->cache_entry_value_size + P->cache_entry_value_nalign)

local void printf_cache(void *self)
{
  cache_t *object = self;

  PRINTF("cache_name=%s\n", bdata(object->cache_name));

  PRINTF("%s_entry_key_type=%d\n", object->cache_name,
    object->cache_entry_key_type);

  PRINTF("%s_entry_key_size=%d\n", object->cache_name,
    (int) object->cache_entry_key_size);

  PRINTF("%s_entry_key_nalign=%d\n", object->cache_name,
    object->cache_entry_key_nalign);

  PRINTF("%s_entry_value_size=%d\n", object->cache_name,
    (int) object->cache_entry_value_size);

  PRINTF("%s_entry_value_nalign=%d\n", object->cache_name,
    object->cache_entry_value_nalign);

  PRINTF("%s_nentries=%lld\n", object->cache_name,
    object->cache_nentries);

  PRINTF("%s_nhits=%lld\n", object->cache_name,
    object->cache_nhits);

  PRINTF("%s_nstored=%lld\n", object->cache_name,
    object->cache_nstored);

  PRINTF("%s_nupdates=%lld\n", object->cache_name,
    object->cache_nupdates);

  PRINTF("%s_noverwrites=%lld\n", object->cache_name,
    object->cache_noverwrites);

  i64_t nunused = 0;

  for (i64_t icache_entry = 0; icache_entry < object->cache_nentries;
       icache_entry++)
  {
    void *cache_entry = (i8_t *) object->cache_entry_values + 
                        CACHE_ENTRY_SIZE(object) * icache_entry;

    if (memcmp(cache_entry, object->cache_entry_key_default,
               object->cache_entry_key_size) == 0)

      nunused++;
  }

  PRINTF("%s_nunused=%lld or %.2f percent\n", object->cache_name,
    nunused, nunused * 100.0 / object->cache_nentries);
}

int check_entry_in_cache(void *self,
  void *arg_entry_key, void *arg_entry_value)
{
  cache_t *object = self;

  int result = FALSE;

  void *cache_entry = NULL;

  if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  } 
  else
    FATAL("unknown cache_entry_key_type", EXIT_FAILURE)

  if (memcmp(cache_entry, arg_entry_key, object->cache_entry_key_size) == 0)
  {
    object->cache_nhits++;

    if (arg_entry_value != NULL)
    {
      memcpy(arg_entry_value,
             (i8_t *) cache_entry + object->cache_entry_key_size,
             object->cache_entry_value_size);
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

  if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
  {
    ui32_t key;

    memcpy(&key, arg_entry_key, sizeof(ui32_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values +
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
  {
    i64_t key;

    memcpy(&key, arg_entry_key, sizeof(i64_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values + 
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  }
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
  {
    ui64_t key;

    memcpy(&key, arg_entry_key, sizeof(ui64_t));

    i64_t icache_entry = key % object->cache_nentries;

    cache_entry = (i8_t *) object->cache_entry_values + 
                  CACHE_ENTRY_SIZE(object) * icache_entry;
  } 
  else
    FATAL("unknown cache_entry_key_type", EXIT_FAILURE)

  object->cache_nstored++;

  if (memcmp(cache_entry, arg_entry_key, object->cache_entry_key_size) == 0)
  {
    object->cache_nupdates++;
  }
  else if (memcmp(cache_entry, object->cache_entry_key_default,
                  object->cache_entry_key_size) != 0)
  {
    object->cache_noverwrites++;
    result = TRUE;
  }

  memcpy(cache_entry, arg_entry_key, object->cache_entry_key_size);

  memcpy((i8_t *) cache_entry + object->cache_entry_key_size,
         arg_entry_value, object->cache_entry_value_size);

  return(result);
}

void clear_cache(void *self)
{
  cache_t *object = self;

  memcpy(object->cache_entry_values, object->cache_entry_key_default,
         object->cache_entry_key_size);

  void *source = object->cache_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while(itarget < object->cache_nentries)
  {
    void *target = (i8_t *) object->cache_entry_values + 
                   CACHE_ENTRY_SIZE(object) * itarget;

    while((itarget + nsource) > object->cache_nentries) nsource--;

    memcpy(target, source, object->cache_entry_key_size * nsource);

    itarget += nsource;

    nsource *= 2;
  }
}

void construct_cache(void *self, char *arg_cache_name, i64_t arg_cache_size,
  int arg_entry_key_type, void *arg_entry_key_default,
  size_t arg_entry_value_size, void *arg_entry_value_default)
{
  cache_t *object = self;
  
  object->cache_name = bfromcstr(arg_cache_name);

  object->cache_entry_key_type = arg_entry_key_type;

  if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI32_T)
    object->cache_entry_key_size = sizeof(ui32_t);
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_I64_T)
    object->cache_entry_key_size = sizeof(i64_t);
  else if (object->cache_entry_key_type == CACHE_ENTRY_KEY_TYPE_UI64_T)
    object->cache_entry_key_size = sizeof(ui64_t);
  else
    FATAL("unknown cache_key_type", EXIT_FAILURE)

  object->cache_entry_key_nalign = 0;
#ifdef ALIGN8
  while(((object->cache_entry_key_size +
          object->cache_entry_key_nalign) % 8) != 0)
    object->cache_entry_key_nalign++;
#endif

  //copy the default key value

  MALLOC(object->cache_entry_key_default, i8_t, object->cache_entry_key_size)

  memcpy(object->cache_entry_key_default, arg_entry_key_default,
    object->cache_entry_key_size);

  HARDBUG(arg_entry_value_size < 1)

  object->cache_entry_value_size = arg_entry_value_size;

  object->cache_entry_value_nalign = 0;
#ifdef ALIGN8
  while(((object->cache_entry_value_size +
          object->cache_entry_value_nalign) % 8) != 0)
    object->cache_entry_value_nalign++;
#endif

  object->cache_nentries = first_prime_below(arg_cache_size /
                                           CACHE_ENTRY_SIZE(object));

  HARDBUG(object->cache_nentries < 3)

  MALLOC(object->cache_entry_values, i8_t,
    CACHE_ENTRY_SIZE(object) * object->cache_nentries)

  memcpy(object->cache_entry_values, object->cache_entry_key_default,
         object->cache_entry_key_size);

  memcpy((i8_t *) object->cache_entry_values + object->cache_entry_key_size,
         arg_entry_value_default, object->cache_entry_value_size);

  void *source = object->cache_entry_values;
  i64_t itarget = 1;
  i64_t nsource = 1;

  while(itarget < object->cache_nentries)
  {
    void *target = (i8_t *) object->cache_entry_values + 
                   CACHE_ENTRY_SIZE(object) * itarget;

    while((itarget + nsource) > object->cache_nentries) nsource--;

    memcpy(target, source, CACHE_ENTRY_SIZE(object) * nsource);

    itarget += nsource;

    nsource *= 2;
  }

  object->cache_nhits = 0;
  object->cache_nstored = 0;
  object->cache_nupdates = 0;
  object->cache_noverwrites = 0;
}

void test_caches(void)
{
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
    i64_t j = randull(0) % 100000000;

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
    ui64_t j = randull(0) % 100000000;

    ui64_t v = 2 * j;

    store_entry_in_cache(&b, &j, &v);

    ui64_t w;

    HARDBUG(!check_entry_in_cache(&b, &j, &w))

    HARDBUG(w != v)
  }

  printf_cache(&b);
}
