//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef BucketsH
#define BucketsH

#define BUCKET_LINEAR 1
#define BUCKET_LOG    2

typedef struct 
{
  double bucket_size;
  double bucket_min;
  double bucket_max;
  int bucket_scale;

  i64_t bucket_nlt_min;
  i64_t bucket_nge_max;

  int nbuckets;
  i64_t *buckets;

  pter_t printf_bucket;

} bucket_t;

void clear_bucket(void *);
void update_bucket(void *, double);
void printf_bucket(void *);
void construct_bucket(void *, double, double, double, int);
void test_buckets(void);

#endif
