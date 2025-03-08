//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#define NRETRIES 5

#define NAG            '$'

#define RESULT_WON     "1-0"
#define RESULT_DRAW    "1/2-1/2"
#define RESULT_LOST    "0-1"
#define RESULT_UNKNOWN "*"

#define TOKEN_SKIP    0
#define TOKEN_MOVE    1
#define TOKEN_WON     2
#define TOKEN_DRAW    3
#define TOKEN_LOST    4
#define TOKEN_UNKNOWN 5

local int get_next_token(bstring arg_bstring, int *arg_ichar, bstring arg_btoken)
{
  int result = INVALID;

  HARDBUG(bassigncstr(arg_btoken, "NULL") == BSTR_ERR)

  while((*arg_ichar < blength(arg_bstring)) and
        (bchar(arg_bstring, *arg_ichar) == ' ')) (*arg_ichar)++;

  if (*arg_ichar >= blength(arg_bstring)) return(result);

  int jchar = *arg_ichar;

  if (strncmp(bdata(arg_bstring) + jchar, RESULT_WON,
              strlen(RESULT_WON)) == 0)
  {
    jchar += strlen("1-0");

    result = TOKEN_WON;

  }
  else if (strncmp(bdata(arg_bstring) + jchar, RESULT_DRAW,
                   strlen(RESULT_DRAW)) == 0)
  {
    jchar += strlen("1/2-1/2");

    result = TOKEN_DRAW;
  }
  else if (strncmp(bdata(arg_bstring) + jchar, RESULT_LOST,
                   strlen(RESULT_LOST)) == 0)
  {
    jchar += strlen("0-1");

    result = TOKEN_LOST;
  }
  else if (strncmp(bdata(arg_bstring) + jchar, RESULT_UNKNOWN,
                   strlen(RESULT_UNKNOWN)) == 0)
  {
    jchar += strlen("*");

    result = TOKEN_UNKNOWN;
  }
  else if ((bchar(arg_bstring, jchar) == '"') or
           (bchar(arg_bstring, jchar) == '[') or
           (bchar(arg_bstring, jchar) == '(') or
           (bchar(arg_bstring, jchar) == '{'))
  {  
    char bopen = bchar(arg_bstring, jchar);

    char bclose = '"';

    if (bopen == '[') bclose = ']';

    if (bopen == '(') bclose = ')';

    if (bopen == '{') bclose = '}';
    
    jchar++;

    int nest = 1;

    while(jchar < blength(arg_bstring)) 
    {
      if (bchar(arg_bstring, jchar) == bopen) ++nest;

      if (bchar(arg_bstring, jchar) == bclose)
      {
        if (--nest == 0) break;
      }

      jchar++;
    }
    HARDBUG(nest != 0)

    jchar++;

    result = TOKEN_SKIP;
  }
  else if (bchar(arg_bstring, jchar) == NAG)
  {
    jchar++;

    while ((jchar < blength(arg_bstring)) and
           isdigit(bchar(arg_bstring, jchar))) jchar++;

    result = TOKEN_SKIP;
  }
  else
  {
    //move number or move

    //read until space or '.'
  
    while (jchar < blength(arg_bstring))
    {
      if (bchar(arg_bstring, jchar) == ' ') break;
      if (bchar(arg_bstring, jchar) == '.') break;
  
      jchar++;
    }
  
    result = TOKEN_MOVE;

    if ((jchar < blength(arg_bstring)) and
        (bchar(arg_bstring, jchar) == '.'))
    {
      jchar++;
    
      result = TOKEN_SKIP;
    }
  }

  HARDBUG(jchar <= *arg_ichar)

  HARDBUG(bassignmidstr(arg_btoken, arg_bstring, *arg_ichar, jchar - *arg_ichar) == BSTR_ERR)

  *arg_ichar = jchar;

  return(result);
}

void read_games(char *arg_name)
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

  BSTRING(string);

  file2bstring(arg_name, string);

  board_t board;

  construct_board(&board, STDOUT, FALSE);

  board_t *with = &board;

  bucket_t bucket_nman;

  bucket_t bucket_rows[NPIECES_MAX + 1];

  bucket_t bucket_kings[NPIECES_MAX + 1];

  construct_bucket(&bucket_nman, "bucket_nman",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_rows + npieces, "bucket_rows",
                     1, 0, 9, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_kings + npieces, "bucket_kings",
                     1, 0, NPIECES_MAX, BUCKET_LINEAR);

  i64_t ngames = 0;
  i64_t nerrors = 0;

  state_t game_state;

  int ichar = 0;

  BSTRING(btoken)

  BSTRING(bmove_string)

  BSTRING(bfen)
 
  while(TRUE)
  {
    int itoken = INVALID;

    construct_state(&game_state);

    //read game into game_state

    while(TRUE)
    {
      //read tokens until result

      itoken = get_next_token(string, &ichar, btoken);

      if (itoken == INVALID) break;

      //PRINTF("itoken=%d btoken=%s\n", itoken, bdata(btoken));

      if (itoken == TOKEN_SKIP) continue;

      if (itoken != TOKEN_MOVE) break;

      game_state.push_move(&game_state, bdata(btoken), "");
    }

    if (itoken == INVALID) break;

    if (itoken == TOKEN_UNKNOWN) continue;
 
    int game_result = INVALID;

    if (itoken == TOKEN_WON)
      game_result = 2;
    else if (itoken == TOKEN_DRAW)
      game_result = 1;
    else if (itoken == TOKEN_LOST)
      game_result = 0;
    else 
      FATAL("invalid token", EXIT_FAILURE)

    HARDBUG(game_result == INVALID)

    ++ngames;

    if ((ngames % 1000) == 0) PRINTF("ngames=%lld\n", ngames);

    //replay the game

    fen2board(with, game_state.get_starting_position(&game_state), TRUE);

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))
      
      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) == BSTR_ERR)

      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(with, &moves_list, FALSE);

#ifdef DEBUG
      check_moves(with, &moves_list);
#endif

      int imove = INVALID;

      if ((imove = search_move(&moves_list, bmove_string)) == INVALID)
      {  
        print_board(with);

        PRINTF("move not found: %s\n", bdata(bmove_string));

        fprintf_moves_list(&moves_list, STDOUT, TRUE);

        ++nerrors;

        break;
      }

      //update statistics

      update_bucket(&bucket_nman, BIT_COUNT(with->board_black_man_bb));

      for (int i = 1; i <= 50; ++i)
      {
        int iboard = map[i];

        if (with->board_black_man_bb & BITULL(iboard))
        {
          update_bucket(bucket_rows + BIT_COUNT(with->board_black_man_bb),
            (i - 1) / 5);
        }
      }

      update_bucket(bucket_kings + BIT_COUNT(with->board_black_man_bb),
        BIT_COUNT(with->board_black_king_bb));

      //update book

      if (moves_list.nmoves > 0)
      {
        move2bstring(&moves_list, imove, bmove_string);
   
        int result = game_result;

        if (IS_BLACK(with->board_colour2move)) result = 2 - result;

        board2fen(with, bfen, FALSE);

        int position_id = query_position(db, bdata(bfen), 0);

        if (position_id == INVALID)
          position_id = insert_position(db, bdata(bfen), 0);

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

      do_move(with, imove, &moves_list);
    }

    destroy_state(&game_state);
  }

  BDESTROY(bfen)
 
  BDESTROY(bmove_string)

  BDESTROY(btoken)

  BDESTROY(string)

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

  HARDBUG(sqlite3_close(db) != SQLITE_OK)

  printf_bucket(&bucket_nman);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_rows + npieces);
  }

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_kings + npieces);
  }

  PRINTF("ngames=%lld nerrors=%lld\n", ngames, nerrors);
}

local int *semaphore = NULL;
local MPI_Win win;

void gen_db(char *arg_db_name,
  i64_t arg_npositions_max, int arg_mcts_depth, double arg_mcts_time_limit)
{
#ifdef DEBUG
  arg_npositions_max = 1000;
#endif
  PRINTF("npositions_max=%lld\n", arg_npositions_max);

  BSTRING(bdb_name)

  HARDBUG(bassigncstr(bdb_name, arg_db_name) == BSTR_ERR)

  //sqlite3 *db = (void *) INVALID;
  sqlite3 *db = NULL;

  fbuffer_t fdb;

  if (db != NULL)
  {
    if (my_mpi_globals.MY_MPIG_id_slave <= 0)
    {
      int rc = sqlite3_open(bdata(bdb_name), &db);
    
      if (rc != SQLITE_OK)
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
               arg_db_name, sqlite3_errmsg(db));
    
        FATAL("sqlite3", EXIT_FAILURE)
      }
  
      create_tables(db, 0);
  
      HARDBUG(execute_sql(db, "PRAGMA journal_mode = WAL;", FALSE, 0) !=
              SQLITE_OK)
    }

    my_mpi_barrier("after create_tables", my_mpi_globals.MY_MPIG_comm_slaves,
      TRUE);
  
    //open database
  
    if (my_mpi_globals.MY_MPIG_id_slave > 0)
    {
      int rc = sqlite3_open(bdata(bdb_name), &db);
    
      if (rc != SQLITE_OK)
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
               arg_db_name, sqlite3_errmsg(db));
    
        FATAL("sqlite3", EXIT_FAILURE)
      }
    }

    my_mpi_win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
      my_mpi_globals.MY_MPIG_comm_slaves, &semaphore, &win);

    if (semaphore != NULL) *semaphore = 0;

    my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_slaves, 0, win);
  }
  else
  {
    construct_fbuffer(&fdb, bdb_name, NULL, TRUE);
  }

  my_timer_t timer;

  construct_my_timer(&timer, "gen_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  my_random_t gen_db_random;

  if (my_mpi_globals.MY_MPIG_nslaves == 0)
    construct_my_random(&gen_db_random, 0);
  else
    construct_my_random(&gen_db_random, INVALID);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;
  
  i64_t npositions = arg_npositions_max; 

  if (my_mpi_globals.MY_MPIG_nslaves > 0)
    npositions = arg_npositions_max / my_mpi_globals.MY_MPIG_nslaves;

  HARDBUG(npositions < 1)

  PRINTF("npositions=%lld\n", npositions);

  i64_t ngames = 0;
  i64_t ngames_failed = 0;

  i64_t iposition = 0;
  i64_t iposition_prev = 0;

  BSTRING(bmove_string)

  BSTRING(bfen)

  bucket_t bucket_root_score;

  construct_bucket(&bucket_root_score, "bucket_root_score",
                   SCORE_MAN, SCORE_LOST, SCORE_WON, BUCKET_LINEAR);

  bucket_t bucket_npieces;

  construct_bucket(&bucket_npieces, "bucket_npieces",
                   1, 0, 2 * NPIECES_MAX, BUCKET_LINEAR);

  while(iposition < npositions)
  {
    ++ngames;

    char *starting_position;

    starting_position = STARTING_POSITION;

    string2board(&(with->S_board), starting_position, TRUE);

    PRINTF("auto play\n");

    //auto play 
    //no time limit

    mcts_globals.mcts_globals_time_limit = 0.0;

    int nply = 0;

    cJSON *game = cJSON_CreateObject();

    HARDBUG(game == NULL)
    
    cJSON *game_moves = cJSON_AddArrayToObject(game, "game_moves");

    HARDBUG(game_moves == NULL)

    int best_score = SCORE_PLUS_INFINITY;

    while(TRUE)
    {
      if (nply == MCTS_PLY_MAX) break;
 
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      if (moves_list.nmoves == 0) break;

      int best_pv;

      reset_my_timer(&(with->S_timer));

      int root_score = return_material_score(&(with->S_board));

      best_score = mcts_search(with, root_score, 0, arg_mcts_depth,
        &moves_list, &best_pv, TRUE);

      if (best_pv == INVALID) break;
    
      move2bstring(&moves_list, best_pv, bmove_string);

      print_board(&(with->S_board));

      PRINTF("nply=%d best_move=%s root_score=%d best_score=%d\n",
        nply, bdata(bmove_string), best_score);

      cJSON *game_move = cJSON_CreateObject();

      HARDBUG(game_move == NULL)

      HARDBUG(cJSON_AddStringToObject(game_move, CJSON_MOVE_STRING_ID,
                                      bdata(bmove_string)) == NULL)

      HARDBUG(cJSON_AddNumberToObject(game_move, CJSON_MOVE_SCORE_ID,
                                      best_score) == NULL)

      HARDBUG(cJSON_AddItemToArray(game_moves, game_move) == FALSE)

      ++nply;

      do_move(&(with->S_board), best_pv, &moves_list);
    }

    HARDBUG(best_score == SCORE_PLUS_INFINITY)

    int wdl;

    int egtb_mate = read_endgame(with, with->S_board.board_colour2move, &wdl);

    if (egtb_mate == ENDGAME_UNKNOWN)
    {
      ++ngames_failed;

      PRINTF("ngames=%lld ngames_failed=%lld (%.2f%%)\n",
        ngames, ngames_failed, ngames_failed * 100.0 / ngames);

      continue;
    }
    
    int result = INVALID;

    if (egtb_mate == INVALID)
      result = 1;
    else if (egtb_mate > 0)
      result = 2;
    else
      result = 0;

    //result as seen from white

    if (IS_BLACK(with->S_board.board_colour2move)) result = 2 - result;

    PRINTF("replay\n");

    //loop over all moves
    //use time limit

    mcts_globals.mcts_globals_time_limit = arg_mcts_time_limit;

    string2board(&(with->S_board), starting_position, TRUE);

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_moves)
    {
      cJSON *cjson_move_string =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(cjson_move_string))

      HARDBUG(bassigncstr(bmove_string,
                          cJSON_GetStringValue(cjson_move_string)) == BSTR_ERR)

      cJSON *cjson_move_score =
        cJSON_GetObjectItem(game_move, CJSON_MOVE_SCORE_ID);

      HARDBUG(!cJSON_IsNumber(cjson_move_score))

      //int best_score = round(cJSON_GetNumberValue(cjson_move_score));

      moves_list_t moves_list;
  
      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      HARDBUG(moves_list.nmoves < 1)
     
      int best_pv;

      HARDBUG((best_pv = search_move(&moves_list, bmove_string)) == INVALID)

      print_board(&(with->S_board));
 
      if ((moves_list.nmoves <= 1) or (moves_list.ncaptx > 0))
      {
        PRINTF("position rejected nmoves=%d ncaptx=%d\n",
               moves_list.nmoves, moves_list.ncaptx);

        do_move(&(with->S_board), best_pv, &moves_list);

        continue;
      }

      int root_score = return_material_score(&(with->S_board));

      reset_my_timer(&(with->S_timer));

      int temp_pv;

      best_score = mcts_search(with, root_score, 0, MCTS_PLY_MAX, 
        &moves_list, &temp_pv, TRUE);

      PRINTF("root_score=%d best_score=%d\n", root_score, best_score);

      if (abs(root_score - best_score) > (SCORE_MAN / 4))
      {
        PRINTF("position rejected root_score=%d best_score=%d\n",
               root_score, best_score);

        do_move(&(with->S_board), best_pv, &moves_list);

        continue;
      }

      int npieces = return_npieces(&(with->S_board));

      PRINTF("position accepted npieces=%d root_score=%d best_score=%d\n",
             npieces, root_score, best_score);

      update_bucket(&bucket_root_score, root_score);
      update_bucket(&bucket_npieces, npieces);

      board2fen(&(with->S_board), bfen, FALSE);

      if (db != NULL)
      {
        my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);
        
        HARDBUG(execute_sql(db, "BEGIN TRANSACTION;", FALSE, NRETRIES) !=
                SQLITE_OK)
    
        int position_id = query_position(db, bdata(bfen), 0);
        
        if (position_id == INVALID)
        {
          position_id = insert_position(db, bdata(bfen), 0);
    
          BSTRING(bmove)
    
          for (int imove = 0; imove < moves_list.nmoves; imove++)
          {
            move2bstring(&moves_list, imove, bmove);
                 
            (void) insert_move(db, position_id, bdata(bmove), 0);
          }
    
          BDESTROY(bmove)
        }

        int move_id = query_move(db, position_id, bdata(bmove_string), 0);
  
        HARDBUG(move_id == INVALID)
    
        if (IS_WHITE(with->S_board.board_colour2move))
        {
          if (result == 2)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 1, 0, 0, 0);
          else if (result == 1)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 1, 0, 0);
          else if (result == 0)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 0, 1, 0);
          else
            FATAL("unknown result", EXIT_FAILURE)
        }
        else
        {
          if (result == 2)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 0, 1, 0);
          else if (result == 1)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 1, 0, 0);
          else if (result == 0)
            increment_nwon_ndraw_nlost(db, position_id, move_id, 1, 0, 0, 0);
          else
            FATAL("unknown result", EXIT_FAILURE)
        }
          
        HARDBUG(execute_sql(db, "COMMIT;", FALSE, NRETRIES) != SQLITE_OK)

        my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);
      }
      else
      {
        board2fen(&(with->S_board), bfen, FALSE);
      
        append_fbuffer(&fdb, "%s {0.000000}\n", bdata(bfen));
      }

      ++iposition;

      if ((iposition - iposition_prev) > 100)
      {
        iposition_prev = iposition;
      
        PRINTF("iposition=%lld time=%.2f (%.2f positions/second)\n",
          iposition, return_my_timer(&timer, FALSE),
          iposition / return_my_timer(&timer, FALSE));
      }

      do_move(&(with->S_board), best_pv, &moves_list);
    }

    cJSON_Delete(game);
  }
 
  if (db == NULL) flush_fbuffer(&fdb, 0);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  PRINTF("generating %lld positions took %.2f seconds\n",
    iposition, return_my_timer(&timer, FALSE));
 
  printf_bucket(&bucket_root_score);
  printf_bucket(&bucket_npieces);

  my_mpi_barrier("after gen", my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  my_mpi_allreduce(MPI_IN_PLACE, &ngames, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &iposition, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("ngames=%lld\n", ngames);

  PRINTF("iposition=%lld\n", iposition);
  
  my_mpi_bucket_aggregate(&bucket_root_score,
    my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_bucket_aggregate(&bucket_npieces,
    my_mpi_globals.MY_MPIG_comm_slaves);

  printf_bucket(&bucket_root_score);
  printf_bucket(&bucket_npieces);

  if (db != NULL)
  {
    if (my_mpi_globals.MY_MPIG_id_slave > 0)
      HARDBUG(sqlite3_close(db) != SQLITE_OK)
  
    my_mpi_barrier("before wal_checkpoint",
                   my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

    if (my_mpi_globals.MY_MPIG_id_slave <= 0)
    {
      wal_checkpoint(db);

      HARDBUG(sqlite3_close(db) != SQLITE_OK)

      HARDBUG(sqlite3_open(bdata(bdb_name), &db) != SQLITE_OK)

      wal_checkpoint(db);

      HARDBUG(sqlite3_close(db) != SQLITE_OK)
    }
  
    my_mpi_barrier("after wal_checkpoint",
                   my_mpi_globals.MY_MPIG_comm_slaves, TRUE);
  }
 
  BDESTROY(bdb_name)
}

void update_db(char *arg_db_name, int arg_nshoot_outs,
  int arg_mcts_depth, double arg_mcts_time_limit)
{
  mcts_globals.mcts_globals_time_limit = arg_mcts_time_limit;

  BSTRING(bdb_name)

  HARDBUG(bassigncstr(bdb_name, arg_db_name) == BSTR_ERR)

  sqlite3 *db = NULL;

  //open database

  if (my_mpi_globals.MY_MPIG_id_slave <= 0)
  {
    int rc = sqlite3_open(bdata(bdb_name), &db);
  
    if (rc != SQLITE_OK)
    {
      PRINTF("Cannot open database: %s err_msg=%s\n",
             arg_db_name, sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(execute_sql(db, "PRAGMA journal_mode = WAL;", FALSE, 0) !=
            SQLITE_OK)
  }

  my_mpi_barrier("after open database", my_mpi_globals.MY_MPIG_comm_slaves,
                 TRUE);

  if (my_mpi_globals.MY_MPIG_id_slave > 0)
  {
    HARDBUG(my_mpi_globals.MY_MPIG_nslaves == 0)

    int rc = sqlite3_open(bdata(bdb_name), &db);
  
    if (rc != SQLITE_OK)
    {
      PRINTF("Cannot open database: %s err_msg=%s\n",
             arg_db_name, sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }
  }

  HARDBUG(execute_sql(db, "PRAGMA cache_size=-131072;", FALSE, 0) != SQLITE_OK)

  my_mpi_win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MY_MPIG_comm_slaves, &semaphore, &win);

  if (semaphore != NULL) *semaphore = 0;

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_slaves, 0, win);

  my_timer_t timer;

  construct_my_timer(&timer, "update_pos", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  //query the number of positions

  const char *sql = "SELECT COUNT(*) FROM positions;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG((rc = execute_sql(db, stmt, TRUE, 0)) != SQLITE_ROW)

  i64_t npositions = sqlite3_column_int(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("npositions=%lld\n", npositions);

  i64_t iposition = 0;
  i64_t nupdates = 0;

  BSTRING(bfen)

  BSTRING(bmove_string)

  sql_buffer_t sql_buffer;

  construct_sql_buffer(&sql_buffer, db,
                       my_mpi_globals.MY_MPIG_comm_slaves, win, INVALID);

  for (i64_t position_id = 1; position_id <= npositions; ++position_id)
  {
    if (my_mpi_globals.MY_MPIG_nslaves > 0)
    {
      if (((position_id - 1) % my_mpi_globals.MY_MPIG_nslaves) !=
          my_mpi_globals.MY_MPIG_id_slave) continue;
    }

    ++nupdates;

    if ((++iposition % 1000) == 0) PRINTF("iposition=%lld\n", iposition);

    sql = "SELECT position FROM positions WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
             sql, rc, sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(sqlite3_bind_int(stmt, 1, position_id) != SQLITE_OK)

    HARDBUG((rc = execute_sql(db, stmt, TRUE, NRETRIES)) != SQLITE_ROW)

    HARDBUG(bassigncstr(bfen,
                        (const char *) sqlite3_column_text(stmt, 0)) == BSTR_ERR)

    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    fen2board(&(with->S_board), bdata(bfen), FALSE);

    print_board(&(with->S_board));

    PRINTF("position_id=%lld FEN=%s\n", position_id, bdata(bfen));

    moves_list_t moves_list;
  
    construct_moves_list(&moves_list);

    gen_moves(&(with->S_board), &moves_list, FALSE);

    HARDBUG(moves_list.nmoves < 1)

    sql =
     "SELECT id, move, nwon, ndraw, nlost from moves WHERE position_id = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  
    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }
  
    HARDBUG(sqlite3_bind_int(stmt, 1, position_id) != SQLITE_OK)

    int move_id[MOVES_MAX];
    int nwon[MOVES_MAX];
    int ndraw[MOVES_MAX];
    int nlost[MOVES_MAX];

    for (int imove = 0; imove < moves_list.nmoves; imove++)
      move_id[imove] = nwon[imove] = ndraw[imove] = nlost[imove] = INVALID;
  
    while ((rc = execute_sql(db, stmt, TRUE, NRETRIES)) == SQLITE_ROW)
    {
      const unsigned char *move = sqlite3_column_text(stmt, 1);

      HARDBUG(bassigncstr(bmove_string, (char *) move) == BSTR_ERR)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bmove_string)) == INVALID)
    
      HARDBUG(move_id[imove] != INVALID)
      HARDBUG(nwon[imove] != INVALID)
      HARDBUG(ndraw[imove] != INVALID)
      HARDBUG(nlost[imove] != INVALID)

      move_id[imove] = sqlite3_column_int(stmt, 0);
      nwon[imove] = sqlite3_column_int(stmt, 2);
      ndraw[imove] = sqlite3_column_int(stmt, 3);
      nlost[imove] = sqlite3_column_int(stmt, 4);
  
      PRINTF("move=%s imove=%d move_id=%d nwon=%d ndraw=%d nlost=%d\n",
             bdata(bmove_string), imove, move_id[imove],
             nwon[imove], ndraw[imove], nlost[imove]);
    }//while ((rc

    HARDBUG(rc != SQLITE_DONE)

    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      HARDBUG(move_id[imove] == INVALID)
      HARDBUG(nwon[imove] == INVALID)
      HARDBUG(ndraw[imove] == INVALID)
      HARDBUG(nlost[imove] == INVALID)
    }

    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      move2bstring(&moves_list, imove, bmove_string);

      int nshoot_outs = arg_nshoot_outs -
                        (nwon[imove] + ndraw[imove] + nlost[imove]);

      if (nshoot_outs <= 0)
      {
        PRINTF("skipping move=%s nwon=%d ndraw=%d nlost=%d\n",
               bdata(bmove_string), nwon[imove], ndraw[imove], nlost[imove]);

        continue;
      }

      PRINTF("updating move=%s nwon=%d ndraw=%d nlost=%d\n",
             bdata(bmove_string), nwon[imove], ndraw[imove], nlost[imove]);

      int nwon_prev = nwon[imove];
      int ndraw_prev = ndraw[imove];
      int nlost_prev = nlost[imove];

      do_move(&(with->S_board), imove, &moves_list);

      reset_my_timer(&(with->S_timer));

      double result = 
        -mcts_shoot_outs(with, 0, &nshoot_outs,
                         arg_mcts_depth, arg_mcts_time_limit,
                         nlost + imove, ndraw + imove, nwon + imove);

      undo_move(&(with->S_board), imove, &moves_list);

      print_board(&(with->S_board));

      PRINTF("done move=%s nwon=%d ndraw=%d nlost=%d result=%.6f\n",
             bdata(bmove_string), nwon[imove], ndraw[imove], nlost[imove],
             result);

      HARDBUG(nwon[imove] < nwon_prev)
      HARDBUG(ndraw[imove] < ndraw_prev)
      HARDBUG(nlost[imove] < nlost_prev)

      append_sql_buffer(&sql_buffer,
        "UPDATE moves SET nwon = %d, ndraw = %d, nlost = %d"
        " WHERE position_id = %lld AND id = %d;",
        nwon[imove], ndraw[imove], nlost[imove], position_id, move_id[imove]);
    }//imove

    //flush_sql_buffer(&sql_buffer, 0);
  }//iposition

  flush_sql_buffer(&sql_buffer, 0);

  BDESTROY(bmove_string)

  BDESTROY(bfen)

  PRINTF("nupdates=%lld\n", nupdates);

  PRINTF("updating %lld positions took %.2f seconds\n",
    nupdates, return_my_timer(&timer, FALSE));

  my_mpi_allreduce(MPI_IN_PLACE, &nupdates, 1, MPI_LONG_LONG_INT,
                   MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("my_mpi_allreduce:nupdates=%lld\n", nupdates);

  //close database

  if (my_mpi_globals.MY_MPIG_id_slave > 0)
    HARDBUG(sqlite3_close(db) != SQLITE_OK)

  my_mpi_barrier("before wal_checkpoint",
                 my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  if (my_mpi_globals.MY_MPIG_id_slave <= 0)
  {
    wal_checkpoint(db);
    HARDBUG(sqlite3_close(db) != SQLITE_OK)
    HARDBUG(sqlite3_open(bdata(bdb_name), &db) != SQLITE_OK)
    wal_checkpoint(db);

    HARDBUG(sqlite3_close(db) != SQLITE_OK)
  }

  my_mpi_barrier("after wal_checkpoint",
                 my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  BDESTROY(bdb_name)
}

