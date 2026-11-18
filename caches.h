// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef CachesH
#define CachesH

enum
{
  CACHE_ENTRY_KEY_TYPE_UI32_T = 1,
  CACHE_ENTRY_KEY_TYPE_I64_T = 2,
  CACHE_ENTRY_KEY_TYPE_UI64_T = 3
};

typedef struct
{
  bstring C_name;

  int C_entry_key_type;

  size_t C_entry_key_size;

  int C_entry_key_nalign;

  size_t C_entry_value_size;

  int C_entry_value_nalign;

  void *C_entry_key_default;

  void *C_entry_values;

  my_int64_t C_nentries;

  my_int64_t C_nhits;

  my_int64_t C_nstored;

  my_int64_t C_nupdates;

  my_int64_t C_noverwrites;
} cache_t;

int check_entry_in_cache(cache_t *arg_with_cache,
                         const void *arg_entry_key,
                         void *arg_entry_value);
int store_entry_in_cache(cache_t *arg_with_cache,
                         const void *arg_entry_key,
                         const void *arg_entry_value);
void clear_cache(cache_t *arg_with_cache);
void construct_cache(cache_t *arg_with_cache,
                     const char *arg_cache_name,
                     my_int64_t arg_cache_size,
                     int arg_entry_key_type,
                     const void *arg_entry_key_default,
                     size_t arg_entry_value_size,
                     const void *arg_entry_value_default);

void test_caches(void);

#endif
