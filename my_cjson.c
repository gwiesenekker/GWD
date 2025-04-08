//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

void construct_my_cjson(void *self)
{
  my_cjson_t *object = self;

  HARDBUG((object->MCJ_cjson = cJSON_CreateObject()) == NULL)

  HARDBUG((object->MCJ_bstring = bfromcstr("")) == NULL)
}

void my_cjson_add_int(void *self, char *arg_id, int arg_int)
{
  my_cjson_t *object = self;

  HARDBUG(cJSON_AddNumberToObject(object->MCJ_cjson, arg_id, arg_int) == 
          NULL)
}

void my_cjson_set_int(void *self, char *arg_id, int arg_int)
{
  my_cjson_t *object = self;

  cJSON *number;

  HARDBUG((number = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) ==
          NULL)

  HARDBUG(!cJSON_IsNumber(number))

  cJSON_SetNumberValue(number, arg_int);
}

int my_cjson_get_int(void *self, char *arg_id)
{
  my_cjson_t *object = self;

  cJSON *number;

  HARDBUG((number = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) == 
          NULL)

  HARDBUG(!cJSON_IsNumber(number))

   return(round(cJSON_GetNumberValue(number)));
}

void my_cjson_add_cstring(void *self, char *arg_id, char *arg_cstring)
{
  my_cjson_t *object = self;

  HARDBUG(cJSON_AddStringToObject(object->MCJ_cjson, arg_id, arg_cstring) == 
          NULL)
}

void my_cjson_set_cstring(void *self, char *arg_id, char *arg_cstring)
{
  my_cjson_t *object = self;

  cJSON *cstring;

  HARDBUG((cstring = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) ==
          NULL)

  HARDBUG(!cJSON_IsString(cstring))

  cJSON_SetValuestring(cstring, arg_cstring);
}

char *my_cjson_get_cstring(void *self, char *arg_id)
{
  my_cjson_t *object = self;

  cJSON *cstring;

  HARDBUG((cstring = cJSON_GetObjectItem(object->MCJ_cjson, arg_id)) == 
          NULL)

  HARDBUG(!cJSON_IsString(cstring))

   return(cJSON_GetStringValue(cstring));
}

char *my_cjson2cstring(void *self)
{
  my_cjson_t *object = self;

  char *cstring;

  HARDBUG((cstring = cJSON_PrintUnformatted(object->MCJ_cjson)) == NULL)

  HARDBUG(bassigncstr(object->MCJ_bstring, cstring) == BSTR_ERR)

  free(cstring);

  return(bdata(object->MCJ_bstring));

}

#define CJSON_ID_NAME "name"
#define CJSON_ID_INT "int"

void test_my_cjson(void)
{
  PRINTF("test_my_cjson\n");

  my_cjson_t test;

  construct_my_cjson(&test);

  PRINTF("test=%s\n", my_cjson2cstring(&test));

  my_cjson_add_cstring(&test, CJSON_ID_NAME, "n");
  my_cjson_add_int(&test, CJSON_ID_INT, 65);

  PRINTF("test=%s\n", my_cjson2cstring(&test));

  char *c  = my_cjson_get_cstring(&test, CJSON_ID_NAME);
  int n = my_cjson_get_int(&test, CJSON_ID_INT);

  PRINTF("c=%s n=%d\n", c, n);

  my_cjson_set_cstring(&test, CJSON_ID_NAME, "n updated");
  my_cjson_set_int(&test, CJSON_ID_INT, 130);

  PRINTF("test=%s\n", my_cjson2cstring(&test));

  c  = my_cjson_get_cstring(&test, CJSON_ID_NAME);
  n = my_cjson_get_int(&test, CJSON_ID_INT);

  PRINTF("c=%s n=%d\n", c, n);
}

