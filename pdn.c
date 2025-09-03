//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

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

#define NPLY_PAUSE    10

local int get_next_token(bstring arg_bstring, int *arg_ichar,
  bstring arg_btoken)
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

void read_games(char *arg_db_name, char *arg_pdn_name)
{
  sqlite3 *db;

  int rc = my_sqlite3_open(arg_db_name, &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s\n", my_sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(execute_sql(db, "PRAGMA foreign_keys = ON;", FALSE) != SQLITE_OK)

  create_tables(db);

  BSTRING(string);

  file2bstring(arg_pdn_name, string);

  board_t board;

  construct_board(&board, STDOUT);

  board_t *with = &board;

  bucket_t bucket_nman;

  bucket_t bucket_rows[NPIECES_MAX + 1];

  bucket_t bucket_kings[NPIECES_MAX + 1];

  bucket_t bucket_nmoves[NPIECES_MAX + 1];

  bucket_t bucket_nblocked[NPIECES_MAX + 1];

  construct_bucket(&bucket_nman, "bucket_nman",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_rows + npieces, "bucket_rows",
                     1, 0, 9, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_kings + npieces, "bucket_kings",
                     1, 0, NPIECES_MAX, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_nmoves + npieces, "bucket_nmoves",
                     1, 0, NMOVES_MAX, BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
    construct_bucket(bucket_nblocked + npieces, "bucket_nblocked",
                     1, 0, NMOVES_MAX, BUCKET_LINEAR);

  i64_t ngames = 0;
  i64_t nerrors = 0;

  game_state_t game_state;

  int ichar = 0;

  BSTRING(btoken)

  BSTRING(bmove_string)

  BSTRING(bfen)
 
  construct_game_state(&game_state);

  while(TRUE)
  {
    int itoken = INVALID;

    //read game into game_state

    game_state.clear_moves(&game_state);

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
 
    ++ngames;

    if ((ngames % 1000) == 0) PRINTF("ngames=%lld\n", ngames);

    //replay the game

    fen2board(with, game_state.get_starting_position(&game_state));

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(with, &moves_list, FALSE);

      //update statistics

      update_bucket(&bucket_nman, BIT_COUNT(with->B_black_man_bb));

      for (int i = 1; i <= 50; ++i)
      {
        int ifield = square2field[i];

        if (with->B_black_man_bb & BITULL(ifield))
        {
          update_bucket(bucket_rows + BIT_COUNT(with->B_black_man_bb),
            (i - 1) / 5);
        }
      }

      update_bucket(bucket_kings + BIT_COUNT(with->B_black_man_bb),
        BIT_COUNT(with->B_black_king_bb));

      if (IS_WHITE(with->B_colour2move))
      {
        update_bucket(bucket_nmoves + BIT_COUNT(with->B_white_man_bb),
                      moves_list.ML_nmoves);

        update_bucket(bucket_nblocked + BIT_COUNT(with->B_white_man_bb),
                      moves_list.ML_nblocked);
      }
      else
      {
        update_bucket(bucket_nmoves + BIT_COUNT(with->B_black_man_bb),
                      moves_list.ML_nmoves);

        update_bucket(bucket_nblocked + BIT_COUNT(with->B_black_man_bb),
                      moves_list.ML_nblocked);
      }

      //update book

#ifdef DEBUG
      check_moves(with, &moves_list);
#endif

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(with, bfen, FALSE);

        i64_t position_id = query_position(db, bdata(bfen), NULL);

        if (position_id == INVALID)
          position_id = insert_position(db, bdata(bfen));
        
        const char *sql =
          "UPDATE positions"
          " SET frequency = frequency + 1"
          " WHERE id = ?;";
      
        sqlite3_stmt *stmt;
      
        my_sqlite3_prepare_v2(db, sql, &stmt);
      
        HARDBUG(my_sqlite3_bind_int64(stmt, 1, position_id) != SQLITE_OK)
      
        HARDBUG(execute_sql(db, stmt, TRUE) != SQLITE_DONE)
      
        HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)
      }

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))
      
      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove = INVALID;

      if ((imove = search_move(&moves_list, bmove_string)) == INVALID)
      {  
        print_board(with);

        PRINTF("move not found: %s\n", bdata(bmove_string));

        fprintf_moves_list(&moves_list, STDOUT, TRUE);

        ++nerrors;

        break;
      }

      do_move(with, imove, &moves_list);
    }
  }

  destroy_game_state(&game_state);

  BDESTROY(bfen)
 
  BDESTROY(bmove_string)

  BDESTROY(btoken)

  BDESTROY(string)

  PRINTF("cleaning up..\n");

  const char *sql = 
    "DELETE FROM positions WHERE frequency = 1;";

  HARDBUG(execute_sql(db, sql, FALSE) != SQLITE_OK)

  PRINTF("..done\n");

  HARDBUG(execute_sql(db, "VACUUM;", FALSE) != SQLITE_OK)

  HARDBUG(my_sqlite3_close(db) != SQLITE_OK)

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

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_nmoves + npieces);
  }

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)  
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_nblocked + npieces);
  }

  PRINTF("ngames=%lld nerrors=%lld\n", ngames, nerrors);
}

void gen_db(my_sqlite3_t *arg_db, i64_t arg_npositions, int arg_mcts_depth)
{
#ifdef DEBUG
  arg_npositions = 1000;
#endif
  PRINTF("arg_npositions=%lld\n", arg_npositions);

  my_timer_t timer;

  construct_my_timer(&timer, "gen_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;
  
  i64_t ntodo = arg_npositions; 

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = arg_npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  i64_t ngames = 0;
  i64_t ngames_failed = 0;

  i64_t ndone = 0;

  BSTRING(bmove_string)

  BSTRING(bfen)

  game_state_t game_state;

  construct_game_state(&game_state);

  while(ndone < ntodo)
  {
    ++ngames;

    fen2board(&(with->S_board), game_state.get_starting_position(&game_state));

    game_state.clear_moves(&game_state);

    PRINTF("auto play\n");

    //auto play 
    //no time limit

    mcts_globals.mcts_globals_time_limit = 0.0;

    int nply = 0;

    int best_score = SCORE_PLUS_INFINITY;

    while(TRUE)
    {
      if (nply == MCTS_PLY_MAX) break;
 
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      if (moves_list.ML_nmoves == 0) break;

      int best_pv;

      reset_my_timer(&(with->S_timer));

      int root_score = return_material_score(&(with->S_board));

      if (nply > NPLY_PAUSE) pause_my_printf(with->S_my_printf);

      best_score = mcts_search(with, root_score, 0, arg_mcts_depth,
        &moves_list, &best_pv, TRUE);

      if (best_pv == INVALID) break;
    
      move2bstring(&moves_list, best_pv, bmove_string);

      print_board(&(with->S_board));

      PRINTF("nply=%d best_move=%s root_score=%d best_score=%d\n",
        nply, bdata(bmove_string), root_score, best_score);

      game_state.push_move(&game_state, bdata(bmove_string), "");

      ++nply;

      do_move(&(with->S_board), best_pv, &moves_list);
    }

    resume_my_printf(with->S_my_printf);

    HARDBUG(best_score == SCORE_PLUS_INFINITY)

    int wdl;

    int egtb_mate = read_endgame(with, with->S_board.B_colour2move, &wdl);

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

    HARDBUG(result == INVALID)

    //result as seen from white

    if (IS_BLACK(with->S_board.B_colour2move)) result = 2 - result;

    PRINTF("replay\n");

    //loop over all moves

    fen2board(&(with->S_board), game_state.get_starting_position(&game_state));

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(&(with->S_board), bfen, FALSE);

        if ((IS_WHITE(with->S_board.B_colour2move) and (result == 2)) or
            (IS_BLACK(with->S_board.B_colour2move) and (result == 0)))
        {
          append_sql_buffer(arg_db,
            "INSERT INTO positions (fen, frequency, nwon) VALUES ('%s', 1, 1)"
            " ON CONFLICT(fen) DO UPDATE SET "
            " frequency = frequency + 1, nwon = nwon + 1;",
            bdata(bfen));
        }
        else if ((IS_WHITE(with->S_board.B_colour2move) and (result == 0)) or
                 (IS_BLACK(with->S_board.B_colour2move) and (result == 2)))
        {
          append_sql_buffer(arg_db,
            "INSERT INTO positions (fen, frequency, nlost) VALUES ('%s', 1, 1)"
            " ON CONFLICT(fen) DO UPDATE SET "
            " frequency = frequency + 1, nlost = nlost + 1;",
            bdata(bfen));
        }
        else
        {
          HARDBUG(result != 1)

          append_sql_buffer(arg_db,
            "INSERT INTO positions (fen, frequency, ndraw) VALUES ('%s', 1, 1)"
            " ON CONFLICT(fen) DO UPDATE SET "
            " frequency = frequency + 1, ndraw = ndraw + 1;",
            bdata(bfen));
        }

        update_progress(&progress);

        ++ndone;
      }

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))

      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bmove_string)) == INVALID)

      do_move(&(with->S_board), imove, &moves_list);
    }
  }

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  destroy_game_state(&game_state);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  PRINTF("generating %lld positions took %.2f seconds\n",
    ndone, return_my_timer(&timer, FALSE));
 
  my_mpi_barrier_v3("after gen", my_mpi_globals.MMG_comm_slaves,
                    SEMKEY_GEN_DB_BARRIER, TRUE);

  my_mpi_allreduce(MPI_IN_PLACE, &ndone, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  PRINTF("my_mpi_all_reduce(ndone)=%lld\n", ndone);
}

#define NPOSITIONS_MAX 100000000
#define NGAMES 100

void update_db(my_sqlite3_t *arg_db, int arg_mcts_depth)
{
  i64_t *ids;

  MY_MALLOC_BY_TYPE(ids, i64_t, NPOSITIONS_MAX)

  char *sql = "SELECT id FROM positions WHERE"
              " frequency = (SELECT MIN(frequency) FROM positions)"
              " ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);

  i64_t npositions = 0;

  int rc;

  while ((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) == SQLITE_ROW)
  {
    i64_t id = my_sqlite3_column_int64(stmt, 0);

    HARDBUG(npositions >= NPOSITIONS_MAX)

    ids[npositions++] = id;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  my_mpi_barrier_v3("after query ids", my_mpi_globals.MMG_comm_slaves,
                    SEMKEY_UPDATE_DB_BARRIER, FALSE);

  PRINTF("npositions=%lld\n", npositions);

  my_timer_t timer;

  construct_my_timer(&timer, "update_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  i64_t ntodo = npositions;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  BSTRING(bmove_string)

  BSTRING(bfen)

  game_state_t game_state;

  construct_game_state(&game_state);

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (my_mpi_globals.MMG_nslaves > 0)
    {
      if ((iposition % my_mpi_globals.MMG_nslaves) !=
          my_mpi_globals.MMG_id_slave) continue;
    }

    update_progress(&progress);

    i64_t id = ids[iposition];

    HARDBUG(id < 1)

    PRINTF("id=%lld\n", id);

    sql = "SELECT fen FROM positions WHERE id = ?;";

    my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);
  
    HARDBUG(my_sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK)
  
    HARDBUG((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) != SQLITE_ROW)
  
    const unsigned char *fen = my_sqlite3_column_text(stmt, 0);

    HARDBUG(bassigncstr(bfen, (char *) fen) == BSTR_ERR)

    HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

    PRINTF("iposition=%lld id=%lld fen=%s\n", iposition, id, bdata(bfen));

    int egtb_mate = ENDGAME_UNKNOWN;

    int igame = INVALID;

    game_state.set_starting_position(&game_state, bdata(bfen));

    PUSH_NAME("update_db::for (igame")

    for (igame = 1; igame <= NGAMES; ++igame)
    {
      fen2board(&(with->S_board), game_state.get_starting_position(&game_state));

      game_state.clear_moves(&game_state);
  
      PRINTF("igame=%d auto play\n", igame);

      //auto play 
      //no time limit
  
      mcts_globals.mcts_globals_time_limit = 0.0;
  
      int nply = 0;
  
      int best_score = SCORE_PLUS_INFINITY;
  
      while(TRUE)
      {
        if (nply == MCTS_PLY_MAX) break;
   
        moves_list_t moves_list;
  
        construct_moves_list(&moves_list);
  
        gen_moves(&(with->S_board), &moves_list, FALSE);
  
        if (moves_list.ML_nmoves == 0) break;
  
        int best_pv;
  
        reset_my_timer(&(with->S_timer));
  
        int root_score = return_material_score(&(with->S_board));
  
        if (nply > NPLY_PAUSE) pause_my_printf(with->S_my_printf);

        best_score = mcts_search(with, root_score, 0, arg_mcts_depth,
          &moves_list, &best_pv, TRUE);
  
        if (best_pv == INVALID) break;
      
        move2bstring(&moves_list, best_pv, bmove_string);
  
        print_board(&(with->S_board));
  
        PRINTF("nply=%d best_move=%s root_score=%d best_score=%d\n",
          nply, bdata(bmove_string), root_score, best_score);
  
        game_state.push_move(&game_state, bdata(bmove_string), "");
  
        ++nply;
  
        do_move(&(with->S_board), best_pv, &moves_list);
      }

      resume_my_printf(with->S_my_printf);

      HARDBUG(best_score == SCORE_PLUS_INFINITY)

      int wdl;

      egtb_mate = read_endgame(with, with->S_board.B_colour2move, &wdl);

      if (egtb_mate == ENDGAME_UNKNOWN)
      {
        PRINTF("igame=%d failed\n", igame);
  
        continue;
      }

      PRINTF("igame=%d success\n", igame);

      break;
    }

    POP_NAME("update_db::for (igame")

    if (egtb_mate == ENDGAME_UNKNOWN)
    {
      PRINTF("deleting fen=%s\n", bdata(bfen));

      append_sql_buffer(arg_db,
        "DELETE FROM positions where fen = '%s';",
        bdata(bfen));

      continue;
    }

    int result = INVALID;

    if (egtb_mate == INVALID)
      result = 1;
    else if (egtb_mate > 0)
      result = 2;
    else
      result = 0;

    HARDBUG(result == INVALID)

    //result as seen from white

    if (IS_BLACK(with->S_board.B_colour2move)) result = 2 - result;

    PRINTF("replay\n");

    //loop over all moves

    fen2board(&(with->S_board), game_state.get_starting_position(&game_state));

    cJSON *game_move;

    PUSH_NAME("update_db::cJSON_ArrayForEach(game_move")

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with->S_board), &moves_list, FALSE);

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(&(with->S_board), bfen, FALSE);

        if ((IS_WHITE(with->S_board.B_colour2move) and (result == 2)) or
            (IS_BLACK(with->S_board.B_colour2move) and (result == 0)))
        {
          append_sql_buffer(arg_db,
            "UPDATE positions SET"
            " frequency = frequency + 1, nwon = nwon + 1 WHERE fen = '%s';",
            bdata(bfen));
        }
        else if ((IS_WHITE(with->S_board.B_colour2move) and (result == 0)) or
                 (IS_BLACK(with->S_board.B_colour2move) and (result == 2)))
        {
          append_sql_buffer(arg_db,
            "UPDATE positions SET"
            " frequency = frequency + 1, nlost = nlost + 1 WHERE fen = '%s';",
            bdata(bfen));
        }
        else
        {
          HARDBUG(result != 1)

          append_sql_buffer(arg_db,
            "UPDATE positions SET"
            " frequency = frequency + 1, ndraw = ndraw + 1 WHERE fen = '%s';",
            bdata(bfen));
        }

      }

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))

      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bmove_string)) == INVALID)

      do_move(&(with->S_board), imove, &moves_list);
    }

    POP_NAME("update_db::cJSON_ArrayForEach(game_move")
  }

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  destroy_game_state(&game_state);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  PRINTF("updating %lld positions took %.2f seconds\n",
    ntodo, return_my_timer(&timer, FALSE));
 
  my_mpi_barrier_v3("after update", my_mpi_globals.MMG_comm_slaves,
                    SEMKEY_UPDATE_DB_BARRIER, TRUE);

  my_mpi_allreduce(MPI_IN_PLACE, &ntodo, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  PRINTF("my_mpi_all_reduce(ntodo)=%lld\n", ntodo);
}

void update_db_v2(my_sqlite3_t *arg_db, int arg_mcts_depth, int arg_frequency)
{
  i64_t *ids;

  MY_MALLOC_BY_TYPE(ids, i64_t, NPOSITIONS_MAX)

  char *sql = "SELECT id FROM positions WHERE"
              " frequency < ?"
              " ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);

  HARDBUG(my_sqlite3_bind_int(stmt, 1, arg_frequency) != SQLITE_OK)

  i64_t npositions = 0;

  int rc;

  while ((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) == SQLITE_ROW)
  {
    i64_t id = my_sqlite3_column_int64(stmt, 0);

    HARDBUG(npositions >= NPOSITIONS_MAX)

    ids[npositions] = id;

    npositions++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  my_mpi_barrier_v3("after query ids", my_mpi_globals.MMG_comm_slaves,
                    SEMKEY_UPDATE_DB_BARRIER, FALSE);

  PRINTF("npositions=%lld\n", npositions);

  my_timer_t timer;

  construct_my_timer(&timer, "update_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  i64_t ntodo = npositions;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  BSTRING(bmove_string)

  BSTRING(bfen)

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (my_mpi_globals.MMG_nslaves > 0)
    {
      if ((iposition % my_mpi_globals.MMG_nslaves) !=
          my_mpi_globals.MMG_id_slave) continue;
    }

    update_progress(&progress);

    i64_t id = ids[iposition];

    HARDBUG(id < 1)

    PRINTF("id=%lld\n", id);

    sql = "SELECT fen, frequency FROM positions WHERE id = ?;";

    my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);
  
    HARDBUG(my_sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK)
  
    HARDBUG((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) != SQLITE_ROW)
  
    const unsigned char *fen = my_sqlite3_column_text(stmt, 0);

    HARDBUG(bassigncstr(bfen, (char *) fen) == BSTR_ERR)

    int frequency = my_sqlite3_column_int(stmt, 1);

    HARDBUG(frequency >= arg_frequency)

    HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

    PRINTF("iposition=%lld id=%lld frequency=%d fen=%s\n$",
           iposition, id, frequency, bdata(bfen));

    fen2board(&(with->S_board), bdata(bfen));

    int nshoot_outs = arg_frequency - frequency;

    int nwon = 0;
    int ndraw = 0;
    int nlost = 0;

    double mcts_result =
      mcts_shoot_outs(with, 0, &nshoot_outs,
      arg_mcts_depth, 0.0, &nwon, &ndraw, &nlost);

    nshoot_outs = nwon + ndraw + nlost;

    PRINTF("nshoot_outs=%d nwon=%d ndraw=%d nlost=%d mcts_result=%.6f\n$",
           nshoot_outs, nwon, ndraw, nlost, mcts_result);

    HARDBUG(nshoot_outs > (arg_frequency - frequency))

    if (nshoot_outs < (arg_frequency - frequency))
    {
      print_board(&(with->S_board));

      PRINTF("WARNING: nshoot_outs < (arg_frequency - frequency)\n");
    }

    append_sql_buffer(arg_db,
      "UPDATE positions SET"
      " frequency = frequency + %d,"
      " nwon = nwon + %d, ndraw = ndraw + %d, nlost = nlost + %d"     
      " WHERE fen = '%s';",
      nshoot_outs, nwon, ndraw, nlost, bdata(bfen));
  }

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  PRINTF("updating %lld positions took %.2f seconds\n",
    ntodo, return_my_timer(&timer, FALSE));
 
  my_mpi_barrier_v3("after update_v2", my_mpi_globals.MMG_comm_slaves,
                    SEMKEY_UPDATE_DB_BARRIER, TRUE);

  my_mpi_allreduce(MPI_IN_PLACE, &ntodo, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  PRINTF("my_mpi_all_reduce(ntodo)=%lld\n", ntodo);
}

