// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#ifndef RandomH
#define RandomH

typedef struct
{
  ui64_t MR_state[4];
} my_random_t;

void shuffle(int *, int n, my_random_t *, int);
void construct_my_random(my_random_t *, ui64_t *);
void seed_my_random(my_random_t *, ui64_t);
void copy_my_random(my_random_t *, const my_random_t *);
ui64_t return_my_random(my_random_t *);

void test_my_random(void);

#endif
