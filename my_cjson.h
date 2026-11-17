// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
#ifndef My_cjsonH
#define My_cjsonH

typedef struct
{
  cJSON *MCJ_cjson;

  bstring MCJ_bstring;
} my_cjson_t;

void construct_my_cjson(my_cjson_t *);

void my_cjson_add_int(my_cjson_t *, const char *, int);
void my_cjson_set_int(my_cjson_t *, const char *, int);
int my_cjson_get_int(const my_cjson_t *, const char *);

void my_cjson_add_cstring(my_cjson_t *, const char *, const char *);
void my_cjson_set_cstring(my_cjson_t *, const char *, const char *);
char *my_cjson_get_cstring(const my_cjson_t *, const char *);

char *my_cjson2cstring(my_cjson_t *);

void test_my_cjson(void);

#endif
