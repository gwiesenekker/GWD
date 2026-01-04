//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#include "globals.h"

// the game state is maintained in a cJSON object

local void printf_game_state(game_state_t *self)
{
  game_state_t *object = self;

  PRINTF("state=%s\n", object->get_game_state(object));
  PRINTF("event=%s\n", object->get_event(object));
  PRINTF("date=%s\n", object->get_date(object));
  PRINTF("white=%s\n", object->get_white(object));
  PRINTF("black=%s\n", object->get_black(object));
  PRINTF("result=%s\n", object->get_result(object));
  PRINTF("starting_position=%s\n", object->get_starting_position(object));
  PRINTF("depth=%d\n", object->get_depth(object));
  PRINTF("time=%d\n", object->get_time(object));
}

local void set_game_state(game_state_t *self, char *arg_string)
{
  game_state_t *object = self;

  object->cjson_object = cJSON_Parse(arg_string);

  if (object->cjson_object == NULL)
  {
    const char *error = cJSON_GetErrorPtr();

    if (error != NULL)
      fprintf(stderr, "json error before: %s\n", error);

    FATAL("gwd.json error", EXIT_FAILURE)
  }
}

local void set_event(game_state_t *self, char *arg_event)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_event);
}

local void set_date(game_state_t *self, char *arg_date)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_date);
}

local void set_white(game_state_t *self, char *arg_white)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_white);
}

local void set_black(game_state_t *self, char *arg_black)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_black);
}

local void set_result(game_state_t *self, char *arg_result)
{
  game_state_t *object = self;

  cJSON *cjson_item =
      cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_result);
}

local void set_starting_position(game_state_t *self, char *arg_position)
{
  game_state_t *object = self;

  cJSON *cjson_item =
      cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_position);
}

local void push_move(game_state_t *self, char *arg_move, char *arg_comment)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  cJSON *cjson_move = cJSON_CreateObject();

  HARDBUG(cjson_move == NULL)

  HARDBUG(cJSON_AddStringToObject(cjson_move, CJSON_MOVE_STRING_ID, arg_move) ==
          NULL)

  if (arg_comment != NULL)
    HARDBUG(cJSON_AddStringToObject(cjson_move,
                                    CJSON_COMMENT_STRING_ID,
                                    arg_comment) == NULL)

  cJSON_AddItemToArray(cjson_item, cjson_move);
}

local int pop_move(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  int n = cJSON_GetArraySize(cjson_item);

  if (n > 0)
    cJSON_DeleteItemFromArray(cjson_item, n - 1);

  return (n);
}

local void clear_moves(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  cJSON_Delete(cjson_item->child);

  cjson_item->child = NULL;
}

local void set_depth(game_state_t *self, int arg_depth)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, arg_depth);
}

local void set_time(game_state_t *self, int arg_time)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, arg_time);
}

local void save(game_state_t *self, char *arg_name)
{
  game_state_t *object = self;

  FILE *fsave;

  HARDBUG((fsave = fopen(arg_name, "w")) == NULL)

  fprintf(fsave, "%s\n", object->get_game_state(object));

  FCLOSE(fsave)
}

local void save2pdn(game_state_t *self, char *arg_pdn)
{
  game_state_t *object = self;

  int fd = compat_lock_file(arg_pdn);

  HARDBUG(fd == -1)

  compat_fdprintf(fd, "[Event \"%s\"]\n", object->get_event(object));
  compat_fdprintf(fd, "[Date \"%s\"]\n", object->get_date(object));
  compat_fdprintf(fd, "[White \"%s\"]\n", object->get_white(object));
  compat_fdprintf(fd, "[Black \"%s\"]\n", object->get_black(object));
  compat_fdprintf(fd, "[Result \"%s\"]\n", object->get_result(object));
  compat_fdprintf(fd,
                  "[FEN \"%s\"]\n\n",
                  object->get_starting_position(object));

  int iply = 0;

  cJSON *game_move;

  cJSON_ArrayForEach(game_move, object->get_moves(object))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    cJSON *comment_string =
        cJSON_GetObjectItem(game_move, CJSON_COMMENT_STRING_ID);

    if ((iply % 2) == 0)
      compat_fdprintf(fd, " %d.", iply / 2 + 1);

    if (comment_string == NULL)
    {
      compat_fdprintf(fd, " %s", cJSON_GetStringValue(move_string));
    }
    else
    {
      compat_fdprintf(fd,
                      " %s {%s}",
                      cJSON_GetStringValue(move_string),
                      cJSON_GetStringValue(comment_string));
    }

    iply++;

    if ((iply > 0) and ((iply % 10) == 0))
      compat_fdprintf(fd, "\n");
  }
  compat_fdprintf(fd, " %s\n\n", object->get_result(object));

  compat_unlock_file(fd);
}

local char *get_game_state(game_state_t *self)
{
  game_state_t *object = self;

  HARDBUG(cJSON_PrintPreallocated(object->cjson_object,
                                  object->string,
                                  MY_LINE_MAX,
                                  FALSE) == 0)

  return (object->string);
}

local char *get_event(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_date(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_white(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_black(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_result(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item =
      cJSON_GetObjectItem(object->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_starting_position(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item =
      cJSON_GetObjectItem(object->cjson_object, CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local cJSON *get_moves(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  return (cjson_item);
}

local int get_depth(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return (round(cJSON_GetNumberValue(cjson_item)));
}

local int get_time(game_state_t *self)
{
  game_state_t *object = self;

  cJSON *cjson_item = cJSON_GetObjectItem(object->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return (round(cJSON_GetNumberValue(cjson_item)));
}

local void load(game_state_t *self, char *arg_name)
{
  game_state_t *object = self;

  FILE *fload;

  HARDBUG((fload = fopen(arg_name, "r")) == NULL)

  char string[MY_LINE_MAX];

  strcpy(string, "");

  while (TRUE)
  {
    char line[MY_LINE_MAX];

    if (fgets(line, MY_LINE_MAX, fload) == NULL)
      break;

    size_t n = strlen(line);

    if (n > 0)
    {
      if (line[n - 1] == '\n')
        line[n - 1] = '\0';
    }
    strcat(string, " ");
    strcat(string, line);
  }
  FCLOSE(fload)

  object->set_game_state(object, string);
}

void construct_game_state(game_state_t *self)
{
  game_state_t *object = self;

  object->cjson_object = cJSON_CreateObject();

  char event[MY_LINE_MAX];

  time_t t = time(NULL);

  HARDBUG(strftime(event, MY_LINE_MAX, "%Y.%m.%d %H:%M:%S", localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object,
                                  CJSON_EVENT_ID,
                                  event) == NULL)

  char date[MY_LINE_MAX];

  HARDBUG(strftime(date, MY_LINE_MAX, "%Y.%m.%d", localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_DATE_ID, date) ==
          NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object,
                                  CJSON_WHITE_ID,
                                  "White") == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object,
                                  CJSON_BLACK_ID,
                                  "Black") == NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object, CJSON_RESULT_ID, "*") ==
          NULL)

  HARDBUG(cJSON_AddStringToObject(object->cjson_object,
                                  CJSON_STARTING_POSITION_ID,
                                  STARTING_POSITION2FEN) == NULL)

  HARDBUG(cJSON_AddArrayToObject(object->cjson_object, CJSON_MOVES_ID) == NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object, CJSON_DEPTH_ID, 64) ==
          NULL)

  HARDBUG(cJSON_AddNumberToObject(object->cjson_object, CJSON_TIME_ID, 30) ==
          NULL)

  object->printf_game_state = printf_game_state;

  object->set_game_state = set_game_state;
  object->set_event = set_event;
  object->set_date = set_date;
  object->set_white = set_white;
  object->set_black = set_black;
  object->set_result = set_result;
  object->set_starting_position = set_starting_position;
  object->push_move = push_move;
  object->clear_moves = clear_moves;
  object->pop_move = pop_move;
  object->set_depth = set_depth;
  object->set_time = set_time;
  object->save = save;
  object->save2pdn = save2pdn;

  object->get_game_state = get_game_state;
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

void destroy_game_state(game_state_t *self)
{
  game_state_t *object = self;

  cJSON_Delete(object->cjson_object);
}

void *construct_state(state_t *self, size_t arg_size, int arg_nstates)
{
  state_t *object = self;

  HARDBUG(arg_size < 1)
  HARDBUG(arg_nstates < 2)

  object->S_size = arg_size;
  object->S_nstates = arg_nstates;

  // MY_MALLOC_VOID(object->S_states, object->S_size * object->S_nstates)
  MY_MALLOC_BY_TYPE(object->S_states, void *, object->S_nstates)

  for (int istate = 0; istate < object->S_nstates; istate++)
    MY_MALLOC_VOID(object->S_states[istate], object->S_size)

  object->S_icurrent = 0;
  object->S_current = object->S_states[0];

  return (object->S_current);
}

void *push_state(state_t *self)
{
  state_t *object = self;

  void *current = object->S_current;

  object->S_icurrent++;

  HARDBUG(object->S_icurrent >= object->S_nstates)

  object->S_current = (char *)object->S_states[object->S_icurrent];

  memcpy(object->S_current, current, object->S_size);

  return (object->S_current);
}

void *pop_state(state_t *self)
{
  state_t *object = self;

  object->S_icurrent--;

  HARDBUG(object->S_icurrent < 0)

  object->S_current = object->S_states[object->S_icurrent];

  return (object->S_current);
}

void *reset_state(state_t *self)
{
  state_t *object = self;

  object->S_icurrent = 0;
  object->S_current = object->S_states[0];

  return (object->S_current);
}

#define NTEST 1000

void test_states(void)
{
  game_state_t a;

  construct_game_state(&a);

  PRINTF("a after init:\n");
  a.printf_game_state(&a);

  a.clear_moves(&a);

  PRINTF("a after clear_moves:\n");
  a.printf_game_state(&a);

  a.set_starting_position(&a,
                          "W:W28,31,32,35,36,37,38,39,40,42,43,45,47:B3,7,8,11,"
                          "12,13,15,19,20,21,23,26,29.");

  a.push_move(&a, "31-26", NULL);

  a.push_move(&a, "17-22", "{only move}");

  a.set_depth(&a, 32);

  a.set_time(&a, 10);

  PRINTF("a after config:\n");
  a.printf_game_state(&a);

  a.pop_move(&a);

  PRINTF("a after pop_move:\n");
  a.printf_game_state(&a);

  a.clear_moves(&a);

  PRINTF("a after clear_moves\n");
  a.printf_game_state(&a);

  a.push_move(&a, "31-26", NULL);

  PRINTF("a after push_move\n");
  a.printf_game_state(&a);

  state_t f;

  int *g;

  g = construct_state(&f, sizeof(int), NTEST);

  for (int i = 0; i <= NTEST - 2; ++i)
  {
    *g = i;

    g = push_state(&f);

    HARDBUG(f.S_icurrent != (i + 1))

    HARDBUG(*g != i)
  }

  HARDBUG(f.S_icurrent != (NTEST - 1))

  HARDBUG(*g != (NTEST - 2))

  for (int i = NTEST - 2; i >= 0; --i)
  {
    g = pop_state(&f);

    HARDBUG(f.S_icurrent != i);

    HARDBUG(*g != i)
  }

  HARDBUG(f.S_icurrent != 0)

  HARDBUG(*g != 0)
}
