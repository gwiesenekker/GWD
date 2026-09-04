// SCU REVISION 8.12462 vr  4 sep 2026  7:33:55 CEST
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
