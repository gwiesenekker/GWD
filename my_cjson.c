//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"

void construct_my_cjson(void *self)
{
  my_cjson_t *object = self;

  HARDBUG((object->MCJ_cjson = cJSON_CreateObject()) == NULL)

  HARDBUG((object->MCJ_bcjson = bfromcstr("")) == NULL)
}

void my_cjson_add_number(void *self, char *arg_id, int arg_number)
{
  my_cjson_t *object = self;

  HARDBUG(cJSON_AddNumberToObject(object->MCJ_cjson, arg_id, arg_number) == 
          NULL)
}

void my_cjson_set_number(void *self, char *arg_id, int arg_number)
{
  my_cjson_t *object = self;

  cJSON *number;

  HARDBUG((number = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) ==
          NULL)

  HARDBUG(!cJSON_IsNumber(number))

  cJSON_SetNumberValue(number, arg_number);
}

int my_cjson_get_number(void *self, char *arg_id)
{
  my_cjson_t *object = self;

  cJSON *number;

  HARDBUG((number = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) == 
          NULL)

  HARDBUG(!cJSON_IsNumber(number))

   return(round(cJSON_GetNumberValue(number)));
}

void my_cjon2string(void *self)
{
}

void test_my_cjson(void)
{
  PRINTF("test_my_cjson\n");

  my_cjson_t test;

  construct_my_cjson(&test);

  my_cjson_add_number(&test, "n", 65);

  int  n = my_cjson_get_number(&test, "n");

  PRINTF("n=%d\n", n);

  my_cjson_set_number(&test, "n", 130);

  n = my_cjson_get_number(&test, "n");

  PRINTF("n=%d\n", n);
}

