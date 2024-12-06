//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef RandomH
#define RandomH

typedef struct
{
  ui64_t MR_state[4];
} my_random_t;

void shuffle(int *, int n, my_random_t *);
void construct_my_random(void *, i64_t);
ui64_t return_my_random(void *);

void test_my_random(void);

#endif

