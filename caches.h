// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#ifndef CachesH
#define CachesH

#define CACHE_ENTRY_KEY_TYPE_UI32_T 1
#define CACHE_ENTRY_KEY_TYPE_I64_T 2
#define CACHE_ENTRY_KEY_TYPE_UI64_T 3

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

  i64_t C_nentries;

  i64_t C_nhits;

  i64_t C_nstored;

  i64_t C_nupdates;

  i64_t C_noverwrites;
} cache_t;

int check_entry_in_cache(cache_t *, void *, void *);
int store_entry_in_cache(cache_t *, void *, void *);
void clear_cache(cache_t *);
void construct_cache(cache_t *, char *, i64_t, int, void *, size_t, void *);

void test_caches(void);

#endif
