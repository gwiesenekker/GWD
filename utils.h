// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef UtilsH
#define UtilsH

#include "common.h"
#include "bstrlib.h"
#include "cJSON.h"

static const double CENTI_SECOND = 0.01;
static const double MILLI_SECOND = 0.001;
static const double MICRO_SECOND = 0.000001;

int fexists(const char *arg_name);

my_int64_t first_prime_below(my_int64_t arg_number);
void init_crc32(uint32_t *arg_crc);
void update_crc32(uint32_t *arg_crc, const void *arg_data, uint32_t arg_nbytes);
uint32_t return_crc32(const void *arg_data, uint32_t arg_nbytes);
my_uint64_t
return_crc64(const void *arg_data, my_uint64_t arg_nbytes, int arg_flip);
void file2bstring(const char *arg_name, bstring arg_bstring);
void file2cjson(const char *arg_name, cJSON **arg_cjson);
cJSON *cJSON_FindItemInObject(const cJSON *arg_cjson, const char *arg_name);

void init_utils(void);
void test_utils(void);

#endif
