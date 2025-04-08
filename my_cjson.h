//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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
