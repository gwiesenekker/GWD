// SCU REVISION 8.1278 zo 15 nov 2026 14:53:59 CET
#include "globals.h"

#define NARGS_MAX 256
#define HUB_TIME_RESERVE 0.050 // seconds for getting the move back to the GUI

typedef struct
{
  char *arg_name;
  char *arg_value;
} arg_t;

typedef struct
{
  char *command;
  int command_nargs;
  arg_t command_args[NARGS_MAX];
} command_t;

local void line2command(char *arg_line, command_t *arg_with_command)
{
  arg_with_command->command = NULL;
  arg_with_command->command_nargs = 0;

  char *c = arg_line;

  // command

  while (isspace(*c))
    c++;
  if (*c == '\0')
    return;

  arg_with_command->command = c;

  while (isgraph(*c))
    c++;

  if (*c == '\0')
    return;

  *c = '\0';

  c++;

  PRINTF("command=%s\n", arg_with_command->command);

  // params

  while (TRUE)
  {
    while (isspace(*c))
      c++;

    if (*c == '\0')
      return;

    HARDBUG(arg_with_command->command_nargs >= NARGS_MAX)

    arg_t *with_param =
      arg_with_command->command_args + arg_with_command->command_nargs;

    with_param->arg_name = c;
    with_param->arg_value = NULL;

    while (isgraph(*c) and (*c != '='))
      c++;

    if (isspace(*c))
    {
      *c = '\0';
      c++;
      while (isspace(*c))
        c++;
    }

    if (*c != '=')
    {
      PRINTF("arg_name=%s\n", with_param->arg_name);
    }
    else
    {
      *c = '\0';
      c++;

      PRINTF("arg_name=%s\n", with_param->arg_name);

      while (isspace(*c))
        c++;
      HARDBUG(*c == '\0')

      if (*c == '"')
      {
        c++;

        with_param->arg_value = c;

        while ((*c != '\0') and (*c != '"'))
          c++;
        HARDBUG(*c == '\0');
        *c = '\0';
        c++;
      }
      else
      {
        with_param->arg_value = c;

        while (isgraph(*c))
          c++;

        if (*c == '\0')
          return;

        *c = '\0';

        c++;
      }
      PRINTF("arg_value=%s\n", with_param->arg_value);
    }
    arg_with_command->command_nargs++;
  }
}

local void write_to_hub(char *arg_line)
{
  int nline = strlen(arg_line);

  PRINTF("write_to_hub: %d:%s", nline, arg_line);

  HARDBUG(arg_line[nline - 1] != '\n')

  HARDBUG(compat_write(1, arg_line, nline) != nline)
}

local char *pgets(char *arg_s, int arg_size)
{
  // read pipe until '\n'

  int i = 0;

  while (TRUE)
  {
    int n;
    char c;

    // error

    if ((n = compat_read(0, &c, 1)) == INVALID)
      return (NULL);

    // eof

    if (n == 0)
      return (NULL);

    HARDBUG(i >= (arg_size - 1))

    if ((arg_s[i++] = c) == '\n')
      break;
  }

  arg_s[i++] = '\0';

  return (arg_s);
}

local int read_from_hub(char arg_line[MY_LINE_MAX])
{
  PRINTF("waiting for input..\n");

  if (pgets(arg_line, MY_LINE_MAX) == NULL)
    return (INVALID);

  PRINTF("read_from_hub: %s", arg_line);

  return (TRUE);
}

local int hub_input(void) { return (compat_poll()); }

// A level can arrive before pos. Keep the GUI report until go think identifies
// the position, so a repeated search or a ponder does not consume a move.
typedef struct
{
  double time_left;
  double increment;
  int moves_left;
} hub_level_t;

local void read_clock_level(command_t *arg_command, hub_level_t *arg_level)
{
  arg_level->time_left = -1.0;
  arg_level->moves_left = INVALID;
  arg_level->increment = 0.0;

  for (int iarg = 0; iarg < arg_command->command_nargs; ++iarg)
  {
    arg_t *arg = &arg_command->command_args[iarg];
    HARDBUG(arg->arg_value == NULL)
    if (compat_strcasecmp(arg->arg_name, "moves") == 0)
    {
      HARDBUG(my_sscanf(arg->arg_value, "%d", &arg_level->moves_left) != 1)
      HARDBUG(arg_level->moves_left < 1)
    }
    else if (compat_strcasecmp(arg->arg_name, "time") == 0)
    {
      HARDBUG(my_sscanf(arg->arg_value, "%lf", &arg_level->time_left) != 1)
    }
    else if (compat_strcasecmp(arg->arg_name, "inc") == 0)
    {
      HARDBUG(my_sscanf(arg->arg_value, "%lf", &arg_level->increment) != 1)
    }
    else
      FATAL("unknown clock parameter", EXIT_FAILURE)
  }
  HARDBUG((!isfinite(arg_level->time_left)) or (arg_level->time_left < 0.0))
  HARDBUG((!isfinite(arg_level->increment)) or (arg_level->increment < 0.0))
}

void hub(void)
{
  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search = &search;

  string2board(&(with_search->S_board), STARTING_POSITION);

  game_state_t game_state;

  construct_game_state(&game_state);

  time_control_t time_control;
  construct_time_control(&time_control);
  hub_level_t level;
  int clock_active = FALSE;
  int level_pending = FALSE;
  int move_played = FALSE;
  hash_key_t last_key = 0;
  double default_time_limit = options.time_limit;

  while (TRUE)
  {
    char line[MY_LINE_MAX];

    if (read_from_hub(line) == INVALID)
      return;

    command_t command;

    line2command(line, &command);

    if (command.command == NULL)
      continue;

    if (compat_strcasecmp(command.command, "quit") == 0)
    {
      break;
    }
    else if (compat_strcasecmp(command.command, "new-game") == 0)
    {
      construct_time_control(&time_control);
      clock_active = FALSE;
      level_pending = FALSE;
      move_played = FALSE;
      options.time_limit = default_time_limit;
      options.time_ntrouble = 0;
    }
    else if (compat_strcasecmp(command.command, "pos") == 0)
    {
      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      char *start_board = command.command_args[0].arg_value;

      HARDBUG(start_board == NULL)

      HARDBUG(compat_strcasecmp(name, "pos") != 0)

      string2board(&(with_search->S_board), start_board);

      PRINTF("starting position\n");

      print_board(&(with_search->S_board));

      BSTRING(bfen)

      board2fen(&(with_search->S_board), bfen, FALSE);

      game_state.set_starting_position(&game_state, bdata(bfen));

      BDESTROY(bfen)

      while (game_state.pop_move(&game_state) > 0)
        ;

      if (command.command_nargs == 2)
      {
        name = command.command_args[1].arg_name;

        HARDBUG(compat_strcasecmp(name, "moves") != 0)

        BSTRING(b)

        bassigncstr(b, " ");

        BSTRING(bmoves)

        HARDBUG(bassigncstr(bmoves, command.command_args[1].arg_value) ==
                BSTR_ERR)

        struct bstrList *btokens;

        HARDBUG((btokens = bsplits(bmoves, b)) == NULL)

        PRINTF("got %d moves\n", btokens->qty);

        moves_list_t moves_list;

        construct_moves_list(&moves_list);

        for (int itoken = 0; itoken < btokens->qty; itoken++)
        {
          PRINTF("got move %s\n", bdata(btokens->entry[itoken]));

          gen_moves(&(with_search->S_board), &moves_list);

          int imove;

          HARDBUG((imove = search_move(&moves_list, btokens->entry[itoken])) ==
                  INVALID)

          game_state.push_move(&game_state,
                               bdata(btokens->entry[itoken]),
                               NULL);

          do_move(&(with_search->S_board), imove, &moves_list);
        }

        HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

        BDESTROY(bmoves)

        BDESTROY(b)

        PRINTF("current position\n");

        print_board(&(with_search->S_board));
      }

      continue;
    }
    else if (compat_strcasecmp(command.command, "level") == 0)
    {
      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      char *value = command.command_args[0].arg_value;

      if (compat_strcasecmp(name, "depth") == 0)
      {
        clock_active = FALSE;
        HARDBUG(value == NULL)

        int d;

        HARDBUG(my_sscanf(value, "%d", &d) != 1)

        HARDBUG(d < 1)
        HARDBUG(d >= DEPTH_MAX)

        PRINTF("got depth %d\n", d);

        options.depth_limit = d;
      }
      else if (compat_strcasecmp(name, "move-time") == 0)
      {
        clock_active = FALSE;
        HARDBUG(value == NULL)

        int t;

        HARDBUG(my_sscanf(value, "%d", &t) != 1)

        HARDBUG(t < 1)

        PRINTF("got move-time %d\n", t);

        options.time_limit = t;

        options.time_trouble[0] = options.time_limit / 2.0;
        options.time_trouble[1] = options.time_limit / 4.0;
        options.time_ntrouble = 2;
      }
      else if ((compat_strcasecmp(name, "moves") == 0) or
               (compat_strcasecmp(name, "time") == 0) or
               (compat_strcasecmp(name, "inc") == 0))
      {
        read_clock_level(&command, &level);
        clock_active = TRUE;
        level_pending = is_configured_time_control(&time_control);
        if (!level_pending)
          configure_time_control(level.time_left, level.moves_left, level.increment,
                                 &time_control);
      }
      else if (compat_strcasecmp(name, "infinite") == 0)
      {
        clock_active = FALSE;
        PRINTF("found infinite\n");

        options.time_limit = 10000.0;
        options.time_ntrouble = 0;
      }
      else
        PRINTF("ignoring %s\n", name);
    }
    else if (compat_strcasecmp(command.command, "go") == 0)
    {
      options.depth_limit = 64;

      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      HARDBUG(name == NULL)

      int clock_think = (compat_strcasecmp(name, "think") == 0) and clock_active;
      if (clock_think)
      {
        if (level_pending)
        {
          int played = move_played and (last_key != with_search->S_board.B_key);
          sync_time_control(level.time_left, level.moves_left, level.increment,
                            played, &time_control);
          level_pending = FALSE;
        }
        set_time_limit(&time_control, HUB_TIME_RESERVE);
        PRINTF("hub clock imove=%d moves_left=%d time=%.3f limit=%.6f\n",
               return_moves_done(&time_control), return_moves_left(&time_control),
               return_time_left(&time_control), options.time_limit);
      }
      else if (compat_strcasecmp(name, "ponder") == 0)
      {
        options.time_limit = 10000.0;
        options.time_ntrouble = 0;
      }
      else if (compat_strcasecmp(name, "analyze") == 0)
      {
        if (options.ponder)
          options.time_limit = 10000.0;
        else
        {
          options.time_limit = 1.0;
          options.depth_limit = 1.0;
        }

        options.time_ntrouble = 0;
      }

      print_board(&(with_search->S_board));

      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with_search->S_board), &moves_list);

      HARDBUG(moves_list.ML_nmoves == 0)

      check_moves(&(with_search->S_board), &moves_list);

      if (compat_strcasecmp(name, "mcts") == 0)
      {
        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_STATE,
                game_state.get_game_state(&game_state));

        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_MCTS,
                "hub/hub");

        // poll

        int abort = FALSE;
        message_t message;

        while (TRUE)
        {
          if (!abort and hub_input())
          {
            PRINTF("got input!\n");

            enqueue(return_thread_queue(thread_alpha_beta_master),
                    MESSAGE_ABORT_SEARCH,
                    "hub/hub");

            abort = TRUE;
          }

          if (dequeue(&main_queue, &message) == INVALID)
          {
            HARDBUG(compat_sleep(CENTI_SECOND) != 0)

            continue;
          }

          if (message.message_id == MESSAGE_INFO)
          {
            PRINTF("got info %s\n", bdata(message.message_text));

            char info[MY_LINE_MAX];

            snprintf(info,
                     MY_LINE_MAX,
                     "info %s\n",
                     bdata(message.message_text));

            write_to_hub(info);
          }
          else if (message.message_id == MESSAGE_RESULT)
          {
            PRINTF("got result %s\n", bdata(message.message_text));

            break;
          }
          else
            FATAL("message.message_id error", EXIT_FAILURE)
        } // while(TRUE)

        double result, lcb, ucb;

        HARDBUG(my_sscanf(bdata(message.message_text),
                          "%lf%lf%lf",
                          &result,
                          &lcb,
                          &ucb) != 3)

        snprintf(line,
                 MY_LINE_MAX,
                 "done result=%.6f"
                 " lcb=%.6f"
                 " ucb=%.6f"
                 " lcb-ucb=%.6f\n",
                 result,
                 lcb,
                 ucb,
                 lcb - ucb);

        write_to_hub(line);
      }
      else
      {
        // think, ponder or analyze

        BSTRING(bmove_string)

        if (options.use_book)
          return_book_move(&(with_search->S_board), &moves_list, bmove_string);

        HARDBUG(bassigncstr(bmove_string, "NULL") == BSTR_ERR)

        if ((compat_strcasecmp(name, "think") == 0) and
            (moves_list.ML_nmoves == 1))
        {
          move2bstring(&moves_list, 0, bmove_string);
        }
        else
        {
          if ((compat_strcasecmp(name, "think") == 0) and options.use_book)
            return_book_move(&(with_search->S_board),
                             &moves_list,
                             bmove_string);

          if (compat_strcasecmp(bdata(bmove_string), "NULL") == 0)
          {
            enqueue(return_thread_queue(thread_alpha_beta_master),
                    MESSAGE_STATE,
                    game_state.get_game_state(&game_state));

            enqueue(return_thread_queue(thread_alpha_beta_master),
                    MESSAGE_GO,
                    "hub/hub");

            int abort = FALSE;
            message_t message;

            while (TRUE)
            {
              if (!abort and hub_input())
              {
                PRINTF("got input!\n");

                enqueue(return_thread_queue(thread_alpha_beta_master),
                        MESSAGE_ABORT_SEARCH,
                        "hub/hub");

                abort = TRUE;
              }

              if (dequeue(&main_queue, &message) == INVALID)
              {
                HARDBUG(compat_sleep(CENTI_SECOND) != 0)

                continue;
              }

              if (message.message_id == MESSAGE_INFO)
              {
                PRINTF("got info %s\n", bdata(message.message_text));

                char info[MY_LINE_MAX];

                snprintf(info,
                         MY_LINE_MAX,
                         "info %s\n",
                         bdata(message.message_text));

                write_to_hub(info);
              }
              else if (message.message_id == MESSAGE_RESULT)
              {
                PRINTF("got result %s\n", bdata(message.message_text));

                break;
              }
              else
                FATAL("message.message_id error", EXIT_FAILURE)
            } // while(TRUE)

            CSTRING(cmove_string, blength(message.message_text))

            HARDBUG(
              my_sscanf(bdata(message.message_text), "%s", cmove_string) != 1)

            HARDBUG(bassigncstr(bmove_string, cmove_string) == BSTR_ERR)

            CDESTROY(cmove_string)
          }
        }

        HARDBUG(compat_strcasecmp(bdata(bmove_string), "NULL") == 0)

        snprintf(line, MY_LINE_MAX, "done move=%s\n", bdata(bmove_string));

        write_to_hub(line);

        if (clock_think)
        {
          move_played = TRUE;
          last_key = with_search->S_board.B_key;
        }

        BDESTROY(bmove_string)
      }
    }
    else if (compat_strcasecmp(command.command, "ping") == 0)
    {
      snprintf(line, MY_LINE_MAX, "pong\n");

      write_to_hub(line);
    }
    else
    {
      PRINTF("ignoring command %s\n", command.command);
    }
  }
  destroy_game_state(&game_state);
}

void init_hub(void)
{
  char line[MY_LINE_MAX];

  PRINTF("waiting for 'hub'..\n");

  if (read_from_hub(line) == INVALID)
    return;

  command_t command;

  line2command(line, &command);

  HARDBUG(command.command == NULL)

  HARDBUG(compat_strcasecmp(command.command, "hub") != 0)

  snprintf(line,
           MY_LINE_MAX,
           "id name=Gwd version=%s author=\"GW KB\" country=NL\n",
           options.hub_version);

  write_to_hub(line);

  cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

  HARDBUG(!cJSON_IsArray(parameters))

  // loop over parameters

  cJSON *parameter;

  cJSON_ArrayForEach(parameter, parameters)
  {
    cJSON *cjson_ui = cJSON_GetObjectItem(parameter, "ui");

    if (cjson_ui == NULL)
      continue;

    HARDBUG(!cJSON_IsString(cjson_ui))

    char *ui = cJSON_GetStringValue(cjson_ui);

    if (compat_strcasecmp(ui, "false") == 0)
      continue;

    cJSON *cjson_name = cJSON_GetObjectItem(parameter, "name");

    char *parameter_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(parameter_name == NULL)

    cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

    char *parameter_type = cJSON_GetStringValue(cjson_type);

    HARDBUG(parameter_type == NULL)

    if (compat_strcasecmp(parameter_type, "int") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      int value = round(cJSON_GetNumberValue(cjson_value));

      snprintf(line,
               MY_LINE_MAX,
               "param name=%s type=int value=%d\n",
               ui,
               value);
    }
    else if (compat_strcasecmp(parameter_type, "double") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      double value = cJSON_GetNumberValue(cjson_value);

      snprintf(line,
               MY_LINE_MAX,
               "param name=%s type=real value=%.2f\n",
               ui,
               value);
    }
    else if (compat_strcasecmp(parameter_type, "string") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      char *string = cJSON_GetStringValue(cjson_value);

      HARDBUG(string == NULL)

      snprintf(line,
               MY_LINE_MAX,
               "param name=%s type=string value=\"%s\"\n",
               ui,
               string);
    }
    else if (compat_strcasecmp(parameter_type, "bool") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsBool(cjson_value))

      if (cJSON_IsFalse(cjson_value))
        snprintf(line,
                 MY_LINE_MAX,
                 "param name=%s type=bool value=false\n",
                 ui);
      else
        snprintf(line, MY_LINE_MAX, "param name=%s type=bool value=true\n", ui);
    }
    else
      FATAL("unknown parameter_type", EXIT_FAILURE)

    write_to_hub(line);
  }
  write_to_hub("wait\n");

  while (TRUE)
  {
    PRINTF("waiting for 'set-param' or 'init'..\n");

    HARDBUG(read_from_hub(line) == INVALID)

    PRINTF("got line: %s\n", line);

    line2command(line, &command);

    if (command.command == NULL)
      continue;

    if (compat_strcasecmp(command.command, "init") == 0)
    {
      write_to_hub("ready\n");
      break;
    }

    HARDBUG(compat_strcasecmp(command.command, "set-param") != 0)

    HARDBUG(command.command_nargs < 2)

    HARDBUG(compat_strcasecmp(command.command_args[0].arg_name, "name") != 0)

    char *name = command.command_args[0].arg_value;

    HARDBUG(compat_strcasecmp(command.command_args[1].arg_name, "value") != 0)

    char *value = command.command_args[1].arg_value;

    PRINTF("name=%s value=%s\n", name, value);

    cJSON_ArrayForEach(parameter, parameters)
    {
      cJSON *cjson_ui = cJSON_GetObjectItem(parameter, "ui");

      if (cjson_ui == NULL)
        continue;

      HARDBUG(!cJSON_IsString(cjson_ui))

      char *ui = cJSON_GetStringValue(cjson_ui);

      if (compat_strcasecmp(ui, name) != 0)
        continue;

      cJSON *cjson_name = cJSON_GetObjectItem(parameter, "name");

      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      PRINTF("ui=%s parameter_name=%s\n", ui, parameter_name);

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

      char *parameter_type = cJSON_GetStringValue(cjson_type);

      HARDBUG(parameter_type == NULL)

      if (compat_strcasecmp(parameter_type, "int") == 0)
      {
        int ivalue;

        HARDBUG(my_sscanf(value, "%d", &ivalue) != 1)

        PRINTF("ivalue=%d\n", ivalue);

        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, ivalue);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (compat_strcasecmp(parameter_type, "double") == 0)
      {
        double dvalue;

        HARDBUG(my_sscanf(value, "%lf", &dvalue) != 1)

        PRINTF("dvalue=%.2f\n", dvalue);

        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, dvalue);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (compat_strcasecmp(parameter_type, "string") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsString(cjson_value))

        cJSON_SetValuestring(cjson_value, value);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (compat_strcasecmp(parameter_type, "bool") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsBool(cjson_value))

        if (compat_strcasecmp(value, "false") == 0)
          cJSON_SetBoolValue(cjson_value, 0);
        else
          cJSON_SetBoolValue(cjson_value, 1);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else
        FATAL("unknown parameter_type", EXIT_FAILURE)
    }
  }
}
