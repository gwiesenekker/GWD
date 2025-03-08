//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#ifndef MyMallocH
#define MyMallocH

#define MARK_BLOCK_READ_ONLY(P, S) register_pointer(#P, __FILE__, __FUNC__, __LINE__, P, S, FALSE, TRUE);

#define MY_MALLOC(P, T, N) HARDBUG(((P) = my_malloc(#P, __FILE__, __FUNC__, __LINE__, sizeof(T) * (N))) == NULL)

#define MY_FREE_AND_NULL(P) {my_free(P); (P) = NULL;}

#define PUSH_LEAK(P) push_leak(#P, __FILE__, __FUNC__, __LINE__);

#define POP_LEAK(P) pop_leak(#P, __FILE__, __FUNC__, __LINE__);

#define BSTRING(B)\
  bstring B = NULL; if (B == NULL) {PUSH_LEAK(#B) HARDBUG((B = bfromcstr("")) == NULL)}

#define BDESTROY(B) POP_LEAK(#B) HARDBUG(bdestroy(B) == BSTR_ERR)

#define CSTRING(C, N)\
  char *C = NULL; if (C == NULL) {PUSH_LEAK(#C) HARDBUG((C = malloc(N + 1)) == NULL)}

#define CDESTROY(C) POP_LEAK(#C) free(C); C = NULL;

void register_pointer(char *, char *, const char *, int, void *, size_t,
  int, int);
void *deregister_pointer(void *);

void *my_malloc(char *, char *, const char *, int, size_t);
void my_free(void *);

void print_my_leaks(int);
void push_leak(char *, char *, const char *, int);
void pop_leak(char *, char *, const char *, int);

void init_my_malloc(void);

void fin_my_malloc(int);

#endif

