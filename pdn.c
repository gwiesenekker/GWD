//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"

#undef SIGMOID

#define TOKEN_ERROR           (-1)
#define TOKEN_READY           0
#define TOKEN_LSQUARE_BRACKET 1
#define TOKEN_RSQUARE_BRACKET 2
#define TOKEN_LCURLY_BRACKET  3
#define TOKEN_LPARENTHESIS    4
#define TOKEN_COLON           5
#define TOKEN_COMMA           6
#define TOKEN_DOT             7
#define TOKEN_DOLLAR          8
#define TOKEN_MOVE            9
#define TOKEN_CAPTURE         10
#define TOKEN_STRING          11
#define TOKEN_RESULT          12
#define TOKEN_INTEGER         13
#define TOKEN_WORD            14

#define RESULT_WON      "1-0"
#define RESULT_WON_ALT  "2-0"
#define RESULT_DRAW     "1/2-1/2"
#define RESULT_DRAW_ALT "1-1"
#define RESULT_LOST     "0-1"
#define RESULT_LOST_ALT "0-2"
#define RESULT_UNKNOWN "*"

local char *pline;
local char token[MY_LINE_MAX];
local int itoken;
local char error[MY_LINE_MAX];

local void get_next_token(void)
{
  local char *ptoken;
  int i;

  ptoken = token;
  *ptoken = '\0';

  while (isspace(*pline))
    pline++;
  if (*pline == '\0')
  {
    itoken = TOKEN_READY;
    goto label_token;
  }
  if (*pline == '[')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_LSQUARE_BRACKET;
    goto label_token;
  }
  if (*pline == ']')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_RSQUARE_BRACKET;
    goto label_token;
  }
  if (*pline == '{')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_LCURLY_BRACKET;
    goto label_token;
  }
  if (*pline == '(')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_LPARENTHESIS;
    goto label_token;
  }
  if (*pline == ':')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_COLON;
    goto label_token;
  }
  if (*pline == ',')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_COMMA;
    goto label_token;
  }
  if (*pline == '.')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_DOT;
    goto label_token;
  }
  if (*pline == '$')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_DOLLAR;
    goto label_token;
  }
  if (*pline == '-')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_MOVE;
    goto label_token;
  }
  if (*pline == 'x')
  {
    *ptoken++ = *pline++;
    itoken = TOKEN_CAPTURE;
    goto label_token;
  }
  if (*pline == '"')
  {
    pline++;
    while(TRUE)
    {
      if (*pline == '\0')
      {
        snprintf(error, MY_LINE_MAX, "unterminated string");
        itoken = TOKEN_ERROR;  
        break;
      }  
      if (*pline == '"')
      {
        pline++;
        itoken = TOKEN_STRING;
        break;
      }
      *ptoken++ = *pline++;
    }
    goto label_token;
  }
  if (strncmp(pline, RESULT_WON, strlen(RESULT_WON)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_WON)]))
    {
      for (i = 0; i < strlen(RESULT_WON); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_WON_ALT, strlen(RESULT_WON_ALT)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_WON_ALT)]))
    {
      for (i = 0; i < strlen(RESULT_WON_ALT); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_DRAW, strlen(RESULT_DRAW)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_DRAW)]))
    {
      for (i = 0; i < strlen(RESULT_DRAW); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_DRAW_ALT, strlen(RESULT_DRAW_ALT)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_DRAW_ALT)]))
    {
      for (i = 0; i < strlen(RESULT_DRAW_ALT); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_LOST, strlen(RESULT_LOST)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_LOST)]))
    {
      for (i = 0; i < strlen(RESULT_LOST); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_LOST_ALT, strlen(RESULT_LOST_ALT)) == 0)
  {  
    if (!isdigit(pline[strlen(RESULT_LOST_ALT)]))
    {
      for (i = 0; i < strlen(RESULT_LOST_ALT); i++)
        *ptoken++ = *pline++;
      itoken = TOKEN_RESULT;
      goto label_token;
    }
  }
  if (strncmp(pline, RESULT_UNKNOWN, strlen(RESULT_UNKNOWN)) == 0)
  {  
    for (i = 0; i < strlen(RESULT_UNKNOWN); i++)
      *ptoken++ = *pline++;
    itoken = TOKEN_RESULT;
    goto label_token;
  }
  if (isdigit(*pline))
  {
    while(isdigit(*pline))
      *ptoken++ = *pline++;
    itoken = TOKEN_INTEGER;
    goto label_token;
  }
  if (isalpha(*pline))
  {
    while(isalpha(*pline))
      *ptoken++ = *pline++;
    itoken = TOKEN_WORD;
    goto label_token;
  }
  else
  {
    snprintf(error, MY_LINE_MAX, "invalid character '%c'", *pline);
    itoken = TOKEN_ERROR;
  }
  label_token:
  *ptoken = '\0';
}

void read_games(char *name)
{
  (void) remove("book.db");

  sqlite3 *db;

  int rc = sqlite3_open(":memory:", &db);

  //int rc = sqlite3_open("book.db", &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s\n", sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(execute_sql(db, "PRAGMA foreign_keys = ON;", FALSE, 0) != SQLITE_OK)

  create_tables(db, 0);

  HARDBUG(execute_sql(db, "BEGIN TRANSACTION;", FALSE, 0) != SQLITE_OK)

  FILE *fname;

  HARDBUG((fname = fopen(name, "r")) == NULL)

  FILE *fstring;

  HARDBUG((fstring = fopen("pos.str", "w")) == NULL)

  board_t board;

  construct_board(&board, STDOUT);

  board_t *with = &board;

  i64_t ngames = 0;
  int game = FALSE;
  int move_error = FALSE;
  int result = INVALID;
  int iply = 0;
  int nply = INVALID;

  long iline = 0;
  char line[MY_LINE_MAX];
  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    ++iline;
    pline = line;
    get_next_token();
    while(TRUE)
    {
      if (itoken == TOKEN_ERROR)
      {
        PRINTF("%ld:%s\n", iline, line);
        PRINTF("%s\n", error);
        FATAL("error in PDN", EXIT_FAILURE)
      }

      if (itoken == TOKEN_READY)
        break;

      if (itoken == TOKEN_LSQUARE_BRACKET)
      {
        get_next_token();
        if (itoken != TOKEN_WORD)
        {
          PRINTF("%ld:%s\n", iline, line);
          FATAL("WORD expected after '[' in PDN", EXIT_FAILURE)
        }
        if (compat_strcasecmp(token, "Event") == 0)
        {
          game = TRUE;
          iply = 0;
          nply = INVALID;

          string2board(with, STARTING_POSITION);

          get_next_token();
          if (itoken != TOKEN_STRING)
          {
            PRINTF("%ld:%s\n", iline, line);
            FATAL("STRING expected after '[WORD' in PDN", EXIT_FAILURE)
          }
        }
        else if (compat_strcasecmp(token, "Result") == 0)
        {
          HARDBUG(!game)
          get_next_token();
          if (itoken != TOKEN_STRING)
          {
            PRINTF("%ld:%s\n", iline, line);
            FATAL("STRING expected after '[Result' in PDN", EXIT_FAILURE)
          } 
          if ((compat_strcasecmp(token, RESULT_WON) == 0) or
              (compat_strcasecmp(token, RESULT_WON_ALT) == 0))
          {
            result = 2;
          }
          else if ((compat_strcasecmp(token, RESULT_DRAW) == 0) or
                   (compat_strcasecmp(token, RESULT_DRAW_ALT) == 0))
          {
            result = 1;
          }
          else if ((compat_strcasecmp(token, RESULT_LOST) == 0) or
                   (compat_strcasecmp(token, RESULT_LOST_ALT) == 0))
          {
            result = 0;
          }
          else  
          {
            result = INVALID;
          }
        }
        else if (compat_strcasecmp(token, "PlyCount") == 0)
        {
          HARDBUG(!game)
          get_next_token();
          if (itoken != TOKEN_STRING)
          {
            PRINTF("%ld:%s\n", iline, line);
            FATAL("STRING expected after '[PlyCount' in PDN", EXIT_FAILURE)
          } 
          if (sscanf(token, "%d", &nply) != 1)
          {
            PRINTF("%ld:%s\n", iline, line);
            FATAL("Number expected after '[PlyCount' in PDN", EXIT_FAILURE)
          }
          HARDBUG(nply < 0)
        }
        else
        {
          get_next_token();
          if (itoken != TOKEN_STRING)
          {
            PRINTF("%ld:%s\n", iline, line);
            FATAL("STRING expected after '[WORD' in PDN", EXIT_FAILURE)
          }
        }
        get_next_token();
        if (itoken != TOKEN_RSQUARE_BRACKET)
        {
          PRINTF("%ld:%s\n", iline, line);
          FATAL("] expected after '[WORD STRING' in PDN", EXIT_FAILURE)
        }

        get_next_token();
      }
      else if (itoken == TOKEN_LCURLY_BRACKET)
      {  
        int nest = 1;
        while(nest > 0)
        {
          if (*pline == '\0')
          {
            if (fgets(line, MY_LINE_MAX, fname) == NULL)
            {
              PRINTF("%ld:%s\n", iline, line);
              FATAL("unterminated comment in PDN", EXIT_FAILURE)
            }
            ++iline;
            pline = line;
            continue;
          }
          if (*pline == '{') ++nest;
          if (*pline == '}') --nest;
          ++pline;
        }
        get_next_token();
      }
      else if (itoken == TOKEN_LPARENTHESIS)
      {  
        int nest = 1;
        while(nest > 0)
        {
          if (*pline == '\0')
          {
            if (fgets(line, MY_LINE_MAX, fname) == NULL)
            {
              PRINTF("%ld:%s\n", iline, line);
              FATAL("unterminated comment in PDN", EXIT_FAILURE)
            }
            ++iline;
            pline = line;
            continue;
          }
          if (*pline == '(') ++nest;
          if (*pline == ')') --nest;
          ++pline;
        }
        get_next_token();
      }
      else if (itoken == TOKEN_DOLLAR)
      {
        get_next_token();
        if (itoken != TOKEN_INTEGER)
        {
          PRINTF("%ld:%s\n", iline, line);
          FATAL("INTEGER expected after '$' in PDN", EXIT_FAILURE)
        }
        get_next_token();
      }
      else if (itoken == TOKEN_INTEGER)
      {
        BSTRING(bmove_string)

        HARDBUG(bassigncstr(bmove_string, token) != BSTR_OK)

        HARDBUG(!game)

        get_next_token();

        if ((itoken == TOKEN_MOVE) or (itoken == TOKEN_CAPTURE))
        {
          while(TRUE)
          {
            HARDBUG(bcatcstr(bmove_string, token) != BSTR_OK)

            get_next_token();

            if (itoken != TOKEN_INTEGER)
            {
              PRINTF("%ld:%s\n", iline, line);

              FATAL("INTEGER expected after 'INTEGER-x' in PDN", EXIT_FAILURE)
            }

            HARDBUG(bcatcstr(bmove_string, token) != BSTR_OK)

            get_next_token();

            if ((itoken != TOKEN_MOVE) and (itoken != TOKEN_CAPTURE)) break;
          }

          //HARDBUG(return_white_score(with) != -return_black_score(with))

          moves_list_t moves_list;

          construct_moves_list(&moves_list);

          gen_moves(with, &moves_list, FALSE);

#ifdef DEBUG
          check_moves(with, &moves_list);
#endif
          int can_capture;

          if (IS_WHITE(with->board_colour2move))
            can_capture = black_can_capture(with, FALSE);
          else
            can_capture = white_can_capture(with, FALSE);
      
          if (move_error)
          {
            //PRINTF("%ld:%s\n", iline, line);

            PRINTF("ignoring move: %s\n", bdata(bmove_string));

            goto label_move_error;
          }

          int imove;

          if ((imove = search_move(&moves_list, bmove_string)) == INVALID)
          {  
            print_board(with);

            PRINTF("move not found: %s\n", bdata(bmove_string));

            fprintf_moves_list(&moves_list, STDOUT, TRUE);

            PRINTF("%ld:%s\n", iline, line);

            move_error = TRUE;

            goto label_move_error;
          }

          //bmove_string does not contain leading 0

          move2bstring(&moves_list, imove, bmove_string);
   
          if (moves_list.nmoves == 1) goto label_skip;

          //now add position and move to book

          if (result != INVALID)
          {
            if (IS_BLACK(with->board_colour2move)) result = 2 - result;

            char *position = board2string(with, FALSE);

            int position_id = query_position(db, position, 0);

            if (position_id == INVALID)
              position_id = insert_position(db, position, 0);

            int move_id = query_move(db, position_id, bdata(bmove_string), 0);

            if (move_id == INVALID)
              move_id = insert_move(db, position_id, bdata(bmove_string), 0);
  
            if (result == 2)
              increment_nwon_ndraw_nlost(db, position_id, move_id, 1, 0, 0, 0);
            else if (result == 1)
              increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 1, 0, 0);
            else if (result == 0)
              increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 0, 1, 0);
          }

          label_skip:

          if (FALSE and (result != INVALID) and
              IS_WHITE(with->board_colour2move) and
              (moves_list.nmoves > 1) and
              (moves_list.ncaptx == 0) and
              (with->board_white_king_bb == 0) and
              (with->board_black_king_bb == 0) and
              (BIT_COUNT(with->board_white_man_bb |
                         with->board_black_man_bb) > 8) and
              !can_capture)
          {
            HARDBUG(fprintf(fstring, "%s %d %d %d #result iply nply\n",
                            board2string(with, FALSE),
                            result, iply, nply) < 0)
          }
          do_move(with, imove, &moves_list);

          label_move_error:

          ++iply;
        }
        else if (itoken == TOKEN_DOT)
        {
          while(TRUE)
          {
            get_next_token();
            if (itoken != TOKEN_DOT) break;
          }
        }
        else
        {
          PRINTF("%ld:%s\n", iline, line);
          FATAL("unexpected token in PDN", EXIT_FAILURE)
        }

        BDESTROY(bmove_string)
      }
      else if (itoken == TOKEN_RESULT)
      {
        game = FALSE;    
        move_error = FALSE;

        result = INVALID;

        ++ngames;

        if ((ngames % 1000) == 0)
          PRINTF("processed %lld games\n", ngames);

        get_next_token();
      }
      else
      {
        PRINTF("%ld:%s\n", iline, line);
        FATAL("unexpected token in PDN", EXIT_FAILURE)
      }
    }
  }
  FCLOSE(fname)
  FCLOSE(fstring)

  HARDBUG(execute_sql(db, "END TRANSACTION;", FALSE, 0) != SQLITE_OK)

  PRINTF("cleaning up..\n");

  HARDBUG(execute_sql(db, "BEGIN TRANSACTION;", FALSE, 0) != SQLITE_OK)

  const char *delete_moves_sql =
    "DELETE FROM moves "
    "WHERE position_id IN ("
    "  SELECT position_id "
    "  FROM moves "
    "  GROUP BY position_id "
    "  HAVING COUNT(*) = 1"
    ");";

  HARDBUG(execute_sql(db, delete_moves_sql, FALSE, 0) != SQLITE_OK)

  const char *delete_positions_sql =
    "DELETE FROM positions "
    "WHERE id NOT IN ("
    "  SELECT DISTINCT position_id "
    "  FROM moves"
    ");";

  HARDBUG(execute_sql(db, delete_positions_sql, FALSE, 0) != SQLITE_OK)

  HARDBUG(execute_sql(db, "END TRANSACTION;", FALSE, 0) != SQLITE_OK)

  PRINTF("..done\n");

  HARDBUG(execute_sql(db, "VACUUM;", FALSE, 0) != SQLITE_OK)

  backup_db(db, "book.db");

  sqlite3_close(db);

  PRINTF("read %lld games\n", ngames);
}

#define NBUFFER_MAX 1000000

local void flush_buffer(bstring bname, bstring bbuffer)
{
  int fd = compat_lock_file(bdata(bname));

  HARDBUG(fd == -1)

  HARDBUG(compat_write(fd, bdata(bbuffer), blength(bbuffer)) !=
                       blength(bbuffer))

  compat_unlock_file(fd);

  HARDBUG(bassigncstr(bbuffer, "") != BSTR_OK)
}

local void append_fen(bstring fen, double result, int egtb_mate,
  int *nbuffer, bstring bname, bstring bbuffer)
{
  if (*nbuffer >= NBUFFER_MAX)
  {
    flush_buffer(bname, bbuffer);

    *nbuffer = 0;
  }

  bformata(bbuffer, "%s {%.6f} egtb_mate=%d\n",
           bdata(fen), result, egtb_mate);

  (*nbuffer)++;
}

#define DEPTH_GEN    4
#define DEPTH_MCTS   (INVALID)
#define NSHOOT_OUTS  100

void gen_pos(i64_t npositions_max, int npieces)
{
  options.time_limit = 0.5;
  options.time_ntrouble = 0;
  options.use_reductions = FALSE;

  PRINTF("options.material_only=%d\n", options.material_only);
  PRINTF("options.use_reductions=%d\n", options.use_reductions);

#ifndef USE_OPENMPI
  FATAL("gen_pos disabled", EXIT_FAILURE)
#else
#ifdef DEBUG
  npositions_max = 1000;
#endif
  PRINTF("npositions_max=%lld\n", npositions_max);

  HARDBUG(my_mpi_globals.MY_MPIG_nslaves < 1)

  my_timer_t timer;

  construct_my_timer(&timer, "gen_pos", STDOUT, FALSE);

  reset_my_timer(&timer);

  BSTRING(gen_bname)

  bassigncstr(gen_bname, "gen.fen");

  int gen_nbuffer = 0;

  BSTRING(gen_bbuffer)

  my_random_t gen_pos_random;

  if (my_mpi_globals.MY_MPIG_nslaves == 1)
    construct_my_random(&gen_pos_random, 0);
  else
    construct_my_random(&gen_pos_random, INVALID);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;
  
  i64_t npositions = npositions_max / my_mpi_globals.MY_MPIG_nslaves;
  HARDBUG(npositions < 1)

  PRINTF("npositions=%lld\n", npositions);

  i64_t ngames = 0;
  i64_t ngames_failed = 0;
  i64_t nwon = 0;
  i64_t ndraw = 0;
  i64_t nlost = 0;

  i64_t iposition = 0;
  i64_t iposition_prev = 0;

  ui64_t alpha_beta_cache_size = options.alpha_beta_cache_size;

  BSTRING(bbest_move)

  BSTRING(bfen)

  while(iposition < npositions)
  {
    ++ngames;

    options.material_only = TRUE;

    options.alpha_beta_cache_size = 0;

    PRINTF("options.material_only=%d\n", options.material_only);

    PRINTF("options.alpha_beta_cache_size=%lldd\n",
           options.alpha_beta_cache_size);

    string2board(&(with->S_board), STARTING_POSITION);

    //autoplay 

    int nply = 0;

    cJSON *game = cJSON_CreateObject();

    HARDBUG(game == NULL)
    
    cJSON *game_moves = cJSON_AddArrayToObject(game, "game_moves");

    HARDBUG(game_moves == NULL)

    int ncount = 0;

    while(TRUE)
    {
      if (nply == MCTS_PLY_MAX) break;
 
      ncount = 
        BIT_COUNT(with->S_board.board_white_man_bb |
                  with->S_board.board_white_king_bb |
                  with->S_board.board_black_man_bb |
                  with->S_board.board_black_king_bb);

      if (ncount < npieces) break;

      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      if (moves_list.nmoves == 0) break;

      do_search(with, &moves_list, 1, DEPTH_GEN,
        SCORE_MINUS_INFINITY, &gen_pos_random);

      int best_score = with->S_best_score;

      move2bstring(&moves_list, with->S_best_move, bbest_move);

      print_board(&(with->S_board));

      PRINTF("nply=%d best_move=%s best_score=%d\n",
        nply, bdata(bbest_move), best_score);

      cJSON *game_move = cJSON_CreateObject();

      HARDBUG(game_move == NULL)

      HARDBUG(cJSON_AddStringToObject(game_move, CJSON_MOVE_STRING_ID,
                                      bdata(bbest_move)) == NULL)

      HARDBUG(cJSON_AddNumberToObject(game_move, CJSON_MOVE_SCORE_ID,
                                      best_score) == NULL)

      HARDBUG(cJSON_AddItemToArray(game_moves, game_move) == FALSE)

      ++nply;

      if (with->S_best_score_kind == SEARCH_BEST_SCORE_EGTB) break;

      do_move(&(with->S_board), with->S_best_move, &moves_list);
    }

    if (ncount > npieces)
    {
      ++ngames_failed;
 
      PRINTF("ngames=%lld ngames_failed=%lld (%.2f%%)\n",
        ngames, ngames_failed, ngames_failed * 100.0 / ngames);

      continue;
    }

    //now loop over all moves

    if (npieces > 6) options.material_only = FALSE;

    options.alpha_beta_cache_size = alpha_beta_cache_size;

    PRINTF("options.material_only=%d\n", options.material_only);

    PRINTF("options.alpha_beta_cache_size=%lld\n",
           options.alpha_beta_cache_size);

    string2board(&(with->S_board), STARTING_POSITION);

    int iply = 0;

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_moves)
    {
      cJSON *cjson_move_string =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(cjson_move_string))

      HARDBUG(bassigncstr(bbest_move,
                          cJSON_GetStringValue(cjson_move_string)) != BSTR_OK)

      cJSON *cjson_move_score =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_SCORE_ID);

      HARDBUG(!cJSON_IsNumber(cjson_move_score))

      int best_score = round(cJSON_GetNumberValue(cjson_move_score));

      moves_list_t moves_list;
  
      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      HARDBUG(moves_list.nmoves < 1)
     
      int best_pv;

      HARDBUG((best_pv = search_move(&moves_list, bbest_move)) == INVALID)

      ncount = 
        BIT_COUNT(with->S_board.board_white_man_bb |
                  with->S_board.board_white_king_bb |
                  with->S_board.board_black_man_bb |
                  with->S_board.board_black_king_bb);

      if ((moves_list.nmoves > 1) and
          (moves_list.ncaptx == 0) and
          (ncount == npieces))
      {
        print_board(&(with->S_board));

        reset_my_timer(&(with->S_timer));

        double mcts_result = mcts_shoot_outs(with, iply,
          NSHOOT_OUTS, DEPTH_MCTS);

        int egtb_mate =
          read_endgame(with, with->S_board.board_colour2move, TRUE);

        board2fen(with->S_board.board_colour2move,
          with->S_board.board_white_man_bb,
          with->S_board.board_black_man_bb,
          with->S_board.board_white_king_bb,
          with->S_board.board_black_king_bb,
          bfen, FALSE);

        PRINTF("bfen=%s best_score=%d egtb_mate=%d mcts_result=%.6f\n",
          bdata(bfen), best_score, egtb_mate, mcts_result);

        if (egtb_mate != ENDGAME_UNKNOWN)
        {
          if (((egtb_mate == INVALID) and (fabs(mcts_result) <= 0.20)) or
              ((egtb_mate > 0) and (mcts_result >= 0.60)) or
              ((egtb_mate <= 0) and (mcts_result <= -0.60)))
          {
            append_fen(bfen, mcts_result, egtb_mate,
                       &gen_nbuffer, gen_bname, gen_bbuffer);

            ++iposition;

            if ((iposition - iposition_prev) > 100)
            {
              iposition_prev = iposition;
    
              PRINTF("iposition=%lld time=%.2f (%.2f positions/second)\n",
                iposition, return_my_timer(&timer, FALSE),
                iposition / return_my_timer(&timer, FALSE));
            }
          }
        }
      }

      do_move(&(with->S_board), best_pv, &moves_list);

      ++iply;
    }

    cJSON_Delete(game);
  }

  BDESTROY(bfen)

  BDESTROY(bbest_move)

  //write remaining

  if (gen_nbuffer > 0) flush_buffer(gen_bname, gen_bbuffer);

  BDESTROY(gen_bbuffer)

  BDESTROY(gen_bname)

  PRINTF("generating %lld positions took %.2f seconds\n",
    iposition, return_my_timer(&timer, FALSE));

  my_mpi_barrier("after gen", my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  MPI_Allreduce(MPI_IN_PLACE, &ngames, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nwon, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &ndraw, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nlost, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &iposition, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("ngames=%lld nwon=%lld ndraw=%lld nlost=%lld\n",
    ngames, nwon, ndraw, nlost);

  PRINTF("iposition=%lld\n", iposition);
#endif
}

