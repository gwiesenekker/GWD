//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"

//the game state is maintained in a cJSON object

local void printf_state(void *self)
{
  state_t *object = self;

  PRINTF("state=%s\n", object->get_state(object));
  PRINTF("event=%s\n", object->get_event(object));
  PRINTF("date=%s\n", object->get_date(object));
  PRINTF("white=%s\n", object->get_white(object));
  PRINTF("black=%s\n", object->get_black(object));
  PRINTF("result=%s\n", object->get_result(object));
  PRINTF("starting_position=%s\n", object->get_starting_position(object));
  PRINTF("depth=%d\n", object->get_depth(object));
  PRINTF("time=%d\n", object->get_time(object));
}

local void set_state(void *self, char *string)
{
  state_t *object = self;

  object->cjson_object = cJSON_Parse(string);

  if (object->cjson_object == NULL)
  { 
    const char *error = cJSON_GetErrorPtr();

    if (error != NULL)
      fprintf(stderr, "json error before: %s\n", error);

    FATAL("gwd.json error", EXIT_FAILURE)
  }
}

local void set_event(void *self, char *event)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, event);
}

local void set_date(void *self, char *date)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, date);
}

local void set_white(void *self, char *white)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, white);
}

local void set_black(void *self, char *black)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, black);
}

local void set_result(void *self, char *result)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, result);
}

local void set_starting_position(void *self, char *position)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, position);
}

local void push_move(void *self, char *move, char *comment)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  cJSON *cjson_move = cJSON_CreateObject();
 
  HARDBUG(cjson_move == NULL)

  HARDBUG(cJSON_AddStringToObject(cjson_move, CJSON_MOVE_STRING_ID,
                                  move) == NULL)

  if (comment != NULL)
    HARDBUG(cJSON_AddStringToObject(cjson_move, CJSON_COMMENT_STRING_ID,
                                comment) == NULL)

  cJSON_AddItemToArray(cjson_item, cjson_move);
}

local int pop_move(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  int n = cJSON_GetArraySize(cjson_item);

  if (n > 0) cJSON_DeleteItemFromArray(cjson_item, n - 1);

  return(n);
}

local void set_depth(void *self, int depth)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, depth);
}

local void set_time(void *self, int time)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, time);
}

local void save(void *self, char *name)
{
  state_t *object = self;

  FILE *fsave;

  HARDBUG((fsave = fopen(name, "w")) == NULL)

  fprintf(fsave, "%s\n", object->get_state(object));

  FCLOSE(fsave)
}

local void save2pdn(void *self, char *pdn)
{
  state_t *object = self;

  int fd = compat_lock_file(pdn);

  HARDBUG(fd == -1)

  compat_fdprintf(fd, "[Event \"%s\"]\n", object->get_event(object));
  compat_fdprintf(fd, "[Date \"%s\"]\n", object->get_date(object));
  compat_fdprintf(fd, "[White \"%s\"]\n", object->get_white(object));
  compat_fdprintf(fd, "[Black \"%s\"]\n", object->get_black(object));
  compat_fdprintf(fd, "[Result \"%s\"]\n", object->get_result(object));
  compat_fdprintf(fd, "[FEN \"%s\"]\n\n", object->get_starting_position(object));

  int iply = 0;

  cJSON *game_move;

  cJSON_ArrayForEach(game_move, object->get_moves(object))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    cJSON *comment_string = cJSON_GetObjectItem(game_move,
                                                CJSON_COMMENT_STRING_ID);

    if ((iply % 2) == 0) compat_fdprintf(fd, " %d.", iply / 2 + 1);

    if (comment_string == NULL)
    {
      compat_fdprintf(fd, " %s", cJSON_GetStringValue(move_string));
    }
    else
    {
      compat_fdprintf(fd, " %s {%s}", cJSON_GetStringValue(move_string),
                                  cJSON_GetStringValue(comment_string));
    } 

    iply++;

    if ((iply > 0) and ((iply % 10) == 0)) compat_fdprintf(fd, "\n");
  }
  compat_fdprintf(fd, " %s\n\n", object->get_result(object));

  compat_unlock_file(fd);
}

local char *get_state(void *self)
{
  state_t *object = self;

  HARDBUG(cJSON_PrintPreallocated(object->cjson_object, object->string,
                                  MY_LINE_MAX, FALSE) == 0)

  return(object->string);
}

local char *get_event(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_date(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_white(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_black(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_result(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_starting_position(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local cJSON *get_moves(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  return(cjson_item);
}

local int get_depth(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return(round(cJSON_GetNumberValue(cjson_item)));
}

local int get_time(void *self)
{
  state_t *object = self;

  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return(round(cJSON_GetNumberValue(cjson_item)));
}

local void load(void *self, char *name)
{
  state_t *object = self;

  FILE *fload;

  HARDBUG((fload = fopen(name, "r")) == NULL)

  char string[MY_LINE_MAX];

  strcpy(string, "");
  
  while(TRUE)
  {
    char line[MY_LINE_MAX];
 
    if (fgets(line, MY_LINE_MAX, fload) == NULL) break;

    size_t n = strlen(line);

    if (n > 0)
    {
      if (line[n - 1] == '\n') line[n - 1] = '\0';
    }
    strcat(string, " ");
    strcat(string, line);
  }
  FCLOSE(fload)

  object->set_state(object, string);
}

void construct_state(void *self)
{
  state_t *object = self;

  object->cjson_object = cJSON_CreateObject();
  
  char event[MY_LINE_MAX];

  time_t t = time(NULL);

  HARDBUG(strftime(event, MY_LINE_MAX, "%Y.%m.%d %H:%M:%S",
                   localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_EVENT_ID,
                                  event) == NULL)

  char date[MY_LINE_MAX];

  HARDBUG(strftime(date, MY_LINE_MAX, "%Y.%m.%d",
                   localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_DATE_ID,
                                  date) == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_WHITE_ID,
                                  "White") == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_BLACK_ID,
                                  "Black") == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_RESULT_ID,
                                  "*") == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object,
                                  CJSON_STARTING_POSITION_ID,
                                  STARTING_POSITION2FEN) == NULL)

  HARDBUG(cJSON_AddArrayToObject(object->cjson_object,
                                 CJSON_MOVES_ID) == NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object,
                                  CJSON_DEPTH_ID, 64) == NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object, CJSON_TIME_ID,
                                  30) == NULL)

  object->printf_state = printf_state;

  object->set_state = set_state;
  object->set_event = set_event;
  object->set_date = set_date;
  object->set_white = set_white;
  object->set_black = set_black;
  object->set_result = set_result;
  object->set_starting_position = set_starting_position;
  object->push_move = push_move;
  object->pop_move = pop_move;
  object->set_depth = set_depth;
  object->set_time = set_time;
  object->save = save;
  object->save2pdn = save2pdn;

  object->get_state = get_state;
  object->get_event = get_event;
  object->get_date = get_date;
  object->get_white = get_white;
  object->get_black = get_black;
  object->get_result = get_result;
  object->get_starting_position = get_starting_position;
  object->get_moves = get_moves;
  object->get_depth = get_depth;
  object->get_time = get_time;
  object->load = load;
}

void destroy_state(void *self)
{
  state_t *object = self;

  cJSON_Delete(object->cjson_object);
}

void test_states(void)
{
  state_t a;

  construct_state(&a);

  a.set_starting_position(&a, "[FEN \"W:W28,31,32,35,36,37,38,39,40,42,43,45,47:B3,7,8,11,12,13,15,19,20,21,23,26,29.\"]");

  a.push_move(&a, "31-26", NULL);

  a.push_move(&a, "17-22", "{only move}");

  a.set_depth(&a, 32);

  a.set_time(&a, 10);

  a.printf_state(&a);

  a.pop_move(&a);

  state_t b;

  construct_state(&b);
  
  b.printf_state(&b);
}

