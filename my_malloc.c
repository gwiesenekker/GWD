//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

#define NPAD (4 * 64)

typedef struct
{
  bstring MM_name;
  bstring MM_file;
  bstring MM_func;
  int MM_line;
  i64_t MM_size;

  void *MM_pointer;

  int MM_pad;

  ui8_t *MM_prefix;
  ui64_t MM_prefix_crc64;

  ui8_t *MM_postfix;
  ui64_t MM_postfix_crc64;

  int MM_read_only;
  ui64_t MM_crc64;
} my_malloc_t;

local my_random_t my_malloc_random;
local int nmy_mallocs_max = 128;
local int nmy_mallocs = 0;
local my_malloc_t *my_mallocs = NULL;
local my_mutex_t my_malloc_mutex;

void register_pointer(char *arg_name,
  char *arg_file, const char *arg_func, int arg_line,
  void *arg_pointer, size_t arg_size, int arg_pad, int arg_read_only)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  if (nmy_mallocs >= nmy_mallocs_max)
  {
    nmy_mallocs_max += nmy_mallocs / 2;

    PRINTF("DOUBLING nmy_mallocs=%d nmy_mallocs_max=%d"
           " name=%s file=%s func=%s line=%d size=%zu\n",
           nmy_mallocs, nmy_mallocs_max,
           arg_name, arg_file, arg_func, arg_line, arg_size);

    my_mallocs = realloc(my_mallocs, nmy_mallocs_max * sizeof(my_malloc_t));

    HARDBUG(my_mallocs == NULL)
  }

  my_malloc_t *object = my_mallocs + nmy_mallocs;

  HARDBUG((object->MM_name = bfromcstr(arg_name)) == NULL)

  HARDBUG((object->MM_file = bfromcstr(arg_file)) == NULL)

  HARDBUG((object->MM_func = bfromcstr(arg_func)) == NULL)

  object->MM_line = arg_line;
 
  object->MM_size = arg_size;

  object->MM_pad = arg_pad;

  if (!arg_pad)
  {
    object->MM_pointer = arg_pointer;

    object->MM_prefix = object->MM_postfix = NULL;
    object->MM_prefix_crc64 = object->MM_postfix_crc64 = 0;
  }
  else
  {
    object->MM_prefix = arg_pointer;

    for (int i = 0; i < NPAD; i++)
      object->MM_prefix[i] = return_my_random(&my_malloc_random) % 256;

    object->MM_prefix_crc64 = return_crc64(object->MM_prefix, NPAD, TRUE);

    object->MM_pointer = arg_pointer + NPAD;

    object->MM_postfix = arg_pointer + NPAD + object->MM_size;

    for (int i = 0; i < NPAD; i++)
      object->MM_postfix[i] = return_my_random(&my_malloc_random) % 256;

    object->MM_postfix_crc64 = return_crc64(object->MM_postfix, NPAD, TRUE);
  }

  object->MM_read_only = arg_read_only;

  object->MM_crc64 = 0;

  if (arg_read_only)
    object->MM_crc64 = return_crc64(arg_pointer, arg_size, TRUE);

  nmy_mallocs++;

  HARDBUG(compat_mutex_unlock(&my_malloc_mutex) != 0)
}

void *my_malloc(char *arg_name, char *arg_file, const char *arg_func,
  int arg_line, size_t arg_size)
{
  void *result;

  MALLOC(result, NPAD + arg_size + NPAD)

  if (result == NULL)
  {
    PRINTF("MALLOC ERROR name=%s file=%s func=%s line=%d size=%zu\n",
           arg_name, arg_file, arg_func, arg_line, arg_size);

    FATAL("MALLOC ERROR", EXIT_FAILURE)
  }

  register_pointer(arg_name, arg_file, arg_func, arg_line,
    result, arg_size, TRUE, FALSE);

  return(result + NPAD);
}

void init_my_malloc(void)
{
  construct_my_random(&my_malloc_random, 0);

  my_mallocs = malloc(sizeof(my_malloc_t) * nmy_mallocs_max);

  HARDBUG(my_mallocs == NULL)
  
  HARDBUG(compat_mutex_init(&my_malloc_mutex) != 0)
}

local void heap_sort_i64(i64_t n, i64_t *p, i64_t *a)
{
  i64_t m = n;
  i64_t i, j, k, t;

  if (n < 2) return;

  for (i = 0; i < n; i++) p[i] = i;

  k = n / 2;
  --n;

  while(TRUE)
  {
    if (k > 0)
    {
      t = p[--k];
    }
    else
    {
      t = p[n];
      p[n] = p[0];
      if (--n == 0)
      {
        p[0] = t;
        break;
      }
    }
    i = k;
    j = 2 * k + 1;
    while(j <= n)
    {
      if ((j < n) and (a[p[j]] > a[p[j + 1]])) j++;
      if (a[t] > a[p[j]])
      {  
        p[i] = p[j];
        i = j;
        j = 2 * j + 1;
      }
      else
      {
        j = n + 1;
      }
    }  
    p[i] = t;
  }

  for (i = 0; i < m - 1; i++) HARDBUG(a[p[i]] < a[p[i + 1]])
}

void fin_my_malloc(int verbose)
{
  int corrupt = FALSE;

  i64_t total_size = 0;

  //gather sizes

  i64_t *sizes, *psizes;

  MALLOC(sizes, nmy_mallocs * sizeof(i64_t))
  
  HARDBUG(sizes == NULL)

  MALLOC(psizes, nmy_mallocs * sizeof(i64_t))

  HARDBUG(psizes == NULL)

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + imalloc;

    sizes[imalloc] = object->MM_size;
    psizes[imalloc] = imalloc;
  }

  heap_sort_i64(nmy_mallocs, psizes, sizes);

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + imalloc;

    total_size += object->MM_size;

    if (verbose)
      PRINTF("imalloc=%d name=%s file=%s func=%s line=%d size=%lld\n",
             imalloc,
             bdata(object->MM_name),
             bdata(object->MM_file),
             bdata(object->MM_func),
             object->MM_line,
             object->MM_size);

    if (object->MM_pad)
    {
      if (object->MM_prefix_crc64 !=
          return_crc64(object->MM_prefix, NPAD, TRUE))
      {
        PRINTF("WARNING: PREFIX BLOCK %s CORRUPT!\n", bdata(object->MM_name));
 
        corrupt = TRUE;
      }
      if (object->MM_postfix_crc64 !=
          return_crc64(object->MM_postfix, NPAD, TRUE))
      {
        PRINTF("WARNING: POSTFIX BLOCK %s CORRUPT!\n", bdata(object->MM_name));
 
        corrupt = TRUE;
      }
    }

    if (object->MM_read_only)
    {
      if (object->MM_crc64 !=
          return_crc64(object->MM_pointer, object->MM_size, TRUE))
      {
        PRINTF("WARNING: BLOCK %s CORRUPT!\n", bdata(object->MM_name));
  
        corrupt = TRUE;
      }
      PRINTF("BLOCK %s(%llX) OK!\n", bdata(object->MM_name), object->MM_crc64);
    }
  }
  if (corrupt) FATAL("BLOCK(S) CORRUPTED", EXIT_FAILURE)

  PRINTF("nmy_mallocs=%d total_size=%lld(%lld MB)\n",
    nmy_mallocs, total_size, total_size / MBYTE);

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + psizes[imalloc];

    total_size += object->MM_size;

    PRINTF("imalloc=%d name=%s file=%s func=%s line=%d size=%lld\n",
           imalloc,
           bdata(object->MM_name),
           bdata(object->MM_file),
           bdata(object->MM_func),
           object->MM_line,
           object->MM_size);

    if (!verbose and (object->MM_size < MBYTE)) break;
  }
}

