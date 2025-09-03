//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef MyMallocH
#define MyMallocH

#define NSTACK_MAX 1024

#define MARK_ARRAY_READ_ONLY(P, S) register_heap(#P, __FILE__, __FUNC__, __LINE__, P, S, FALSE, TRUE);

#define MY_MALLOC_VOID(P, N) HARDBUG(((P) = my_malloc(#P, __FILE__, __FUNC__, __LINE__, N)) == NULL)

#define MY_MALLOC_BY_TYPE(P, T, N) HARDBUG(((P) = (T *) my_malloc(#P, __FILE__, __FUNC__, __LINE__, sizeof(T) * (N))) == NULL)

#define MY_FREE_AND_NULL(P) {my_free(P); (P) = NULL;}

#ifdef DEBUG

#define PUSH_NAME(P) push_name(#P, __FILE__, __FUNC__, __LINE__);

#define POP_NAME(P) pop_name(#P, __FILE__, __FUNC__, __LINE__);

#else

#define PUSH_NAME(P)

#define POP_NAME(P)

#endif

#define BSTRING(B)\
  bstring B = NULL; if (B == NULL) {PUSH_NAME(#B) HARDBUG((B = bfromcstr("")) == NULL)}

#define BDESTROY(B) POP_NAME(#B) HARDBUG(bdestroy(B) == BSTR_ERR)

#define CSTRING(C, N)\
  char *C = NULL; if (C == NULL) {PUSH_NAME(#C) HARDBUG((C = malloc(N + 1)) == NULL)}

#define CDESTROY(C) POP_NAME(#C) free(C); C = NULL;

void mark_pointer_read_only(void *);
void register_heap(char *, char *, const char *, int, void *, size_t,
  int, int);
void *deregister_heap(void *);

void *my_malloc(char *, char *, const char *, int, size_t);
void my_free(void *);

void print_my_names(int);
void push_name(char *, char *, const char *, int);
void pop_name(char *, char *, const char *, int);

void init_my_malloc(void);

void check_my_malloc(void);

void fin_my_malloc(int);

#endif

