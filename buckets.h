//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef BucketsH
#define BucketsH

#define BUCKET_LINEAR 1
#define BUCKET_LOG    2

typedef struct bucket
{
  //generic properties and methods

  int object_id;

  //specific properties

  double bucket_size;
  double bucket_min;
  double bucket_max;
  int bucket_scale;

  i64_t bucket_nlt_min;
  i64_t bucket_nge_max;

  int nbuckets;
  i64_t *buckets;

  pter_t printf_bucket;

  void (*define_bucket)(struct bucket *, double, double, double, int);
  void (*update_bucket)(struct bucket *, double);
  void (*clear_bucket)(struct bucket *);
} bucket_t;

extern class_t *bucket_class;

void init_bucket_class(void);
void test_bucket_class(void);

#endif
