//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef MyMallocH
#define MyMallocH

#define MARK_BLOCK_READ_ONLY(P, S) register_pointer(#P, __FILE__, __FUNC__, __LINE__, P, S, FALSE, TRUE);

#define MY_MALLOC(P, T, N) HARDBUG(((P) = my_malloc(#P, __FILE__, __FUNC__, __LINE__, sizeof(T) * (N))) == NULL)

#define MY_FREE_AND_NULL(P) {_mm_free(P); (P) = NULL;}

void register_pointer(char *, char *, const char *, int, void *, size_t,
  int, int);

void *my_malloc(char *, char *, const char *, int, size_t);

void init_my_malloc(void);

void fin_my_malloc(int);

#endif

