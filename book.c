//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#include "globals.h"

#define NPOSITIONS_MAX 100000000

#define EXPANSION_NMOVES 2
#define EXPANSION_EVALUATION (SCORE_MAN / 4)

local my_random_t book_random;

local sqlite3 *book;

void init_book(void) { construct_my_random(&book_random, INVALID); }

local int help_walk_book_alpha_beta(sqlite3 *arg_db,
                                    board_t *object,
                                    int arg_depth,
                                    int arg_alpha,
                                    int arg_beta)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list);

  if (moves_list.ML_nmoves == 0)
    return (SCORE_LOST + arg_depth);

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    position_id =
        query_position_id(arg_db, bdata(bfen), options.book_position_frequency);

    BDESTROY(bfen)

    if (position_id == INVALID)
      return (SCORE_PLUS_INFINITY);
  }

  int best_score = arg_alpha;

  int nmoves = 0;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int temp_score = SCORE_MINUS_INFINITY;

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      ++nmoves;

      do_move(object, imove, &moves_list, FALSE);

      int minimax_score = -help_walk_book_alpha_beta(arg_db,
                                                     object,
                                                     arg_depth + 1,
                                                     -arg_beta,
                                                     -best_score);

      undo_move(object, imove, &moves_list, FALSE);

      if (minimax_score == SCORE_MINUS_INFINITY)
        return (SCORE_PLUS_INFINITY);

      temp_score = minimax_score;
    }
    else
    {
      BSTRING(bmove)

      move2bstring(&moves_list, imove, bmove);

      i64_t move_id;

      move_id = query_move(arg_db,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency);

      BDESTROY(bmove)

      if (move_id == INVALID)
        continue;

      int evaluation = query_evaluation(arg_db, move_id);

      if (evaluation == SCORE_PLUS_INFINITY)
        continue;

      temp_score = evaluation;

      ++nmoves;

      do_move(object, imove, &moves_list, FALSE);

      int minimax_score = -help_walk_book_alpha_beta(arg_db,
                                                     object,
                                                     arg_depth + 1,
                                                     -arg_beta,
                                                     -best_score);

      undo_move(object, imove, &moves_list, FALSE);

      if (minimax_score != SCORE_MINUS_INFINITY)
        temp_score = minimax_score;
    }

    HARDBUG(temp_score == SCORE_MINUS_INFINITY)

    if (temp_score > best_score)
      best_score = temp_score;

    if (best_score >= arg_beta)
      break;
  }

  if (nmoves == 0)
    return (SCORE_PLUS_INFINITY);

  HARDBUG(best_score == SCORE_PLUS_INFINITY)

  return (best_score);
}

local void
help_walk_book(sqlite3 *arg_db, board_t *object, int arg_depth, bstring arg_bpv)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list);

  HARDBUG(moves_list.ML_nmoves == 0)

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    position_id =
        query_position_id(arg_db, bdata(bfen), options.book_position_frequency);

    BDESTROY(bfen)

    if (position_id == INVALID)
    {
      PRINTF("%s (position_id == INVALID)\n", bdata(arg_bpv));

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

      do_move(object, imove, &moves_list, FALSE);

      help_walk_book(arg_db, object, arg_depth + 1, bpv);

      undo_move(object, imove, &moves_list, FALSE);

      BDESTROY(bpv)
    }
    else
    {
      HARDBUG(position_id == INVALID)

      i64_t move_id;

      move_id = query_move(arg_db,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency);

      if (move_id == INVALID)
        continue;

      int evaluation = query_evaluation(arg_db, move_id);

      if (evaluation == SCORE_PLUS_INFINITY)
        continue;

      do_move(object, imove, &moves_list, FALSE);

      int minimax_score = -help_walk_book_alpha_beta(arg_db,
                                                     object,
                                                     arg_depth,
                                                     SCORE_MINUS_INFINITY,
                                                     SCORE_PLUS_INFINITY);

      BSTRING(bpv)

      HARDBUG(bformata(bpv,
                       "%s %s[%d, %d]",
                       bdata(arg_bpv),
                       bdata(bmove),
                       evaluation,
                       minimax_score) == BSTR_ERR)

      help_walk_book(arg_db, object, arg_depth + 1, bpv);

      undo_move(object, imove, &moves_list, FALSE);

      BDESTROY(bpv)
    }
  }

  BDESTROY(bmove)
}

void open_book(void)
{
  PRINTF("opening book..\n");

  sqlite3 *disk;

  int rc =
      my_sqlite3_open_v2(options.book_name, &disk, SQLITE_OPEN_READONLY, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database %s: %s\n",
           options.book_name,
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

  snprintf(attach_sql,
           MY_LINE_MAX,
           "ATTACH DATABASE '%s' AS disk;",
           options.book_name);

  rc = my_sqlite3_exec(book, attach_sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s errmsg=%s\n",
           attach_sql,
           my_sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  const char *sql = "SELECT name, sql FROM sqlite_master WHERE type='table'";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(disk, sql, &stmt);

  while (my_sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *table_name = (const char *)my_sqlite3_column_text(stmt, 0);

    const char *create_table_sql =
        (const char *)my_sqlite3_column_text(stmt, 1);

    if (strcmp(table_name, "sqlite_sequence") == 0)
      continue;

    rc = my_sqlite3_exec(book, create_table_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s errmsg=%s\n",
             create_table_sql,
             my_sqlite3_errmsg(book));

      FATAL("sqlite3", EXIT_FAILURE)
    }

    char create_sql[MY_LINE_MAX];

    snprintf(create_sql,
             MY_LINE_MAX,
             "INSERT INTO %s SELECT * FROM disk.%s",
             table_name,
             table_name);

    rc = my_sqlite3_exec(book, create_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s errmsg=%s\n",
             create_sql,
             my_sqlite3_errmsg(book));

      FATAL("sqlite3", EXIT_FAILURE)
    }
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  sql = "DETACH DATABASE disk;";

  rc = my_sqlite3_exec(book, sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s errmsg=%s\n",
           sql,
           my_sqlite3_errmsg(book));

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

void return_book_move(board_t *object,
                      moves_list_t *arg_moves_list,
                      bstring arg_bbook_move)
{
  HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list);

  if (moves_list.ML_nmoves == 0)
    return;

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    position_id =
        query_position_id(book, bdata(bfen), options.book_position_frequency);

    BDESTROY(bfen)

    if (position_id == INVALID)
      return;
  }

  int sort[NMOVES_MAX];

  if (options.book_randomness == 0)
    shuffle(sort, arg_moves_list->ML_nmoves, NULL);
  else
    shuffle(sort, arg_moves_list->ML_nmoves, &book_random);

  int best_score = SCORE_MINUS_INFINITY;

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int jmove = sort[imove];

    int evaluation = SCORE_MINUS_INFINITY;
    int minimax_score = SCORE_MINUS_INFINITY;
    int temp_score = SCORE_MINUS_INFINITY;

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      HARDBUG(position_id != INVALID)

      do_move(object, jmove, &moves_list, FALSE);

      minimax_score = -help_walk_book_alpha_beta(book,
                                                 object,
                                                 0,
                                                 SCORE_MINUS_INFINITY,
                                                 SCORE_PLUS_INFINITY);

      undo_move(object, jmove, &moves_list, FALSE);

      if (minimax_score == SCORE_MINUS_INFINITY)
        continue;

      temp_score = minimax_score;
    }
    else
    {
      HARDBUG(position_id == INVALID)

      // does the move occur in the book?

      move2bstring(&moves_list, jmove, bmove);

      i64_t move_id;

      move_id = query_move(book,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency);

      if (move_id == INVALID)
        continue;

      // does the move have an evaluation?

      evaluation = query_evaluation(book, move_id);

      if (evaluation == SCORE_PLUS_INFINITY)
        continue;

      temp_score = evaluation;

      // optionally the minimax score

      if (options.book_minimax)
      {
        do_move(object, jmove, &moves_list, FALSE);

        minimax_score = -help_walk_book_alpha_beta(book,
                                                   object,
                                                   0,
                                                   SCORE_MINUS_INFINITY,
                                                   SCORE_PLUS_INFINITY);

        undo_move(object, jmove, &moves_list, FALSE);

        // the minimax score is always preferred

        if (minimax_score != SCORE_MINUS_INFINITY)
          temp_score = minimax_score;
      }
    }

    HARDBUG(temp_score == SCORE_MINUS_INFINITY)

    move2bstring(&moves_list, jmove, bmove);

    PRINTF("book_move=%s evaluation=%d minimax_score=%d temp_score=%d\n",
           bdata(bmove),
           evaluation,
           minimax_score,
           temp_score);

    if (options.book_randomness > 0)
      temp_score /= options.book_randomness;

    if (temp_score > best_score)
    {
      move2bstring(&moves_list, jmove, arg_bbook_move);

      best_score = temp_score;
    }
  } // for (int imove

  BDESTROY(bmove)

  PRINTF("book_move=%s best_score=%d\n", bdata(arg_bbook_move), best_score);

  if (best_score < 0)
  {
    PRINTF("REJECTING BOOK MOVE!\n");

    HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)
  }
}

typedef struct
{
  i64_t P_id;
  int P_depth;
} position_t;

local i64_t npositions;
local position_t *positions;

local void query_positions(sqlite3 *arg_db, int arg_depth)
{
  MY_MALLOC_BY_TYPE(positions, position_t, NPOSITIONS_MAX)

  char *sql = "SELECT id, depth FROM positions "
              "WHERE depth = ? ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(my_sqlite3_bind_int(stmt, 1, arg_depth) != SQLITE_OK)

  npositions = 0;

  int rc;

  while ((rc = execute_sql(arg_db, stmt, TRUE)) == SQLITE_ROW)
  {
    HARDBUG(npositions >= NPOSITIONS_MAX)

    position_t *with = positions + npositions;

    with->P_id = my_sqlite3_column_int64(stmt, 0);
    with->P_depth = my_sqlite3_column_int(stmt, 1);

    npositions++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("npositions=%lld\n", npositions);
}

local void return_position_moves(sqlite3 *arg_db,
                                 i64_t arg_position_id,
                                 board_t *arg_board,
                                 moves_list_t *arg_moves_list)
{
  BSTRING(bfen)

  HARDBUG(query_position_fen(arg_db, arg_position_id, bfen) == INVALID)

  fen2board(arg_board, bdata(bfen));

  BDESTROY(bfen)

  gen_moves(arg_board, arg_moves_list);

  HARDBUG(arg_moves_list->ML_nmoves == 0)
  HARDBUG(arg_moves_list->ML_nmoves == 1)
  HARDBUG(arg_moves_list->ML_ncaptx > 0)
}

local void add_evaluations(my_sqlite3_t *arg_db,
                           search_t *arg_search,
                           i64_t arg_position_id,
                           moves_list_t *arg_moves_list,
                           int arg_centi_seconds)
{
  i64_t move_ids[NMOVES_MAX];
  bstring bmoves[NMOVES_MAX];

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  // gather all moves that do not have an evaluation yet

  char *sql = "SELECT m.id, m.move"
              " FROM moves m"
              " LEFT JOIN evaluations e on e.move_id = m.id"
              " WHERE m.position_id = ? AND e.move_id IS NULL;";

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

    HARDBUG(bassigncstr(bmoves[nmoves], (char *)move) == BSTR_ERR)

    nmoves++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("nmoves=%d\n", nmoves);

  for (int imove = 0; imove < nmoves; imove++)
  {
    int jmove;

    HARDBUG((jmove = search_move(arg_moves_list, bmoves[imove])) == INVALID)

    do_move(&(arg_search->S_board), jmove, arg_moves_list, FALSE);

    moves_list_t your_moves_list;

    construct_moves_list(&your_moves_list);

    gen_moves(&(arg_search->S_board), &your_moves_list);

    int evaluation = SCORE_MINUS_INFINITY;

    if (your_moves_list.ML_nmoves == 0)
    {
      evaluation = SCORE_WON_ABSOLUTE;
    }
    else
    {
      options.time_limit = arg_centi_seconds / 100.0;
      options.time_ntrouble = 0;

      pause_my_printf(arg_search->S_my_printf);

      do_search(arg_search,
                &your_moves_list,
                INVALID,
                INVALID,
                SCORE_MINUS_INFINITY,
                FALSE);

      resume_my_printf(arg_search->S_my_printf);

      evaluation = -arg_search->S_best_score;
    }

    HARDBUG(evaluation == SCORE_MINUS_INFINITY)
    HARDBUG(evaluation == SCORE_PLUS_INFINITY)

    undo_move(&(arg_search->S_board), jmove, arg_moves_list, FALSE);

    PRINTF("adding imove=%d move_ids[imove]=%lld bdata(bmoves[imove])=%s"
           " evaluation=%d\n",
           imove,
           move_ids[imove],
           bdata(bmoves[imove]),
           evaluation);

    append_sql_buffer(arg_db,
                      "INSERT OR IGNORE INTO evaluations (move_id, evaluation) "
                      "VALUES (%lld, %d);",
                      move_ids[imove],
                      evaluation);
  }

  flush_sql_buffer(arg_db, 0);

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)
}

// loops over all positions

local void add_evaluations2book_worker(my_sqlite3_t *arg_db,
                                       search_t *arg_search,
                                       int arg_centi_seconds)
{
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
          my_mpi_globals.MMG_id_slave)
        continue;
    }

    update_progress(&progress);

    i64_t position_id = positions[iposition].P_id;

    HARDBUG(position_id < 1)

    PRINTF("iposition=%lld position_id=%lld\n", iposition, position_id);

    moves_list_t moves_list;

    return_position_moves(arg_db->MS_db,
                          position_id,
                          &(arg_search->S_board),
                          &moves_list);

    add_evaluations(arg_db,
                    arg_search,
                    position_id,
                    &moves_list,
                    arg_centi_seconds);

  } // for (int iposition

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);
}

// only executed by MASTER

local void add_or_update_moves(my_sqlite3_t *arg_db, board_t *arg_board)
{
  BSTRING(bfen)

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  BSTRING(bmove)

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    i64_t position_id = positions[iposition].P_id;

    HARDBUG(query_position_fen(arg_db->MS_db, position_id, bfen) == INVALID)

    fen2board(arg_board, bdata(bfen));

    gen_moves(arg_board, &moves_list);

    HARDBUG(moves_list.ML_nmoves == 0)
    HARDBUG(moves_list.ML_nmoves == 1)
    HARDBUG(moves_list.ML_ncaptx > 0)

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      move2bstring(&moves_list, imove, bmove);

      append_sql_buffer(arg_db,
                        "INSERT OR IGNORE INTO moves (position_id, move) "
                        "VALUES (%lld, '%s');",
                        position_id,
                        bdata(bmove));
    }
  }

  flush_sql_buffer(arg_db, 0);

  BDESTROY(bmove)

  BDESTROY(bfen)
}

void add_evaluations2book(my_sqlite3_t *arg_db,
                          int arg_centi_seconds,
                          int arg_add_moves)
{
  ui64_t seed = return_my_random(&book_random);

  PRINTF("seed=%llX\n", seed);

  search_t search;

  construct_search(&search, STDOUT, NULL);

  fen2board(&(search.S_board), STARTING_POSITION2FEN);

  query_positions(arg_db->MS_db, 0);

  if (arg_add_moves > 0)
  {
    if (my_mpi_globals.MMG_id_slave <= 0)
    {
      PRINTF("adding moves to positions..\n");

      add_or_update_moves(arg_db, &(search.S_board));
    }

    my_mpi_barrier_v3("after adding or updating moves",
                      my_mpi_globals.MMG_comm_slaves,
                      my_mpi_globals.MMG_semkey_gen_book_barrier,
                      FALSE);
  }

  PRINTF("adding evaluations to moves..\n");

  add_evaluations2book_worker(arg_db, &search, arg_centi_seconds);
}

local void
add_position(my_sqlite3_t *arg_db, search_t *arg_search, int arg_depth)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_search->S_board), &moves_list);

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(&(arg_search->S_board), bfen, FALSE);

    append_sql_buffer(arg_db,
                      "INSERT OR IGNORE INTO positions (fen, depth) "
                      "VALUES ('%s', %d);",
                      bdata(bfen),
                      arg_depth);

    BDESTROY(bfen)
  }
  else
  {
    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(&(arg_search->S_board), imove, &moves_list, FALSE);

      add_position(arg_db, arg_search, arg_depth);

      undo_move(&(arg_search->S_board), imove, &moves_list, FALSE);
    }
  }
}

local void expand_book_worker(my_sqlite3_t *arg_db,
                              i64_t arg_position_id,
                              search_t *arg_search,
                              int arg_depth,
                              int arg_centi_seconds)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_search->S_board), &moves_list);

  if (moves_list.ML_nmoves == 0)
    return;

  HARDBUG((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))

  print_board(&(arg_search->S_board));

  PRINTF("expanding position..\n");

  PRINTF("adding moves..\n");

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    move2bstring(&moves_list, imove, bmove);

    append_sql_buffer(
        arg_db,
        "INSERT OR IGNORE INTO moves (position_id, move) VALUES (%lld, '%s');",
        arg_position_id,
        bdata(bmove));
  }

  flush_sql_buffer(arg_db, 0);

  PRINTF("adding evaluations..\n");

  add_evaluations(arg_db,
                  arg_search,
                  arg_position_id,
                  &moves_list,
                  arg_centi_seconds);

  BSTRING(bsql)

  int limit = EXPANSION_NMOVES;

  if (arg_depth <= 1)
    limit = NMOVES_MAX;

  PRINTF("expanding moves limit=%d\n", limit);

  bformata(bsql,
           "SELECT m.move, e.evaluation "
           "FROM moves m "
           "JOIN evaluations e ON m.id = e.move_id "
           "WHERE m.position_id = ? "
           "AND ABS(e.evaluation) <= %d "
           "ORDER BY e.evaluation DESC LIMIT %d;",
           EXPANSION_EVALUATION,
           limit);

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MS_db, bdata(bsql), &stmt);

  HARDBUG(my_sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  int nmoves = 0;

  while (execute_sql(arg_db->MS_db, stmt, TRUE) == SQLITE_ROW)
  {
    HARDBUG(nmoves >= NMOVES_MAX)

    const unsigned char *move = my_sqlite3_column_text(stmt, 0);

    int evaluation = my_sqlite3_column_int(stmt, 1);

    HARDBUG(bassigncstr(bmove, (char *)move) == BSTR_ERR)

    int jmove;

    HARDBUG((jmove = search_move(&moves_list, bmove)) == INVALID)

    PRINTF("expanding move=%s evaluation=%d\n", move, evaluation);

    do_move(&(arg_search->S_board), jmove, &moves_list, FALSE);

    add_position(arg_db, arg_search, arg_depth + 1);

    undo_move(&(arg_search->S_board), jmove, &moves_list, FALSE);

    nmoves++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  BDESTROY(bsql)

  BDESTROY(bmove)

  PRINTF("nmoves=%d\n", nmoves);

  flush_sql_buffer(arg_db, 0);
}

void expand_book(my_sqlite3_t *arg_db,
                 char *arg_fen,
                 int arg_depth,
                 int arg_centi_seconds)
{
  search_t search;

  construct_search(&search, STDOUT, NULL);

  BSTRING(bfen)

  // slave 0 adds all positions to the book

  if (my_mpi_globals.MMG_id_slave <= 0)
  {
    PRINTF("adding positions..\n");

    my_bstream_t my_bstream;

    construct_my_bstream(&my_bstream, arg_fen);

    while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
    {
      bstring bline = my_bstream.BS_bline;

      if (bchar(bline, 0) == '*')
        break;
      if (bchar(bline, 0) == '#')
        continue;
      if (strncmp(bdata(bline), "//", 2) == 0)
        continue;

      CSTRING(cfen, blength(bline))

      HARDBUG(my_sscanf(bdata(bline), "%s", cfen) != 1)

      HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

      fen2board(&(search.S_board), bdata(bfen));

      CDESTROY(cfen)

      print_board(&(search.S_board));

      add_position(arg_db, &search, 0);
    }

    destroy_my_bstream(&my_bstream);

    flush_sql_buffer(arg_db, 0);
  }

  my_mpi_barrier_v3("after adding positions",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_add_positions_barrier,
                    FALSE);

  // all slaves gather the positions

  query_positions(arg_db->MS_db, arg_depth);

  my_mpi_barrier_v3("after querying positions",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_query_positions_barrier,
                    FALSE);

  // now we analyze the positions in parallel

  i64_t ntodo = npositions;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = npositions / my_mpi_globals.MMG_nslaves + 1;

  PRINTF("ntodo=%lld\n", ntodo);

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  i64_t npositions_mpi = 0;

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (my_mpi_globals.MMG_nslaves > 0)
    {
      if ((iposition % my_mpi_globals.MMG_nslaves) !=
          my_mpi_globals.MMG_id_slave)
        continue;
    }

    update_progress(&progress);

    ++npositions_mpi;

    HARDBUG(query_position_fen(arg_db->MS_db,
                               positions[iposition].P_id,
                               bfen) == INVALID)

    fen2board(&(search.S_board), bdata(bfen));

    print_board(&(search.S_board));

    expand_book_worker(arg_db,
                       positions[iposition].P_id,
                       &search,
                       arg_depth,
                       arg_centi_seconds);
  }

  finalize_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  PRINTF("npositions_mpi=%lld\n", npositions_mpi);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &npositions_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  HARDBUG(npositions != npositions_mpi)
}

void walk_book(char *arg_db_name, char *arg_fen)
{
  sqlite3 *db;

  HARDBUG(!fexists(arg_db_name))

  int rc = my_sqlite3_open(arg_db_name, &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s errmsg=%s\n",
           arg_db_name,
           my_sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  // we start with the starting position

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
