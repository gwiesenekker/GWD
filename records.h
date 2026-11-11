// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#ifndef RecordsH
#define RecordsH

#define FIELD_TYPE_INT 0
#define FIELD_TYPE_STRING 1
#define FIELD_TYPE_ARRAY 2

typedef struct record
{
  cJSON *R_cjson;

  bstring R_string;
} record_t;

void construct_record(record_t *);
void add_field(record_t *, char *, int, void *);
void set_field(record_t *, char *, void *);
void get_field(record_t *, char *, void *);
void set_record(record_t *, bstring);
bstring get_record(record_t *);

void test_records(void);

#endif
