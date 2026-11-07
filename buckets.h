// SCU REVISION 8.1256 za  7 nov 2026 11:28:24 CET
#ifndef BucketsH
#define BucketsH

#define BUCKET_LINEAR 1
#define BUCKET_LOG 2

typedef struct
{
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

void clear_bucket(bucket_t *);
void update_bucket(bucket_t *, double);
void printf_bucket(bucket_t *);
void construct_bucket(bucket_t *, char *, double, double, double, int);

void my_mpi_bucket_aggregate(bucket_t *, MPI_Comm);

void test_buckets(void);

#endif
