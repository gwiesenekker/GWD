// SCU REVISION 8.1255 za 31 okt 2026  9:15:42 CET
#include "globals.h"

void construct_my_cjson(my_cjson_t *arg_with_my_cjson)
{
  HARDBUG((arg_with_my_cjson->MCJ_cjson = cJSON_CreateObject()) == NULL)

  HARDBUG((arg_with_my_cjson->MCJ_bstring = bfromcstr("")) == NULL)
}

void my_cjson_add_int(my_cjson_t *arg_with_my_cjson, char *arg_id, int arg_int)
{
  HARDBUG(cJSON_AddNumberToObject(arg_with_my_cjson->MCJ_cjson,
                                  arg_id,
                                  arg_int) == NULL)
}

void my_cjson_set_int(my_cjson_t *arg_with_my_cjson, char *arg_id, int arg_int)
{
  cJSON *number;

  HARDBUG((number =
             cJSON_GetObjectItem(arg_with_my_cjson->MCJ_cjson, arg_id)) == NULL)

  HARDBUG(!cJSON_IsNumber(number))

  cJSON_SetNumberValue(number, arg_int);
}

int my_cjson_get_int(my_cjson_t *arg_with_my_cjson, char *arg_id)
{
  cJSON *number;

  HARDBUG((number =
             cJSON_GetObjectItem(arg_with_my_cjson->MCJ_cjson, arg_id)) == NULL)

  HARDBUG(!cJSON_IsNumber(number))

  return (round(cJSON_GetNumberValue(number)));
}

void my_cjson_add_cstring(my_cjson_t *arg_with_my_cjson,
                          char *arg_id,
                          char *arg_cstring)
{
  HARDBUG(cJSON_AddStringToObject(arg_with_my_cjson->MCJ_cjson,
                                  arg_id,
                                  arg_cstring) == NULL)
}

void my_cjson_set_cstring(my_cjson_t *arg_with_my_cjson,
                          char *arg_id,
                          char *arg_cstring)
{
  cJSON *cstring;

  HARDBUG((cstring =
             cJSON_GetObjectItem(arg_with_my_cjson->MCJ_cjson, arg_id)) == NULL)

  HARDBUG(!cJSON_IsString(cstring))

  cJSON_SetValuestring(cstring, arg_cstring);
}

char *my_cjson_get_cstring(my_cjson_t *arg_with_my_cjson, char *arg_id)
{
  cJSON *cstring;

  HARDBUG((cstring =
             cJSON_GetObjectItem(arg_with_my_cjson->MCJ_cjson, arg_id)) == NULL)

  HARDBUG(!cJSON_IsString(cstring))

  return (cJSON_GetStringValue(cstring));
}

char *my_cjson2cstring(my_cjson_t *arg_with_my_cjson)
{
  char *cstring;

  HARDBUG((cstring = cJSON_PrintUnformatted(arg_with_my_cjson->MCJ_cjson)) ==
          NULL)

  HARDBUG(bassigncstr(arg_with_my_cjson->MCJ_bstring, cstring) == BSTR_ERR)

  free(cstring);

  return (bdata(arg_with_my_cjson->MCJ_bstring));
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

  char *c = my_cjson_get_cstring(&test, CJSON_ID_NAME);
  int n = my_cjson_get_int(&test, CJSON_ID_INT);

  PRINTF("c=%s n=%d\n", c, n);

  my_cjson_set_cstring(&test, CJSON_ID_NAME, "n updated");
  my_cjson_set_int(&test, CJSON_ID_INT, 130);

  PRINTF("test=%s\n", my_cjson2cstring(&test));

  c = my_cjson_get_cstring(&test, CJSON_ID_NAME);
  n = my_cjson_get_int(&test, CJSON_ID_INT);

  PRINTF("c=%s n=%d\n", c, n);
}
