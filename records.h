// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef RecordsH
#define RecordsH

#include "bstrlib.h"
#include "cJSON.h"

enum
{
  FIELD_TYPE_INT = 0,
  FIELD_TYPE_STRING = 1,
  FIELD_TYPE_ARRAY = 2
};

typedef struct record
{
  cJSON *R_cjson;

  bstring R_string;
} record_t;

void construct_record(record_t *arg_with_record);
void add_field(record_t *arg_with_record,
               const char *arg_field_name,
               int arg_field_type,
               const void *arg_field_value);
void set_field(record_t *arg_with_record,
               const char *arg_field_name,
               const void *arg_value);
void get_field(const record_t *arg_with_record,
               const char *arg_field_name,
               void *arg_value);
void set_record(record_t *arg_with_record, const_bstring arg_bstring);
bstring get_record(record_t *arg_with_record);

void test_records(void);

#endif
