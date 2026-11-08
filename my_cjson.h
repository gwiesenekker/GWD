// SCU REVISION 8.1260 zo  8 nov 2026 17:20:46 CET
#ifndef My_cjsonH
#define My_cjsonH

typedef struct
{
  cJSON *MCJ_cjson;

  bstring MCJ_bstring;
} my_cjson_t;

void construct_my_cjson(my_cjson_t *);

void my_cjson_add_int(my_cjson_t *, char *, int);
void my_cjson_set_int(my_cjson_t *, char *, int);
int my_cjson_get_int(my_cjson_t *, char *);

void my_cjson_add_cstring(my_cjson_t *, char *, char *);
void my_cjson_set_cstring(my_cjson_t *, char *, char *);
char *my_cjson_get_cstring(my_cjson_t *, char *);

char *my_cjson2cstring(my_cjson_t *);

void test_my_cjson(void);

#endif
