//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef RandomH
#define RandomH

typedef struct
{
  ui64_t MR_state[4];
} my_random_t;

void construct_my_random(void *, i64_t);
ui64_t return_my_random(void *);

void test_my_random(void);

#endif

