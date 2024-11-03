//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
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

#define the_pv(X) cat2(X, _pv)
#define my_pv     the_pv(my_colour)
#define your_pv   the_pv(your_colour)

local char *pline;
local char token[MY_LINE_MAX];
local int itoken;
local char error[MY_LINE_MAX];

local void white_pv(int, board_t *, pv_t *, double, FILE *, i64_t *);
local void black_pv(int, board_t *, pv_t *, double, FILE *, i64_t *);

#define MY_BIT      WHITE_BIT
#define YOUR_BIT    BLACK_BIT
#define my_colour   white
#define your_colour black

#include "pdn.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define YOUR_BIT    WHITE_BIT
#define my_colour   black
#define your_colour white

#include "pdn.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

local void do_pv(int npieces, board_t *with, pv_t *best_pv, double pv_score,
  FILE *ffen, i64_t *iposition)
{
  if (IS_WHITE(with->board_colour2move))
    white_pv(npieces, with, best_pv, pv_score, ffen, iposition);
  else
    black_pv(npieces, with, best_pv, pv_score, ffen, iposition);
}

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

  int iboard = create_board(STDOUT, NULL);

  board_t *with = return_with_board(iboard);

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
          string2board(STARTING_POSITION, iboard);

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
        char move_string[MY_LINE_MAX];

        strcpy(move_string, token);
        HARDBUG(!game)

        get_next_token();
        if ((itoken == TOKEN_MOVE) or (itoken == TOKEN_CAPTURE))
        {
          while(TRUE)
          {
            strcat(move_string, token);

            get_next_token();
            if (itoken != TOKEN_INTEGER)
            {
              PRINTF("%ld:%s\n", iline, line);
              FATAL("INTEGER expected after 'INTEGER-x' in PDN", EXIT_FAILURE)
            }
            strcat(move_string, token);

            get_next_token();
            if ((itoken != TOKEN_MOVE) and (itoken != TOKEN_CAPTURE)) break;
          }

          //HARDBUG(return_white_score(with) != -return_black_score(with))

          moves_list_t moves_list;

          create_moves_list(&moves_list);

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
            PRINTF("ignoring move: %s\n", move_string);
            goto label_move_error;
          }

          int imove;

          if ((imove = search_move(&moves_list, move_string)) == INVALID)
          {  
            print_board(iboard);

            PRINTF("move not found: %s\n", move_string);

            moves_list.printf_moves(&moves_list, TRUE);

            PRINTF("%ld:%s\n", iline, line);

            move_error = TRUE;

            goto label_move_error;
          }

          //move_string does not contain leading 0

          strcpy(move_string, moves_list.move2string(&moves_list, imove));
   
          if (moves_list.nmoves == 1) goto label_skip;

          //now add position and move to book

          if (result != INVALID)
          {
            if (IS_BLACK(with->board_colour2move)) result = 2 - result;

            char *position = with->board2string(with, FALSE);

            int position_id = query_position(db, position, 0);

            if (position_id == INVALID)
              position_id = insert_position(db, position, 0);

            int move_id = query_move(db, position_id, move_string, 0);
            if (move_id == INVALID)
              move_id = insert_move(db, position_id, move_string, 0);
  
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
                                 with->board2string(with, FALSE),
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

local double return_sigmoid(double x)
{
  return(2.0 / (1.0 + exp(-x)) - 1.0);
}

#define DEPTH_GEN    4
#define DEPTH_FINAL  64
#define DEPTH_SEARCH 64

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

  FILE *ffen;

  HARDBUG(my_mpi_globals.MY_MPIG_nslaves < 1)

  my_timer_t timer;

  construct_my_timer(&timer, "gen_pos", STDOUT, FALSE);

  reset_my_timer(&timer);

  my_random_t gen_pos_random;

  if (my_mpi_globals.MY_MPIG_nslaves == 1)
  {
    char fen[MY_LINE_MAX];

    snprintf(fen, MY_LINE_MAX, "gen-%d.fen", npieces);

    HARDBUG((ffen = fopen(fen, "w")) == NULL)

    construct_my_random(&gen_pos_random, 0);
  }
  else
  {
    char fen[MY_LINE_MAX];

    snprintf(fen, MY_LINE_MAX, "gen-%d.fen;%d;%d",
      npieces, my_mpi_globals.MY_MPIG_id_slave, my_mpi_globals.MY_MPIG_nslaves);

    HARDBUG((ffen = fopen(fen, "w")) == NULL)

    construct_my_random(&gen_pos_random, INVALID);
  }

  int iboard = create_board(STDOUT, NULL);
  board_t *with = return_with_board(iboard);
  
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

  while(iposition < npositions)
  {
    ++ngames;

    options.material_only = TRUE;

    options.alpha_beta_cache_size = 0;

    PRINTF("options.material_only=%d\n", options.material_only);
    PRINTF("options.alpha_beta_cache_size=%lldd\n",
           options.alpha_beta_cache_size);

    string2board(STARTING_POSITION, iboard);

    //autoplay 

    int nply = 0;

    cJSON *game = cJSON_CreateObject();

    HARDBUG(game == NULL)
    
    cJSON *game_moves = cJSON_AddArrayToObject(game, "game_moves");

    HARDBUG(game_moves == NULL)

    int ncount = 0;

    while(TRUE)
    {
      HARDBUG(nply > MCTS_PLY_MAX)

      moves_list_t moves_list;

      create_moves_list(&moves_list);

      gen_moves(with, &moves_list, FALSE);

      if (moves_list.nmoves == 0) break;

      search(with, &moves_list, 1, DEPTH_GEN,
        SCORE_MINUS_INFINITY, &gen_pos_random);

      int best_score = with->board_search_best_score;

      char best_move[MY_LINE_MAX];

      strcpy(best_move,
        moves_list.move2string(&moves_list, with->board_search_best_move));

      print_board(iboard);

      PRINTF("nply=%d best_move=%s best_score=%d\n",
        nply, best_move, best_score);

      ncount = 
        BIT_COUNT(with->board_white_man_bb |
                  with->board_white_king_bb |
                  with->board_black_man_bb |
                  with->board_black_king_bb);

      //if (best_score > (SCORE_WON - NODE_MAX)) break;

      //if (best_score < (SCORE_LOST + NODE_MAX)) break;

      //if (with->board_search_best_score_kind == SEARCH_BEST_SCORE_EGTB) break;

      //if (ncount < npieces) break;

      if (nply == MCTS_PLY_MAX) break;
 
      cJSON *game_move = cJSON_CreateObject();

      HARDBUG(game_move == NULL)

      HARDBUG(cJSON_AddStringToObject(game_move, CJSON_MOVE_STRING_ID,
                                    best_move) == NULL)

      HARDBUG(cJSON_AddNumberToObject(game_move, CJSON_MOVE_SCORE_ID,
                                    best_score) == NULL)

      HARDBUG(cJSON_AddItemToArray(game_moves, game_move) == FALSE)

      ++nply;

      if (with->board_search_best_score_kind == SEARCH_BEST_SCORE_EGTB) break;

      do_move(with, with->board_search_best_move, &moves_list);
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

    string2board(STARTING_POSITION, iboard);

    int iply = 0;

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_moves)
    {
      cJSON *cjson_move_string =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(cjson_move_string))

      char best_move[MY_LINE_MAX];

      strncpy(best_move, cJSON_GetStringValue(cjson_move_string), MY_LINE_MAX);

      cJSON *cjson_move_score =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_SCORE_ID);

      HARDBUG(!cJSON_IsNumber(cjson_move_score))

      int best_score = round(cJSON_GetNumberValue(cjson_move_score));

      moves_list_t moves_list;
  
      create_moves_list(&moves_list);

      gen_moves(with, &moves_list, FALSE);

      HARDBUG(moves_list.nmoves < 1)
     
      int best_pv;

      HARDBUG((best_pv = search_move(&moves_list, best_move)) == INVALID)

      ncount = 
        BIT_COUNT(with->board_white_man_bb |
                  with->board_white_king_bb |
                  with->board_black_man_bb |
                  with->board_black_king_bb);

      if ((moves_list.nmoves > 1) and
          (moves_list.ncaptx == 0) and
          (ncount == npieces))
      {
        print_board(iboard);
    
        search(with, &moves_list, 1, DEPTH_SEARCH,
               SCORE_MINUS_INFINITY, &gen_pos_random);

        int search_best_score = with->board_search_best_score;
        int search_best_depth = with->board_search_best_depth;

        char search_best_move[MY_LINE_MAX];

        strcpy(search_best_move,
          moves_list.move2string(&moves_list, with->board_search_best_move));

        double search_result =
          return_sigmoid(with->board_search_best_score / 100.0);

        PRINTF("iply=%d nply=%d ncount=%d"
               " best_move=%s"
               " best_score=%d"
               " search_best_move=%s"
               " search_best_score=%d"
               " search_best_depth=%d"
               " search_result=%.5f\n",
               iply, nply, ncount,
               best_move,
               best_score,
               search_best_move,
               search_best_score,
               search_best_depth,
               search_result);

        char fen[MY_LINE_MAX];
  
        board2fen(with->board_id, fen, FALSE);

        PRINTF("fen=%s\n", fen);

        double search_label = search_result;

        if (IS_BLACK(with->board_colour2move)) search_label = -search_label;
  
        HARDBUG(fprintf(ffen, "%s {%.5f} iply=%d nply=%d ncount=%d"
                              " search_label=W2M"
                              " search_best_move=%s"
                              " search_best_score=%d"
                              " search_best_depth=%d"
                              " search_result=%.5f\n",
                              fen, search_label, iply, nply, ncount,
                              search_best_move,
                              search_best_score,
                              search_best_depth,
                              search_result) < 0)

        HARDBUG(fflush(ffen) != 0)
      
        ++iposition;

        do_move(with, with->board_search_best_move, &moves_list);

        do_pv(npieces, with, with->board_search_best_pv + 1, -search_result,
          ffen, &iposition);

        undo_move(with, with->board_search_best_move, &moves_list);

        if ((iposition - iposition_prev) > 100)
        {
          iposition_prev = iposition;

          PRINTF("iposition=%lld time=%.2f (%.2f positions/second)\n",
            iposition, return_my_timer(&timer, FALSE),
            iposition / return_my_timer(&timer, FALSE));
        }
      }

      do_move(with, best_pv, &moves_list);

      ++iply;
    }
    HARDBUG(iply != nply)

    cJSON_Delete(game);
  }

  FCLOSE(ffen)

  PRINTF("generating %lld positions took %.2f seconds\n",
    iposition, return_my_timer(&timer, FALSE));

  PRINTF("");

  my_mpi_barrier("after gen", my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  MPI_Allreduce(MPI_IN_PLACE, &ngames, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nwon, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &ndraw, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nlost, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("ngames=%lld nwon=%lld ndraw=%lld nlost=%lld\n",
    ngames, nwon, ndraw, nlost);

  MPI_Allreduce(MPI_IN_PLACE, &iposition, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("iposition=%lld\n", iposition);
#endif
}

