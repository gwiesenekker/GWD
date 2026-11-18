// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef UtilsH
#define UtilsH

static const double CENTI_SECOND = 0.01;
static const double MILLI_SECOND = 0.001;
static const double MICRO_SECOND = 0.000001;

extern int zzzzzz_invocation;

_Noreturn void zzzzzz(const char *, const char *, long, const char *, int);
int fexists(const char *);

my_int64_t first_prime_below(my_int64_t);
void init_crc32(uint32_t *);
void update_crc32(uint32_t *, const void *, uint32_t);
uint32_t return_crc32(const void *, uint32_t);
my_uint64_t return_crc64(const void *, my_uint64_t, int);
void file2bstring(const char *, bstring);
void file2cjson(const char *, cJSON **);
cJSON *cJSON_FindItemInObject(const cJSON *, const char *);

void init_utils(void);
void test_utils(void);

#endif
