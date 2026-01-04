//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
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

void construct_record(void *);
void add_field(void *, char *, int, void *);
void set_field(void *, char *, void *);
void get_field(void *, char *, void *);
void set_record(void *, bstring);
bstring get_record(void *);

void test_records(void);

#endif
