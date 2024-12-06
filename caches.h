//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef CachesH
#define CachesH

#define CACHE_ENTRY_KEY_TYPE_UI32_T  1
#define CACHE_ENTRY_KEY_TYPE_I64_T   2
#define CACHE_ENTRY_KEY_TYPE_UI64_T  3

typedef struct
{
  bstring cache_name;

  int cache_entry_key_type;

  size_t cache_entry_key_size;

  int cache_entry_key_nalign;

  size_t cache_entry_value_size;

  int cache_entry_value_nalign;

  void *cache_entry_key_default;

  void *cache_entry_values;
 
  i64_t cache_nentries;

  i64_t cache_nhits;

  i64_t cache_nstored;

  i64_t cache_nupdates;

  i64_t cache_noverwrites;

  pter_t printf_cache;

  my_mutex_t cache_mutex;
} cache_t;

int check_entry_in_cache(void *, void *, void *);
int store_entry_in_cache(void *, void *, void *);
void clear_cache(void *);
void construct_cache(void *, char *, i64_t, int, void *, size_t, void *);

void test_caches(void);

#endif

