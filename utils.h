//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef UtilsH
#define UtilsH

#define NANO_SECONDS 1000000000
#define DECI_SECOND  0.1
#define CENTI_SECOND 0.01
#define MILLI_SECOND 0.001
#define MICRO_SECOND 0.000001

extern int zzzzzz_invocation;

void zzzzzz(char *, const char *, long, char *, int);
int fexists(char *);
ui64_t randull(int);
double randdouble(int);

i64_t first_prime_below(i64_t);
void init_crc32(ui32_t *);
void update_crc32(ui32_t *, void *, ui32_t);
ui32_t return_crc32(void *, ui32_t);
ui64_t return_crc64(void *, ui64_t, int);
void file2cjson(char *, cJSON **);
cJSON *cJSON_FindItemInObject(cJSON *, char *);

void init_utils(void);
void test_utils(void);

#endif

