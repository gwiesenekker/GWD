// SCU REVISION 8.1255 za 31 okt 2026  9:15:42 CET
#ifndef RandomH
#define RandomH

typedef struct
{
  ui64_t MR_state[4];
} my_random_t;

void shuffle(int *, int n, my_random_t *, int);
void construct_my_random(my_random_t *, ui64_t *);
ui64_t return_my_random(my_random_t *);

void test_my_random(void);

#endif
