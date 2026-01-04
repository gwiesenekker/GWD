//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef BucketsH
#define BucketsH

#define BUCKET_LINEAR 1
#define BUCKET_LOG 2

typedef struct
{
  my_printf_t *bucket_my_printf;

  bstring bucket_name;

  double bucket_size;
  double bucket_min;
  double bucket_max;
  int bucket_scale;

  i64_t bucket_nlt_min;
  i64_t bucket_nge_max;

  int nbuckets;
  i64_t *buckets;
} bucket_t;

void clear_bucket(void *);
void update_bucket(void *, double);
void printf_bucket(void *);
void construct_bucket(void *, char *, double, double, double, int);

void my_mpi_bucket_aggregate(void *, MPI_Comm);

void test_buckets(void);

#endif
