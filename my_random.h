//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef RandomH
#define RandomH

typedef struct
{
  ui64_t MR_state[4];
} my_random_t;

void shuffle(int *, int n, my_random_t *);
void construct_my_random(my_random_t *, i64_t);
ui64_t return_my_random(my_random_t *);

void test_my_random(void);

#endif
