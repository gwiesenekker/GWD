//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"


//TODO: naming conventions type-qualifier, so not white_score but score_white

#define GWD_JSON "gwd.json"
#define OVERRIDES_JSON "overrides.json"

void *STDOUT;

my_random_t main_random;

queue_t main_queue;

//options

options_t options;

local void solve_problems(char *arg_name)
{
  BEGIN_BLOCK(__FUNC__)

  FILE *fname, *ftmp;
  
  HARDBUG((fname = fopen(arg_name, "r")) == NULL)
  HARDBUG((ftmp = fopen("tmp.fen", "w")) == NULL)

  int nproblems = 0;
  int nsolved = 0;

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  print_board(&(with->S_board));

  char line[MY_LINE_MAX];

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '*') break;
    if (*line == '#') continue;

    char fen[MY_LINE_MAX];

    HARDBUG(sscanf(line, "%[^\n]", fen) != 1)

    ++nproblems;

    fen2board(&(with->S_board), fen);

    print_board(&(with->S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with->S_board), &moves_list, FALSE);

    check_moves(&(with->S_board), &moves_list);

    fprintf_moves_list(&moves_list, STDOUT, FALSE);

    BSTRING(book_move)

    if (options.use_book)
      return_book_move(&(with->S_board), &moves_list, book_move);
    else
      PRINTF("WARNING: book is not enabled!\n");

    BDESTROY(book_move)

    if (moves_list.nmoves > 0)
    {
      //options.hub=1;
      do_search(with, &moves_list,
        INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

      if ((with->S_best_score -
           with->S_root_simple_score) > (SCORE_MAN / 2))
      {
        ++nsolved;
        PRINTF("solved problem #%d\n", nproblems);
      }
      else
      {
        PRINTF("did not solve problem #%d\n", nproblems);
        HARDBUG(fprintf(ftmp, "%s\n", fen) < 0)
      }
    }
  }
  FCLOSE(fname)
  FCLOSE(ftmp)

  PRINTF("solved %d out of %d problems\n", nsolved, nproblems);
  END_BLOCK
}

local void solve_problems_multi_threaded(char *arg_name)
{
  enqueue(return_thread_queue(thread_alpha_beta_master),
    MESSAGE_SOLVE, arg_name);

  while(TRUE)
  { 
    message_t message;

    if (dequeue(&main_queue, &message) != INVALID)
    {
      if (message.message_id == MESSAGE_INFO)
      {
        PRINTF("got info %s\n", bdata(message.message_text));
      }
      else if (message.message_id == MESSAGE_READY)
      {
        break;
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }
    compat_sleep(CENTI_SECOND);
  }
}

local char *secs2string(int t)
{
  local char string[MY_LINE_MAX];
  int hours;
  int minutes;
  int seconds;

  hours = t / 3600;
  minutes = (t - hours * 3600) / 60;
  seconds = t - hours * 3600 - minutes * 60;
  HARDBUG((hours * 3600 + minutes * 60 + seconds) != t)

  snprintf(string, MY_LINE_MAX, "%02d:%02d:%02d", hours, minutes, seconds);
  return(string);
}

#define GAME_MOVES 40

void play_game(char name[MY_LINE_MAX], char colour[MY_LINE_MAX],
  int game_minutes, int used_minutes)
{
  my_random_t play_random;

  construct_my_random(&play_random, 0);

  state_t game_state;

  construct_state(&game_state);

  if (fexists(name))
  {
    game_state.load(&game_state, name);
  }
  else
  {
    if (*colour == 'w')
    {
      game_state.set_white(&game_state, "Human");
      game_state.set_black(&game_state, "GWD");
    }
    else
    {
      game_state.set_white(&game_state, "GWD");
      game_state.set_black(&game_state, "Human");
    }
  }

  int human_colour;

  if (*colour == 'w')
    human_colour = WHITE_BIT;
  else
    human_colour = BLACK_BIT;

  double game_time = game_minutes * 60.0;
  double game_time_used = used_minutes * 60.0;

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  int automatic = FALSE;

  int ndone = 0;

  while(TRUE)
  {
    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_STATE, game_state.get_state(&game_state));

    state2board(&(with->S_board), &game_state);

    print_board(&(with->S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with->S_board), &moves_list, FALSE);

    if (moves_list.nmoves == 0)
    {
      PRINTF("mate!\n");

      goto label_return;
    }

    if (with->S_board.board_colour2move == human_colour)
    {
      //configure thread for pondering

      if (options.ponder)
      {
        options.time_limit = 10000.0;
        options.time_ntrouble = 0;

        PRINTF("\nPondering..\n");

        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_GO, "main/play_game");
      }

      if (!automatic and options.sync_clock and (ndone % 9) == 0)
      {
        PRINTF("\nMinutes remaining "
               " (press <ENTER> to accept the default (%s)?\n",
               secs2string(round(game_time - game_time_used)));

        char line[MY_LINE_MAX];

        if (fgets(line, MY_LINE_MAX, stdin) == NULL) goto label_return;

        int t;

        if (sscanf(line, "%d", &t) == 1)
        {
          game_time_used = game_time - t * 60.0;

          PRINTF("remaining %s minutes\n",
            secs2string(round(game_time - game_time_used)));
        }
      }

      BSTRING(bbest_move)

      double t1 = compat_time();

      while(TRUE)
      {
        BSTRING(banswer)

        if (automatic) 
        {
          HARDBUG(bassigncstr(banswer, ".") != BSTR_OK)
        }
        else
        {
          PRINTF("\nEnter your move or any one of the following options:\n\n"
                 "'?': show the list of legal moves;\n"
                 "'b': show the book moves;\n"
                 "'u': undo a move;\n"
                 "'q': quit;\n"
                 "'.': play a random move;\n"
                 "'a': play random moves automatically from now on.\n\n");

          char line[MY_LINE_MAX];

          if (fgets(line, MY_LINE_MAX, stdin) == NULL) goto label_return;

          CSTRING(canswer, strlen(line))

          if (sscanf(line, "%s", canswer) != 1)
          {
            PRINTF("\neh?\n");

            continue;
          }

          HARDBUG(bassigncstr(banswer, canswer) != BSTR_OK)

          CDESTROY(canswer)
        }

        if (bchar(banswer, 0) == '?')
        {
          fprintf_moves_list(&moves_list, STDOUT, FALSE);

          continue;
        }
   
        if (bchar(banswer, 0) == 'b')
        {
          BSTRING(book_move)

          if (options.use_book)
            return_book_move(&(with->S_board), &moves_list, book_move);
          else
            PRINTF("WARNING: book is not enabled!\n");

          BDESTROY(book_move)

          continue;
        }

        if (bchar(banswer, 0) == 'u')
        {
          game_state.pop_move(&game_state);

          goto label_undo;
        }

        if (bchar(banswer, 0) == 'q')
        {
          enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_ABORT_SEARCH, "main/play_game");

          goto label_return;
        }

        if (bchar(banswer, 0) == '.')
        {
          int imove = return_my_random(&play_random) % moves_list.nmoves;

          move2bstring(&moves_list, imove, bbest_move);

          PRINTF("randomly selected move %s\n", bdata(bbest_move));

          break;
        }

        if (bchar(banswer, 0) == 'a')
        {
          automatic = TRUE;
          continue;
        }

        int imove = search_move(&moves_list, banswer);

        if (imove == INVALID)
        {
          fprintf_moves_list(&moves_list, STDOUT, FALSE);

          PRINTF("eh?\n\n");

          continue;
        }

        move2bstring(&moves_list, imove, bbest_move);

        BDESTROY(banswer)

        break;
      }
      
      double t2 = compat_time();

      double wait_time = t2 - t1;

      PRINTF("waited %s\n", secs2string(round(wait_time)));

      char comment[MY_LINE_MAX];

      snprintf(comment, MY_LINE_MAX, "%d", 0);

      game_state.push_move(&game_state, bdata(bbest_move), comment);

      ++ndone;

      //cancel pondering and dequeue messages

      label_undo:
   
      if (options.ponder)
      {
        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_ABORT_SEARCH, "main/play_game");

        while(TRUE)
        {
          message_t message;
         
          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got hint %s\n", bdata(message.message_text));
              break;
            }
          }
          compat_sleep(CENTI_SECOND);
        }
      }
      BDESTROY(bbest_move)
    }
    else
    {
      BSTRING(bbest_move)

      int best_score;

      double t1 = compat_time();

      if (moves_list.nmoves == 1)
      {
        move2bstring(&moves_list, 0, bbest_move);

        best_score = 0;

        goto label_break;
      }

      //check book move
 
      HARDBUG(bassigncstr(bbest_move, "NULL") != BSTR_OK)
 
      if (options.use_book)
        return_book_move(&(with->S_board), &moves_list, bbest_move);

      if (compat_strcasecmp(bdata(bbest_move), "NULL") != 0)
      {
        PRINTF("book move=%s\n", bdata(bbest_move));

        best_score = 0;

        goto label_break;
      }

      //configure thread for search

      options.time_limit =
        (game_time - game_time_used) / GAME_MOVES;

      if (options.time_limit <= 0.1) options.time_limit = 0.1;

      options.time_trouble[0] =
        (game_time - game_time_used - options.time_limit) / GAME_MOVES;

      if (options.time_trouble[0] <= 0.1) options.time_trouble[0] = 0.1;

      options.time_trouble[1] =
        (game_time - game_time_used -
         options.time_limit - options.time_trouble[0]) / GAME_MOVES;

      if (options.time_trouble[1] <= 0.1) options.time_trouble[1] = 0.1;

      options.time_ntrouble = 2;

      enqueue(return_thread_queue(thread_alpha_beta_master),
        MESSAGE_GO, "main/play_game");

      while(TRUE)
      {
        message_t message;
        
        if (dequeue(&main_queue, &message) != INVALID)
        {
          if (message.message_id == MESSAGE_INFO)
          {
            PRINTF("got info %s\n", bdata(message.message_text));
          }
          else if (message.message_id == MESSAGE_RESULT)
          {
            PRINTF("\ngot result %s\n", bdata(message.message_text));

            CSTRING(cbest_move, blength(message.message_text))

            HARDBUG(sscanf(bdata(message.message_text), "%s%d",
                           cbest_move, &best_score) != 2)

            HARDBUG(bassigncstr(bbest_move, cbest_move) != BSTR_OK)

            CDESTROY(cbest_move)

            goto label_break;
          }
          else
            FATAL("message.message_id error", EXIT_FAILURE)
        } 
        compat_sleep(CENTI_SECOND);
      }

      double t2;

      label_break:

      t2 = compat_time();

      double move_time = t2 - t1;

      game_time_used += move_time;

      PRINTF("used   %s\n", secs2string(round(move_time)));

      PRINTF("total  %s\n", secs2string(round(game_time_used)));

      PRINTF("remain %s\n", secs2string(round(game_time - game_time_used)));

      PRINTF("\n* * * * * * * * * * %s %d\n\n",
        bdata(bbest_move), best_score);

      char comment[MY_LINE_MAX];

      snprintf(comment, MY_LINE_MAX, "%d", best_score);

      game_state.push_move(&game_state, bdata(bbest_move), comment);

      BDESTROY(bbest_move)
    }
  }
  label_return:

  game_state.save(&game_state, name);

  game_state.save2pdn(&game_state, "play.pdn");

  destroy_state(&game_state);
}

local void gen_random(char *name, int nwhite, int nblack)
{
  my_random_t temp_random;

  construct_my_random(&temp_random, 0);

  FILE *frandom;

  HARDBUG((frandom = fopen(name, "w")) == NULL)

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  for (int iwhite = 1; iwhite <= nwhite; ++iwhite)
  {
    for (int iblack = 1; iblack <= nblack; ++iblack)
    {
      char s[MY_LINE_MAX];
      PRINTF("random %d %d\n", iwhite, iblack);

      if ((return_my_random(&temp_random) % 2) == 0)
        s[0] = 'w';
      else
        s[0] = 'b';

      for (int i = 1; i <= 50; ++i) s[i] = *nn;
      for (int i = 1; i <= iwhite; ++i)
      {
        if ((return_my_random(&temp_random) % 4) == 0)
        {
          int j;
          while(TRUE)
          {
            j = return_my_random(&temp_random) % 50 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *wX;
        }
        else
        {
          int j;
          while(TRUE)
          {
            j = return_my_random(&temp_random) % 45 + 6;
            if (s[j] == *nn) break;
          }
          s[j] = *wO;
        }
      }

      for (int i = 1; i <= iblack; ++i)
      {
        if ((return_my_random(&temp_random) % 4) == 0)
        {
          int j;
          while(TRUE)
          {
            j = return_my_random(&temp_random) % 50 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *bX;
        }
        else
        {
          int j;
          while(TRUE)
          {
            j = return_my_random(&temp_random) % 45 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *bO;
        }
      }
      s[51] = '\0';

      string2board(&(with->S_board), s);

      print_board(&(with->S_board));

      HARDBUG(compat_strcasecmp(s, board2string(&(with->S_board), FALSE)) != 0)

      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      check_moves(&(with->S_board), &moves_list);

      if (moves_list.nmoves > 0)
      {
        BSTRING(bfen)

        board2fen(with->S_board.board_colour2move,
          with->S_board.board_white_man_bb,
          with->S_board.board_black_man_bb,
          with->S_board.board_white_king_bb,
          with->S_board.board_black_king_bb,
          bfen, FALSE);

        HARDBUG(fprintf(frandom, "%s\n", bdata(bfen)) < 0)

        BDESTROY(bfen)
      }
    }
  }
  FCLOSE(frandom)
}

local void test_cJSON(void)
{
  const unsigned int resolution_numbers[3][2] =
  {
    {1280, 720},
    {1920, 1080},
    {3840, 2160}
  };

  cJSON *monitor = cJSON_CreateObject(); 

  HARDBUG(monitor == NULL)

  HARDBUG(cJSON_AddStringToObject(monitor, "name", "Awesome 4K") == NULL)

  cJSON *resolutions = NULL;

  HARDBUG((resolutions = cJSON_AddArrayToObject(monitor, "resolutions")) ==
          NULL)

  for (size_t index = 0;
       index < (sizeof(resolution_numbers) / (2 * sizeof(int)));
       ++index)
  {
    cJSON *resolution = cJSON_CreateObject();
  
    HARDBUG(resolution == NULL)

    HARDBUG(cJSON_AddNumberToObject(resolution, "width",
                                resolution_numbers[index][0]) == NULL)

    HARDBUG(cJSON_AddNumberToObject(resolution, "height",
                                resolution_numbers[index][1]) == NULL)

    cJSON_AddItemToArray(resolutions, resolution);
  }

  char string[MY_LINE_MAX];

  HARDBUG(cJSON_PrintPreallocated(monitor, string, MY_LINE_MAX, FALSE) == 0)

  cJSON_Delete(monitor);

  PRINTF("monitor=%s\n", string);
}

#define ARGV_MAX 16
#define ARGS_TXT "args.txt"

cJSON *gwd_json;

local void load_gwd_json(void)
{
  file2cjson(GWD_JSON, &gwd_json);

  //set defaults

  strcpy(options.hub_version, "NULL");

  strcpy(options.overrides, "NULL");

  options.verbose = 1;

  strcpy(options.networks, "NULL");
  strcpy(options.network_name, "NULL");

  options.network_evaluation_min = -100;
  options.network_evaluation_max = -25;
  options.network_evaluation_time = 30;

  options.time_limit = 30.0;

  options.time_control_ntrouble = 3;
  options.time_control_mean = 30;
  options.time_control_sigma = 20;
  options.time_control_method = 0;

  //TO DO this should be moved out of options_t

  options.time_ntrouble = 0;

  options.wall_time_threshold = 5;

  options.depth_limit = 64;

  options.use_book = TRUE;
  strncpy(options.book_name, "book.db", MY_LINE_MAX);
  options.book_randomness = 0;

  options.ponder = TRUE;

  options.egtb_level = 7;

  options.material_only = FALSE;

  options.captures_are_transparent = TRUE;
  options.returned_depth_includes_captures = FALSE;

  options.quiescence_extension_search_delta = 25;
  options.pv_extension_search_delta = 25;

  options.use_reductions = TRUE;

  options.reduction_depth_root = 2;
  options.reduction_depth_leaf = 2;

  options.reduction_mean = 20;
  options.reduction_sigma = 10;

  options.reduction_delta = 25;
  options.reduction_max = 80;
  options.reduction_max = 50;
  options.reduction_min = 20;

  options.use_single_reply_extensions = TRUE;

  options.sync_clock = FALSE;

  options.alpha_beta_cache_size = 1024;
  options.pv_cache_fraction = 10;

  options.nthreads_alpha_beta = 4;
  options.lazy_smp_policy = 1;

  options.nslaves = 0;

  strcpy(options.egtb_dir, "NULL");
  strcpy(options.egtb_ram, "egtb56");
  strcpy(options.egtb_ram_wdl, "egtb3");

  options.egtb_entry_cache_size = 4;

  strcpy(options.dxp_server_ip, "127.0.0.1");
  options.dxp_port = 27531;
  options.dxp_initiator = TRUE;
  options.dxp_game_time = 10;
  options.dxp_game_moves = 75;
  options.dxp_games = 1000;
  strcpy(options.dxp_ballot, "NULL");
  options.dxp_book = FALSE;
  options.dxp_annotate_level = 1;
  options.dxp_strict_gameend = FALSE;
  strcpy(options.dxp_tag, "NULL");

  options.hub_server_game_time = 10;
  options.hub_server_game_moves = 75;
  options.hub_server_games = 1000;
  strcpy(options.hub_server_ballot, "ballot2.fen");
  strcpy(options.hub_server_client, "NULL");
  options.hub_annotate_level = 1;

  options.nthreads = INVALID;
  options.mode = INVALID;

  options.hub = FALSE;
}

local void parse_parameters(void)
{
  cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
  HARDBUG(!cJSON_IsArray(parameters))

  //loop over parameters

  cJSON *parameter;

  cJSON_ArrayForEach(parameter, parameters)
  {
    //{"name": "time_limit", "type": "int", "value": 30, "ui": "false"},
 
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

      int ivalue = round(cJSON_GetNumberValue(cjson_value));
 
      if (compat_strcasecmp(parameter_name, "verbose") == 0)  
        options.verbose = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_min") == 0)  
        options.network_evaluation_min = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_max") == 0)  
        options.network_evaluation_max = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_time") == 0)  
        options.network_evaluation_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_ntrouble") == 0)  
        options.time_control_ntrouble = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_mean") == 0)  
        options.time_control_mean = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_sigma") == 0)  
        options.time_control_sigma = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_method") == 0)  
        options.time_control_method = ivalue;

      else if (compat_strcasecmp(parameter_name, "wall_time_threshold") == 0)  
        options.wall_time_threshold = ivalue;

      else if (compat_strcasecmp(parameter_name, "depth_limit") == 0)  
        options.depth_limit = ivalue;

      else if (compat_strcasecmp(parameter_name, "book_randomness") == 0)  
        options.book_randomness = ivalue;

      else if (compat_strcasecmp(parameter_name, "egtb_level") == 0)  
      {
        if (ivalue < NENDGAME_MAX)
        {
          cJSON *cjson_cli = cJSON_GetObjectItem(parameter, "cli");

          if (cJSON_IsBool(cjson_cli))
          {
             HARDBUG(!cJSON_IsTrue(cjson_cli))
  
             options.egtb_level = ivalue;
          }
          else
          {
            PRINTF("egtb_level only accepted from command-line\n");
          }
        }
      }

      else if (compat_strcasecmp(parameter_name,
                                 "quiescence_extension_search_delta") == 0)  
        options.quiescence_extension_search_delta = ivalue;

      else if (compat_strcasecmp(parameter_name,
                                 "pv_extension_search_delta") == 0)  
        options.pv_extension_search_delta = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_depth_root") == 0)  
        options.reduction_depth_root = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_depth_leaf") == 0)  
        options.reduction_depth_leaf = ivalue;

      else if (compat_strcasecmp(parameter_name,
                             "reduction_mean") == 0)  
        options.reduction_mean = ivalue;

      else if (compat_strcasecmp(parameter_name,
                             "reduction_sigma") == 0)  
        options.reduction_sigma = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_delta") == 0)  
        options.reduction_delta = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_max") == 0)  
        options.reduction_max = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_strong") == 0)  
        options.reduction_strong = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_weak") == 0)  
        options.reduction_weak = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_min") == 0)  
        options.reduction_min = ivalue;

      else if (compat_strcasecmp(parameter_name, "alpha_beta_cache_size") == 0)  
        options.alpha_beta_cache_size = ivalue;

      else if (compat_strcasecmp(parameter_name, "pv_cache_fraction") == 0)  
        options.pv_cache_fraction = ivalue;

      else if (compat_strcasecmp(parameter_name, "nthreads_alpha_beta") == 0)  
        options.nthreads_alpha_beta = ivalue;

      else if (compat_strcasecmp(parameter_name, "lazy_smp_policy") == 0)  
        options.lazy_smp_policy = ivalue;

      else if (compat_strcasecmp(parameter_name, "nslaves") == 0)  
        options.nslaves = ivalue;

      else if (compat_strcasecmp(parameter_name, "egtb_entry_cache_size") == 0)  
        options.egtb_entry_cache_size = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_port") == 0)  
        options.dxp_port = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_game_time") == 0)  
        options.dxp_game_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_game_moves") == 0)  
        options.dxp_game_moves = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_games") == 0)  
        options.dxp_games = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_annotate_level") == 0)  
        options.dxp_annotate_level = ivalue;

      else if (compat_strcasecmp(parameter_name, "hub_server_game_time") == 0)  
        options.hub_server_game_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "hub_server_game_moves") == 0)  
        options.hub_server_game_moves = ivalue;

      else if (compat_strcasecmp(parameter_name, "hub_server_games") == 0)  
        options.hub_server_games = ivalue;

      else if (compat_strcasecmp(parameter_name, "hub_annotate_level") == 0)  
        options.hub_annotate_level = ivalue;

      else
        PRINTF("ignoring int parameter %s\n", parameter_name);
    }
    else if (compat_strcasecmp(parameter_type, "double") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      double dvalue = cJSON_GetNumberValue(cjson_value);
 
      if (compat_strcasecmp(parameter_name, "time_limit") == 0)  
        options.time_limit = dvalue;

      else
        PRINTF("ignoring double parameter %s with value %.2f\n",
          parameter_name, dvalue);
    }
    else if (compat_strcasecmp(parameter_type, "string") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsString(cjson_value));

      char *svalue = cJSON_GetStringValue(cjson_value);

      HARDBUG(svalue == NULL)

      if (compat_strcasecmp(parameter_name, "hub_version") == 0)  
        strncpy(options.hub_version, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "overrides") == 0)  
        strncpy(options.overrides, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "networks") == 0)  
        strncpy(options.networks, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "network_name") == 0)  
        strncpy(options.network_name, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "book_name") == 0)  
        strncpy(options.book_name, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "egtb_dir") == 0)  
        strncpy(options.egtb_dir, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "egtb_ram") == 0)  
        strncpy(options.egtb_ram, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "egtb_ram_wdl") == 0)  
        strncpy(options.egtb_ram_wdl, svalue, MY_LINE_MAX);

      else if (compat_strcasecmp(parameter_name, "dxp_server_ip") == 0)  
        strncpy(options.dxp_server_ip, svalue, MY_LINE_MAX);
      
      else if (compat_strcasecmp(parameter_name, "dxp_ballot") == 0)  
        strncpy(options.dxp_ballot, svalue, MY_LINE_MAX);
      
      else if (compat_strcasecmp(parameter_name, "dxp_tag") == 0)  
        strncpy(options.dxp_tag, svalue, MY_LINE_MAX);
      
      else if (compat_strcasecmp(parameter_name, "hub_server_ballot") == 0)  
        strncpy(options.hub_server_ballot, svalue, MY_LINE_MAX);
      
      else if (compat_strcasecmp(parameter_name, "hub_server_client") == 0)  
        strncpy(options.hub_server_client, svalue, MY_LINE_MAX);
      
      else
        PRINTF("ignoring string parameter %s\n", parameter_name);
    }
    else if (compat_strcasecmp(parameter_type, "bool") == 0)
    { 
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsBool(cjson_value))

      int bvalue = 0;
      if (cJSON_IsTrue(cjson_value))
        bvalue = 1;

      if (compat_strcasecmp(parameter_name, "use_book") == 0)  
        options.use_book = bvalue;

      else if (compat_strcasecmp(parameter_name, "ponder") == 0)  
        options.ponder = bvalue;

      else if (compat_strcasecmp(parameter_name, "material_only") == 0)  
        options.material_only = bvalue;

      else if (compat_strcasecmp(parameter_name, "captures_are_transparent") == 0)  
        options.captures_are_transparent = bvalue;

      else if (compat_strcasecmp(parameter_name,
                             "returned_depth_includes_captures") == 0)  
        options.returned_depth_includes_captures = bvalue;

      else if (compat_strcasecmp(parameter_name, "use_reductions") == 0)  
        options.use_reductions = bvalue;

      else if (compat_strcasecmp(parameter_name,
                             "use_single_reply_extensions") == 0)  
        options.use_single_reply_extensions = bvalue;

      else if (compat_strcasecmp(parameter_name, "sync_clock") == 0)  
        options.sync_clock = bvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_initiator") == 0)  
        options.dxp_initiator = bvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_book") == 0)  
        options.dxp_book = bvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_strict_gameend") == 0)  
        options.dxp_strict_gameend = bvalue;

      else
        PRINTF("ignoring bool parameter %s\n", parameter_name);
    }
    else
    {
      PRINTF("unknown parameter_type %s\n", parameter_type);
      exit(EXIT_FAILURE);
    }
  }
}

cJSON *overrides_json;

local void load_overrides(void)
{
  file2cjson(OVERRIDES_JSON, &overrides_json);

  cJSON *overrides = cJSON_GetObjectItem(overrides_json, options.overrides);

  HARDBUG(!cJSON_IsArray(overrides))
  
  cJSON *override;

  cJSON_ArrayForEach(override, overrides)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(override, "name");
 
    char *override_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(override_name == NULL)
    
    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
    HARDBUG(!cJSON_IsArray(parameters))

    //loop over parameters
  
    cJSON *parameter;
  
    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");
  
      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, override_name) != 0) continue;
  
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "cli");

      if (cJSON_IsTrue(cjson_value))
      {
        nfound = 1;

        break;
      }

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");
    
      char *parameter_type = cJSON_GetStringValue(cjson_type);
      
      HARDBUG(parameter_type == NULL)
  
      if (compat_strcasecmp(parameter_type, "int") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))
  
        int ivalue = round(cJSON_GetNumberValue(cjson_value));

        cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, ivalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "double") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))
  
        double dvalue = cJSON_GetNumberValue(cjson_value);

        cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, dvalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "string") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");
  
        char *svalue = cJSON_GetStringValue(cjson_value);
  
        HARDBUG(svalue == NULL)
  
        cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsString(cjson_value))

        cJSON_SetStringValue(cjson_value, svalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "bool") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");
  
        HARDBUG(!cJSON_IsBool(cjson_value))
  
        int bvalue;

        if (cJSON_IsFalse(cjson_value))
          bvalue = 0;
        else
          bvalue = 1;

        cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsBool(cjson_value))

        cJSON_SetBoolValue(cjson_value, bvalue);

        nfound++;
      }
      else
        FATAL("unknown parameter_type", EXIT_FAILURE)
    }
    if (nfound != 1)
    {
      PRINTF("unknown override %s\n", override_name);
      exit(EXIT_FAILURE);
    }
  }
 
  //check CJSON_CSV_ID

  cJSON *cjson_csv =
    cJSON_GetObjectItem(overrides_json, CJSON_CSV_ID);

  HARDBUG(cjson_csv == NULL)

  cJSON *cjson_path = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_PATH_ID);
 
  HARDBUG(!cJSON_IsString(cjson_path))

  char *path = cJSON_GetStringValue(cjson_path);
  
  HARDBUG(path == NULL)

  cJSON *columns = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_COLUMNS_ID);

  HARDBUG(!cJSON_IsArray(columns))

  cJSON *column;

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");
 
    HARDBUG(!cJSON_IsString(cjson_name))

    char *column_name = cJSON_GetStringValue(cjson_name);
 
    HARDBUG(column_name == NULL)

    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
    HARDBUG(!cJSON_IsArray(parameters))

    //loop over parameters
  
    cJSON *parameter;
  
    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");
  
      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, column_name) != 0) continue;
  
      nfound++;
    }
    if (nfound != 1)
    {
      PRINTF("unknown column %s\n", column_name);
      exit(EXIT_FAILURE);
    }
  }
}

void results2csv(int nwon, int ndraw, int nlost, int nunknown)
{
  cJSON *cjson_csv =
    cJSON_GetObjectItem(overrides_json, CJSON_CSV_ID);

  HARDBUG(cjson_csv == NULL)

  cJSON *cjson_path = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_PATH_ID);
 
  HARDBUG(!cJSON_IsString(cjson_path))

  char *path = cJSON_GetStringValue(cjson_path);
  
  HARDBUG(path == NULL)

  cJSON *columns = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_COLUMNS_ID);

  HARDBUG(!cJSON_IsArray(columns))

  //header

  char header[MY_LINE_MAX];

  strcpy(header, "version,date");

  cJSON *column;

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");
  
    HARDBUG(!cJSON_IsString(cjson_name))
 
    strcat(header, ",");
    strcat(header, cJSON_GetStringValue(cjson_name));
  }
  strcat(header, ",nwon,ndraw,nlost,nunknown\n");

  PRINTF("header=%s", header);

  FILE *fcsv;

  if (!fexists(path))
  {
    if ((fcsv = fopen(path, "w")) == NULL)
    {
      PRINTF("WARNING: cannot open file %s!\n", path);
      return;
    }

    HARDBUG(fprintf(fcsv, "%s", header) < 0)
  }
  else
  {
    //check if header has changed
  
    HARDBUG((fcsv = fopen(path, "r")) == NULL)

    char line[MY_LINE_MAX];

    HARDBUG(fgets(line, MY_LINE_MAX, fcsv) == NULL)

    FCLOSE(fcsv)

    if (strcmp(line, header) == 0)
    {
      HARDBUG((fcsv = fopen(path, "a")) == NULL)
    }
    else
    {
      PRINTF("header has changed!\n");

      char stamp[MY_LINE_MAX];

      time_t t = time(NULL);

      HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S",
                       localtime(&t)) == 0)

      char newpath[MY_LINE_MAX];

      snprintf(newpath, MY_LINE_MAX, "%s-%s", path, stamp);

      remove(newpath);

      HARDBUG(rename(path, newpath) == -1)

      HARDBUG((fcsv = fopen(path, "w")) == NULL)

      HARDBUG(fprintf(fcsv, "%s", header) < 0)
    }
  }

  HARDBUG(fcsv == NULL)

  char stamp[MY_LINE_MAX];

  time_t t = time(NULL);

  HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S",
                   localtime(&t)) == 0)

  HARDBUG(fprintf(fcsv, "\"%s\",\"%s\"", REVISION, stamp) < 0)

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");
 
    HARDBUG(!cJSON_IsString(cjson_name))

    char *column_name = cJSON_GetStringValue(cjson_name);
 
    HARDBUG(column_name == NULL)

    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
    HARDBUG(!cJSON_IsArray(parameters))

    //loop over parameters
  
    cJSON *parameter;
  
    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");
  
      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, column_name) != 0) continue;
  
      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");
    
      char *parameter_type = cJSON_GetStringValue(cjson_type);
      
      HARDBUG(parameter_type == NULL)
  
      if (compat_strcasecmp(parameter_type, "int") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))
  
        int ivalue = round(cJSON_GetNumberValue(cjson_value));

        HARDBUG(fprintf(fcsv, ",%d", ivalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "double") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsNumber(cjson_value))
  
        double dvalue = cJSON_GetNumberValue(cjson_value);

        HARDBUG(fprintf(fcsv, ",%.2f", dvalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "string") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        char *svalue = cJSON_GetStringValue(cjson_value);
  
        HARDBUG(svalue == NULL)
  
        HARDBUG(fprintf(fcsv, ",\"%s\"", svalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "bool") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
        HARDBUG(!cJSON_IsBool(cjson_value))
  
        int bvalue;

        if (cJSON_IsFalse(cjson_value))
          bvalue = 0;
        else
          bvalue = 1;

        HARDBUG(fprintf(fcsv, ",%d", bvalue) < 0)

        nfound++;
      }
      else
        FATAL("unknown parameter_type", EXIT_FAILURE)
    }
    if (nfound != 1)
    {
      PRINTF("unknown column %s\n", column_name);
      exit(EXIT_FAILURE);
    }
  }
  fprintf(fcsv, ",%d,%d,%d,%d\n", nwon, ndraw, nlost, nunknown);

  FCLOSE(fcsv)
}

#define PRINTF_CFG_B(I) PRINTF("options." #I "=%s\n",\
  options.I ? "true" : "false");
#define PRINTF_CFG_I(I) PRINTF("options." #I "=%d\n", options.I);
#define PRINTF_CFG_D(D) PRINTF("options." #D "=%.2f\n", options.D);
#define PRINTF_CFG_I64(I64) PRINTF("options." #I64 "=%lld\n", options.I64);
#define PRINTF_CFG_S(S) PRINTF("options." #S "=%s\n", options.S);

int main(int argc, char **argv)
{
  FILE *farg;

  if ((farg = fopen(ARGS_TXT, "w")) == NULL)
  {
    PRINTF("could not open '%s'\n", ARGS_TXT);
    exit(EXIT_FAILURE);
  }
  for (int iarg = 0; iarg < argc; iarg++)
  {
    if (fprintf(farg, "%d %s\n", iarg, argv[iarg]) < 0)
    {
      PRINTF("could not fprintf '%s'\n", ARGS_TXT);
      exit(EXIT_FAILURE);
    }
  }
  if (fclose(farg) == EOF)
  {
    PRINTF("could not fclose '%s'\n", ARGS_TXT);
    exit(EXIT_FAILURE);
  }

  FILE *fversion;

  if ((fversion = fopen("version.txt", "w")) == NULL)
  {
    PRINTF("could not open 'version.txt'\n");
    exit(EXIT_FAILURE);
  }

  fprintf(fversion, "%s\n", REVISION);

  if (fclose(fversion) == EOF)
  {
    PRINTF("could not fclose 'version.txt'\n");
    exit(EXIT_FAILURE);
  }

  //special argument

  if ((argc > 1) and (compat_strcasecmp(argv[1], "--revision") == 0))
  {
    fprintf(stdout, "%s\n", REVISION);
    exit(EXIT_SUCCESS);
  }

  init_my_malloc();

  load_gwd_json();

  parse_parameters();

  if (compat_strcasecmp(options.hub_version, "NULL") == 0)
  {
    PRINTF("required parameter hub_version not found in gwd.json!"); 
    exit(EXIT_FAILURE);
  }
  if (compat_strcasecmp(options.overrides, "NULL") == 0)
  {
    PRINTF("required parameter overrides not found in gwd.json!"); 
    exit(EXIT_FAILURE);
  }
  if (compat_strcasecmp(options.network_name, "NULL") == 0)
  {
    PRINTF("required parameter network_name not found in gwd.json!"); 

    exit(EXIT_FAILURE);
  }

  options.mode = INVALID;

  //parse command line arguments 

  int iarg = 1;

  int narg = 1;

  while(argv[narg] != NULL) ++narg;

  HARDBUG(narg != argc)
  
  while (argv[iarg] != NULL)
  {  
    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);
 
    HARDBUG(!cJSON_IsArray(parameters))

    //loop over parameters

    cJSON *parameter;

    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      //{"name": "time_limit", "type": "int", "value": 30, "ui": "false"},
      // "args": ["--depth_limit", "-d"]}
 
      cJSON *cjson_args = cJSON_GetObjectItem(parameter, "args");

      if (cjson_args == NULL) continue;
  
      HARDBUG(!cJSON_IsArray(cjson_args))

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

      char *parameter_type = cJSON_GetStringValue(cjson_type);

      HARDBUG(parameter_type == NULL)

      cJSON *cjson_arg;

      cJSON_ArrayForEach(cjson_arg, cjson_args)
      {
        char *arg = cJSON_GetStringValue(cjson_arg);
  
        HARDBUG(arg == NULL)

        if (compat_strcasecmp(parameter_type, "int") == 0)
        {
          char format[MY_LINE_MAX];
  
          sprintf(format, "%s=%%d", arg);
  
          int ivalue;
  
          if (sscanf(argv[iarg], format, &ivalue) != 1) continue;
  
          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
          HARDBUG(!cJSON_IsNumber(cjson_value))
  
          cJSON_SetNumberValue(cjson_value, ivalue);
  
          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "double") == 0)
        {
          char format[MY_LINE_MAX];
  
          sprintf(format, "%s=%%lf", arg);
  
          double dvalue;
  
          if (sscanf(argv[iarg], format, &dvalue) != 1) continue;
  
          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
          HARDBUG(!cJSON_IsNumber(cjson_value))
  
          cJSON_SetNumberValue(cjson_value, dvalue);
  
          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "string") == 0)
        {
          char format[MY_LINE_MAX];
  
          sprintf(format, "%s=%%s", arg);
  
          char svalue[MY_LINE_MAX];
  
          if (sscanf(argv[iarg], format, svalue) != 1) continue;
  
          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
          HARDBUG(!cJSON_IsString(cjson_value))
  
          cJSON_SetStringValue(cjson_value, svalue);

          cJSON_AddBoolToObject(parameter, "cli", TRUE);
  
          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "bool") == 0)
        {
          char format[MY_LINE_MAX];
  
          sprintf(format, "%s=%%s", arg);
  
          char svalue[MY_LINE_MAX];
  
          if (sscanf(argv[iarg], format, svalue) != 1) continue;
  
          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");
  
          HARDBUG(!cJSON_IsBool(cjson_value))
  
          if ((compat_strcasecmp(svalue, "false") == 0) or
              (compat_strcasecmp(svalue, "off") == 0) or
              (compat_strcasecmp(svalue, "0") == 0))
            cJSON_SetBoolValue(cjson_value, 0);
          else
            cJSON_SetBoolValue(cjson_value, 1);

          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
      }
    }
    HARDBUG(nfound > 1)
    if (nfound == 0) break;

    iarg++;
  }

  //we have to parse the parameters again
  //because overrides could have been set from the cli

  parse_parameters();

  load_overrides();

  //we have to parse the parameters again
  //because these could have been overridden

  parse_parameters();

  //query physical memory before MPI processes are started

  int physical_memory = return_physical_memory();
  int physical_cpus = return_physical_cpus();

  //MPI

  if ((iarg == narg) or (compat_strcasecmp(argv[iarg], "hub") == 0))
  {
    //do not initialize OpenMPI
 
    my_mpi_globals.MY_MPIG_init = FALSE;

    my_mpi_globals.MY_MPIG_nglobal = 0;
    my_mpi_globals.MY_MPIG_id_global = INVALID;

    my_mpi_globals.MY_MPIG_nslaves = 0;
    my_mpi_globals.MY_MPIG_id_slave = INVALID;
  }
  else
  {
   //we always initialize OpenMPI even if there are no slaves

#ifdef USE_OPENMPI
    if (MPI_Init(&narg, &argv) != MPI_SUCCESS)
    {
      PRINTF("MPI_Init failed!\n");
      exit(EXIT_FAILURE);
    }

    my_mpi_globals.MY_MPIG_init = TRUE;

    my_mpi_globals.MY_MPIG_nslaves = options.nslaves;
  
    int mpi_nworld;
  
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_nworld);
  
#if COMPAT_OS == COMPAT_OS_LINUX
    MPI_Comm mpi_comm_parent;

    MPI_Comm_get_parent(&mpi_comm_parent);
  
    if (mpi_comm_parent == MPI_COMM_NULL)
    {
      if (mpi_nworld > 1)
      {
        PRINTF("do not use mpirun but use --nslaves=\n");
        exit(EXIT_FAILURE);
      }
      if (my_mpi_globals.MY_MPIG_nslaves > 0)
      {
        if (RUNNING_ON_VALGRIND)
        {
          MPI_Comm_spawn("valgrind", argv, my_mpi_globals.MY_MPIG_nslaves,
            MPI_INFO_NULL, 0, MPI_COMM_SELF,
            &my_mpi_globals.MY_MPIG_comm_slaves, MPI_ERRCODES_IGNORE);
        }
        else
        {
          MPI_Comm_spawn(argv[0], argv + 1, my_mpi_globals.MY_MPIG_nslaves,
            MPI_INFO_NULL, 0, MPI_COMM_SELF,
            &my_mpi_globals.MY_MPIG_comm_slaves, MPI_ERRCODES_IGNORE);
        }
        MPI_Intercomm_merge(my_mpi_globals.MY_MPIG_comm_slaves, FALSE,
                            &my_mpi_globals.MY_MPIG_comm_global);
      }
      else
      {
        //MPI_Comm_dup(MPI_COMM_WORLD, &my_mpi_globals.MY_MPIG_comm_global);
        my_mpi_globals.MY_MPIG_comm_global = MPI_COMM_WORLD;
        my_mpi_globals.MY_MPIG_comm_slaves = MPI_COMM_NULL;
      }
      my_mpi_globals.MY_MPIG_id_slave = INVALID;
    }
    else 
    {
      MPI_Intercomm_merge(mpi_comm_parent, TRUE,
                          &my_mpi_globals.MY_MPIG_comm_global);
  
      //MPI_Comm_dup(MPI_COMM_WORLD, &my_mpi_globals.MY_MPIG_comm_slaves);
      my_mpi_globals.MY_MPIG_comm_slaves = MPI_COMM_WORLD;

      MPI_Comm_rank(my_mpi_globals.MY_MPIG_comm_slaves,
                    &my_mpi_globals.MY_MPIG_id_slave);
    }
#else
    if (my_mpi_globals.MY_MPIG_nslaves > 0)
    {
      PRINTF("do not use --nslaves= but mpirun\n");
      exit(EXIT_FAILURE);
    }

    HARDBUG(mpi_nworld < 1)

    int mpi_id_world;

    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_id_world);
  
    my_mpi_globals.MY_MPIG_comm_global = MPI_COMM_WORLD;
    my_mpi_globals.MY_MPIG_comm_slaves = MPI_COMM_NULL;
  
    int slave = (mpi_id_world == 0) ? 0 : 1;
  
    MPI_Comm_split(MPI_COMM_WORLD, slave, mpi_id_world,
                   &my_mpi_globals.MY_MPIG_comm_slaves);
  
    if (!slave)
    {
      my_mpi_globals.MY_MPIG_comm_slaves = MPI_COMM_NULL;
  
      my_mpi_globals.MY_MPIG_id_slave = INVALID;
    }
    else
    {
      MPI_Comm_rank(my_mpi_globals.MY_MPIG_comm_slaves,
                    &my_mpi_globals.MY_MPIG_id_slave);
    }

    MPI_Allreduce(&slave, &my_mpi_globals.MY_MPIG_nslaves, 1, MPI_INT, MPI_SUM,
      MPI_COMM_WORLD);
  
    HARDBUG(my_mpi_globals.MY_MPIG_nslaves != (mpi_nworld - 1))
#endif

    MPI_Comm_size(my_mpi_globals.MY_MPIG_comm_global,
                  &my_mpi_globals.MY_MPIG_nglobal);

    HARDBUG(my_mpi_globals.MY_MPIG_nglobal > my_mpi_globals.MY_MPIG_NGLOBAL_MAX)
  
    HARDBUG(my_mpi_globals.MY_MPIG_nglobal !=
            (my_mpi_globals.MY_MPIG_nslaves + 1))
  
    MPI_Comm_rank(my_mpi_globals.MY_MPIG_comm_global,
                  &my_mpi_globals.MY_MPIG_id_global);
  
    if (my_mpi_globals.MY_MPIG_id_global == 0)
    {
      HARDBUG(my_mpi_globals.MY_MPIG_id_slave != INVALID)
    }
    else
    {
      HARDBUG(my_mpi_globals.MY_MPIG_id_slave == INVALID)
    }

    //Sync

    MPI_Barrier(my_mpi_globals.MY_MPIG_comm_global);
  
    MPI_Bcast(&physical_memory, 1, MPI_INT, 0,
              my_mpi_globals.MY_MPIG_comm_global);
   
#endif
  }

  //now that we have the MPI context we can initialize 

  init_my_printf();

  static my_printf_t my_stdout;

  construct_my_printf(&my_stdout, "stdout", TRUE);

  STDOUT = &my_stdout;

  options.hub = FALSE;

  if ((iarg == narg) or (compat_strcasecmp(argv[iarg], "hub") == 0))
  {
    options.hub = TRUE;
   
    init_hub();

    parse_parameters();
  }

  //modify options

  options.nthreads = options.nthreads_alpha_beta;

  PRINTF("Version is %s\n", REVISION);

  PRINTF("sizeof(int)=%lld\n", (i64_t) sizeof(int));
  PRINTF("sizeof(long)=%lld\n", (i64_t) sizeof(long));
  PRINTF("sizeof(long long)=%lld\n", (i64_t) sizeof(long long));

  PRINTF("physical_memory=%d MBYTE\n", physical_memory);

  PRINTF("physical_cpus=%d\n", physical_cpus);

  char cpu_flags[MY_LINE_MAX];

  return_cpu_flags(cpu_flags);

  PRINTF("cpu flags=%s\n", cpu_flags);

#ifdef USE_HARDWARE_CRC32
  PRINTF("using HARDWARE crc32\n");
#else
  PRINTF("using SOFTWARE crc32\n");
#endif

#ifdef USE_HARDWARE_RAND
  PRINTF("using HARDWARE rand\n");
#else
  PRINTF("using SOFTWARE rand\n");
#endif

#ifdef USE_AVX2_INTRINSICS
  PRINTF("using __mm256 intrinsics for AVX2\n");
#else
  PRINTF("using GCC for AVX2\n");
#endif

  PRINTF_CFG_S(hub_version);
  PRINTF_CFG_S(overrides);

  PRINTF_CFG_I(verbose);

  PRINTF_CFG_S(networks);
  PRINTF_CFG_S(network_name);
  PRINTF_CFG_I(network_evaluation_min);
  PRINTF_CFG_I(network_evaluation_max);
  PRINTF_CFG_I(network_evaluation_time);

  PRINTF_CFG_D(time_limit);

  PRINTF_CFG_I(time_control_ntrouble);
  PRINTF_CFG_I(time_control_mean);
  PRINTF_CFG_I(time_control_sigma);
  PRINTF_CFG_I(time_control_method);

  PRINTF_CFG_I(wall_time_threshold);

  PRINTF_CFG_I(depth_limit);

  PRINTF_CFG_B(use_book);
  PRINTF_CFG_S(book_name);
  PRINTF_CFG_I(book_randomness);

  PRINTF_CFG_B(ponder);

  PRINTF_CFG_I(egtb_level);

  PRINTF_CFG_B(material_only);

  PRINTF_CFG_B(captures_are_transparent);
  PRINTF_CFG_B(returned_depth_includes_captures);

  PRINTF_CFG_I(quiescence_extension_search_delta);
  PRINTF_CFG_I(pv_extension_search_delta);

  PRINTF_CFG_B(use_reductions);

  PRINTF_CFG_I(reduction_depth_root);
  PRINTF_CFG_I(reduction_depth_leaf);

  PRINTF_CFG_I(reduction_mean);
  PRINTF_CFG_I(reduction_sigma);

  PRINTF_CFG_I(reduction_delta);
  PRINTF_CFG_I(reduction_max);
  PRINTF_CFG_I(reduction_strong);
  PRINTF_CFG_I(reduction_weak);
  PRINTF_CFG_I(reduction_min);

  PRINTF_CFG_B(use_single_reply_extensions);

  PRINTF_CFG_B(sync_clock);

  PRINTF_CFG_I64(alpha_beta_cache_size);
  PRINTF_CFG_I(pv_cache_fraction);

  PRINTF_CFG_I(nthreads_alpha_beta);
  PRINTF_CFG_I(lazy_smp_policy);

  PRINTF_CFG_I(nslaves);

  PRINTF_CFG_S(egtb_dir);
  PRINTF_CFG_S(egtb_ram);
  PRINTF_CFG_S(egtb_ram_wdl);

  PRINTF_CFG_I64(egtb_entry_cache_size);

  PRINTF_CFG_S(dxp_server_ip);
  PRINTF_CFG_I(dxp_port);
  PRINTF_CFG_B(dxp_initiator);
  PRINTF_CFG_I(dxp_game_time);
  PRINTF_CFG_I(dxp_game_moves);
  PRINTF_CFG_I(dxp_games);
  PRINTF_CFG_S(dxp_ballot);
  PRINTF_CFG_B(dxp_book);
  PRINTF_CFG_I(dxp_annotate_level);
  PRINTF_CFG_B(dxp_strict_gameend);
  PRINTF_CFG_S(dxp_tag);

  PRINTF_CFG_I(hub_server_game_time);
  PRINTF_CFG_I(hub_server_game_moves);
  PRINTF_CFG_I(hub_server_games);
  PRINTF_CFG_S(hub_server_ballot);
  PRINTF_CFG_S(hub_server_client);
  PRINTF_CFG_I(hub_annotate_level);

  PRINTF_CFG_I(nthreads);

  PRINTF("my_mpi_globals.MY_MPIG_nslaves=%d"
         " my_mpi_globals.MY_MPIG_id_slave=%d\n",
    my_mpi_globals.MY_MPIG_nslaves, my_mpi_globals.MY_MPIG_id_slave);

  PRINTF("my_mpi_globals.MY_MPIG_nglobal=%d"
         " my_mpi_globals.MY_MPIG_id_global=%d\n",
    my_mpi_globals.MY_MPIG_nglobal, my_mpi_globals.MY_MPIG_id_global);

#ifdef USE_OPENMPI
  if (my_mpi_globals.MY_MPIG_nglobal > 0)
    my_mpi_barrier("main", my_mpi_globals.MY_MPIG_comm_global, TRUE);
#endif
  
  if (((options.nslaves > 0) and (my_mpi_globals.MY_MPIG_id_slave == INVALID))
      or RUNNING_ON_VALGRIND)
  {
    options.alpha_beta_cache_size = 128;

    PRINTF("WARNING: CACHE AND PRELOAD EGTB HAVE BEEN ADJUSTED!\n");
  }

  //hardware checks
  
  int memory =
    options.alpha_beta_cache_size + 
    options.egtb_entry_cache_size * options.nthreads;

  PRINTF("memory=%d MBYTE\n", memory);

  HARDBUG(memory >= physical_memory)

  HARDBUG(options.nthreads_alpha_beta > physical_cpus)

  //parameter checks

  HARDBUG(options.quiescence_extension_search_delta < 0)
  HARDBUG(options.pv_extension_search_delta < 0)

  HARDBUG(options.reduction_depth_root < 0)
  HARDBUG(options.reduction_depth_leaf < 0)

  HARDBUG(options.reduction_delta < 0)
  HARDBUG(options.reduction_max < 0)
  HARDBUG(options.reduction_strong < 0)
  HARDBUG(options.reduction_weak < 0)
  HARDBUG(options.reduction_min < 0)

  INIT_PROFILE
  
  //BEGIN_BLOCK("main-init")

    if (my_mpi_globals.MY_MPIG_nglobal > 0) test_mpi();

    construct_my_random(&main_random, 0);

    init_utils(); 

    //test_my_timers();
    //FATAL("test_my_timers", 1)

    construct_queue(&main_queue, "main_queue", STDOUT);

    init_book();

    init_boards();

    init_endgame();

    init_mcts();

    init_search();

  //END_BLOCK

  //BEGIN_BLOCK("main-tests")

    //tests

    //test_my_printf();

    test_my_cjson();

    test_buckets();

    test_my_random();
    //FATAL("test_my_random", EXIT_FAILURE)

    test_cJSON();

    test_my_object_class();

    if (options.use_book) open_book();
    //FATAL("test_book", EXIT_FAILURE)

    test_utils();

    //test_my_timers();

    test_records();

    test_caches();

    //test_network();

    //test_nmsimplex();

    //test_queues();
    //FATAL("test_queues", 1)

    test_states();

    test_stats();
    //FATAL("test_stats", EXIT_FAILURE)

    //test_threads();

  //END_BLOCK

  if (options.hub)
  {
    options.mode = GAME_MODE;

    start_threads();

    hub();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
          MESSAGE_EXIT_THREAD, "main/hub");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_EXIT_THREAD, "main/hub");

    PRINTF("before join\n");
    join_threads();
  
    goto label_hub;
  }

  HARDBUG(argv[iarg] == NULL)

  BEGIN_BLOCK("main-thread")

  //main loop

  if (compat_strcasecmp(argv[iarg], "solve_problems") == 0)
  { 
    iarg++;
    HARDBUG(argv[iarg] == NULL)

    solve_problems(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "solve_problems_multi_threaded") == 0)
  {
    iarg++;
    HARDBUG(argv[iarg] == NULL)
  
    start_threads();

    HARDBUG(thread_alpha_beta_master == NULL)
  
    solve_problems_multi_threaded(argv[iarg]);
  
    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
          MESSAGE_EXIT_THREAD, "main/solve");  
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_EXIT_THREAD, "main/solve");

    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "hub_server") == 0)
  {
    i64_t memory_slaves = memory * options.nslaves;

    HARDBUG(memory_slaves >= (physical_memory / 2))

    int nthreads_slaves = options.nthreads_alpha_beta * options.nslaves;
  
    if (options.ponder)
      HARDBUG(nthreads_slaves > (physical_cpus / 2))
    else
      HARDBUG(nthreads_slaves > physical_cpus)

    if ((my_mpi_globals.MY_MPIG_nslaves < 1) or
        (my_mpi_globals.MY_MPIG_id_slave != INVALID))
    {
      cJSON *hub_server_client =
        cJSON_GetObjectItem(overrides_json, options.hub_server_client);

      HARDBUG(hub_server_client == NULL)

      cJSON *cjson_dir =
        cJSON_GetObjectItem(hub_server_client, CJSON_HUB_CLIENT_DIR);

      char *dir = cJSON_GetStringValue(cjson_dir);

      HARDBUG(dir == NULL)

      cJSON *cjson_exe =
        cJSON_GetObjectItem(hub_server_client, CJSON_HUB_CLIENT_EXE);

      char *exe = cJSON_GetStringValue(cjson_exe);

      HARDBUG(exe == NULL)

      cJSON *cjson_arg =
        cJSON_GetObjectItem(hub_server_client, CJSON_HUB_CLIENT_ARG);

      char *arg = cJSON_GetStringValue(cjson_arg);

      HARDBUG(arg == NULL)

      PRINTF("dir=%s\n", dir);
      PRINTF("exe=%s\n", exe);
      PRINTF("arg=%s\n", arg);

#if COMPAT_OS == COMPAT_OS_LINUX
      pipe_t parent2child[2];
      pipe_t child2parent[2];

      HARDBUG(compat_pipe(parent2child) == -1)
  
      HARDBUG(compat_pipe(child2parent) == -1)
  
      pid_t pid;
  
      HARDBUG((pid = compat_fork()) == -1)
    
      if (pid == 0)
      {
        HARDBUG(compat_pipe_close(parent2child[1]) == -1)
    
        HARDBUG(compat_pipe_close(child2parent[0]) == -1)
    
        HARDBUG(compat_dup2(parent2child[0], 0) == -1)
  
        HARDBUG(compat_dup2(child2parent[1], 1) == -1)
  
        HARDBUG(compat_chdir(dir) == -1)


        if (compat_strcasecmp(arg, "NULL") == 0)
        {
          HARDBUG(execlp(exe, exe, NULL) == -1)
        }
        else
        {
          HARDBUG(execlp(exe, exe, arg, NULL) == -1)
        }
  
        FATAL("should not get here!", EXIT_FAILURE)
      }

      HARDBUG(compat_pipe_close(parent2child[0]) == -1)
  
      HARDBUG(compat_pipe_close(child2parent[1]) == -1)

      start_threads();

      hub_server(parent2child[1], child2parent[0]);
#else
      pipe_t parent2child[2];

      //pipe for stdin

      HARDBUG(compat_pipe(parent2child) == -1)

      HARDBUG(!SetHandleInformation(parent2child[1], HANDLE_FLAG_INHERIT, 0))

      //pipe for stdout

      pipe_t child2parent[2];

      HARDBUG(compat_pipe(child2parent) == -1)

      HARDBUG(!SetHandleInformation(child2parent[0], HANDLE_FLAG_INHERIT, 0))

      PROCESS_INFORMATION piProcInfo;
      STARTUPINFO siStartInfo;
  
      ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
      ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
      siStartInfo.cb = sizeof(STARTUPINFO);
      siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
      siStartInfo.hStdInput = parent2child[0];
      siStartInfo.hStdOutput = child2parent[1];
      siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
  
      BOOL bSuccess = CreateProcess(NULL, // No module name (use command line)
                       (LPSTR) exe,       // Command line
                       NULL,              // Process handle not inheritable
                       NULL,              // Thread handle not inheritable
                       TRUE,              // Set handle inheritance to TRUE
                       0,                 // No creation flags
                       NULL,              // Use parent's environment block
                       (LPSTR) dir,       // Set working directory
                       &siStartInfo,      // Pointer to STARTUPINFO structure
                       &piProcInfo);      // Pointer to PROCESS_INFORMATION 

      HARDBUG(!bSuccess)
      
      start_threads();

      hub_server(parent2child[1], child2parent[0]);
#endif
  
      if (options.nthreads > 1)
      {
        for (int ithread = 1; ithread < options.nthreads; ithread++)
          enqueue(return_thread_queue(threads + ithread),
            MESSAGE_EXIT_THREAD, "main/hub_server");
      }
  
      enqueue(return_thread_queue(thread_alpha_beta_master),
        MESSAGE_EXIT_THREAD, "main/hub_server");

      PRINTF("before join\n");
      join_threads();
    }
  }
  else if (compat_strcasecmp(argv[iarg], "gen_random") == 0)
  {
    gen_random("random.fen", 20, 20);
  }
  else if (compat_strcasecmp(argv[iarg], "gen_endgame") == 0)
  {
    gen_random("endgame.fen", 2, 2);
  }
  else if (compat_strcasecmp(argv[iarg], "read_games") == 0)
  {
    iarg++;
    HARDBUG(argv[iarg] == NULL)
  
    read_games(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "gen_book") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)
 
    int eval_time;

    HARDBUG(sscanf(argv[iarg], "%d", &eval_time) != 1)

    HARDBUG(eval_time < 0)

    iarg++;

    HARDBUG(argv[iarg] == NULL)
 
    int depth_limit;

    HARDBUG(sscanf(argv[iarg], "%d", &depth_limit) != 1)

    HARDBUG(depth_limit < 1)

    if (my_mpi_globals.MY_MPIG_nslaves < 1)
    {
      PRINTF("gen_book requires --nslaves >= 1\n");

      FATAL("gen_book", EXIT_FAILURE);
    }

    if (my_mpi_globals.MY_MPIG_id_slave != INVALID)
      gen_book(eval_time, depth_limit);
  }
  else if (compat_strcasecmp(argv[iarg], "walk_book") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)
 
    int eval_time;

    HARDBUG(sscanf(argv[iarg], "%d", &eval_time) != 1)

    HARDBUG(eval_time < 0)

    walk_book(eval_time);
  }
  else if (compat_strcasecmp(argv[iarg], "count_book") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)
 
    int depth_limit;

    HARDBUG(sscanf(argv[iarg], "%d", &depth_limit) != 1)

    count_book(depth_limit);
  }
  else if (compat_strcasecmp(argv[iarg], "gen_pos") == 0)
  {
    iarg++;
    HARDBUG(argv[iarg] == NULL)
 
    i64_t npositions_max;

    HARDBUG(sscanf(argv[iarg], "%lld", &npositions_max) != 1)

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    int npieces;

    HARDBUG(sscanf(argv[iarg], "%d", &npieces) != 1)

    if (my_mpi_globals.MY_MPIG_nslaves < 1)
    {
      PRINTF("gen_pos requires --nslaves >= 1\n");
      FATAL("gen_pos", EXIT_FAILURE);
    }

    i64_t memory_slaves = memory * options.nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    int nthreads_slaves = options.nthreads_alpha_beta * options.nslaves;
  
    //HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MY_MPIG_id_slave != INVALID)
      gen_pos(npositions_max, npieces);
  }
  else if (compat_strcasecmp(argv[iarg], "play_game") == 0)
  {
    //name

    iarg++;
    HARDBUG(argv[iarg] == NULL)
   
    char name[MY_LINE_MAX];
    HARDBUG(sscanf(argv[iarg], "%s", name) != 1)

    //colour

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    char colour[MY_LINE_MAX];
    HARDBUG(sscanf(argv[iarg], "%s", colour) != 1)

    //game_minutes

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    int game_minutes;
    HARDBUG(sscanf(argv[iarg], "%d", &game_minutes) != 1)

    //used_minutes

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    int used_minutes;
    HARDBUG(sscanf(argv[iarg], "%d", &used_minutes) != 1)

    options.mode = GAME_MODE;

    start_threads();

    while(TRUE)
    { 
      char line[MY_LINE_MAX];

      if (fexists(name))
      {
        PRINTF("\nDo you want to resume the game from %s (y/n)?\n", name);

        if (fgets(line, MY_LINE_MAX, stdin) == NULL) break;

        CSTRING(canswer, strlen(line))

        if (sscanf(line, "%s", canswer) != 1) strcpy(canswer, "y");

        if (*canswer == 'y')
        {
          PRINTF("Resuming game from %s\n", name);
        }
        else
        {
          PRINTF("New game %s\n", name);

          remove(name);
        }

        CDESTROY(canswer)
      }

      play_game(name, colour, game_minutes, used_minutes);

      PRINTF("\nDo you want to play another game (y/n)?\n");

      if (fgets(line, MY_LINE_MAX, stdin) == NULL) break;

      CSTRING(canswer, strlen(line))

      if (sscanf(line, "%s", canswer) != 1)
      {
        PRINTF("\neh?\n");

        continue;
      }

      char *answer = canswer;

      CDESTROY(canswer)

      if (*answer == 'n') break;
      
      for (int ihistory = 10; ihistory > 1; --ihistory)
      {
        char oldpath[MY_LINE_MAX];
        char newpath[MY_LINE_MAX];

        snprintf(oldpath, MY_LINE_MAX, "%s-%d", name, ihistory - 1);
        snprintf(newpath, MY_LINE_MAX, "%s-%d", name, ihistory);

        remove(newpath);

        rename(oldpath, newpath);
      }
      
      char newpath[MY_LINE_MAX];
  
      snprintf(newpath, MY_LINE_MAX, "%s-1", name);

      remove(newpath);

      rename(name, newpath);
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
          MESSAGE_EXIT_THREAD, "main/play_game");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_EXIT_THREAD, "main/play_game");

    PRINTF("before join\n");
    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "fen2network") == 0)
  { 
    iarg++;
    HARDBUG(argv[iarg] == NULL)

    if (options.nslaves < 1)
    {
      PRINTF("fen2network requires --nslaves >= 1\n");
      FATAL("fen2network", EXIT_FAILURE);
    }

    i64_t memory_slaves = memory * options.nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    int nthreads_slaves = options.nthreads_alpha_beta * options.nslaves;
  
    //HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MY_MPIG_id_slave != INVALID) fen2network(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "fen2bar") == 0)
  { 
    iarg++;
    HARDBUG(argv[iarg] == NULL)

    if (options.nslaves < 1)
    {
      PRINTF("fen2network requires --nslaves >= 1\n");
      FATAL("fen2network", EXIT_FAILURE);
    }

    i64_t memory_slaves = memory * options.nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    int nthreads_slaves = options.nthreads_alpha_beta * options.nslaves;
  
    //HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MY_MPIG_id_slave != INVALID) fen2bar(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "dxp_server") == 0)
  {
    HARDBUG(memory >= (physical_memory / 2))
    
    if (options.ponder)
      HARDBUG(options.nthreads_alpha_beta > (physical_cpus / 2))
    else
      HARDBUG(options.nthreads_alpha_beta > physical_cpus)

    start_threads();

    dxp_server();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
          MESSAGE_EXIT_THREAD, "main/dxp_server");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_EXIT_THREAD, "main/dxp_server");

    PRINTF("before join\n");
    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "dxp_client") == 0)
  {
    HARDBUG(memory >= (physical_memory / 2))
    
    if (options.ponder)
      HARDBUG(options.nthreads_alpha_beta > (physical_cpus / 2))
    else
      HARDBUG(options.nthreads_alpha_beta > physical_cpus)

    start_threads();

    dxp_client();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
          MESSAGE_EXIT_THREAD, "main/dxp_client");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
      MESSAGE_EXIT_THREAD, "main/dxp_client");

    PRINTF("before join\n");
    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "recompress_endgame") == 0)
  { 
    iarg++;
    HARDBUG(argv[iarg] == NULL)

    char name[MY_LINE_MAX];

    strncpy(name, argv[iarg], MY_LINE_MAX);

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    int level;

    HARDBUG(sscanf(argv[iarg], "%d", &level) != 1)

    iarg++;
    HARDBUG(argv[iarg] == NULL)

    int nblock;

    HARDBUG(sscanf(argv[iarg], "%d", &nblock) != 1)

    recompress_endgame(name, level, nblock);
  }
  else
  {
    PRINTF("unknown option or command %s\n", argv[iarg]);

    FATAL("unknown command", EXIT_FAILURE)
  }

  label_hub:

#ifdef USE_OPENMPI
  if (my_mpi_globals.MY_MPIG_nglobal > 0)
    my_mpi_barrier("main", my_mpi_globals.MY_MPIG_comm_global, TRUE);
#endif
  END_BLOCK

  //BEGIN_BLOCK("main-final")

    //finalization

    fin_endgame();
 
    fin_search();

    fin_my_malloc(FALSE);
  
    PRINTF("", NULL);
  
  //END_BLOCK

  DUMP_PROFILE(1)

  FATAL("OK", EXIT_SUCCESS)

  return(EXIT_SUCCESS);
}
