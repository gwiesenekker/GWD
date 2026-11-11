// SCU REVISION 8.1265 wo 11 nov 2026  2:19:30 CET
#include "globals.h"

// the game state is maintained in a cJSON object

local void printf_game_state(game_state_t *arg_with_game_state)
{
  PRINTF("state=%s\n",
         arg_with_game_state->get_game_state(arg_with_game_state));
  PRINTF("event=%s\n", arg_with_game_state->get_event(arg_with_game_state));
  PRINTF("date=%s\n", arg_with_game_state->get_date(arg_with_game_state));
  PRINTF("white=%s\n", arg_with_game_state->get_white(arg_with_game_state));
  PRINTF("black=%s\n", arg_with_game_state->get_black(arg_with_game_state));
  PRINTF("result=%s\n", arg_with_game_state->get_result(arg_with_game_state));
  PRINTF("starting_position=%s\n",
         arg_with_game_state->get_starting_position(arg_with_game_state));
  PRINTF("depth=%d\n", arg_with_game_state->get_depth(arg_with_game_state));
  PRINTF("time=%d\n", arg_with_game_state->get_time(arg_with_game_state));
}

local void set_game_state(game_state_t *arg_with_game_state, char *arg_string)
{
  arg_with_game_state->cjson_object = cJSON_Parse(arg_string);

  if (arg_with_game_state->cjson_object == NULL)
  {
    const char *error = cJSON_GetErrorPtr();

    if (error != NULL)
      fprintf(stderr, "json error before: %s\n", error);

    FATAL("gwd.json error", EXIT_FAILURE)
  }
}

local void set_event(game_state_t *arg_with_game_state, char *arg_event)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_event);
}

local void set_date(game_state_t *arg_with_game_state, char *arg_date)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_date);
}

local void set_white(game_state_t *arg_with_game_state, char *arg_white)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_white);
}

local void set_black(game_state_t *arg_with_game_state, char *arg_black)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_black);
}

local void set_result(game_state_t *arg_with_game_state, char *arg_result)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_result);
}

local void set_starting_position(game_state_t *arg_with_game_state,
                                 char *arg_position)
{
  cJSON *cjson_item = cJSON_GetObjectItem(arg_with_game_state->cjson_object,
                                          CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  cJSON_SetValuestring(cjson_item, arg_position);
}

local void
push_move(game_state_t *arg_with_game_state, char *arg_move, char *arg_comment)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_MOVES_ID);

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

local int pop_move(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  int n = cJSON_GetArraySize(cjson_item);

  if (n > 0)
    cJSON_DeleteItemFromArray(cjson_item, n - 1);

  return (n);
}

local void clear_moves(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  cJSON_Delete(cjson_item->child);

  cjson_item->child = NULL;
}

local void set_depth(game_state_t *arg_with_game_state, int arg_depth)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, arg_depth);
}

local void set_time(game_state_t *arg_with_game_state, int arg_time)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  cJSON_SetNumberValue(cjson_item, arg_time);
}

local void save(game_state_t *arg_with_game_state, char *arg_name)
{
  FILE *fsave;

  HARDBUG((fsave = fopen(arg_name, "w")) == NULL)

  fprintf(fsave,
          "%s\n",
          arg_with_game_state->get_game_state(arg_with_game_state));

  FCLOSE(fsave)
}

local void save2pdn(game_state_t *arg_with_game_state, char *arg_pdn)
{
  int fd = compat_lock_file(arg_pdn, TRUE);

  HARDBUG(fd == -1)

  HARDBUG(compat_fdprintf(fd,
                          "[Event \"%s\"]\n",
                          arg_with_game_state->get_event(arg_with_game_state)) <
          0)
  HARDBUG(compat_fdprintf(fd,
                          "[Date \"%s\"]\n",
                          arg_with_game_state->get_date(arg_with_game_state)) <
          0)
  HARDBUG(compat_fdprintf(fd,
                          "[White \"%s\"]\n",
                          arg_with_game_state->get_white(arg_with_game_state)) <
          0)
  HARDBUG(compat_fdprintf(fd,
                          "[Black \"%s\"]\n",
                          arg_with_game_state->get_black(arg_with_game_state)) <
          0)
  HARDBUG(
    compat_fdprintf(fd,
                    "[Result \"%s\"]\n",
                    arg_with_game_state->get_result(arg_with_game_state)) < 0)
  HARDBUG(compat_fdprintf(fd,
                          "[FEN \"%s\"]\n\n",
                          arg_with_game_state->get_starting_position(
                            arg_with_game_state)) < 0)

  char *fen = arg_with_game_state->get_starting_position(arg_with_game_state);

  int iply = 0;

  // quick hack

  if (*fen == 'B')
    HARDBUG(compat_fdprintf(fd, "%d...", iply + 1) < 0)

  cJSON *game_move;

  cJSON_ArrayForEach(game_move,
                     arg_with_game_state->get_moves(arg_with_game_state))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    cJSON *comment_string =
      cJSON_GetObjectItem(game_move, CJSON_COMMENT_STRING_ID);

    if ((iply % 2) == 0)
      HARDBUG(compat_fdprintf(fd, " %d.", iply / 2 + 1) < 0)

    if (comment_string == NULL)
    {
      HARDBUG(compat_fdprintf(fd, " %s", cJSON_GetStringValue(move_string)) < 0)
    }
    else
    {
      HARDBUG(compat_fdprintf(fd,
                              " %s {%s}",
                              cJSON_GetStringValue(move_string),
                              cJSON_GetStringValue(comment_string)) < 0)
    }

    iply++;

    if ((iply > 0) and ((iply % 10) == 0))
      HARDBUG(compat_fdprintf(fd, "\n") < 0)
  }
  HARDBUG(
    compat_fdprintf(fd,
                    " %s\n\n",
                    arg_with_game_state->get_result(arg_with_game_state)) < 0)

  HARDBUG(compat_unlock_file(fd) != 0)
}

local char *get_game_state(game_state_t *arg_with_game_state)
{
  HARDBUG(cJSON_PrintPreallocated(arg_with_game_state->cjson_object,
                                  arg_with_game_state->string,
                                  MY_LINE_MAX,
                                  FALSE) == 0)

  return (arg_with_game_state->string);
}

local char *get_event(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_EVENT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_date(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_DATE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_white(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_WHITE_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_black(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_BLACK_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_result(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_RESULT_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local char *get_starting_position(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item = cJSON_GetObjectItem(arg_with_game_state->cjson_object,
                                          CJSON_STARTING_POSITION_ID);

  HARDBUG(!cJSON_IsString(cjson_item))

  return (cJSON_GetStringValue(cjson_item));
}

local cJSON *get_moves(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_MOVES_ID);

  HARDBUG(!cJSON_IsArray(cjson_item))

  return (cjson_item);
}

local int get_depth(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_DEPTH_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return (round(cJSON_GetNumberValue(cjson_item)));
}

local int get_time(game_state_t *arg_with_game_state)
{
  cJSON *cjson_item =
    cJSON_GetObjectItem(arg_with_game_state->cjson_object, CJSON_TIME_ID);

  HARDBUG(!cJSON_IsNumber(cjson_item))

  return (round(cJSON_GetNumberValue(cjson_item)));
}

local void load(game_state_t *arg_with_game_state, char *arg_name)
{
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

  arg_with_game_state->set_game_state(arg_with_game_state, string);
}

void construct_game_state(game_state_t *arg_with_game_state)
{
  arg_with_game_state->cjson_object = cJSON_CreateObject();

  char event[MY_LINE_MAX];

  time_t t = time(NULL);

  HARDBUG(strftime(event, MY_LINE_MAX, "%Y.%m.%d %H:%M:%S", localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_EVENT_ID,
                                  event) == NULL)

  char date[MY_LINE_MAX];

  HARDBUG(strftime(date, MY_LINE_MAX, "%Y.%m.%d", localtime(&t)) == 0)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_DATE_ID,
                                  date) == NULL)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_WHITE_ID,
                                  "White") == NULL)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_BLACK_ID,
                                  "Black") == NULL)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_RESULT_ID,
                                  "*") == NULL)

  HARDBUG(cJSON_AddStringToObject(arg_with_game_state->cjson_object,
                                  CJSON_STARTING_POSITION_ID,
                                  STARTING_POSITION2FEN) == NULL)

  HARDBUG(cJSON_AddArrayToObject(arg_with_game_state->cjson_object,
                                 CJSON_MOVES_ID) == NULL)

  HARDBUG(cJSON_AddNumberToObject(arg_with_game_state->cjson_object,
                                  CJSON_DEPTH_ID,
                                  64) == NULL)

  HARDBUG(cJSON_AddNumberToObject(arg_with_game_state->cjson_object,
                                  CJSON_TIME_ID,
                                  30) == NULL)

  arg_with_game_state->printf_game_state = printf_game_state;

  arg_with_game_state->set_game_state = set_game_state;
  arg_with_game_state->set_event = set_event;
  arg_with_game_state->set_date = set_date;
  arg_with_game_state->set_white = set_white;
  arg_with_game_state->set_black = set_black;
  arg_with_game_state->set_result = set_result;
  arg_with_game_state->set_starting_position = set_starting_position;
  arg_with_game_state->push_move = push_move;
  arg_with_game_state->clear_moves = clear_moves;
  arg_with_game_state->pop_move = pop_move;
  arg_with_game_state->set_depth = set_depth;
  arg_with_game_state->set_time = set_time;
  arg_with_game_state->save = save;
  arg_with_game_state->save2pdn = save2pdn;

  arg_with_game_state->get_game_state = get_game_state;
  arg_with_game_state->get_event = get_event;
  arg_with_game_state->get_date = get_date;
  arg_with_game_state->get_white = get_white;
  arg_with_game_state->get_black = get_black;
  arg_with_game_state->get_result = get_result;
  arg_with_game_state->get_starting_position = get_starting_position;
  arg_with_game_state->get_moves = get_moves;
  arg_with_game_state->get_depth = get_depth;
  arg_with_game_state->get_time = get_time;
  arg_with_game_state->load = load;
}

void destroy_game_state(game_state_t *arg_with_game_state)
{
  cJSON_Delete(arg_with_game_state->cjson_object);
}

void *construct_state(state_t *arg_with_state, size_t arg_size, int arg_nstates)
{
  HARDBUG(arg_size < 1)
  HARDBUG(arg_nstates < 2)

  arg_with_state->S_size = arg_size;
  arg_with_state->S_nstates = arg_nstates;

  // MY_MALLOC_VOID(arg_with_state->S_states, arg_with_state->S_size *
  // arg_with_state->S_nstates)
  MY_MALLOC_BY_TYPE(arg_with_state->S_states, void *, arg_with_state->S_nstates)

  for (int istate = 0; istate < arg_with_state->S_nstates; istate++)
    MY_MALLOC_VOID(arg_with_state->S_states[istate], arg_with_state->S_size)

  arg_with_state->S_icurrent = 0;
  arg_with_state->S_current = arg_with_state->S_states[0];

  return (arg_with_state->S_current);
}

void *push_state(state_t *arg_with_state)
{
  void *current = arg_with_state->S_current;

  arg_with_state->S_icurrent++;

  HARDBUG(arg_with_state->S_icurrent >= arg_with_state->S_nstates)

  arg_with_state->S_current =
    (char *)arg_with_state->S_states[arg_with_state->S_icurrent];

  memcpy(arg_with_state->S_current, current, arg_with_state->S_size);

  return (arg_with_state->S_current);
}

void *pop_state(state_t *arg_with_state)
{
  arg_with_state->S_icurrent--;

  HARDBUG(arg_with_state->S_icurrent < 0)

  arg_with_state->S_current =
    arg_with_state->S_states[arg_with_state->S_icurrent];

  return (arg_with_state->S_current);
}

void *reset_state(state_t *arg_with_state)
{
  arg_with_state->S_icurrent = 0;
  arg_with_state->S_current = arg_with_state->S_states[0];

  return (arg_with_state->S_current);
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
