//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

#define NPAD (4 * 64)

#define NMY_NAMES_MAX 1024

typedef struct
{
  bstring MM_name;
  bstring MM_file;
  bstring MM_func;
  int MM_line;
  i64_t MM_size;

  void *MM_heap;
  void *MM_pointer;

  int MM_pad;

  ui8_t *MM_prefix;
  ui64_t MM_prefix_crc64;

  ui8_t *MM_postfix;
  ui64_t MM_postfix_crc64;

  int MM_read_only;
  ui64_t MM_crc64;
} my_malloc_t;

//cannot use something dynamic like bstrings of course

typedef struct
{
  char *MN_name;
  char *MN_file;
  const char *MN_func;
  int MN_line;
  unsigned long MN_pthread_self;
} my_name_t;

local my_random_t my_malloc_random;
local int nmy_mallocs_max = 128;
local int nmy_mallocs = 0;
local my_malloc_t *my_mallocs = NULL;

local int nmy_names = 0;
local my_name_t *my_names = NULL;

local my_mutex_t my_malloc_mutex;

local my_mutex_t my_names_mutex;

#include "xxhash.h"

// Function to hash an array using XXH3_64bits
ui64_t return_XXH3_64(const void *data, size_t len) {
    // Allocate a state struct
    XXH3_state_t* state = XXH3_createState();
    if (state == NULL) {
        fprintf(stderr, "XXH3_createState failed: Out of memory\n");
        return 0;
    }

    // Reset the state to start hashing
    if (XXH3_64bits_reset(state) == XXH_ERROR) {
        fprintf(stderr, "XXH3_64bits_reset failed\n");
        XXH3_freeState(state);
        return 0;
    }

    // Process input in chunks (if needed)
    size_t chunkSize = 4096; // Process in 4KB chunks like the file example
    const char *ptr = (const char*)data;
    size_t processed = 0;

    while (processed < len) {
        size_t toProcess = (len - processed < chunkSize) ? (len - processed) : chunkSize;
        if (XXH3_64bits_update(state, ptr + processed, toProcess) == XXH_ERROR) {
            fprintf(stderr, "XXH3_64bits_update failed\n");
            XXH3_freeState(state);
            return 0;
        }
        processed += toProcess;
    }

    // Finalize and retrieve the hash
    XXH64_hash_t hash = XXH3_64bits_digest(state);

    // Free the allocated state
    XXH3_freeState(state);

    return hash;
}


void mark_pointer_read_only(void *arg_pointer)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  HARDBUG(nmy_mallocs <= 0)

  int imalloc;

  for (imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + imalloc;

    if (object->MM_pointer == arg_pointer) break;
  }

  HARDBUG(imalloc >= nmy_mallocs)

  my_malloc_t *object = my_mallocs + imalloc;

  object->MM_read_only = TRUE;

  object->MM_crc64 =
    return_XXH3_64(object->MM_pointer, object->MM_size);

  HARDBUG(compat_mutex_unlock(&my_malloc_mutex) != 0)
}

void register_heap(char *arg_name,
  char *arg_file, const char *arg_func, int arg_line,
  void *arg_heap, size_t arg_size, int arg_pad, int arg_read_only)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  if (nmy_mallocs >= nmy_mallocs_max)
  {
    nmy_mallocs_max += nmy_mallocs / 2;

    PRINTF("EXPANDING nmy_mallocs=%d nmy_mallocs_max=%d"
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

  object->MM_heap = arg_heap;

  if (!arg_pad)
  {
    object->MM_pointer = arg_heap;

    object->MM_prefix = object->MM_postfix = NULL;
    object->MM_prefix_crc64 = object->MM_postfix_crc64 = 0;
  }
  else
  {
    object->MM_prefix = arg_heap;

    for (int i = 0; i < NPAD; i++)
      object->MM_prefix[i] = return_my_random(&my_malloc_random) % 256;

    object->MM_prefix_crc64 = return_XXH3_64(object->MM_prefix, NPAD);

    object->MM_pointer = (char *) arg_heap + NPAD;

    object->MM_postfix = (ui8_t *) ((char *) arg_heap + NPAD + object->MM_size);

    for (int i = 0; i < NPAD; i++)
      object->MM_postfix[i] = return_my_random(&my_malloc_random) % 256;

    object->MM_postfix_crc64 = return_XXH3_64(object->MM_postfix, NPAD);
  }

  object->MM_read_only = arg_read_only;

  object->MM_crc64 = 0;

  if (arg_read_only)
    object->MM_crc64 = return_XXH3_64(object->MM_pointer, object->MM_size);

  nmy_mallocs++;

  HARDBUG(compat_mutex_unlock(&my_malloc_mutex) != 0)
}

void *deregister_heap(void *arg_pointer)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  HARDBUG(nmy_mallocs <= 0)

  void *result = NULL;

  int imalloc;

  for (imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + imalloc;

    if (object->MM_pointer != arg_pointer) continue;

    result = object->MM_heap;

    if (object->MM_pad)
    {
      if (object->MM_prefix_crc64 !=
          return_XXH3_64(object->MM_prefix, NPAD))
      {
        PRINTF("WARNING: PREFIX BLOCK %s CORRUPT!\n", bdata(object->MM_name));

        result = NULL;

        break;
      }
      else
      {
        PRINTF("PREFIX BLOCK %s OK!\n", bdata(object->MM_name));
      }

      if (object->MM_postfix_crc64 !=
          return_XXH3_64(object->MM_postfix, NPAD))
      {
        PRINTF("WARNING: POSTFIX BLOCK %s CORRUPT!\n", bdata(object->MM_name));
 
        result = NULL;

        break;
      }
      else
      {
        PRINTF("POSTFIX BLOCK %s OK!\n", bdata(object->MM_name));
      }
    }
   
    if (object->MM_read_only)
    {
      if (object->MM_crc64 !=
          return_XXH3_64(object->MM_pointer, object->MM_size))
      {
        PRINTF("WARNING: READONLY BLOCK %s CORRUPT!\n",
               bdata(object->MM_name));

        result = NULL;

        break;
      }
      else
      {
        PRINTF("READONLY BLOCK %s(%llX) OK!\n",
               bdata(object->MM_name), object->MM_crc64);
      }
    }


    break;
  }

  HARDBUG(imalloc >= nmy_mallocs)

  --nmy_mallocs;

  HARDBUG(nmy_mallocs < 0)

  if (imalloc != nmy_mallocs)
    my_mallocs[imalloc] = my_mallocs[nmy_mallocs];

  HARDBUG(compat_mutex_unlock(&my_malloc_mutex) != 0)

  return(result);
}

void *my_malloc(char *arg_name, char *arg_file, const char *arg_func,
  int arg_line, size_t arg_size)
{
  void *result;

  COMPAT_MALLOC(result, NPAD + arg_size + NPAD)

  if (result == NULL)
  {
    PRINTF("COMPAT_MALLOC ERROR name=%s file=%s func=%s line=%d size=%zu\n",
           arg_name, arg_file, arg_func, arg_line, arg_size);

    FATAL("COMPAT_MALLOC ERROR", EXIT_FAILURE)
  }

  register_heap(arg_name, arg_file, arg_func, arg_line,
    result, arg_size, TRUE, FALSE);

  return((char *) result + NPAD);
}

void my_free(void *arg_pointer)
{
  void *heap;

  HARDBUG((heap = deregister_heap(arg_pointer)) == NULL)
 
  FREE(heap)
}

void print_my_names(int arg_all)
{
  for (int iname = nmy_names - 1; iname >= 0; --iname)
  {
    if (arg_all or (my_names[iname].MN_pthread_self == compat_pthread_self()))
    {
      PRINTF("iname=%d name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
        iname,
        my_names[iname].MN_name,
        my_names[iname].MN_file,
        my_names[iname].MN_func,
        my_names[iname].MN_line,
        my_names[iname].MN_pthread_self);
    }
  }
}

void push_name(char *arg_name, char *arg_file, const char *arg_func,
  int arg_line)
{
  HARDBUG(compat_mutex_lock(&my_names_mutex) != 0)

  if (nmy_names >= NMY_NAMES_MAX)
  {
    PRINTF("PUSH_NAME name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           arg_name, arg_file, arg_func, arg_line, compat_pthread_self());

    print_my_names(FALSE);

    FATAL("TOO MANY PUSH_NAMES", EXIT_FAILURE)
  }
  
  my_names[nmy_names].MN_name = arg_name;
  my_names[nmy_names].MN_file = arg_file;
  my_names[nmy_names].MN_func = arg_func;
  my_names[nmy_names].MN_line = arg_line;
  my_names[nmy_names].MN_pthread_self = compat_pthread_self();

  nmy_names++;

  HARDBUG(compat_mutex_unlock(&my_names_mutex) != 0)
}

void pop_name(char *arg_name, char *arg_file, const char *arg_func,
  int arg_line)
{
  HARDBUG(compat_mutex_lock(&my_names_mutex) != 0)
  
  if (nmy_names == 0)
  {
    PRINTF("POP_NAME name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           arg_name, arg_file, arg_func, arg_line, compat_pthread_self());

    FATAL("NO PUSHED LEAKS", EXIT_FAILURE)
  }

  int iname;

  for (iname = nmy_names - 1; iname >= 0; --iname)
    if (my_names[iname].MN_pthread_self == compat_pthread_self()) break;

  if (iname < 0)
  {
    PRINTF("POP_NAME name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           arg_name, arg_file, arg_func, arg_line, compat_pthread_self());

    print_my_names(FALSE);

    FATAL("POP_NAME DID NOT FIND ANY PUSH_NAME", EXIT_FAILURE)
  }

  if (strcmp(my_names[iname].MN_name, arg_name) != 0)
  {
    PRINTF("POP_NAME name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           arg_name, arg_file, arg_func, arg_line, compat_pthread_self());

    print_my_names(FALSE);

    FATAL("POP_NAME DID NOT FIND CORRESPONDING PUSH_NAME", EXIT_FAILURE)
  }

  for (int jname = iname; jname < nmy_names; jname++)
    my_names[jname] = my_names[jname + 1];

  nmy_names--;

  HARDBUG(compat_mutex_unlock(&my_names_mutex) != 0)
}

void init_my_malloc(void)
{
  construct_my_random(&my_malloc_random, 0);

  my_mallocs = malloc(sizeof(my_malloc_t) * nmy_mallocs_max);

  HARDBUG(my_mallocs == NULL)
  
  my_names = malloc(sizeof(my_name_t) * NMY_NAMES_MAX);

  HARDBUG(my_names == NULL)

  HARDBUG(compat_mutex_init(&my_malloc_mutex) != 0)

  HARDBUG(compat_mutex_init(&my_names_mutex) != 0)

  char *f = "The quick brown fox jumps over the lazy dog";
  char s[MY_LINE_MAX];

  ui64_t c = return_XXH3_64(f, strlen(f));

  (void) snprintf(s, MY_LINE_MAX, "%#llX", c);

  //PRINTF("the XXH3_64 of '%s' is %s hex\n", f, s);

  HARDBUG(strcmp(s, "0XCE7D19A5418FB365") != 0)
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

local void printf_imalloc(int imalloc)
{
  HARDBUG(imalloc < 0)
  HARDBUG(imalloc >= nmy_mallocs)

  my_malloc_t *object = my_mallocs + imalloc;

  PRINTF("imalloc=%d name=%s file=%s func=%s line=%d size=%lld\n",
         imalloc,
         bdata(object->MM_name),
         bdata(object->MM_file),
         bdata(object->MM_func),
         object->MM_line,
         object->MM_size);
}

void check_my_malloc(void)
{
  int corrupt = FALSE;

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + imalloc;

    if (object->MM_pad)
    {
      if (object->MM_prefix_crc64 !=
          return_XXH3_64(object->MM_prefix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: PREFIX BLOCK CORRUPT!\n");
 
        corrupt = TRUE;
      }

      if (object->MM_postfix_crc64 !=
          return_XXH3_64(object->MM_postfix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: POSTFIX BLOCK CORRUPT!\n");
 
        corrupt = TRUE;
      }
    }

    if (object->MM_read_only)
    {
      if (object->MM_crc64 !=
          return_XXH3_64(object->MM_pointer, object->MM_size))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: READONLY BLOCK CORRUPT!\n");
  
        corrupt = TRUE;
      }
    }
  }

  if (corrupt) FATAL("BLOCK(S) CORRUPTED", EXIT_FAILURE)
}

void fin_my_malloc(int arg_verbose)
{
  int corrupt = FALSE;

  i64_t S_total_size = 0;

  //gather sizes

  i64_t *sizes, *psizes;

  COMPAT_MALLOC(sizes, nmy_mallocs * sizeof(i64_t))
  
  HARDBUG(sizes == NULL)

  COMPAT_MALLOC(psizes, nmy_mallocs * sizeof(i64_t))

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

    S_total_size += object->MM_size;

    if (object->MM_pad)
    {
      if (object->MM_prefix_crc64 !=
          return_XXH3_64(object->MM_prefix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: PREFIX BLOCK CORRUPT!\n");
 
        corrupt = TRUE;
      }

      if (object->MM_postfix_crc64 !=
          return_XXH3_64(object->MM_postfix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: POSTFIX BLOCK CORRUPT!\n");
 
        corrupt = TRUE;
      }
    }

    if (object->MM_read_only)
    {
      if (object->MM_crc64 !=
          return_XXH3_64(object->MM_pointer, object->MM_size))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: READONLY BLOCK CORRUPT!\n");
  
        corrupt = TRUE;
      }
      else
      {
        if (arg_verbose)
          PRINTF("READONLY BLOCK %s(%llX) OK!\n", bdata(object->MM_name), object->MM_crc64);
      }
    }
  }

  if (corrupt) FATAL("BLOCK(S) CORRUPTED", EXIT_FAILURE)

  PRINTF("nmy_mallocs=%d S_total_size=%lld(%lld MB)\n",
    nmy_mallocs, S_total_size, S_total_size / MBYTE);

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *object = my_mallocs + psizes[imalloc];

    S_total_size += object->MM_size;

    printf_imalloc(imalloc);

    if (!arg_verbose and (object->MM_size < MBYTE)) break;
  }

  if (nmy_names > 0)
  {
    print_my_names(TRUE);

    PRINTF("MEMORY LEAK DETECTED!\n");
  }
}

