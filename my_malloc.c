// SCU REVISION 8.1262 di 10 nov 2026 15:00:45 CET
#include "globals.h"

#define NPAD (4 * 64)

_Static_assert(NPAD % 64 == 0,
               "allocation prefix must preserve 64-byte alignment");

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

// cannot use something dynamic like bstrings of course

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

typedef struct my_name_stack
{
  my_name_t names[NMY_NAMES_MAX];
  int count;
  my_mutex_t mutex;
  struct my_name_stack *next;
} my_name_stack_t;

/* Process-lifetime registry: never retain pointers to expired TLS storage.
 * Entries also preserve unbalanced stacks from threads which have exited. */
local my_name_stack_t *name_stacks = NULL;
local _Thread_local my_name_stack_t *name_stack = NULL;

local my_mutex_t my_malloc_mutex;

local my_mutex_t my_names_mutex;

#include "xxhash.h"

// Function to hash an array using XXH3_64bits
ui64_t return_XXH3_64(const void *data, size_t len)
{
  // Allocate a state struct

  XXH3_state_t *state;

  HARDBUG((state = XXH3_createState()) == NULL)

  // Reset the state to start hashing

  HARDBUG(XXH3_64bits_reset(state) == XXH_ERROR)

  // Process input in chunks (if needed)

  size_t chunkSize = 4096; // Process in 4KB chunks like the file example

  const char *ptr = (const char *)data;
  size_t processed = 0;

  while (processed < len)
  {
    size_t toProcess =
      (len - processed < chunkSize) ? (len - processed) : chunkSize;

    HARDBUG(XXH3_64bits_update(state, ptr + processed, toProcess) == XXH_ERROR)

    processed += toProcess;
  }

  // Finalize and retrieve the hash

  XXH64_hash_t hash = XXH3_64bits_digest(state);

  // Free the allocated state

  XXH3_freeState(state);

  return (hash);
}

void mark_pointer_read_only(void *arg_pointer)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  HARDBUG(nmy_mallocs <= 0)

  int imalloc;

  for (imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *with_my_malloc = my_mallocs + imalloc;

    if (with_my_malloc->MM_pointer == arg_pointer)
      break;
  }

  HARDBUG(imalloc >= nmy_mallocs)

  my_malloc_t *with_my_malloc = my_mallocs + imalloc;

  with_my_malloc->MM_read_only = TRUE;

  with_my_malloc->MM_crc64 =
    return_XXH3_64(with_my_malloc->MM_pointer, with_my_malloc->MM_size);

  HARDBUG(compat_mutex_unlock(&my_malloc_mutex) != 0)
}

void register_heap(char *arg_name,
                   char *arg_file,
                   const char *arg_func,
                   int arg_line,
                   void *arg_heap,
                   size_t arg_size,
                   int arg_pad,
                   int arg_read_only)
{
  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_lock(&my_malloc_mutex) != 0)

  if (nmy_mallocs >= nmy_mallocs_max)
  {
    nmy_mallocs_max += nmy_mallocs / 2;

    PRINTF("EXPANDING nmy_mallocs=%d nmy_mallocs_max=%d"
           " name=%s file=%s func=%s line=%d size=%zu\n",
           nmy_mallocs,
           nmy_mallocs_max,
           arg_name,
           arg_file,
           arg_func,
           arg_line,
           arg_size);

    my_mallocs = realloc(my_mallocs, nmy_mallocs_max * sizeof(my_malloc_t));

    HARDBUG(my_mallocs == NULL)
  }

  my_malloc_t *with_my_malloc = my_mallocs + nmy_mallocs;

  HARDBUG((with_my_malloc->MM_name = bfromcstr(arg_name)) == NULL)

  HARDBUG((with_my_malloc->MM_file = bfromcstr(arg_file)) == NULL)

  HARDBUG((with_my_malloc->MM_func = bfromcstr(arg_func)) == NULL)

  with_my_malloc->MM_line = arg_line;

  with_my_malloc->MM_size = arg_size;

  with_my_malloc->MM_pad = arg_pad;

  with_my_malloc->MM_heap = arg_heap;

  if (!arg_pad)
  {
    with_my_malloc->MM_pointer = arg_heap;

    with_my_malloc->MM_prefix = with_my_malloc->MM_postfix = NULL;
    with_my_malloc->MM_prefix_crc64 = with_my_malloc->MM_postfix_crc64 = 0;
  }
  else
  {
    with_my_malloc->MM_prefix = arg_heap;

    for (int i = 0; i < NPAD; i++)
      with_my_malloc->MM_prefix[i] = return_my_random(&my_malloc_random) % 256;

    with_my_malloc->MM_prefix_crc64 =
      return_XXH3_64(with_my_malloc->MM_prefix, NPAD);

    with_my_malloc->MM_pointer = (char *)arg_heap + NPAD;

    with_my_malloc->MM_postfix =
      (ui8_t *)((char *)arg_heap + NPAD + with_my_malloc->MM_size);

    for (int i = 0; i < NPAD; i++)
      with_my_malloc->MM_postfix[i] = return_my_random(&my_malloc_random) % 256;

    with_my_malloc->MM_postfix_crc64 =
      return_XXH3_64(with_my_malloc->MM_postfix, NPAD);
  }

  with_my_malloc->MM_read_only = arg_read_only;

  with_my_malloc->MM_crc64 = 0;

  if (arg_read_only)
    with_my_malloc->MM_crc64 =
      return_XXH3_64(with_my_malloc->MM_pointer, with_my_malloc->MM_size);

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
    my_malloc_t *with_my_malloc = my_mallocs + imalloc;

    if (with_my_malloc->MM_pointer != arg_pointer)
      continue;

    result = with_my_malloc->MM_heap;

    if (with_my_malloc->MM_pad)
    {
      if (with_my_malloc->MM_prefix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_prefix, NPAD))
      {
        PRINTF("WARNING: PREFIX BLOCK %s CORRUPT!\n",
               bdata(with_my_malloc->MM_name));

        result = NULL;

        break;
      }
      else
      {
        PRINTF("PREFIX BLOCK %s OK!\n", bdata(with_my_malloc->MM_name));
      }

      if (with_my_malloc->MM_postfix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_postfix, NPAD))
      {
        PRINTF("WARNING: POSTFIX BLOCK %s CORRUPT!\n",
               bdata(with_my_malloc->MM_name));

        result = NULL;

        break;
      }
      else
      {
        PRINTF("POSTFIX BLOCK %s OK!\n", bdata(with_my_malloc->MM_name));
      }
    }

    if (with_my_malloc->MM_read_only)
    {
      if (with_my_malloc->MM_crc64 !=
          return_XXH3_64(with_my_malloc->MM_pointer, with_my_malloc->MM_size))
      {
        PRINTF("WARNING: READONLY BLOCK %s CORRUPT!\n",
               bdata(with_my_malloc->MM_name));

        result = NULL;

        break;
      }
      else
      {
        PRINTF("READONLY BLOCK %s(%llX) OK!\n",
               bdata(with_my_malloc->MM_name),
               with_my_malloc->MM_crc64);
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

  return (result);
}

void *my_malloc(char *arg_name,
                char *arg_file,
                const char *arg_func,
                int arg_line,
                size_t arg_size)
{
  void *result;

  HARDBUG(arg_size > SIZE_MAX - 2 * NPAD)

  COMPAT_MALLOC(result, NPAD + arg_size + NPAD)

  if (result == NULL)
  {
    PRINTF("COMPAT_MALLOC ERROR name=%s file=%s func=%s line=%d size=%zu\n",
           arg_name,
           arg_file,
           arg_func,
           arg_line,
           arg_size);

    FATAL("COMPAT_MALLOC ERROR", EXIT_FAILURE)
  }

  register_heap(arg_name,
                arg_file,
                arg_func,
                arg_line,
                result,
                arg_size,
                TRUE,
                FALSE);

  return ((char *)result + NPAD);
}

void my_free(void *arg_pointer)
{
  void *heap;

  HARDBUG((heap = deregister_heap(arg_pointer)) == NULL)

  FREE(heap)
}

local my_name_stack_t *return_name_stack(void)
{
  if (name_stack == NULL)
  {
    /* Do not use the tracked allocator or BSTRING in this diagnostic path. */
    my_name_stack_t *stack = calloc(1, sizeof(*stack));
    HARDBUG(stack == NULL)
    HARDBUG(compat_mutex_init(&stack->mutex) != 0)
    HARDBUG(compat_mutex_lock(&my_names_mutex) != 0)
    stack->next = name_stacks;
    name_stacks = stack;
    HARDBUG(compat_mutex_unlock(&my_names_mutex) != 0)
    name_stack = stack;
  }
  return name_stack;
}

local int print_name_stack(my_name_stack_t *stack)
{
  my_name_t snapshot[NMY_NAMES_MAX];
  HARDBUG(compat_mutex_lock(&stack->mutex) != 0)
  int count = stack->count;
  memcpy(snapshot, stack->names, (size_t)count * sizeof(*snapshot));
  HARDBUG(compat_mutex_unlock(&stack->mutex) != 0)

  /* Never hold a stack lock while printing or invoking fatal diagnostics. */
  for (int i = count - 1; i >= 0; --i)
    PRINTF("iname=%d name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           i,
           snapshot[i].MN_name,
           snapshot[i].MN_file,
           snapshot[i].MN_func,
           snapshot[i].MN_line,
           snapshot[i].MN_pthread_self);
  return count;
}

local int print_all_name_stacks(void)
{
  HARDBUG(compat_mutex_lock(&my_names_mutex) != 0)
  my_name_stack_t *stack = name_stacks;
  HARDBUG(compat_mutex_unlock(&my_names_mutex) != 0)

  /* Links are immutable after publication and entries are never freed. */
  int count = 0;
  for (; stack != NULL; stack = stack->next)
    count += print_name_stack(stack);
  return count;
}

void print_my_names(int arg_all)
{
  if (arg_all)
    (void)print_all_name_stacks();
  else if (name_stack != NULL)
    (void)print_name_stack(name_stack);
}

void push_name(char *arg_name,
               char *arg_file,
               const char *arg_func,
               int arg_line)
{
  my_name_stack_t *stack = return_name_stack();
  HARDBUG(compat_mutex_lock(&stack->mutex) != 0)
  if (stack->count >= NMY_NAMES_MAX)
  {
    HARDBUG(compat_mutex_unlock(&stack->mutex) != 0)
    print_my_names(FALSE);
    FATAL("TOO MANY PUSH_NAMES", EXIT_FAILURE)
  }

  stack->names[stack->count++] =
    (my_name_t){arg_name, arg_file, arg_func, arg_line, compat_pthread_self()};
  HARDBUG(compat_mutex_unlock(&stack->mutex) != 0)
}

void pop_name(char *arg_name,
              char *arg_file,
              const char *arg_func,
              int arg_line)
{
  my_name_stack_t *stack = name_stack;
  if (stack == NULL)
    FATAL("NO PUSHED NAMES", EXIT_FAILURE)

  HARDBUG(compat_mutex_lock(&stack->mutex) != 0)
  int empty = stack->count == 0;
  int mismatch =
    !empty && strcmp(stack->names[stack->count - 1].MN_name, arg_name) != 0;
  if (!empty && !mismatch)
    --stack->count;
  HARDBUG(compat_mutex_unlock(&stack->mutex) != 0)

  if (empty || mismatch)
  {
    PRINTF("POP_NAME name=%s file=%s func=%s line=%d pthread_self=%#lX\n",
           arg_name,
           arg_file,
           arg_func,
           arg_line,
           compat_pthread_self());
    print_my_names(FALSE);
    FATAL("POP_NAME DID NOT FIND CORRESPONDING PUSH_NAME", EXIT_FAILURE)
  }
}

void init_my_malloc(void)
{
  construct_my_random(&my_malloc_random, NULL);

  my_mallocs = malloc(sizeof(my_malloc_t) * nmy_mallocs_max);

  HARDBUG(my_mallocs == NULL)

  HARDBUG(compat_mutex_init(&my_malloc_mutex) != 0)

  HARDBUG(compat_mutex_init(&my_names_mutex) != 0)

  char *f = "The quick brown fox jumps over the lazy dog";
  char s[MY_LINE_MAX];

  ui64_t c = return_XXH3_64(f, strlen(f));

  (void)snprintf(s, MY_LINE_MAX, "%#llX", c);

  // PRINTF("the XXH3_64 of '%s' is %s hex\n", f, s);

  HARDBUG(strcmp(s, "0XCE7D19A5418FB365") != 0)
}

local void heap_sort_i64(i64_t n, i64_t *p, i64_t *a)
{
  i64_t m = n;
  i64_t i, j, k, t;

  if (n < 2)
    return;

  for (i = 0; i < n; i++)
    p[i] = i;

  k = n / 2;
  --n;

  while (TRUE)
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
    while (j <= n)
    {
      if ((j < n) and (a[p[j]] > a[p[j + 1]]))
        j++;
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

  for (i = 0; i < m - 1; i++)
    HARDBUG(a[p[i]] < a[p[i + 1]])
}

local void printf_imalloc(int imalloc)
{
  HARDBUG(imalloc < 0)
  HARDBUG(imalloc >= nmy_mallocs)

  my_malloc_t *with_my_malloc = my_mallocs + imalloc;

  PRINTF("imalloc=%d name=%s file=%s func=%s line=%d size=%lld\n",
         imalloc,
         bdata(with_my_malloc->MM_name),
         bdata(with_my_malloc->MM_file),
         bdata(with_my_malloc->MM_func),
         with_my_malloc->MM_line,
         with_my_malloc->MM_size);
}

void check_my_malloc(void)
{
  int corrupt = FALSE;

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *with_my_malloc = my_mallocs + imalloc;

    if (with_my_malloc->MM_pad)
    {
      if (with_my_malloc->MM_prefix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_prefix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: PREFIX BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }

      if (with_my_malloc->MM_postfix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_postfix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: POSTFIX BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }
    }

    if (with_my_malloc->MM_read_only)
    {
      if (with_my_malloc->MM_crc64 !=
          return_XXH3_64(with_my_malloc->MM_pointer, with_my_malloc->MM_size))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: READONLY BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }
    }
  }

  if (corrupt)
    FATAL("BLOCK(S) CORRUPTED", EXIT_FAILURE)
}

void fin_my_malloc(int arg_verbose)
{
  int corrupt = FALSE;

  i64_t S_total_size = 0;

  // gather sizes

  i64_t *sizes, *psizes;

  COMPAT_MALLOC(sizes, nmy_mallocs * sizeof(i64_t))

  HARDBUG(sizes == NULL)

  COMPAT_MALLOC(psizes, nmy_mallocs * sizeof(i64_t))

  HARDBUG(psizes == NULL)

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *with_my_malloc = my_mallocs + imalloc;

    sizes[imalloc] = with_my_malloc->MM_size;
    psizes[imalloc] = imalloc;
  }

  heap_sort_i64(nmy_mallocs, psizes, sizes);

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *with_my_malloc = my_mallocs + imalloc;

    S_total_size += with_my_malloc->MM_size;

    if (with_my_malloc->MM_pad)
    {
      if (with_my_malloc->MM_prefix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_prefix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: PREFIX BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }

      if (with_my_malloc->MM_postfix_crc64 !=
          return_XXH3_64(with_my_malloc->MM_postfix, NPAD))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: POSTFIX BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }
    }

    if (with_my_malloc->MM_read_only)
    {
      if (with_my_malloc->MM_crc64 !=
          return_XXH3_64(with_my_malloc->MM_pointer, with_my_malloc->MM_size))
      {
        printf_imalloc(imalloc);

        PRINTF("WARNING: READONLY BLOCK CORRUPT!\n");

        corrupt = TRUE;
      }
      else
      {
        if (arg_verbose)
          PRINTF("READONLY BLOCK %s(%llX) OK!\n",
                 bdata(with_my_malloc->MM_name),
                 with_my_malloc->MM_crc64);
      }
    }
  }

  if (corrupt)
    FATAL("BLOCK(S) CORRUPTED", EXIT_FAILURE)

  PRINTF("nmy_mallocs=%d S_total_size=%lld(%lld MB)\n",
         nmy_mallocs,
         S_total_size,
         S_total_size / MBYTE);

  for (int imalloc = 0; imalloc < nmy_mallocs; imalloc++)
  {
    my_malloc_t *with_my_malloc = my_mallocs + psizes[imalloc];

    S_total_size += with_my_malloc->MM_size;

    printf_imalloc(imalloc);

    if (!arg_verbose and (with_my_malloc->MM_size < MBYTE))
      break;
  }

  if (print_all_name_stacks() > 0)
  {
    PRINTF("MEMORY LEAK DETECTED!\n");
  }
}
