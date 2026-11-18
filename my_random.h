// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef RandomH
#define RandomH

typedef struct
{
  my_uint64_t MR_state[4];
} my_random_t;

void shuffle(int *arg_array,
             int arg_narray,
             my_random_t *arg_my_random,
             int arg_isort0);
void construct_my_random(my_random_t *arg_with_my_random,
                         my_uint64_t *arg_seed);
void seed_my_random(my_random_t *arg_with_my_random, my_uint64_t arg_seed);
void copy_my_random(my_random_t *arg_to, const my_random_t *arg_from);
my_uint64_t return_my_random(my_random_t *arg_with_my_random);

void test_my_random(void);

#endif
