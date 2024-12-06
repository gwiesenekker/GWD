//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef My_cjsonH
#define My_cjsonH

typedef struct
{
  cJSON *MCJ_cjson;

  bstring MCJ_bcjson;
} my_cjson_t;

void construct_my_cjson(void *);

void test_my_cjson(void);

#endif
