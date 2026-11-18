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

int check_entry_in_cache(cache_t *, const void *, void *);
int store_entry_in_cache(cache_t *, const void *, const void *);
void clear_cache(cache_t *);
void construct_cache(cache_t *,
                     const char *,
                     my_int64_t,
                     int,
                     const void *,
                     size_t,
                     const void *);

void test_caches(void);

#endif
