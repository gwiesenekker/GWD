// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
#ifndef UtilsH
#define UtilsH

#define NANO_SECONDS 1000000000
#define DECI_SECOND 0.1
#define CENTI_SECOND 0.01
#define MILLI_SECOND 0.001
#define MICRO_SECOND 0.000001

extern int zzzzzz_invocation;

_Noreturn void zzzzzz(const char *, const char *, long, const char *, int);
int fexists(const char *);

i64_t first_prime_below(i64_t);
void init_crc32(ui32_t *);
void update_crc32(ui32_t *, const void *, ui32_t);
ui32_t return_crc32(const void *, ui32_t);
ui64_t return_crc64(const void *, ui64_t, int);
void file2bstring(const char *, bstring);
void file2cjson(const char *, cJSON **);
cJSON *cJSON_FindItemInObject(const cJSON *, const char *);

void init_utils(void);
void test_utils(void);

#endif
