//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef My_cjsonH
#define My_cjsonH

typedef struct
{
  cJSON *MCJ_cjson;

  bstring MCJ_bstring;
} my_cjson_t;

void construct_my_cjson(void *);

void my_cjson_add_int(void *, char *, int);
void my_cjson_set_int(void *, char *, int);
int my_cjson_get_int(void *, char *);

void my_cjson_add_cstring(void *, char *, char *);
void my_cjson_set_cstring(void *, char *, char *);
char *my_cjson_get_cstring(void *, char *);

char *my_cjson2cstring(void *);

void test_my_cjson(void);

#endif
