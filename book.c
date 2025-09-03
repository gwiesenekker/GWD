//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define NPOSITIONS_MAX 100000000

#define EXPANSION_NMOVES      3
#define EXPANSION_EVALUATION1 (SCORE_MAN / 2)
#define EXPANSION_EVALUATION2 (-SCORE_MAN / 4)

local my_random_t book_random;

local sqlite3 *book;

void init_book(void)
{
  construct_my_random(&book_random, INVALID);
}

local int help_walk_book_alpha_beta(sqlite3 *arg_db, board_t *object,
  int arg_depth, int arg_alpha, int arg_beta)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0) return(SCORE_LOST + arg_depth);

  i64_t position_id = INVALID;
 
  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    int nmoves;

    position_id = query_position(arg_db, bdata(bfen), &nmoves);
  
    BDESTROY(bfen)
   
    if (position_id == INVALID) return(SCORE_PLUS_INFINITY);

    if (nmoves == 0) return(SCORE_PLUS_INFINITY);

    HARDBUG(nmoves != moves_list.ML_nmoves)
  }

  int best_score = SCORE_MINUS_INFINITY;

  best_score = arg_alpha;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int temp_score = SCORE_MINUS_INFINITY;

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      do_move(object, imove, &moves_list);
  
      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1,
                                              -arg_beta, -best_score);
    
      undo_move(object, imove, &moves_list);

      if (temp_score == SCORE_MINUS_INFINITY) return(SCORE_PLUS_INFINITY);
    }
    else
    {
      BSTRING(bmove)

      move2bstring(&moves_list, imove, bmove);

      i64_t move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove));

      BDESTROY(bmove)

      HARDBUG(move_id == INVALID)

      int centi_seconds;

      int evaluation = query_evaluation(arg_db, move_id, &centi_seconds);

      do_move(object, imove, &moves_list);

      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1,
                                              -arg_beta, -best_score);

      undo_move(object, imove, &moves_list);

      //if temp_score == SCORE_MINUS_INFINITY
      //the evaluation of the position after the move is unknown

      if (temp_score == SCORE_MINUS_INFINITY) temp_score = evaluation;
    }

    if (temp_score > best_score) best_score = temp_score;

    if (best_score >= arg_beta) break;
  }

  HARDBUG(best_score == SCORE_PLUS_INFINITY)

  return(best_score);
}

local void help_walk_book(sqlite3 *arg_db, board_t *object, int arg_depth,
  bstring arg_bpv)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list, FALSE);

  HARDBUG(moves_list.ML_nmoves == 0)

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);
  
    int nmoves;

    position_id = query_position(arg_db, bdata(bfen), &nmoves);
  
    BDESTROY(bfen)

    if (position_id == INVALID)
    {
      PRINTF("%s (position_id == INVALID)\n", bdata(arg_bpv));
  
      return;
    }

    if (nmoves == 0)
    {
      PRINTF("%s (nmoves == 0)\n", bdata(arg_bpv));
  
      return;
    }
  }

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    move2bstring(&moves_list, imove, bmove);

    if ((moves_list.ML_nmoves == 1) or (moves_list.ML_ncaptx > 0))
    {
      HARDBUG(position_id != INVALID)

      BSTRING(bpv)

      HARDBUG(bformata(bpv, "%s %s(--, --)", bdata(arg_bpv), bdata(bmove)) ==
              BSTR_ERR)

      do_move(object, imove, &moves_list);

      help_walk_book(arg_db, object, arg_depth + 1, bpv);
  
      undo_move(object, imove, &moves_list);

      BDESTROY(bpv)
    }
    else
    {
      HARDBUG(position_id == INVALID)

      i64_t move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove));

      HARDBUG(move_id == INVALID)
  
      int centi_seconds;

      int evaluation = query_evaluation(arg_db, move_id, &centi_seconds);

      HARDBUG(evaluation == SCORE_PLUS_INFINITY)

      do_move(object, imove, &moves_list);

      int best_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth,
                                                  SCORE_MINUS_INFINITY,
                                                  SCORE_PLUS_INFINITY);

      BSTRING(bpv)

      HARDBUG(bformata(bpv, "%s %s[%d(%.2fs), %d]",
              bdata(arg_bpv), bdata(bmove),
              evaluation, centi_seconds / 100.0, best_score) == BSTR_ERR)

      help_walk_book(arg_db, object, arg_depth + 1, bpv);

      undo_move(object, imove, &moves_list);

      BDESTROY(bpv)
    }
  }

  BDESTROY(bmove)
}

void open_book(void)
{
  PRINTF("opening book..\n");

  sqlite3 *disk;

  int rc = my_sqlite3_open_v2(options.book_name, &disk,
                              SQLITE_OPEN_READONLY, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database %s: %s\n", options.book_name,
           my_sqlite3_errmsg(disk));

    FATAL("sqlite3", EXIT_FAILURE)
  }
  
  rc = my_sqlite3_open(":memory:", &book);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s\n", my_sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  char attach_sql[MY_LINE_MAX];

  snprintf(attach_sql, MY_LINE_MAX,
           "ATTACH DATABASE '%s' AS disk;", options.book_name);

  rc = my_sqlite3_exec(book, attach_sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s errmsg=%s\n",
           attach_sql, my_sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  const char *sql = "SELECT name, sql FROM sqlite_master WHERE type='table'";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(disk, sql, &stmt);

  while (my_sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *table_name =
      (const char *) my_sqlite3_column_text(stmt, 0);

    const char *create_table_sql =
      (const char *) my_sqlite3_column_text(stmt, 1);

    if (strcmp(table_name, "sqlite_sequence") == 0) continue;

    rc = my_sqlite3_exec(book, create_table_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s errmsg=%s\n",
             create_table_sql, my_sqlite3_errmsg(book));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    char create_sql[MY_LINE_MAX];

    snprintf(create_sql, MY_LINE_MAX,
      "INSERT INTO %s SELECT * FROM disk.%s", table_name, table_name);

    rc = my_sqlite3_exec(book, create_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s errmsg=%s\n",
             create_sql, my_sqlite3_errmsg(book));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  sql = "DETACH DATABASE disk;";

  rc = my_sqlite3_exec(book, sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s errmsg=%s\n",
           sql, my_sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  my_sqlite3_close(disk);

  if (FALSE)
  {
    board_t board;
    
    construct_board(&board, STDOUT);

    board_t *with = &board;
  
    string2board(with, STARTING_POSITION);
  
    BSTRING(bempty)

    help_walk_book(book, with, 0, bempty);

    BDESTROY(bempty)
  }
}

void return_book_move(board_t *object, moves_list_t *arg_moves_list,
  bstring arg_bbook_move)
{
  HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0) return;

  i64_t position_id = INVALID;
 
  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    int nmoves;

    position_id = query_position(book, bdata(bfen), &nmoves);
  
    BDESTROY(bfen)
   
    if (position_id == INVALID) return;

    if (nmoves == 0) return;

    HARDBUG(nmoves != moves_list.ML_nmoves)
  }

  int sort[NMOVES_MAX];

  if (options.book_randomness == 0)
    shuffle(sort, arg_moves_list->ML_nmoves, NULL);
  else
    shuffle(sort, arg_moves_list->ML_nmoves, &book_random);

  int best_move = INVALID;

  int best_score = SCORE_MINUS_INFINITY;

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int jmove = sort[imove];

    int evaluation = SCORE_MINUS_INFINITY;

    int centi_seconds = 0;

    if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
    {
      move2bstring(&moves_list, jmove, bmove);

      i64_t move_id;
    
      move_id = query_move(book, position_id, bdata(bmove));
  
      HARDBUG(move_id == INVALID)
  
      evaluation = query_evaluation(book, move_id, &centi_seconds);
  
      HARDBUG(evaluation == SCORE_PLUS_INFINITY)
    }

    int minimax_score = SCORE_MINUS_INFINITY;

    if (options.book_minimax or
        (moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      do_move(object, jmove, &moves_list);

      minimax_score = -help_walk_book_alpha_beta(book, object, 0,
                                                 SCORE_MINUS_INFINITY,
                                                 SCORE_PLUS_INFINITY);
  
      undo_move(object, jmove, &moves_list);
    }

    move2bstring(&moves_list, jmove, bmove);

    PRINTF("book_move=%s evaluation=%d(%.2fs) minimax_score=%d\n",
           bdata(bmove), evaluation, centi_seconds / 100.0, minimax_score);

    int temp_score = evaluation;

    if (minimax_score > temp_score) temp_score = minimax_score;

    if (options.book_randomness > 0) temp_score /= options.book_randomness;

    if (temp_score > best_score)
    {
      best_move = jmove;
  
      best_score = temp_score;

      move2bstring(&moves_list, best_move, arg_bbook_move);
    }
  }//for (int imove

  BDESTROY(bmove)

  PRINTF("book_move=%s best_score=%d\n", bdata(arg_bbook_move), best_score);
}

local i64_t npositions;
local i64_t *position_ids;

local void query_position_ids(sqlite3 *arg_db)
{
  char *sql = "SELECT id FROM positions ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  npositions = 0;

  int rc;

  while ((rc = execute_sql(arg_db, stmt, TRUE)) == SQLITE_ROW)
  {
    i64_t position_id = my_sqlite3_column_int64(stmt, 0);

    HARDBUG(npositions >= NPOSITIONS_MAX)

    position_ids[npositions++] = position_id;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("npositions=%lld\n", npositions);
}

local void return_position_moves(sqlite3 *arg_db, i64_t arg_position_id,
  board_t *arg_board, moves_list_t *arg_moves_list)
{
  char *sql = "SELECT fen FROM positions WHERE id = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(my_sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  int rc;

  HARDBUG((rc = execute_sql(arg_db, stmt, TRUE)) != SQLITE_ROW)

  const unsigned char *fen = my_sqlite3_column_text(stmt, 0);

  BSTRING(bfen)

  HARDBUG(bassigncstr(bfen, (char *) fen) == BSTR_ERR)

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  fen2board(arg_board, bdata(bfen));

  BDESTROY(bfen)

  gen_moves(arg_board, arg_moves_list, FALSE);

  HARDBUG(arg_moves_list->ML_nmoves == 0)
  HARDBUG(arg_moves_list->ML_nmoves == 1)
  HARDBUG(arg_moves_list->ML_ncaptx > 0)
}

local void add_evaluations(my_sqlite3_t *arg_db, search_t *arg_search,
  i64_t arg_position_id, moves_list_t *arg_moves_list)
{
  i64_t move_ids[NMOVES_MAX];
  bstring bmoves[NMOVES_MAX];
  int centi_seconds[NMOVES_MAX];

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  char *sql =
    "SELECT m.id, m.move, m.centi_seconds"
    " FROM moves m"
    " LEFT JOIN ("
    "  SELECT move_id, MAX(centi_seconds) AS best_centi_seconds"
    "  FROM evaluations"
    "  GROUP BY move_id"
    " ) e ON m.id = e.move_id"
    " WHERE m.position_id = ? AND ("
    "  e.best_centi_seconds IS NULL OR"
    "  e.best_centi_seconds < m.centi_seconds"
    " );";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);

  HARDBUG(my_sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  int nmoves = 0;

  int rc;

  while ((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) == SQLITE_ROW)
  {
    HARDBUG(nmoves >= NMOVES_MAX)

    move_ids[nmoves] = my_sqlite3_column_int64(stmt, 0);

    const unsigned char *move = my_sqlite3_column_text(stmt, 1);

    HARDBUG(bassigncstr(bmoves[nmoves], (char *) move) == BSTR_ERR)

    centi_seconds[nmoves] = my_sqlite3_column_int(stmt, 2);

    nmoves++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("nmoves=%d\n", nmoves);

  for (int imove = 0; imove < nmoves; imove++)
  {
    int jmove;

    HARDBUG((jmove = search_move(arg_moves_list, bmoves[imove])) == INVALID)

    do_move(&(arg_search->S_board), jmove, arg_moves_list);
    
    moves_list_t your_moves_list;
    
    construct_moves_list(&your_moves_list);
    
    gen_moves(&(arg_search->S_board), &your_moves_list, FALSE);
    
    int evaluation = SCORE_MINUS_INFINITY;

    if (your_moves_list.ML_nmoves == 0)
    {
      evaluation = SCORE_WON_ABSOLUTE;
    }
    else
    {
      options.time_limit = centi_seconds[imove] / 100.0;
      options.time_ntrouble = 0;
    
      do_search(arg_search, &your_moves_list,
                INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

      evaluation = -arg_search->S_best_score;
    }

    undo_move(&(arg_search->S_board), jmove, arg_moves_list);
  
    HARDBUG(evaluation == SCORE_MINUS_INFINITY)
    HARDBUG(evaluation == SCORE_PLUS_INFINITY)

    PRINTF("adding nmoves=%d move_ids[%d]=%lld bdata(bmoves[%d])=%s"
           " centi_seconds[%d]=%d evaluation=%d\n",
           nmoves,
           imove, move_ids[imove],
           imove, bdata(bmoves[imove]),
           imove, centi_seconds[imove], evaluation);
  
    append_sql_buffer(arg_db,
      "INSERT INTO evaluations (move_id, centi_seconds, evaluation)"
      " VALUES (%lld, %d, %d)"
      " ON CONFLICT(move_id, centi_seconds)"
      " DO UPDATE SET evaluation = excluded.evaluation"
      " WHERE excluded.evaluation > evaluations.evaluation;",
      move_ids[imove], centi_seconds[imove], evaluation);
  }

  flush_sql_buffer(arg_db, 0);

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)
}

local void gen_book_worker(my_sqlite3_t *arg_db, search_t *arg_search)
{
  i64_t move_ids[NMOVES_MAX];
  bstring bmoves[NMOVES_MAX];
  int centi_seconds[NMOVES_MAX];

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  moves_list_t moves_list;
 
  i64_t ntodo = npositions;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (my_mpi_globals.MMG_nslaves > 0)
    {
      if ((iposition % my_mpi_globals.MMG_nslaves) !=
          my_mpi_globals.MMG_id_slave) continue;
    }

    update_progress(&progress);

    i64_t position_id = position_ids[iposition];

    HARDBUG(position_id < 1)

    PRINTF("iposition=%lld position_id=%lld\n", iposition, position_id);

    return_position_moves(arg_db->MS_db, position_id,
                          &(arg_search->S_board), &moves_list);

    char *sql =
      "SELECT m.id, m.move, m.centi_seconds"
      " FROM moves m"
      " LEFT JOIN ("
      "  SELECT move_id, MAX(centi_seconds) AS best_centi_seconds"
      "  FROM evaluations"
      "  GROUP BY move_id"
      " ) e ON m.id = e.move_id"
      " WHERE m.position_id = ? AND ("
      "  e.best_centi_seconds IS NULL OR"
      "  e.best_centi_seconds < m.centi_seconds"
      " );";

    sqlite3_stmt *stmt;

    my_sqlite3_prepare_v2(arg_db->MS_db, sql, &stmt);

    HARDBUG(my_sqlite3_bind_int64(stmt, 1, position_id) != SQLITE_OK)

    int nmoves = 0;

    int rc;

    while ((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) == SQLITE_ROW)
    {
      HARDBUG(nmoves >= NMOVES_MAX)

      move_ids[nmoves] = my_sqlite3_column_int64(stmt, 0);

      const unsigned char *move = my_sqlite3_column_text(stmt, 1);

      HARDBUG(bassigncstr(bmoves[nmoves], (char *) move) == BSTR_ERR)

      centi_seconds[nmoves] = my_sqlite3_column_int(stmt, 2);

      nmoves++;
    }

    HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

    PRINTF("nmoves=%d\n", nmoves);

    for (int imove = 0; imove < nmoves; imove++)
    {
      int jmove;

      HARDBUG((jmove = search_move(&moves_list, bmoves[imove])) == INVALID)

      do_move(&(arg_search->S_board), jmove, &moves_list);
    
      moves_list_t your_moves_list;
    
      construct_moves_list(&your_moves_list);
    
      gen_moves(&(arg_search->S_board), &your_moves_list, FALSE);
    
      int evaluation = SCORE_MINUS_INFINITY;

      if (your_moves_list.ML_nmoves == 0)
      {
        evaluation = SCORE_WON_ABSOLUTE;
      }
      else
      {
        options.time_limit = centi_seconds[imove] / 100.0;
        options.time_ntrouble = 0;
    
        do_search(arg_search, &your_moves_list,
                  INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

        evaluation = -arg_search->S_best_score;
      }

      undo_move(&(arg_search->S_board), jmove, &moves_list);
  
      HARDBUG(evaluation == SCORE_MINUS_INFINITY)
      HARDBUG(evaluation == SCORE_PLUS_INFINITY)

      PRINTF("adding move_id=%lld move=%s"
             " centi_seconds=%d evaluation=%d\n",
             move_ids[imove], bdata(bmoves[imove]),
             centi_seconds[imove], evaluation);
  
      append_sql_buffer(arg_db,
        "INSERT INTO evaluations (move_id, centi_seconds, evaluation)"
        " VALUES (%lld, %d, %d);",
        move_ids[imove], centi_seconds[imove], evaluation);
    }
  }//for (int iposition

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)
}

local void add_or_update_moves(sqlite3 *arg_db, board_t *arg_board)
{
  char *sql = "SELECT id, fen, frequency FROM positions;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  BSTRING(bfen)

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  BSTRING(bmove)

  int iposition = 0;

  HARDBUG(execute_sql(arg_db, "BEGIN TRANSACTION;", FALSE) != SQLITE_OK)

  int rc;

  while ((rc = execute_sql(arg_db, stmt, TRUE)) == SQLITE_ROW)
  {
    ++iposition;

    i64_t id = my_sqlite3_column_int64(stmt, 0);

    const unsigned char *fen = my_sqlite3_column_text(stmt, 1);

    int frequency = my_sqlite3_column_int(stmt, 2);

    HARDBUG(bassigncstr(bfen, (char *) fen) == BSTR_ERR)

    fen2board(arg_board, bdata(bfen));

    gen_moves(arg_board, &moves_list, FALSE);

    HARDBUG(moves_list.ML_nmoves == 0)
    HARDBUG(moves_list.ML_nmoves == 1)
    HARDBUG(moves_list.ML_ncaptx > 0)

    if (frequency == 0) continue;

    int bin = (int) (log10(frequency));

    if (bin == 0) continue;

    int centi_seconds = 10 * 100;

    if (bin >= 2) centi_seconds = 30 * 100;

    PRINTF("fen=%s frequency=%d centi_seconds=%d\n",
           fen, frequency, centi_seconds);

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      move2bstring(&moves_list, imove, bmove);
    
      (void) insert_move(arg_db, id, bdata(bmove), centi_seconds);
    }
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, "COMMIT;", FALSE) != SQLITE_OK)

  HARDBUG(iposition != npositions)

  BDESTROY(bmove)

  BDESTROY(bfen)
}

void gen_book(my_sqlite3_t *arg_db, int arg_centi_seconds)
{
  ui64_t seed = return_my_random(&book_random);

  PRINTF("seed=%llX\n", seed);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  fen2board(&(search.S_board), STARTING_POSITION2FEN);

  MY_MALLOC_BY_TYPE(position_ids, i64_t, NPOSITIONS_MAX)

  query_position_ids(arg_db->MS_db);
  
  if (arg_centi_seconds > 0)
  {
    if (my_mpi_globals.MMG_id_slave <= 0)
    {
      PRINTF("adding or updating moves..\n");
  
      add_or_update_moves(arg_db->MS_db, &(search.S_board));
    }

    my_mpi_barrier_v3("after adding or updating moves",
                      my_mpi_globals.MMG_comm_slaves,
                      SEMKEY_GEN_BOOK_BARRIER, FALSE);
  }
  
  PRINTF("adding evaluations to moves..\n");
  
  gen_book_worker(arg_db, &search);
}

void walk_book(char *arg_db_name, char *arg_fen)
{
  sqlite3 *db;

  HARDBUG(!fexists(arg_db_name))

  int rc = my_sqlite3_open(arg_db_name, &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s errmsg=%s\n",
           arg_db_name, my_sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  //we start with the starting position

  board_t board;

  construct_board(&board, STDOUT);

  board_t *with = &board;

  if (arg_fen == NULL)
    string2board(with, STARTING_POSITION);
  else
    fen2board(with, arg_fen);

  BSTRING(bempty)

  help_walk_book(db, with, 0, bempty);

  BDESTROY(bempty)

  HARDBUG(my_sqlite3_close(db) != SQLITE_OK)
}

local void add2book_worker(my_sqlite3_t *arg_db, search_t *arg_search, 
  int arg_depth, int arg_depth_max,
  int arg_centi_seconds1, int arg_centi_seconds2)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_search->S_board), &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0) return;

  if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
  {
    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(&(arg_search->S_board), imove, &moves_list);
       
      add2book_worker(arg_db, arg_search, arg_depth, arg_depth_max,
                      arg_centi_seconds1, arg_centi_seconds2);

      undo_move(&(arg_search->S_board), imove, &moves_list);
    }
  }

  print_board(&(arg_search->S_board));

  if (arg_depth >= arg_depth_max)
  {
    PRINTF("arg_depth(%d) >= arg_depth_max(%d)\n",
           arg_depth, arg_depth_max);

    return;
  }

  //do not add poor positions

  PRINTF("checking position arg_centi_seconds1=%d\n", arg_centi_seconds1);

  options.time_limit = arg_centi_seconds1 / 100.0;
  options.time_ntrouble = 0;
    
  do_search(arg_search, &moves_list,
            INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

  int abs_score = abs(arg_search->S_best_score);

  if (abs_score >= EXPANSION_EVALUATION1)
  {
     PRINTF("abs_score(%d) >= EXPANSION_EVALUATION1(%d)\n",
            abs_score, EXPANSION_EVALUATION1);

     return;
  }

  BSTRING(bfen)

  board2fen(&(arg_search->S_board), bfen, FALSE);

  PRINTF("adding position arg_search->S_best_score=%d\n",
         arg_search->S_best_score);

  append_sql_buffer(arg_db,
    "INSERT OR IGNORE INTO positions (fen) VALUES ('%s');",
    bdata(bfen));

  flush_sql_buffer(arg_db, 0);

  i64_t position_id = query_position(arg_db->MS_db, bdata(bfen), NULL);

  HARDBUG(position_id == INVALID)

  PRINTF("bfen=%s position_id=%lld\n", bdata(bfen), position_id);

  BDESTROY(bfen)

  PRINTF("adding moves arg_centi_seconds2=%d\n", arg_centi_seconds2);

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    move2bstring(&moves_list, imove, bmove);

    append_sql_buffer(arg_db,
      "INSERT INTO moves (position_id, move, centi_seconds)"
      " VALUES (%lld, '%s', %d)"
      " ON CONFLICT(position_id, move) DO UPDATE SET"
      "  centi_seconds = excluded.centi_seconds" 
      " WHERE excluded.centi_seconds > moves.centi_seconds;",
      position_id, bdata(bmove), arg_centi_seconds2);
  }

  BDESTROY(bmove)

  flush_sql_buffer(arg_db, 0);

  PRINTF("adding evaluations..\n");

  add_evaluations(arg_db, arg_search, position_id, &moves_list);

  BSTRING(bsql)

  int limit = EXPANSION_NMOVES;

  if (arg_depth <= 1) limit = NMOVES_MAX;
    
  PRINTF("expanding moves limit=%d\n", limit);

  bstring bmoves[NMOVES_MAX];
  int evaluations[NMOVES_MAX];

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  bformata(bsql, 
    "SELECT m.move, e.evaluation"
    " FROM moves m"
    " JOIN evaluations e ON m.id = e.move_id"
    " WHERE m.position_id = ?"
    " AND e.centi_seconds = ("
    "  SELECT MAX(c.centi_seconds)"
    "  FROM evaluations c"
    "  WHERE c.move_id = m.id and c.evaluation >= %d"
    " )"
    " AND e.evaluation >= %d"
    " ORDER BY e.evaluation DESC LIMIT %d;",
    EXPANSION_EVALUATION2, EXPANSION_EVALUATION2, limit);

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MS_db, bdata(bsql), &stmt);
  
  HARDBUG(my_sqlite3_bind_int64(stmt, 1, position_id) != SQLITE_OK)
  
  int nmoves = 0;
  
  int rc;
  
  while ((rc = execute_sql(arg_db->MS_db, stmt, TRUE)) == SQLITE_ROW)
  {
    HARDBUG(nmoves >= NMOVES_MAX)

    const unsigned char *move = my_sqlite3_column_text(stmt, 0);
  
    HARDBUG(bassigncstr(bmoves[nmoves], (char *) move) == BSTR_ERR)

    evaluations[nmoves] = my_sqlite3_column_int(stmt, 1);
        
    nmoves++;
  }
  
  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  BDESTROY(bsql)

  PRINTF("nmoves=%d\n", nmoves);

  for (int imove = 0; imove < nmoves; imove++)
  {
    int jmove;

    HARDBUG((jmove = search_move(&moves_list, bmoves[imove])) == INVALID)
 
    PRINTF("expanding bmoves[%d]=%s nmoves=%d"
           " arg_depth=%d arg_depth_max=%d evaluations[%d]=%d\n",
           imove, bdata(bmoves[imove]), nmoves, arg_depth, arg_depth_max,
           imove, evaluations[imove]);

    do_move(&(arg_search->S_board), jmove, &moves_list);
       
    add2book_worker(arg_db, arg_search, arg_depth + 1, arg_depth_max,
                    arg_centi_seconds1, arg_centi_seconds2);

    undo_move(&(arg_search->S_board), jmove, &moves_list);
  }

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)
}

void add2book(my_sqlite3_t *arg_db, char *arg_fen,
  int arg_depth_max, int arg_centi_seconds1, int arg_centi_seconds2)
{
  search_t search;

  construct_search(&search, STDOUT, NULL);

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_fen);

  BSTRING(bfen)

  i64_t nfen = 0;
  i64_t nfen_mpi = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*') break;
    if (bchar(bline, 0) == '#') continue;
    if (strncmp(bdata(bline), "//", 2) == 0) continue;

    ++nfen;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        (((nfen - 1) % my_mpi_globals.MMG_nslaves) !=
         my_mpi_globals.MMG_id_slave)) continue;

    ++nfen_mpi;

    CSTRING(cfen, blength(bline))

    HARDBUG(sscanf(bdata(bline), "%s", cfen) != 1)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(&(search.S_board), bdata(bfen));

    CDESTROY(cfen)

    print_board(&(search.S_board));

    add2book_worker(arg_db, &search, 0, arg_depth_max,
                    arg_centi_seconds1, arg_centi_seconds2);
  }

  flush_sql_buffer(arg_db, 0);

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen=%lld nfen_mpi=%lld\n", nfen, nfen_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen != nfen_mpi)
}

void gen_lmr(char *arg_name, int arg_seconds)
{
  BEGIN_BLOCK(__FUNC__)

  options.use_reductions = FALSE;

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  i64_t npositions = 0;
  
  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
    ++npositions;

  PRINTF("npositions=%lld\n", npositions);

  destroy_my_bstream(&my_bstream);

  i64_t ntodo = npositions;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  BSTRING(blmr)

  HARDBUG(bassigncstr(blmr, "lmr.fen") == BSTR_ERR)

  fbuffer_t flmr;

  construct_fbuffer(&flmr, blmr, NULL, TRUE);

  BDESTROY(blmr)

  BSTRING(bfen)
  
  BSTRING(bmove)

  construct_my_bstream(&my_bstream, arg_name);

  i64_t iposition = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    if (my_mpi_globals.MMG_nslaves > 0)
    {
      if ((iposition % my_mpi_globals.MMG_nslaves) !=
          my_mpi_globals.MMG_id_slave) 
      {
        iposition++;

        continue;
      }
    }

    update_progress(&progress);

    bstring bline = my_bstream.BS_bline;
  
    CSTRING(cfen, blength(bline))
  
    HARDBUG(sscanf(bdata(bline), "%s", cfen) != 1)
  
    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)
  
    fen2board(&(with->S_board), bdata(bfen));
  
    CDESTROY(cfen)
  
    print_board(&(with->S_board));
  
    moves_list_t my_moves_list;
  
    construct_moves_list(&my_moves_list);
  
    gen_moves(&(with->S_board), &my_moves_list, FALSE);

    int nwhite_man = BIT_COUNT(with->S_board.B_white_man_bb);
    int nblack_man = BIT_COUNT(with->S_board.B_black_man_bb);
    int nwhite_king = BIT_COUNT(with->S_board.B_white_king_bb);
    int nblack_king = BIT_COUNT(with->S_board.B_black_king_bb);
  
    int npieces = nwhite_man + nblack_man + nwhite_king + nblack_king;

    if ((my_moves_list.ML_ncaptx == 0) and
        (my_moves_list.ML_nmoves > 3) and
        (npieces >= 8))
    {
      for (int imove = 0; imove < my_moves_list.ML_nmoves; imove++)
      {
        move2bstring(&my_moves_list, imove, bmove);
  
        print_board(&(with->S_board));

        PRINTF("move=%s\n", bdata(bmove));

        do_move(&(with->S_board), imove, &my_moves_list);
  
        moves_list_t your_moves_list;
      
        construct_moves_list(&your_moves_list);
      
        gen_moves(&(with->S_board), &your_moves_list, FALSE);

        if (your_moves_list.ML_nmoves > 0)
        {
          options.time_limit = arg_seconds;
          options.time_ntrouble = 0;
        
          do_search(with, &your_moves_list,
                    INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);
    
          board2fen(&(with->S_board), bfen, FALSE);

          for (int idepth = 1; idepth < DEPTH_MAX; idepth++)
          {
            if (with->S_best_score_by_depth[idepth] == SCORE_MINUS_INFINITY)
              break;
    
            int result_wtm = with->S_best_score_by_depth[idepth];

            if (IS_BLACK(with->S_board.B_colour2move)) result_wtm = -result_wtm;

            PRINTF("idepth=%d move=%s result_wtm=%d\n",
                   idepth, bdata(bmove), result_wtm);

             append_fbuffer_fmt(&flmr, "%s {%d} [%d]\n",
                                bdata(bfen),
                                result_wtm,
                                idepth);
          }
        }

        undo_move(&(with->S_board), imove, &my_moves_list);
      }
    }

    iposition++;
  }//while

  destroy_my_bstream(&my_bstream);

  BDESTROY(bmove)

  BDESTROY(bfen)

  finalize_progress(&progress);

  flush_fbuffer(&flmr, 0);
}
