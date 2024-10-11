//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#define GAME_MOVES 40

#define NARGS_MAX 256

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

local void line2command(char *line, command_t *with)
{
  with->command = NULL;
  with->command_nargs = 0;

  char *c = line;

  //command

  while(isspace(*c)) c++;
  if (*c == '\0') return;

  with->command = c;

  while(isgraph(*c)) c++;

  if (*c == '\0') return;

  *c = '\0';

  c++;

  PRINTF("command=%s\n", with->command);

  //params

  while(TRUE)
  {
    while(isspace(*c)) c++;

    if (*c == '\0') return;

    HARDBUG(with->command_nargs >= NARGS_MAX)

    arg_t *with_param = with->command_args + with->command_nargs;

    with_param->arg_name = c;
    with_param->arg_value = NULL;

    while(isgraph(*c) and (*c != '=')) c++;

    if (isspace(*c)) {*c = '\0'; c++; while(isspace(*c)) c++;}

    if (*c != '=')
    {
      PRINTF("arg_name=%s\n", with_param->arg_name);
    }
    else
    {
      *c = '\0'; c++;

      PRINTF("arg_name=%s\n", with_param->arg_name);

      while(isspace(*c)) c++;
      HARDBUG(*c == '\0')

      if (*c == '"')
      {
        c++;
  
        with_param->arg_value = c;
        
        while((*c != '\0') and (*c != '"')) c++;
        HARDBUG(*c == '\0'); *c = '\0'; c++;
      } 
      else
      {
        with_param->arg_value = c;
  
        while(isgraph(*c)) c++;

        if (*c == '\0') return;

        *c = '\0';

        c++;
      }
      PRINTF("arg_value=%s\n", with_param->arg_value);
    }
    with->command_nargs++;
  }
}

local void write_to_hub(char *line)
{
  int nline = strlen(line);

  PRINTF("write_to_hub: %d:%s", nline, line);

  HARDBUG(line[nline - 1] != '\n')

  HARDBUG(my_write(1, line, nline) != nline)
}

local char *pgets(char *s, int size, pipe_t pfd)
{
  //read pipe until '\n'

  int i = 0;

  while(TRUE)
  {
    int n;
    char c;

    //error 

    if (pfd == 0)
    {
      if ((n = my_read(0, &c, 1)) == INVALID) return(NULL);
    }
    else
    {
      if ((n = my_pipe_read(pfd, &c, 1)) == INVALID) return(NULL);
    }

    //eof

    if (n == 0) return(NULL);

    HARDBUG(i >= (size - 1))

    if ((s[i++] = c) == '\n') break;
  }

  s[i++] = '\0';

  return(s);
}

local int read_from_hub(char line[MY_LINE_MAX])
{
  PRINTF("waiting for input..\n");

  if (pgets(line, MY_LINE_MAX, 0) == NULL) return(INVALID);

  PRINTF("read_from_hub: %s", line);

  return(TRUE);
}

local int hub_input(void)
{
  return(my_poll());
}

void hub(void)
{
  int iboard = create_board(STDOUT, INVALID);

  string2board(STARTING_POSITION, iboard);

  state_t *game = state_class->objects_ctor();

  while(TRUE)
  {
    char line[MY_LINE_MAX];

    if (read_from_hub(line) == INVALID) return;

    command_t command;

    line2command(line, &command);

    if (command.command == NULL) continue;

    if (my_strcasecmp(command.command, "quit") == 0)
    { 
      break;
    } 
    else if (my_strcasecmp(command.command, "pos") == 0)
    {
      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      char *start_board = command.command_args[0].arg_value;

      HARDBUG(start_board == NULL)

      HARDBUG(my_strcasecmp(name, "pos") != 0)

      string2board(start_board, iboard);

      print_board(iboard);

      char fen[MY_LINE_MAX];

      board2fen(iboard, fen, FALSE);

      game->set_starting_position(game, fen);

      while(game->pop_move(game) > 0);

      if (command.command_nargs == 2)
      {
        name = command.command_args[1].arg_name;

        char *moves = command.command_args[1].arg_value;

        HARDBUG(my_strcasecmp(name, "moves") != 0)

        int nmoves = 0;

        while(TRUE)
        {
          while(isspace(*moves)) moves++;

          if (*moves == '\0') break;

          char *move_string = moves;

          while(isgraph(*moves)) moves++;

          if (*moves == ' ') {*moves = '\0'; moves++;}

          PRINTF("got move %s\n", move_string);

          game->push_move(game, move_string, NULL);

          nmoves++;
        }
        PRINTF("got %d moves\n", nmoves);
      }
      continue;
    }
    else if (my_strcasecmp(command.command, "level") == 0)
    {
      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      char *value = command.command_args[0].arg_value;

      if (my_strcasecmp(name, "depth") == 0)
      {
        HARDBUG(value == NULL)

        int d;
  
        HARDBUG(sscanf(value, "%d", &d) != 1)
 
        HARDBUG(d < 1)
        HARDBUG(d >= DEPTH_MAX)
  
        PRINTF("got depth %d\n", d);

        options.depth_limit = d;
      }
      else if (my_strcasecmp(name, "move-time") == 0)
      {
        HARDBUG(value == NULL)

        int t;
   
        HARDBUG(sscanf(value, "%d", &t) != 1)
  
        HARDBUG(t < 1)

        PRINTF("got move-time %d\n", t);

        options.time_limit = t;
 
        options.time_trouble[0] = options.time_limit / 2.0;
        options.time_trouble[1] = options.time_limit / 4.0;
        options.time_ntrouble = 2;
      }
      else if (my_strcasecmp(name, "moves") == 0)
      {
        HARDBUG(value == NULL)

        int moves_left;
   
        HARDBUG(sscanf(value, "%d", &moves_left) != 1)

        HARDBUG(command.command_nargs < 2)

        name = command.command_args[1].arg_name;

        value = command.command_args[1].arg_value;
   
        HARDBUG(my_strcasecmp(name, "time") != 0)
  
        HARDBUG(value == NULL)

        int time_left;
   
        HARDBUG(sscanf(value, "%d", &time_left) != 1)
 
        PRINTF("got moves %d time_left %d\n", moves_left, time_left);
        
        options.time_limit = time_left / moves_left;

        if (options.time_limit <= 0.1) options.time_limit = 0.1;

        options.time_trouble[0] =
          (time_left - options.time_limit) / moves_left;

        if (options.time_trouble[0] <= 0.1) options.time_trouble[0] = 0.1;

        options.time_trouble[1] =
          (time_left - options.time_limit - options.time_trouble[0]) /
          moves_left;
    
        if (options.time_trouble[1] <= 0.1) options.time_trouble[1] = 0.1;

        options.time_ntrouble = 2;
      }
      else if (my_strcasecmp(name, "time") == 0)
      {
        HARDBUG(value == NULL)

        double t;
   
        HARDBUG(sscanf(value, "%lf", &t) != 1)
  
        HARDBUG(t < 0.0)

        PRINTF("found time %.2f\n", t);

        options.time_limit = t / GAME_MOVES;

        if (options.time_limit <= 0.1) options.time_limit = 0.1;

        options.time_trouble[0] =
          (t - options.time_limit) / GAME_MOVES;

        if (options.time_trouble[0] <= 0.1) options.time_trouble[0] = 0.1;

        options.time_trouble[1] =
          (t - options.time_limit - options.time_trouble[0]) / GAME_MOVES;
    
        if (options.time_trouble[1] <= 0.1) options.time_trouble[1] = 0.1;

        options.time_ntrouble = 2;
      }
      else if (my_strcasecmp(name, "infinite") == 0)
      {
        PRINTF("found infinite\n");

        options.time_limit = 10000.0;
        options.time_ntrouble = 0;
      }
      else
        PRINTF("ignoring %s\n", name);
    }
    else if (my_strcasecmp(command.command, "go") == 0)
    {
      options.depth_limit = 64;

      HARDBUG(command.command_nargs < 1)

      char *name = command.command_args[0].arg_name;

      HARDBUG(name == NULL)

      if (my_strcasecmp(name, "ponder") == 0)
      {
        options.time_limit = 10000.0;
        options.time_ntrouble = 0;
      }
      else if (my_strcasecmp(name, "analyze") == 0)
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

      print_board(iboard);

      moves_list_t moves_list;

      create_moves_list(&moves_list);

      board_t *with_board = return_with_board(iboard);

      gen_moves(with_board, &moves_list, FALSE);

      HARDBUG(moves_list.nmoves == 0)

      check_moves(with_board, &moves_list);

      char move_string[MY_LINE_MAX];

      strcpy(move_string, "NULL");

      if ((my_strcasecmp(name, "think") == 0) and (moves_list.nmoves == 1))
      {
        strcpy(move_string, moves_list.move2string(&moves_list, 0));
      }
      else
      {
        if ((my_strcasecmp(name, "think") == 0) and options.use_book)
          return_book_move(with_board, &moves_list, move_string);

        if (my_strcasecmp(move_string, "NULL") == 0)
        {
          enqueue(return_thread_queue(thread_alpha_beta_master_id),
            MESSAGE_STATE, game->get_state(game));

          enqueue(return_thread_queue(thread_alpha_beta_master_id),
            MESSAGE_GO, "hub/hub");

          int abort = FALSE;
          message_t message;
  
          while(TRUE)
          {
            if (!abort and hub_input())
            {
              PRINTF("got input!\n");
  
              enqueue(return_thread_queue(thread_alpha_beta_master_id),
                MESSAGE_ABORT_SEARCH, "hub/hub");
  
              abort = TRUE;
            }
  
            if (dequeue(&main_queue, &message) == INVALID)
            {
              my_sleep(CENTI_SECOND);
       
              continue;
            }
  
            if (message.message_id == MESSAGE_INFO)
            {
              PRINTF("got info %s\n", bdata(message.message_text));
  
              char info[MY_LINE_MAX];
  
              snprintf(info, MY_LINE_MAX, "info %s\n",
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
          }//while(TRUE)

          HARDBUG(sscanf(bdata(message.message_text), "%s", move_string) != 1)
        }
      }

      HARDBUG(my_strcasecmp(move_string, "NULL") == 0)

      snprintf(line, MY_LINE_MAX, "done move=%s\n", move_string);
  
      write_to_hub(line);
    }
    else if (my_strcasecmp(command.command, "ping") == 0)
    {
      snprintf(line, MY_LINE_MAX, "pong\n");

      write_to_hub(line);
    }
    else
    {
      PRINTF("ignoring command %s\n", command.command);
    }
  }
  state_class->objects_dtor(game);
}

void init_hub(void)
{
  char line[MY_LINE_MAX];

  PRINTF("waiting for 'hub'..\n");

  if (read_from_hub(line) == INVALID) return;

  command_t command;

  line2command(line, &command);

  HARDBUG(command.command == NULL)

  HARDBUG(my_strcasecmp(command.command, "hub") != 0)

  snprintf(line, MY_LINE_MAX, 
    "id name=Gwd version=%s author=\"GW KB\" country=NL\n",
    options.hub_version);

  write_to_hub(line);

  cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
  HARDBUG(!cJSON_IsArray(parameters))

  //loop over parameters

  cJSON *parameter;

  cJSON_ArrayForEach(parameter, parameters)
  {
    cJSON *cjson_ui = cJSON_GetObjectItem(parameter, "ui");

    if (cjson_ui == NULL) continue;

    HARDBUG(!cJSON_IsString(cjson_ui))

    char *ui = cJSON_GetStringValue(cjson_ui);

    if (my_strcasecmp(ui, "false") == 0) continue;

    cJSON *cjson_name = cJSON_GetObjectItem(parameter, "name");

    char *parameter_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(parameter_name == NULL)

    cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");
  
    char *parameter_type = cJSON_GetStringValue(cjson_type);
    
    HARDBUG(parameter_type == NULL)

    if (my_strcasecmp(parameter_type, "int") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      int value = round(cJSON_GetNumberValue(cjson_value));

      snprintf(line, MY_LINE_MAX, "param name=%s type=int value=%d\n",
        ui, value);
    }
    else if (my_strcasecmp(parameter_type, "double") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      double value = cJSON_GetNumberValue(cjson_value);

      snprintf(line, MY_LINE_MAX, "param name=%s type=real value=%.2f\n",
        ui, value);
    }
    else if (my_strcasecmp(parameter_type, "string") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      char *string = cJSON_GetStringValue(cjson_value);

      HARDBUG(string == NULL)

      snprintf(line, MY_LINE_MAX, "param name=%s type=string value=\"%s\"\n",
        ui, string);
    }
    else if (my_strcasecmp(parameter_type, "bool") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsBool(cjson_value))

      if (cJSON_IsFalse(cjson_value))
        snprintf(line, MY_LINE_MAX, "param name=%s type=bool value=false\n",
          ui);
      else
        snprintf(line, MY_LINE_MAX, "param name=%s type=bool value=true\n",
          ui);
    }
    else
      FATAL("unknown parameter_type", EXIT_FAILURE)

    write_to_hub(line);
  }
  write_to_hub("wait\n");

  while(TRUE)
  {
    PRINTF("waiting for 'set-param' or 'init'..\n");

    HARDBUG(read_from_hub(line) == INVALID)

    PRINTF("got line: %s\n", line);

    line2command(line, &command);

    if (command.command == NULL) continue;

    if (my_strcasecmp(command.command, "init") == 0)
    {
      write_to_hub("ready\n");
      break;
    }

    HARDBUG(my_strcasecmp(command.command, "set-param") != 0)
    
    HARDBUG(command.command_nargs < 2)

    HARDBUG(my_strcasecmp(command.command_args[0].arg_name, "name") != 0)

    char *name = command.command_args[0].arg_value;

    HARDBUG(my_strcasecmp(command.command_args[1].arg_name, "value") != 0)

    char *value = command.command_args[1].arg_value;

    PRINTF("name=%s value=%s\n", name, value);

    cJSON_ArrayForEach(parameter, parameters)
    {
      cJSON *cjson_ui = cJSON_GetObjectItem(parameter, "ui");

      if (cjson_ui == NULL) continue;
  
      HARDBUG(!cJSON_IsString(cjson_ui))

      char *ui = cJSON_GetStringValue(cjson_ui);

      if (my_strcasecmp(ui, name) != 0) continue;

      cJSON *cjson_name = cJSON_GetObjectItem(parameter, "name");
  
      char *parameter_name = cJSON_GetStringValue(cjson_name);
  
      HARDBUG(parameter_name == NULL)

      PRINTF("ui=%s parameter_name=%s\n", ui, parameter_name);

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");
    
      char *parameter_type = cJSON_GetStringValue(cjson_type);
      
      HARDBUG(parameter_type == NULL)
  
      if (my_strcasecmp(parameter_type, "int") == 0)
      {
        int ivalue;

        HARDBUG(sscanf(value, "%d", &ivalue) != 1)
      
        PRINTF("ivalue=%d\n", ivalue);
       
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, ivalue);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (my_strcasecmp(parameter_type, "double") == 0)
      {
        double dvalue;

        HARDBUG(sscanf(value, "%lf", &dvalue) != 1)
      
        PRINTF("dvalue=%.2f\n", dvalue);
       
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, dvalue);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (my_strcasecmp(parameter_type, "string") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsString(cjson_value))

        cJSON_SetStringValue(cjson_value, value);

        cJSON_AddBoolToObject(parameter, "cli", TRUE);
      }
      else if (my_strcasecmp(parameter_type, "bool") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsBool(cjson_value))

        if (my_strcasecmp(value, "false") == 0)
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

local void write_to_hub_client(char *line, pipe_t pfd)
{
  int nline = strlen(line);

  PRINTF("write_to_hub_client: %d:%s", nline, line);

  HARDBUG(line[nline - 1] != '\n')

  HARDBUG(my_pipe_write(pfd, line, nline) != nline)
}


local int read_from_hub_client(char line[MY_LINE_MAX], pipe_t pfd)
{
  PRINTF("waiting for input..\n");

  if (pgets(line, MY_LINE_MAX, pfd) == NULL) return(INVALID);

  PRINTF("read_from_hub_client: %s", line);

  return(TRUE);
}

local char event[MY_LINE_MAX];
local char my_name[MY_LINE_MAX];
local char your_name[MY_LINE_MAX];

local double return_sigmoid(double x)
{
  return(2.0 / (1.0 + exp(-x)) - 1.0);
}

local int play_game(board_t *with, int my_colour,
  pipe_t parent2child, pipe_t child2parent)
{
  int result = INVALID;

  state_t *game = state_class->objects_ctor();

  game->set_event(game, event);

  if (IS_WHITE(my_colour))
  {
    game->set_white(game, my_name);
    game->set_black(game, your_name);
  }
  else
  {
    game->set_white(game, your_name);
    game->set_black(game, my_name);
  }

  char fen[MY_LINE_MAX];

  board2fen(with->board_id, fen, FALSE);

  game->set_starting_position(game, fen);

  char pos[MY_LINE_MAX];

  strcpy(pos, with->board2string(with, TRUE));

  PRINTF("pos=%s\n", pos);

  //setup time allocation

  time_control_t time_control;

  configure_time_control(options.hub_server_game_time * 60,
    options.hub_server_game_moves, &time_control);

  int nmy_game_moves_done = 0;
  int nyour_game_moves_done = 0;

  double my_game_time_left = options.hub_server_game_time * 60.0;
  double your_game_time_left = options.hub_server_game_time * 60.0;

  while(TRUE)
  {
    print_board(with->board_id);

    moves_list_t moves_list;

    create_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    PRINTF("nmy_game_moves_done=%d nyour_game_moves_done=%d\n",
      nmy_game_moves_done, nyour_game_moves_done);

    if (with->board_colour2move != my_colour)
    {
      if (moves_list.nmoves == 0)
      {
        if (IS_WHITE(with->board_colour2move))
          result = 0;
        else
          result = 2;
 
        break;
      }

      if ((nmy_game_moves_done >= options.hub_server_game_moves) and
          (nyour_game_moves_done >= options.hub_server_game_moves))
      {
        result = INVALID;
        break;
      }

      if (options.ponder)
      {
        //configure thread for pondering

        options.time_limit = 10000.0;
        options.time_ntrouble = 0;

        PRINTF("\nPondering..\n");
  
        enqueue(return_thread_queue(thread_alpha_beta_master_id),
          MESSAGE_STATE, game->get_state(game));

        enqueue(return_thread_queue(thread_alpha_beta_master_id),
          MESSAGE_GO, "hub/play_game");
      }

      //prepare pos message

      char moves_string[MY_LINE_MAX];

      strcpy(moves_string, "");

      cJSON *game_move;

      cJSON_ArrayForEach(game_move, game->get_moves(game))
      {
        cJSON *move_string =
          cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

        HARDBUG(!cJSON_IsString(move_string))
    
        if (my_strcasecmp(moves_string, "") == 0)
          strcpy(moves_string, cJSON_GetStringValue(move_string));
        else
        {
          strcat(moves_string, " ");
          strcat(moves_string, cJSON_GetStringValue(move_string));
        }
      }
    
      PRINTF("moves_string=%s\n", moves_string);

      char line[MY_LINE_MAX];

      snprintf(line, MY_LINE_MAX, "pos pos=%s moves=\"%s\"\n",
               pos, moves_string);

      write_to_hub_client(line, parent2child);

      int nyour_game_moves_left =
        options.hub_server_game_moves - nyour_game_moves_done;

      PRINTF("you have %.2f seconds left for %d moves\n",
        your_game_time_left, nyour_game_moves_left);

      snprintf(line, MY_LINE_MAX, "level moves=%d time=%d\n",
        nyour_game_moves_left, (int) round(your_game_time_left));

      write_to_hub_client(line, parent2child);

      write_to_hub_client("go think\n", parent2child);

      PRINTF("waiting for 'info' or 'done'..\n");

      double t1 = my_time();

      command_t command;

      while(TRUE)
      {
        if (read_from_hub_client(line, child2parent) == INVALID) break;
      
        line2command(line, &command);
      
        HARDBUG(command.command == NULL)

        if (my_strcasecmp(command.command, "done") == 0) break;
      }

      double t2 = my_time();

      double move_time_used = t2 - t1;

      your_game_time_left -= move_time_used;

      ++nyour_game_moves_done;

      PRINTF("you used %.2f seconds, you have %.2f seconds remaining\n",
        move_time_used, your_game_time_left);

      if (options.ponder)
      {
        //cancel pondering and dequeue messages

        enqueue(return_thread_queue(thread_alpha_beta_master_id),
          MESSAGE_ABORT_SEARCH, "main/play_game");
  
        message_t message;
         
        while(TRUE)
        {
          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_INFO)
            {
              //PRINTF("got info %s\n", bdata(message.message_text));
            }
            else if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got hint %s\n", bdata(message.message_text));
              break;
            }
            else
              FATAL("message.message_id error", EXIT_FAILURE)
          }
          my_sleep(CENTI_SECOND);
        }
      }

      HARDBUG(command.command_nargs < 1)

      HARDBUG(my_strcasecmp(command.command_args[0].arg_name, "move") != 0)

      char *move_string = command.command_args[0].arg_value;

      int imove;

      HARDBUG((imove = search_move(&moves_list, move_string)) == INVALID)

      game->push_move(game, move_string, NULL);

      do_move(with, imove, &moves_list);
    }
    else
    {  
      //my turn

      if (moves_list.nmoves == 0)
      {
        if (IS_WHITE(with->board_colour2move))
          result = 0;
        else
          result = 2;

        break;
      }

      //check for known endgame

      int egtb_mate = read_endgame(with, with->board_colour2move, TRUE);
    
      if (egtb_mate != ENDGAME_UNKNOWN)
      {
        PRINTF("known endgame egtb_mate=%d\n", egtb_mate);
  
        if (egtb_mate == INVALID)
        {
          result = 1;
        }
        else if (egtb_mate > 0)
        {
          if (IS_WHITE(with->board_colour2move))
            result = 2;
          else
            result = 0;
        }
        else
        {
          if (IS_WHITE(with->board_colour2move))
            result = 0;
          else
            result = 2;
        }
  
        break;
      }

      if ((nmy_game_moves_done >= options.hub_server_game_moves) and
          (nyour_game_moves_done >= options.hub_server_game_moves))
      {
        result = INVALID;
        break;
      }
   
      char best_move[MY_LINE_MAX];
      int best_score;
      int best_depth;

      strcpy(best_move, "NULL");

      message_t message;

      strcpy(bdata(message.message_text), "NULL");

      PRINTF("\nthinking..\n");

      double t1 = my_time();

      if (moves_list.nmoves == 1)
      {
        strcpy(best_move, moves_list.move2string(&moves_list, 0));

        strcpy(bdata(message.message_text), "only move");

        best_score = 0;
 
        best_depth = 0;
      } 
      else
      {
        if (options.use_book)
          return_book_move(with, &moves_list, best_move);
      
        if (my_strcasecmp(best_move, "NULL") != 0)
        {
          strcpy(bdata(message.message_text), "book move");

          best_score = 0;
 
          best_depth = 0;
        }
        else
        {
          //configure thread for search

          set_time_limit(nmy_game_moves_done, &time_control);

          enqueue(return_thread_queue(thread_alpha_beta_master_id),
            MESSAGE_STATE, game->get_state(game));
  
          enqueue(return_thread_queue(thread_alpha_beta_master_id),
            MESSAGE_GO, "hub/play_game");
    
          while(TRUE)
          {
            if (dequeue(&main_queue, &message) != INVALID)
            {
              if (message.message_id == MESSAGE_INFO)
              {
                PRINTF("got info %s\n", bdata(message.message_text));
              }
              else if (message.message_id == MESSAGE_RESULT)
              {
                PRINTF("got result %s\n", bdata(message.message_text));
    
                HARDBUG(sscanf(bdata(message.message_text), "%s%d%d",
                               best_move, &best_score, &best_depth) != 3)
  
                break;
              }
              else
                FATAL("message.message_id error", EXIT_FAILURE)
            } 
            my_sleep(CENTI_SECOND);
          }
        }
      }

      double t2 = my_time();

      double move_time_used = t2 - t1;

      my_game_time_left -= move_time_used;

      PRINTF("I used %.2f seconds, I have %.2f seconds remaining\n",
        move_time_used, my_game_time_left);

      update_time_control(nmy_game_moves_done, move_time_used,
        &time_control);

      nmy_game_moves_done++;

      PRINTF("\n* * * * * * * * * * %s %d %d\n\n",
        best_move, best_score, best_depth);

      if (options.hub_annotate_level == 0)
        game->push_move(game, best_move, NULL);
      else
        game->push_move(game, best_move, bdata(message.message_text));

      int imove;

      HARDBUG((imove = search_move(&moves_list, best_move)) == INVALID)

      //evaluate if needed

      if ((moves_list.nmoves > 1) and
          (options.neural_evaluation_time > 0) and
          (best_score >= options.neural_evaluation_min) and
          (best_score <= options.neural_evaluation_max))
      {
        PRINTF("evaluating..\n");

        options.time_limit = options.neural_evaluation_time;
        options.time_ntrouble = 0;

        enqueue(return_thread_queue(thread_alpha_beta_master_id),
          MESSAGE_GO, "hub/evaluate");
  
        while(TRUE)
        {
          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_INFO)
            {
              PRINTF("got info %s\n", bdata(message.message_text));
            }
            else if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got result %s\n", bdata(message.message_text));
  
              HARDBUG(sscanf(bdata(message.message_text), "%s%d%d",
                             best_move, &best_score, &best_depth) != 3)

              break;
            }
            else
              FATAL("message.message_id error", EXIT_FAILURE)
          } 
          my_sleep(CENTI_SECOND);
        }

        PRINTF("evaluate best_move=%s best_score=%d best_depth=%d\n",
               best_move, best_score, best_depth);

        int fd = my_lock_file("hub.fen");

        HARDBUG(fd == -1)
      
        board2fen(with->board_id, fen, FALSE);

        if (IS_BLACK(with->board_colour2move)) best_score = -best_score;

        my_fdprintf(fd, "%s {%.5f}\n", fen,
                        return_sigmoid(best_score / 100.0));

        my_unlock_file(fd);
      }

      do_move(with, imove, &moves_list);
    }
  }

  char game_result[MY_LINE_MAX];

  strcpy(game_result, "*");

  if (result == 2)
  {
    strcpy(game_result, "2-0");
  }
  else if (result == 1)
  {
    strcpy(game_result, "1-1");
  }
  else if (result == 0)
  {
    strcpy(game_result, "0-2");
  }

  PRINTF("game_result=%s\n", game_result);

  game->set_result(game, game_result);

  game->save2pdn(game, "hub.pdn");

  state_class->objects_dtor(game);

  return(result);
}

local void hub_server_game_initiator(pipe_t parent2child, pipe_t child2parent)
{
  snprintf(my_name, MY_LINE_MAX, "GWD %s %s", REVISION, options.neural_name0);
  snprintf(your_name, MY_LINE_MAX, "%s", options.hub_server_client);

  int iboard = create_board(STDOUT, INVALID);
  board_t *with = return_with_board(iboard);

  int nwon = 0;
  int ndraw = 0;
  int nlost = 0;
  int nunknown = 0;

  FILE *fballot;

  HARDBUG((fballot = fopen(options.hub_server_ballot, "r")) == NULL)

  int nfen = 0;

  while(TRUE)
  {
    char line[MY_LINE_MAX];

    if (fgets(line, MY_LINE_MAX, fballot) == NULL) break;

    char fen[MY_LINE_MAX];

    if (sscanf(line, "%[^\n]", fen) == 0) continue;

    if (strncmp(fen, "//", 2) == 0) continue;

    ++nfen;
  }

  FCLOSE(fballot)

  HARDBUG((fballot = fopen(options.hub_server_ballot, "r")) == NULL)

  PRINTF("nfen=%d\n", nfen);

  int igame = 0;
  
  int ifen = 0;

  int nfen_mpi = 0;

  while(TRUE)
  {
    char fen[MY_LINE_MAX];
    char opening[MY_LINE_MAX];

    strcpy(fen, "NULL");
    strcpy(opening, "");

    while(TRUE)
    {
      char line[MY_LINE_MAX];

      if (fgets(line, MY_LINE_MAX, fballot) == NULL) goto label_break;

      if (sscanf(line, "%[^\n]", fen) == 0) continue;

      if (strncmp(fen, "//", 2) == 0)
      {
        HARDBUG(sscanf(fen, "//%[^\n]", opening) != 1)

        continue;
      }

      break;
    }

    PRINTF("opening=%s fen=%s\n", opening, fen);
 
    ++ifen;

    if (ifen > options.hub_server_games) break;

    if ((my_mpi_globals.MY_MPIG_id_slave != INVALID) and
        (((ifen - 1) % my_mpi_globals.MY_MPIG_nslaves) !=
         my_mpi_globals.MY_MPIG_id_slave)) continue;

    ++nfen_mpi;

    PRINTF("ifen=%d nfen=%d nfen_mpi=%d\n", ifen, nfen, nfen_mpi);

    for (int icolour = 0; icolour < 2; icolour++)
    {
      ++igame;

      //if (igame > options.hub_server_games) goto label_break;

      snprintf(event, MY_LINE_MAX,
               "Game %d, Opening %s, MPI %d;%d", igame, opening,
               my_mpi_globals.MY_MPIG_nglobal,
               my_mpi_globals.MY_MPIG_id_global);

      PRINTF("event=%s\n", event);

      int my_colour;

      if (icolour == 0)
        my_colour = WHITE_BIT;
      else
        my_colour = BLACK_BIT;

      PRINTF("game=%d opening=%s\n", igame, opening);

      fen2board(fen, with->board_id);

      print_board(with->board_id);

      int result = play_game(with, my_colour, parent2child, child2parent);
  
      if (result == 2)
      {
        if (IS_WHITE(my_colour))
          ++nwon;
        else
          ++nlost;
      }
      else if (result == 1)
      {
        ++ndraw;
      } 
      else if (result == 0)
      {
        if (IS_WHITE(my_colour))
          ++nlost;
        else
          ++nwon;
      }
      else
        ++nunknown;
  
      PRINTF("HUB nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
        nwon, ndraw, nlost, nunknown);
    }
  }

  label_break:

#ifdef USE_OPENMPI
  if (my_mpi_globals.MY_MPIG_id_slave != INVALID)
  {
    MPI_Allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_INT,
      MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

    PRINTF("nfen=%d nfen_mpi=%d\n", nfen, nfen_mpi);

    //if options.hub_server_games is hit this is not true
    //HARDBUG(nfen_mpi != nfen)

    MPI_Allreduce(MPI_IN_PLACE, &nwon, 1, MPI_INT,
      MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
    MPI_Allreduce(MPI_IN_PLACE, &ndraw, 1, MPI_INT,
      MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
    MPI_Allreduce(MPI_IN_PLACE, &nlost, 1, MPI_INT,
      MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
    MPI_Allreduce(MPI_IN_PLACE, &nunknown, 1, MPI_INT,
      MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  }
#endif
  PRINTF("HUB nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
    nwon, ndraw, nlost, nunknown);

  if ((my_mpi_globals.MY_MPIG_id_slave == INVALID) or
      (my_mpi_globals.MY_MPIG_id_slave == 0))
    results2csv(nwon, ndraw, nlost, nunknown);
}

void hub_server(pipe_t parent2child, pipe_t child2parent)
{
  write_to_hub_client("hub\n", parent2child);

  PRINTF("waiting for 'wait'..\n");

  while(TRUE)
  {
    char line[MY_LINE_MAX];

    if (read_from_hub_client(line, child2parent) == INVALID) return;

    command_t command;
  
    line2command(line, &command);
  
    HARDBUG(command.command == NULL)
  
    if (my_strcasecmp(command.command, "wait") == 0) break;
  }

  write_to_hub_client("init\n", parent2child);

  PRINTF("waiting for 'ready'..\n");

  while(TRUE)
  {
    char line[MY_LINE_MAX];

    if (read_from_hub_client(line, child2parent) == INVALID) return;

    command_t command;
  
    line2command(line, &command);
  
    HARDBUG(command.command == NULL)
  
    if (my_strcasecmp(command.command, "ready") == 0) break;
  }

  hub_server_game_initiator(parent2child, child2parent);

  write_to_hub_client("quit\n", parent2child);

  //wait for close

  PRINTF("waiting for 'close'..\n");
  return;
  while(TRUE)
  {
    char line[MY_LINE_MAX];

    if (read_from_hub_client(line, child2parent) == INVALID) return;

    command_t command;
  
    line2command(line, &command);
  
    HARDBUG(command.command == NULL)
  }

}

