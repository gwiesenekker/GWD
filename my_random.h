//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
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

