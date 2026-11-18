// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef MyMallocH
#define MyMallocH

#include "common.h"
#include "bstrlib.h"

#define MARK_ARRAY_READ_ONLY(P, S) \
  register_heap(#P, __FILE__, __FUNC__, __LINE__, P, S, FALSE, TRUE);

#define MY_MALLOC_VOID(P, N) \
  HARDBUG(((P) = my_malloc(#P, __FILE__, __FUNC__, __LINE__, N)) == NULL)

#define MY_MALLOC_BY_TYPE(P, T, N)                                           \
  HARDBUG(                                                                   \
    ((P) =                                                                   \
       (T *)my_malloc(#P, __FILE__, __FUNC__, __LINE__, sizeof(T) * (N))) == \
    NULL)

#ifdef DEBUG

#define PUSH_NAME(P) push_name(#P, __FILE__, __FUNC__, __LINE__);

#define POP_NAME(P) pop_name(#P, __FILE__, __FUNC__, __LINE__);

#else

#define PUSH_NAME(P)

#define POP_NAME(P)

#endif

#define BSTRING(B)                                     \
  bstring B = NULL;                                    \
  if (B == NULL)                                       \
  {                                                    \
    PUSH_NAME(#B) HARDBUG((B = bfromcstr("")) == NULL) \
  }

#define BDESTROY(B) \
  HARDBUG((B) == NULL) POP_NAME(#B) HARDBUG(bdestroy(B) == BSTR_ERR)(B) = NULL;

#define CSTRING(C, N)                                  \
  char *C = NULL;                                      \
  if (C == NULL)                                       \
  {                                                    \
    PUSH_NAME(#C) HARDBUG((C = malloc(N + 1)) == NULL) \
  }

#define CDESTROY(C)     \
  POP_NAME(#C) free(C); \
  C = NULL;

my_uint64_t return_XXH3_64(const void *arg_data, size_t arg_nbytes);

void mark_pointer_read_only(void *arg_pointer);
void register_heap(const char *arg_name,
                   const char *arg_file,
                   const char *arg_func,
                   int arg_line,
                   void *arg_heap,
                   size_t arg_size,
                   int arg_pad,
                   int arg_read_only);
void *deregister_heap(void *arg_pointer);

void *my_malloc(const char *arg_name,
                const char *arg_file,
                const char *arg_func,
                int arg_line,
                size_t arg_size);
void my_free(void *arg_pointer);

void *my_large_malloc(const char *arg_name,
                      const char *arg_file,
                      const char *arg_func,
                      int arg_line,
                      size_t arg_size,
                      compat_large_allocation_t *arg_allocation);
void my_large_free(void *arg_pointer,
                   compat_large_allocation_t *arg_allocation);

void print_my_names(int arg_all);
void push_name(const char *arg_name,
               const char *arg_file,
               const char *arg_func,
               int arg_line);
void pop_name(const char *arg_name,
              const char *arg_file,
              const char *arg_func,
              int arg_line);

void init_my_malloc(void);

void check_my_malloc(void);

void fin_my_malloc(int arg_verbose);

#endif
