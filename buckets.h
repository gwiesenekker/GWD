// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef BucketsH
#define BucketsH

enum
{
  BUCKET_LINEAR = 1,
  BUCKET_LOG = 2
};

typedef struct
{
  bstring bucket_name;

  double bucket_size;
  double bucket_min;
  double bucket_max;
  int bucket_scale;

  my_int64_t bucket_nlt_min;
  my_int64_t bucket_nge_max;

  int nbuckets;
  my_int64_t *buckets;
} bucket_t;

void clear_bucket(bucket_t *arg_with_bucket);
void update_bucket(bucket_t *arg_with_bucket, double arg_value);
void printf_bucket(const bucket_t *arg_with_bucket);
void construct_bucket(bucket_t *arg_with_bucket,
                      const char *arg_bucket_name,
                      double arg_bucket_size,
                      double arg_bucket_min,
                      double arg_bucket_max,
                      int arg_bucket_scale);

void my_mpi_bucket_aggregate(bucket_t *arg_with_bucket, MPI_Comm arg_comm);

void test_buckets(void);

#endif
