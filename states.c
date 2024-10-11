//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

//the game state is maintained in a cJSON object
//with the following fields

class_t *state_class;

//the object printer

local void printf_state(void *self)
{
  state_t *object = self;

  PRINTF("object_id=%d\n", object->object_id);

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

//object methods

local void set_state(state_t *object, char *string)
{
  object->cjson_object = cJSON_Parse(string);

  if (object->cjson_object == NULL)
  { 
    const char *error = cJSON_GetErrorPtr();

    if (error != NULL)
      fprintf(stderr, "json error before: %s\n", error);

    FATAL("gwd.json error", EXIT_FAILURE)
  }
}

local void set_event(state_t *object, char *event)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, event);
}

local void set_date(state_t *object, char *date)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, date);
}

local void set_white(state_t *object, char *white)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, white);
}

local void set_black(state_t *object, char *black)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, black);
}

local void set_result(state_t *object, char *result)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, result);
}

local void set_starting_position(state_t *object, char *position)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetStringValue(cjson_item, position);
}

local void push_move(state_t *object, char *move, char *comment)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  cJSON *cjson_move = cJSON_CreateObject();
 
  HARDBUG(cjson_move == NULL)

  HARDBUG(cJSON_AddStringToObject(cjson_move, CJSON_MOVE_STRING_ID, move) == NULL)

  if (comment != NULL)
    HARDBUG(cJSON_AddStringToObject(cjson_move, CJSON_COMMENT_STRING_ID,
                                comment) == NULL)

  cJSON_AddItemToArray(cjson_item, cjson_move);
}

local int pop_move(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  int n = cJSON_GetArraySize(cjson_item);

  if (n > 0) cJSON_DeleteItemFromArray(cjson_item, n - 1);

  return(n);
}

local void set_depth(state_t *object, int depth)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, depth);
}

local void set_time(state_t *object, int time)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, time);
}

local void save(state_t *object, char *name)
{
  FILE *fsave;

  HARDBUG((fsave = fopen(name, "w")) == NULL)

  fprintf(fsave, "%s\n", object->get_state(object));

  FCLOSE(fsave);
}

local void save2pdn(state_t *object, char *pdn)
{
  int fd = my_lock_file(pdn);

  HARDBUG(fd == -1)

  my_fdprintf(fd, "[Event \"%s\"]\n", object->get_event(object));
  my_fdprintf(fd, "[Date \"%s\"]\n", object->get_date(object));
  my_fdprintf(fd, "[White \"%s\"]\n", object->get_white(object));
  my_fdprintf(fd, "[Black \"%s\"]\n", object->get_black(object));
  my_fdprintf(fd, "[Result \"%s\"]\n", object->get_result(object));
  my_fdprintf(fd, "[FEN \"%s\"]\n\n", object->get_starting_position(object));

  int iply = 0;

  cJSON *game_move;

  cJSON_ArrayForEach(game_move, object->get_moves(object))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    cJSON *comment_string = cJSON_GetObjectItem(game_move,
                                                CJSON_COMMENT_STRING_ID);

    if ((iply % 2) == 0) my_fdprintf(fd, " %d.", iply / 2 + 1);

    if (comment_string == NULL)
    {
      my_fdprintf(fd, " %s", cJSON_GetStringValue(move_string));
    }
    else
    {
      my_fdprintf(fd, " %s {%s}", cJSON_GetStringValue(move_string),
                                  cJSON_GetStringValue(comment_string));
    } 

    iply++;

    if ((iply > 0) and ((iply % 10) == 0)) my_fdprintf(fd, "\n");
  }
  my_fdprintf(fd, " %s\n\n", object->get_result(object));

  my_unlock_file(fd);
}

local char *get_state(state_t *object)
{
  HARDBUG(cJSON_PrintPreallocated(object->cjson_object, object->string, MY_LINE_MAX,
                              FALSE) == 0)

  return(object->string);
}

local char *get_event(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_date(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_white(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_black(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_result(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local char *get_starting_position(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return(cJSON_GetStringValue(cjson_item));
}

local cJSON *get_moves(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  return(cjson_item);
}

local int get_depth(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return(round(cJSON_GetNumberValue(cjson_item)));
}

local int get_time(state_t *object)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return(round(cJSON_GetNumberValue(cjson_item)));
}

local void load(state_t *object, char *name)
{
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

local void *construct_state(void)
{
  state_t *object;
  
  MALLOC(object, state_t, 1)

  object->object_id = state_class->register_object(state_class, object);

  object->cjson_object = cJSON_CreateObject();

  char event[MY_LINE_MAX];

  time_t t = time(NULL);

  (void) strftime(event, MY_LINE_MAX, "%Y.%m.%d %H:%M:%S", localtime(&t));

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_EVENT_ID, event) ==
      NULL)

  char date[MY_LINE_MAX];

  (void) strftime(date, MY_LINE_MAX, "%Y.%m.%d", localtime(&t));

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_DATE_ID, date) == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_WHITE_ID, "White") ==
      NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_BLACK_ID, "Black") ==
      NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_RESULT_ID, "*") ==
      NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_STARTING_POSITION_ID,
                              STARTING_POSITION2FEN) == NULL)

  HARDBUG(cJSON_AddArrayToObject(object->cjson_object, CJSON_MOVES_ID) == NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object, CJSON_DEPTH_ID, 64) == NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object, CJSON_TIME_ID, 30) == NULL)

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

  return(object);
}

local void destroy_state(void *self)
{
  state_t *object = self;

  cJSON_Delete(object->cjson_object);

  state_class->deregister_object(state_class, object);
}


//the object iterator

local int iterate_state(void *self)
{
  state_t *object = self;

  PRINTF("iterate object_id=%d\n", object->object_id);

  object->printf_state(object);

  return(0);
}

void init_state_class(void)
{
  state_class = init_class(128, construct_state, destroy_state,
                           iterate_state);
}

void test_state_class(void)
{
  state_t *a = state_class->objects_ctor();

  a->set_starting_position(a, "[FEN \"W:W28,31,32,35,36,37,38,39,40,42,43,45,47:B3,7,8,11,12,13,15,19,20,21,23,26,29.\"]");

  a->push_move(a, "31-26", NULL);

  a->push_move(a, "17-22", "{only move}");

  a->set_depth(a, 32);

  a->set_time(a, 10);

  a->printf_state(a);

  a->pop_move(a);

  state_t *b = state_class->objects_ctor();
  
  b->printf_state(b);

  PRINTF("iterate\n");

  iterate_class(state_class);
}

