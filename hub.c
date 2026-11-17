// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
#include "globals.h"

#define NARGS_MAX 256
#define HUB_TIME_RESERVE 0.050 // seconds for getting the move back to the GUI

#define HUB_WDL_CLOCK_SECONDS 1.0

// Negative means an ordinary search, zero waits for a hit, positive starts the
// move clock.
local _Atomic double ponder_start = -1.0;

local double monotonic_seconds(void)
{
  struct timespec now;

  HARDBUG(compat_monotonic(&now) != 0)

  return (double)now.tv_sec + now.tv_nsec * 1e-9;
}

double search_clock_used(my_timer_t *arg_timer)
{
  if (options.hub)
  {
    double start = atomic_load_explicit(&ponder_start, memory_order_relaxed);

    if (start == 0.0)
      return -1.0;

    if (start > 0.0)
      return monotonic_seconds() - start;
  }

  return return_my_timer(arg_timer, FALSE);
}

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

local void write_to_hub(const char *arg_line)
{
  int nline = strlen(arg_line);

  PRINTF("write_to_hub: %d:%s", nline, arg_line);

  HARDBUG(arg_line[nline - 1] != '\n')

  HARDBUG(compat_write(1, arg_line, nline) != nline)
}

// Keep a partial command across polls so a short read cannot stall a search.
local int read_hub_line(bstring arg_line, bstring arg_input, int arg_wait)
{
  while (arg_wait or compat_poll())
  {
    char c;

    if (compat_read(0, &c, 1) <= 0)
    {
      HARDBUG(btrunc(arg_input, 0) == BSTR_ERR)

      return INVALID;
    }

    HARDBUG(bconchar(arg_input, c) == BSTR_ERR)

    if (c == '\n')
    {
      HARDBUG(bassign(arg_line, arg_input) == BSTR_ERR)
      HARDBUG(btrunc(arg_input, 0) == BSTR_ERR)

      PRINTF("read_from_hub: %s", bdata(arg_line));

      return TRUE;
    }
  }

  return FALSE;
}

local int read_from_hub(bstring arg_line, bstring arg_input)
{
  return read_hub_line(arg_line, arg_input, TRUE);
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

    if (compat_strcasecmp(arg->arg_name, "moves") == 0)
    {
      HARDBUG(arg->arg_value == NULL)
      HARDBUG(my_sscanf(arg->arg_value, "%d", &arg_level->moves_left) != 1)
      HARDBUG(arg_level->moves_left < 1)
    }
    else if (compat_strcasecmp(arg->arg_name, "time") == 0)
    {
      HARDBUG(arg->arg_value == NULL)
      HARDBUG(my_sscanf(arg->arg_value, "%lf", &arg_level->time_left) != 1)
    }
    else if (compat_strcasecmp(arg->arg_name, "inc") == 0)
    {
      HARDBUG(arg->arg_value == NULL)
      HARDBUG(my_sscanf(arg->arg_value, "%lf", &arg_level->increment) != 1)
    }
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
  int fixed_move_time = TRUE;
  int level_pending = FALSE;
  int move_played = FALSE;
  hash_key_t last_key = 0;
  int default_depth_limit = options.depth_limit;
  int level_depth = INVALID;
  double default_time_limit = options.time_limit;

  BSTRING(line)
  BSTRING(pending_line)
  BSTRING(input)
  BSTRING(partial_line)

  write_to_hub("ready\n");

  while (TRUE)
  {
    if (bdata(pending_line)[0] != '\0')
    {
      HARDBUG(bassign(line, pending_line) == BSTR_ERR)
      HARDBUG(btrunc(pending_line, 0) == BSTR_ERR)
    }
    else if (read_from_hub(line, partial_line) == INVALID)
      goto label_return;

    command_t command;

    line2command(bdata(line), &command);

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
      fixed_move_time = TRUE;
      level_pending = FALSE;
      move_played = FALSE;
      level_depth = INVALID;
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

      for (int iarg = 0; iarg < command.command_nargs; ++iarg)
      {
        char *name = command.command_args[iarg].arg_name;

        char *value = command.command_args[iarg].arg_value;

        if (compat_strcasecmp(name, "depth") == 0)
        {
          clock_active = FALSE;
          fixed_move_time = FALSE;

          HARDBUG(value == NULL)

          int d;

          HARDBUG(my_sscanf(value, "%d", &d) != 1)

          HARDBUG(d < 1)
          HARDBUG(d >= DEPTH_MAX)

          PRINTF("got depth %d\n", d);

          level_depth = d;
          options.time_limit = 10000.0;
          options.time_ntrouble = 0;
        }
        else if (compat_strcasecmp(name, "move-time") == 0)
        {
          clock_active = FALSE;
          fixed_move_time = TRUE;

          HARDBUG(value == NULL)

          double t;

          HARDBUG(my_sscanf(value, "%lf", &t) != 1)

          HARDBUG((!isfinite(t)) or (t <= 0.0))

          PRINTF("got move-time %g\n", t);

          level_depth = INVALID;
          options.time_limit = t;

          options.time_trouble[0] = options.time_limit / 2.0;
          options.time_trouble[1] = options.time_limit / 4.0;
          options.time_ntrouble = 2;
        }
        else if ((compat_strcasecmp(name, "moves") == 0) or
                 (compat_strcasecmp(name, "time") == 0) or
                 (compat_strcasecmp(name, "inc") == 0))
        {
          level_depth = INVALID;
          read_clock_level(&command, &level);
          fixed_move_time = FALSE;
          clock_active = TRUE;
          level_pending = is_configured_time_control(&time_control);

          if (!level_pending)
            configure_time_control(level.time_left,
                                   level.moves_left,
                                   level.increment,
                                   TIME_HORIZON_EXACT,
                                   &time_control);
        }
        else if (compat_strcasecmp(name, "infinite") == 0)
        {
          clock_active = FALSE;
          level_depth = INVALID;
          fixed_move_time = FALSE;
          PRINTF("found infinite\n");

          options.time_limit = 10000.0;
          options.time_ntrouble = 0;
        }
        else
          continue;

        break;
      }
    }
    else if (compat_strcasecmp(command.command, "go") == 0)
    {
      options.depth_limit =
        (level_depth == INVALID) ? default_depth_limit : level_depth;

      double saved_time_limit = options.time_limit;
      int saved_time_ntrouble = options.time_ntrouble;

      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      HARDBUG(name == NULL)

      int clock_think =
        (compat_strcasecmp(name, "think") == 0) and clock_active;

      int pondering = (compat_strcasecmp(name, "ponder") == 0);
      double time_left = -1.0;

      if (fixed_move_time and
          ((compat_strcasecmp(name, "think") == 0) or pondering))
        time_left = options.time_limit;

      time_control_t ponder_control;

      if (clock_think or (pondering and clock_active))
      {
        time_control_t *control = &time_control;

        if (pondering)
        {
          // Commit the predicted position's clock report only on a hit.
          ponder_control = time_control;
          control = &ponder_control;
        }

        if (level_pending)
        {
          int played = move_played and (last_key != with_search->S_board.B_key);

          sync_time_control(level.time_left,
                            level.moves_left,
                            level.increment,
                            played,
                            control);

          if (!pondering)
            level_pending = FALSE;
        }
        time_left = return_time_left(control);
        set_time_limit(control, HUB_TIME_RESERVE);
        PRINTF("hub clock imove=%d moves_left=%d time=%.3f limit=%.6f\n",
               return_moves_done(control),
               return_moves_left(control),
               return_time_left(control),
               options.time_limit);
      }
      else if (compat_strcasecmp(name, "analyze") == 0)
      {
        if (options.ponder or (level_depth != INVALID))
          options.time_limit = 10000.0;
        else
        {
          options.time_limit = 1.0;
          options.depth_limit = 1;
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

            BSTRING(info)

            HARDBUG(bassignformat(info,
                                  "info %s\n",
                                  bdata(message.message_text)) == BSTR_ERR)

            write_to_hub(bdata(info));

            BDESTROY(info)
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

        HARDBUG(bassignformat(line,
                              "done result=%.6f"
                              " lcb=%.6f"
                              " ucb=%.6f"
                              " lcb-ucb=%.6f\n",
                              result,
                              lcb,
                              ucb,
                              lcb - ucb) == BSTR_ERR)

        write_to_hub(bdata(line));
      }
      else
      {
        // think, ponder or analyze

        BSTRING(bmove_string)
        BSTRING(bponder_string)

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
            if (pondering)
              atomic_store_explicit(&ponder_start, 0.0, memory_order_relaxed);

            enqueue(return_thread_queue(thread_alpha_beta_master),
                    MESSAGE_STATE,
                    game_state.get_game_state(&game_state));

            int go = MESSAGE_GO;

            if (time_left >= 0.0)
              go = (time_left <= HUB_WDL_CLOCK_SECONDS) ? MESSAGE_GO_SHORT_CLOCK
                                                        : MESSAGE_GO_TIMED;

            enqueue(return_thread_queue(thread_alpha_beta_master),
                    go,
                    "hub/hub");

            int abort = FALSE;
            int result_ready = FALSE;
            message_t message;

            while (TRUE)
            {
              int received =
                abort ? FALSE
                      : read_hub_line(input, partial_line, result_ready);

              if (received != FALSE)
              {
                command_t incoming;

                incoming.command = NULL;

                if (received != INVALID)
                {
                  HARDBUG(bassign(pending_line, input) == BSTR_ERR)

                  line2command(bdata(input), &incoming);
                }

                if ((incoming.command != NULL) and
                    (compat_strcasecmp(incoming.command, "ponder-hit") == 0))
                {
                  HARDBUG(btrunc(pending_line, 0) == BSTR_ERR)

                  if (pondering)
                  {
                    atomic_store_explicit(&ponder_start,
                                          monotonic_seconds(),
                                          memory_order_relaxed);
                    pondering = FALSE;
                    clock_think = clock_active;

                    if (clock_active)
                    {
                      time_control = ponder_control;
                      level_pending = FALSE;
                    }
                    PRINTF("ponder-hit\n");
                  }
                }
                else if ((incoming.command != NULL) and
                         (compat_strcasecmp(incoming.command, "ping") == 0))
                {
                  HARDBUG(btrunc(pending_line, 0) == BSTR_ERR)

                  write_to_hub("pong\n");
                }
                else
                {
                  if (!result_ready)
                    enqueue(return_thread_queue(thread_alpha_beta_master),
                            MESSAGE_ABORT_SEARCH,
                            "hub/hub");
                  abort = TRUE;
                }
              }

              if (result_ready)
              {
                if (pondering and !abort)
                  continue;

                break;
              }

              if (dequeue(&main_queue, &message) == INVALID)
              {
                HARDBUG(compat_sleep(CENTI_SECOND) != 0)

                continue;
              }

              if (message.message_id == MESSAGE_INFO)
              {
                PRINTF("got info %s\n", bdata(message.message_text));

                BSTRING(info)

                HARDBUG(bassignformat(info,
                                      "info %s\n",
                                      bdata(message.message_text)) == BSTR_ERR)

                write_to_hub(bdata(info));

                BDESTROY(info)
              }
              else if (message.message_id == MESSAGE_RESULT)
              {
                PRINTF("got result %s\n", bdata(message.message_text));
                result_ready = TRUE;

                if ((!pondering) or abort)
                  break;
              }
              else
                FATAL("message.message_id error", EXIT_FAILURE)
            } // while(TRUE)

            atomic_store_explicit(&ponder_start, -1.0, memory_order_relaxed);

            char *reply = strstr(bdata(message.message_text), " ponder=");

            if (reply != NULL)
              HARDBUG(bassigncstr(bponder_string, reply) == BSTR_ERR)

            CSTRING(cmove_string, blength(message.message_text))

            HARDBUG(
              my_sscanf(bdata(message.message_text), "%s", cmove_string) != 1)

            HARDBUG(bassigncstr(bmove_string, cmove_string) == BSTR_ERR)

            CDESTROY(cmove_string)
          }
        }

        HARDBUG(compat_strcasecmp(bdata(bmove_string), "NULL") == 0)

        HARDBUG(bassignformat(line,
                              "done move=%s%s\n",
                              bdata(bmove_string),
                              bdata(bponder_string)) == BSTR_ERR)

        write_to_hub(bdata(line));

        if (clock_think)
        {
          move_played = TRUE;
          last_key = with_search->S_board.B_key;
        }

        BDESTROY(bponder_string)

        BDESTROY(bmove_string)
      }

      if (!clock_think)
      {
        options.time_limit = saved_time_limit;
        options.time_ntrouble = saved_time_ntrouble;
      }
    }
    else if (compat_strcasecmp(command.command, "ping") == 0)
    {
      HARDBUG(bassignformat(line, "pong\n") == BSTR_ERR)

      write_to_hub(bdata(line));
    }
    else
    {
      PRINTF("ignoring command %s\n", command.command);
    }
  }
label_return:
  BDESTROY(partial_line)

  BDESTROY(input)

  BDESTROY(pending_line)

  BDESTROY(line)

  destroy_game_state(&game_state);
}

void init_hub(void)
{
  BSTRING(line)
  BSTRING(partial_line)

  PRINTF("waiting for 'hub'..\n");

  if (read_from_hub(line, partial_line) == INVALID)
    goto label_return;

  command_t command;

  line2command(bdata(line), &command);

  HARDBUG(command.command == NULL)

  HARDBUG(compat_strcasecmp(command.command, "hub") != 0)

  HARDBUG(bassignformat(line,
                        "id name=Gwd version=%s author=\"GW KB\" country=NL\n",
                        bdata(options.hub_version)) == BSTR_ERR)

  write_to_hub(bdata(line));

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

      HARDBUG(
        bassignformat(line, "param name=%s type=int value=%d\n", ui, value) ==
        BSTR_ERR)
    }
    else if (compat_strcasecmp(parameter_type, "double") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      double value = cJSON_GetNumberValue(cjson_value);

      HARDBUG(bassignformat(line,
                            "param name=%s type=real value=%.2f\n",
                            ui,
                            value) == BSTR_ERR)
    }
    else if (compat_strcasecmp(parameter_type, "string") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      char *string = cJSON_GetStringValue(cjson_value);

      HARDBUG(string == NULL)

      HARDBUG(bassignformat(line,
                            "param name=%s type=string value=\"%s\"\n",
                            ui,
                            string) == BSTR_ERR)
    }
    else if (compat_strcasecmp(parameter_type, "bool") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsBool(cjson_value))

      if (cJSON_IsFalse(cjson_value))
        HARDBUG(bassignformat(line,
                              "param name=%s type=bool value=false\n",
                              ui) == BSTR_ERR)
      else
        HARDBUG(bassignformat(line,
                              "param name=%s type=bool value=true\n",
                              ui) == BSTR_ERR)
    }
    else
      FATAL("unknown parameter_type", EXIT_FAILURE)

    write_to_hub(bdata(line));
  }
  write_to_hub("wait\n");

  while (TRUE)
  {
    PRINTF("waiting for 'set-param' or 'init'..\n");

    HARDBUG(read_from_hub(line, partial_line) == INVALID)

    PRINTF("got line: %s\n", bdata(line));

    line2command(bdata(line), &command);

    if (command.command == NULL)
      continue;

    if (compat_strcasecmp(command.command, "init") == 0)
      break;

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
label_return:
  BDESTROY(partial_line)

  BDESTROY(line)
}
