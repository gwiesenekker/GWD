//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

#define CJSON_FIELDS_ID      "fields"
#define CJSON_FIELD_NAME_ID  "field_name"
#define CJSON_FIELD_TYPE_ID  "field_type"
#define CJSON_FIELD_VALUE_ID "field_value"

local cJSON *find_field(cJSON *fields, char *arg_field_name)
{
  HARDBUG(!cJSON_IsArray(fields))

  cJSON *result = NULL;

  cJSON *field;

  cJSON_ArrayForEach(field, fields)
  {
    cJSON *field_name =
      cJSON_GetObjectItem(field, CJSON_FIELD_NAME_ID);

    HARDBUG(!cJSON_IsString(field_name))

    if (compat_strcasecmp(cJSON_GetStringValue(field_name),  arg_field_name) == 0)
    {
      result = field;

      break;
    }
  }

  return(result);
}

void construct_record(void *self)
{
  record_t *object = self;
  
  object->R_cjson = cJSON_CreateObject();

  HARDBUG(cJSON_AddArrayToObject(object->R_cjson, CJSON_FIELDS_ID) == NULL)

  HARDBUG((object->R_string = bfromcstr("")) == NULL)
}

void add_field(void *self, char *arg_field_name,
  int arg_field_type, void *arg_field_value)
{
  record_t *object = self;

  cJSON *fields = cJSON_GetObjectItem(object->R_cjson, CJSON_FIELDS_ID);

  HARDBUG(!cJSON_IsArray(fields))

  cJSON *field = find_field(fields, arg_field_name);

  HARDBUG(field != NULL)

  field = cJSON_CreateObject();

  cJSON_AddItemToArray(fields, field);

  HARDBUG(cJSON_AddStringToObject(field, CJSON_FIELD_NAME_ID,
                              arg_field_name) == NULL)

  if (arg_field_type == FIELD_TYPE_INT)   
  {
    int value = 0;

    if (arg_field_value != NULL) value = *((int *) arg_field_value);

    HARDBUG(cJSON_AddNumberToObject(field, CJSON_FIELD_TYPE_ID,
                                FIELD_TYPE_INT) == NULL)

    HARDBUG(cJSON_AddNumberToObject(field, CJSON_FIELD_VALUE_ID, 
                                value) == NULL)
  } 
  else if (arg_field_type == FIELD_TYPE_STRING)
  {
    char value[MY_LINE_MAX];

    strcpy(value, "");

    if (arg_field_value != NULL) strncpy(value, arg_field_value, MY_LINE_MAX);

    HARDBUG(cJSON_AddNumberToObject(field, CJSON_FIELD_TYPE_ID,
                                FIELD_TYPE_STRING) == NULL)

    HARDBUG(cJSON_AddStringToObject(field, CJSON_FIELD_VALUE_ID,
                                value) == NULL)
  }
  else if (arg_field_type == FIELD_TYPE_ARRAY)
  {
    HARDBUG(arg_field_value != NULL)
 
    HARDBUG(cJSON_AddNumberToObject(field, CJSON_FIELD_TYPE_ID,
                                FIELD_TYPE_ARRAY) == NULL)

    HARDBUG(cJSON_AddArrayToObject(field, arg_field_name) == NULL)
  }
  else
    FATAL("unsopported field_type", EXIT_FAILURE)
}

void set_field(void *self, char *arg_field_name, void *arg_value)
{
  record_t *object = self;

  cJSON *fields = cJSON_GetObjectItem(object->R_cjson, CJSON_FIELDS_ID);

  HARDBUG(!cJSON_IsArray(fields))

  cJSON *field = find_field(fields, arg_field_name);

  HARDBUG(field == NULL)

  cJSON *field_type = cJSON_GetObjectItem(field, CJSON_FIELD_TYPE_ID);

  HARDBUG(!cJSON_IsNumber(field_type))

  if (round(cJSON_GetNumberValue(field_type)) == FIELD_TYPE_INT)
  {
    cJSON *field_value = cJSON_GetObjectItem(field, CJSON_FIELD_VALUE_ID);

    HARDBUG(!cJSON_IsNumber(field_value))
  
    cJSON_SetNumberValue(field_value, *(int *) arg_value);
  } 
  else if (round(cJSON_GetNumberValue(field_type)) == FIELD_TYPE_STRING)
  {
    cJSON *field_value = cJSON_GetObjectItem(field, CJSON_FIELD_VALUE_ID);

    HARDBUG(!cJSON_IsString(field_value))

    cJSON_SetStringValue(field_value, (char *) arg_value);
  }
}

void get_field(void *self, char *arg_field_name, void *arg_value)
{
  record_t *object = self;

  cJSON *fields = cJSON_GetObjectItem(object->R_cjson, CJSON_FIELDS_ID);

  HARDBUG(!cJSON_IsArray(fields))

  cJSON *field = find_field(fields, arg_field_name);

  HARDBUG(field == NULL)

  cJSON *field_type = cJSON_GetObjectItem(field, CJSON_FIELD_TYPE_ID);

  HARDBUG(!cJSON_IsNumber(field_type))

  if (round(cJSON_GetNumberValue(field_type)) == FIELD_TYPE_INT)
  {
    cJSON *field_value = cJSON_GetObjectItem(field, CJSON_FIELD_VALUE_ID);

    HARDBUG(!cJSON_IsNumber(field_value))
  
    *((int *) arg_value) = round(cJSON_GetNumberValue(field_value));
  } 
  else if (round(cJSON_GetNumberValue(field_type)) == FIELD_TYPE_STRING)
  {
    cJSON *field_value = cJSON_GetObjectItem(field, CJSON_FIELD_VALUE_ID);

    HARDBUG(!cJSON_IsString(field_value))

    HARDBUG(bassigncstr((bstring) arg_value,
                        cJSON_GetStringValue(field_value)) != BSTR_OK)
  }
}

void set_record(void *self, bstring string)
{
  record_t *object = self;

  object->R_cjson = cJSON_Parse(bdata(string));

  if (object->R_cjson == NULL)
  { 
    const char *error = cJSON_GetErrorPtr();

    if (error != NULL)
      fprintf(stderr, "json error before: %s\n", error);

    FATAL("gwd.json error", EXIT_FAILURE)
  }
}

bstring get_record(void *self)
{
  record_t *object = self;

  balloc(object->R_string, MY_LINE_MAX);

  HARDBUG(cJSON_PrintPreallocated(object->R_cjson, bdata(object->R_string),
                                  MY_LINE_MAX, FALSE) == 0)

  return(object->R_string);
}

void test_records(void)
{
  record_t a;

  construct_record(&a);

  bstring string = bfromcstr("");

  HARDBUG(string == NULL)

  PRINTF("empty record: %s\n", bdata(get_record(&a)));

  add_field(&a, "name", FIELD_TYPE_STRING, "GW");

  get_field(&a, "name", string);

  PRINTF("added name: %s\n", bdata(string));
  PRINTF("added name: %s\n", bdata(get_record(&a)));

  set_field(&a, "name", "GWIESENEKKER");
  get_field(&a, "name", string);

  PRINTF("changed name: %s\n", bdata(string));
  PRINTF("changed name: %s\n", bdata(get_record(&a)));

  int value = 59;

  add_field(&a, "age", FIELD_TYPE_INT, &value);
  get_field(&a, "age", &value);

  PRINTF("added age: %d\n", value);
  PRINTF("added age: %s\n", bdata(get_record(&a)));

  value = 60;

  set_field(&a, "age", &value);
  get_field(&a, "age", &value);

  PRINTF("changed age: %d\n", value);
  PRINTF("changed age: %s\n", bdata(get_record(&a)));

  record_t b;

  construct_record(&b);

  set_record(&b, get_record(&a));

  PRINTF("copied record: %s\n", bdata(get_record(&b)));
}

