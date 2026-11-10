// SCU REVISION 8.1262 di 10 nov 2026 15:00:45 CET
#include "globals.h"

#define NPOSITIONS_MAX 100000000

#define EXPANSION_NMOVES 2
#define EXPANSION_EVALUATION (SCORE_MAN / 4)

local my_random_t book_random;

local sqlite3 *book;

void init_book(void) { construct_my_random(&book_random, NULL); }

local void inline flip_score(score_t *arg_score)
{
  if (!IS_SCORE_NAN(*arg_score))
    *arg_score = -*arg_score;
}

local void help_walk_book_alpha_beta(sqlite3 *arg_db,
                                     board_t *arg_with_board,
                                     int arg_depth,
                                     score_t arg_alpha,
                                     score_t arg_beta,
                                     score_t *arg_best_score)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(arg_with_board, &moves_list);

  *arg_best_score = SCORE(SCORE_MINUS_INFINITY);

  if (moves_list.ML_nmoves == 0)
  {
    *arg_best_score = SCORE(SCORE_LOST_ABSOLUTE + arg_depth);

    return;
  }

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(arg_with_board, bfen, FALSE);

    int flags;

    position_id = query_position_id(arg_db,
                                    bdata(bfen),
                                    options.book_position_frequency,
                                    &flags);

    BDESTROY(bfen)

    if (position_id == INVALID)
    {
      *arg_best_score = SCORE_NOT_A_NUMBER;

      return;
    }

    if (!(flags & FLAG_POSITION_MOVES_EVALUATED))
    {
      *arg_best_score = SCORE_NOT_A_NUMBER;

      return;
    }
  }

  *arg_best_score = arg_alpha;

  int nmoves = 0;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    score_t temp_score = SCORE(SCORE_MINUS_INFINITY);

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      do_move(arg_with_board, imove, &moves_list);

      score_t minimax_score;

      help_walk_book_alpha_beta(arg_db,
                                arg_with_board,
                                arg_depth + 1,
                                -arg_beta,
                                -(*arg_best_score),
                                &minimax_score);

      flip_score(&minimax_score);

      undo_move(arg_with_board, imove, &moves_list);

      if (!IS_SCORE_NAN(minimax_score))
      {
        ++nmoves;

        temp_score = minimax_score;
      }
    }
    else
    {
      BSTRING(bmove)

      move2bstring(&moves_list, imove, bmove);

      i64_t move_id;
      int flags;

      move_id = query_move(arg_db,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency,
                           &flags);

      BDESTROY(bmove)

      HARDBUG(move_id == INVALID)

      score_t evaluation = query_evaluation(arg_db, move_id, FLAG_NULL);

      HARDBUG(IS_SCORE_NAN(evaluation))

      temp_score = evaluation;

      ++nmoves;

      // the move has been expanded,
      // the position after the move has been added to the database
      // but it could have not been evaluated yet

      if (flags & FLAG_MOVE_EXPANDED)
      {
        do_move(arg_with_board, imove, &moves_list);

        score_t minimax_score;

        help_walk_book_alpha_beta(arg_db,
                                  arg_with_board,
                                  arg_depth + 1,
                                  -arg_beta,
                                  -(*arg_best_score),
                                  &minimax_score);

        flip_score(&minimax_score);

        undo_move(arg_with_board, imove, &moves_list);

        if (!IS_SCORE_NAN(minimax_score))
          temp_score = minimax_score;
      }
    }

    if (temp_score > *arg_best_score)
      *arg_best_score = temp_score;

    if (*arg_best_score >= arg_beta)
      break;
  }

  if (nmoves == 0)
  {
    *arg_best_score = SCORE_NOT_A_NUMBER;

    return;
  }
}

local void help_walk_book(sqlite3 *arg_db,
                          board_t *arg_with_board,
                          int arg_depth,
                          bstring arg_bpv)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(arg_with_board, &moves_list);

  HARDBUG(moves_list.ML_nmoves == 0)

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(arg_with_board, bfen, FALSE);

    int flags;

    position_id = query_position_id(arg_db,
                                    bdata(bfen),
                                    options.book_position_frequency,
                                    &flags);

    BDESTROY(bfen)

    if (position_id == INVALID)
    {
      PRINTF("%s (position_id == INVALID)\n", bdata(arg_bpv));

      return;
    }

    if (!(flags & FLAG_POSITION_MOVES_EVALUATED))
    {
      PRINTF("%s !(flags & FLAG_POSITION_MOVES_EVALUATED)\n", bdata(arg_bpv));

      return;
    }
  }

  BSTRING(bmove)

  int nmoves = 0;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    move2bstring(&moves_list, imove, bmove);

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      HARDBUG(position_id != INVALID)

      BSTRING(bpv)

      HARDBUG(bformata(bpv, "%s %s(--, --)", bdata(arg_bpv), bdata(bmove)) ==
              BSTR_ERR)

      ++nmoves;

      do_move(arg_with_board, imove, &moves_list);

      help_walk_book(arg_db, arg_with_board, arg_depth + 1, bpv);

      undo_move(arg_with_board, imove, &moves_list);

      BDESTROY(bpv)
    }
    else
    {
      HARDBUG(position_id == INVALID)

      i64_t move_id;
      int flags;

      move_id = query_move(arg_db,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency,
                           &flags);

      HARDBUG(move_id == INVALID)

      score_t evaluation = query_evaluation(arg_db, move_id, FLAG_NULL);

      HARDBUG(IS_SCORE_NAN(evaluation))

      ++nmoves;

      if (flags & FLAG_MOVE_EXPANDED)
      {
        do_move(arg_with_board, imove, &moves_list);

        score_t minimax_score;

        help_walk_book_alpha_beta(arg_db,
                                  arg_with_board,
                                  arg_depth,
                                  SCORE(SCORE_MINUS_INFINITY),
                                  SCORE(SCORE_PLUS_INFINITY),
                                  &minimax_score);

        flip_score(&minimax_score);

        BSTRING(bpv)

        HARDBUG(bformata(bpv,
                         "%s %s[" PRINTF_SCORE_T ", " PRINTF_SCORE_T "]",
                         bdata(arg_bpv),
                         bdata(bmove),
                         SCORE2PRINTF(evaluation),
                         SCORE2PRINTF(minimax_score)) == BSTR_ERR)

        help_walk_book(arg_db, arg_with_board, arg_depth + 1, bpv);

        undo_move(arg_with_board, imove, &moves_list);

        BDESTROY(bpv)
      }
      else
      {
        BSTRING(bfen)

        board2fen(arg_with_board, bfen, FALSE);

        PRINTF("%s %s[" PRINTF_SCORE_T ", %s]\n",
               bdata(arg_bpv),
               bdata(bmove),
               SCORE2PRINTF(evaluation),
               bdata(bfen));

        BDESTROY(bfen)
      }
    }
  }

  HARDBUG(nmoves == 0)
  // PRINTF("%s (nmoves == 0)\n", bdata(arg_bpv));

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

    construct_board(&board, STDOUT, NETWORK0_ID);

    board_t *with = &board;

    string2board(with, STARTING_POSITION);

    BSTRING(bempty)

    help_walk_book(book, with, 0, bempty);

    BDESTROY(bempty)
  }
}

void return_book_move(board_t *arg_with_board,
                      moves_list_t *arg_moves_list,
                      bstring arg_bbook_move)
{
  HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(arg_with_board, &moves_list);

  if (moves_list.ML_nmoves == 0)
    return;

  i64_t position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(arg_with_board, bfen, FALSE);

    int flags;

    position_id = query_position_id(book,
                                    bdata(bfen),
                                    options.book_position_frequency,
                                    &flags);

    BDESTROY(bfen)

    if (position_id == INVALID)
      return;

    if (!(flags & FLAG_POSITION_MOVES_EVALUATED))
      return;
  }

  int sort[NMOVES_MAX];

  if (options.book_randomness == 0)
    shuffle(sort, arg_moves_list->ML_nmoves, NULL, INVALID);
  else
    shuffle(sort, arg_moves_list->ML_nmoves, &book_random, INVALID);

  score_t best_score = SCORE(SCORE_MINUS_INFINITY);

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int jmove = sort[imove];

    score_t evaluation = SCORE(SCORE_MINUS_INFINITY);
    score_t minimax_score = SCORE(SCORE_MINUS_INFINITY);
    score_t temp_score = SCORE(SCORE_MINUS_INFINITY);

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      HARDBUG(position_id != INVALID)

      do_move(arg_with_board, jmove, &moves_list);

      help_walk_book_alpha_beta(book,
                                arg_with_board,
                                0,
                                SCORE(SCORE_MINUS_INFINITY),
                                SCORE(SCORE_PLUS_INFINITY),
                                &minimax_score);

      flip_score(&minimax_score);

      undo_move(arg_with_board, jmove, &moves_list);

      if (IS_SCORE_NAN(minimax_score))
        continue;

      temp_score = minimax_score;
    }
    else
    {
      HARDBUG(position_id == INVALID)

      move2bstring(&moves_list, jmove, bmove);

      i64_t move_id;
      int flags;

      move_id = query_move(book,
                           position_id,
                           bdata(bmove),
                           options.book_move_frequency,
                           &flags);

      HARDBUG(move_id == INVALID)

      evaluation = query_evaluation(book, move_id, FLAG_NULL);

      HARDBUG(IS_SCORE_NAN(evaluation))

      temp_score = evaluation;

      if (options.book_minimax and (flags & FLAG_MOVE_EXPANDED))
      {
        do_move(arg_with_board, jmove, &moves_list);

        help_walk_book_alpha_beta(book,
                                  arg_with_board,
                                  0,
                                  SCORE(SCORE_MINUS_INFINITY),
                                  SCORE(SCORE_PLUS_INFINITY),
                                  &minimax_score);

        flip_score(&minimax_score);

        undo_move(arg_with_board, jmove, &moves_list);

        // the minimax score is always preferred

        if (!IS_SCORE_NAN(minimax_score))
          temp_score = minimax_score;
      }
    }

    HARDBUG(temp_score == SCORE(SCORE_MINUS_INFINITY))

    move2bstring(&moves_list, jmove, bmove);

    PRINTF("book_move=%s"
           " evaluation=" PRINTF_SCORE_T " minimax_score=" PRINTF_SCORE_T
           " temp_score=" PRINTF_SCORE_T "\n",
           bdata(bmove),
           SCORE2PRINTF(evaluation),
           SCORE2PRINTF(minimax_score),
           SCORE2PRINTF(temp_score));

    if (options.book_randomness > 0)
      temp_score /= options.book_randomness;

    if (temp_score > best_score)
    {
      move2bstring(&moves_list, jmove, arg_bbook_move);

      best_score = temp_score;
    }
  } // for (int imove

  BDESTROY(bmove)

  PRINTF("book_move=%s"
         " best_score=" PRINTF_SCORE_T "\n",
         bdata(arg_bbook_move),
         SCORE2PRINTF(best_score));

  if (best_score < 0)
  {
    PRINTF("REJECTING BOOK MOVE!\n");

    HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)
  }
}

local i64_t npositions;
local i64_t *position_ids;

local void query_positions(sqlite3 *arg_db)
{
  MY_MALLOC_BY_TYPE(position_ids, i64_t, NPOSITIONS_MAX)

  char *sql = "SELECT id, flags FROM positions ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  npositions = 0;

  while (execute_sql(arg_db, stmt, TRUE) == SQLITE_ROW)
  {
    i64_t id = my_sqlite3_column_int64(stmt, 0);

    int flags = my_sqlite3_column_int(stmt, 1);

    if (flags == 0)
    {
      HARDBUG(npositions >= NPOSITIONS_MAX)

      position_ids[npositions] = id;

      npositions++;
    }
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

  HARDBUG(query_position_fen(arg_db, arg_position_id, bfen, FLAG_NULL) ==
          INVALID)

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

  my_sqlite3_prepare_v2(arg_db->MSQL_db, sql, &stmt);

  HARDBUG(my_sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  int nmoves = 0;

  while (execute_sql(arg_db->MSQL_db, stmt, TRUE) == SQLITE_ROW)
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

    do_move(&(arg_search->S_board), jmove, arg_moves_list);

    moves_list_t your_moves_list;

    construct_moves_list(&your_moves_list);

    gen_moves(&(arg_search->S_board), &your_moves_list);

    score_t evaluation = SCORE(SCORE_MINUS_INFINITY);

    if (your_moves_list.ML_nmoves == 0)
    {
      evaluation = SCORE(SCORE_WON_ABSOLUTE);
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
                SCORE(SCORE_MINUS_INFINITY),
                NULL);

      resume_my_printf(arg_search->S_my_printf);

      evaluation = -arg_search->S_best_score;
    }

    HARDBUG(IS_SCORE_NAN(evaluation))
    HARDBUG(evaluation == SCORE(SCORE_MINUS_INFINITY))
    HARDBUG(evaluation == SCORE(SCORE_PLUS_INFINITY))

    undo_move(&(arg_search->S_board), jmove, arg_moves_list);

    PRINTF("adding imove=%d move_ids[imove]=%lld bdata(bmoves[imove])=%s"
           " evaluation=" PRINTF_SCORE_T "\n",
           imove,
           move_ids[imove],
           bdata(bmoves[imove]),
           SCORE2PRINTF(evaluation));

    append_sql_buffer(arg_db,
                      "INSERT OR IGNORE INTO evaluations (move_id, evaluation)"
                      " VALUES (%lld, " PRINTF_SCORE_T ");",
                      move_ids[imove],
                      SCORE2PRINTF(evaluation));
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
  progress_t progress;

  construct_progress(&progress, "add_evaluations2book_worker", npositions, 10);

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (!make_progress(&progress, iposition))
      continue;

    i64_t position_id = position_ids[iposition];

    HARDBUG(position_id < 1)

    PRINTF("iposition=%lld position_id=%lld\n", iposition, position_id);

    moves_list_t moves_list;

    return_position_moves(arg_db->MSQL_db,
                          position_id,
                          &(arg_search->S_board),
                          &moves_list);

    add_evaluations(arg_db,
                    arg_search,
                    position_id,
                    &moves_list,
                    arg_centi_seconds);

  } // for (i64_t iposition

  finalize_progress(&progress);

  destroy_progress(&progress);

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
    i64_t position_id = position_ids[iposition];

    HARDBUG(query_position_fen(arg_db->MSQL_db, position_id, bfen, FLAG_NULL) ==
            INVALID)

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

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  fen2board(&(search.S_board), STARTING_POSITION2FEN);

  query_positions(arg_db->MSQL_db);

  if (arg_add_moves > 0)
  {
    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_lead_slave)
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

local void add_position(my_sqlite3_t *arg_db, search_t *arg_search)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_search->S_board), &moves_list);

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(&(arg_search->S_board), bfen, FALSE);

    append_sql_buffer(arg_db,
                      "INSERT OR IGNORE INTO positions (fen) "
                      "VALUES ('%s');",
                      bdata(bfen));

    BDESTROY(bfen)
  }
  else
  {
    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(&(arg_search->S_board), imove, &moves_list);

      add_position(arg_db, arg_search);

      undo_move(&(arg_search->S_board), imove, &moves_list);
    }
  }
}

local void expand_book_worker(my_sqlite3_t *arg_db,
                              i64_t arg_position_id,
                              search_t *arg_search,
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

  BDESTROY(bmove)

  flush_sql_buffer(arg_db, 0);

  PRINTF("adding evaluations..\n");

  add_evaluations(arg_db,
                  arg_search,
                  arg_position_id,
                  &moves_list,
                  arg_centi_seconds);

  BSTRING(bsql)

  PRINTF("checking moves..\n");

  bformata(bsql,
           "SELECT m.id, m.move, e.evaluation "
           "FROM moves m "
           "JOIN evaluations e ON m.id = e.move_id "
           "WHERE m.position_id = ? "
           "ORDER BY e.evaluation DESC");

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db->MSQL_db, bdata(bsql), &stmt);

  HARDBUG(my_sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  i64_t move_ids[NMOVES_MAX];
  bstring bmoves[NMOVES_MAX];
  score_t evaluations[NMOVES_MAX];

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  int nmoves = 0;

  // DO NOT USE APPEND_SQL INSIDE A STEPPED QUERY!

  while (execute_sql(arg_db->MSQL_db, stmt, TRUE) == SQLITE_ROW)
  {
    HARDBUG(nmoves >= NMOVES_MAX)

    move_ids[nmoves] = my_sqlite3_column_int64(stmt, 0);

    const unsigned char *move = my_sqlite3_column_text(stmt, 1);

    HARDBUG(bassigncstr(bmoves[nmoves], (char *)move) == BSTR_ERR)

    evaluations[nmoves] = my_sqlite3_column_int64(stmt, 2);

    nmoves++;
  }

  HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

  BDESTROY(bsql)

  HARDBUG(nmoves != moves_list.ML_nmoves)

  nmoves = 0;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    PRINTF("imove=%d"
           "  move_ids[imove]=%lld"
           " bmoves[imove]=%s"
           " evaluations[imove]=" PRINTF_SCORE_T "\n",
           imove,
           move_ids[imove],
           bdata(bmoves[imove]),
           SCORE2PRINTF(evaluations[imove]));

    if (ABS_SCORE_T(evaluations[imove]) > SCORE(EXPANSION_EVALUATION))
    {
      PRINTF("evaluations[imove] > SCORE(EXPANSION_EVALUATION)\n");

      append_sql_buffer(arg_db,
                        "UPDATE moves "
                        "SET flags = flags | %d WHERE id = %lld;",
                        FLAG_MOVE_EVALUATION_SIZE,
                        move_ids[imove]);
    }
    else if (nmoves >= EXPANSION_NMOVES)
    {
      PRINTF("nmoves >= EXPANSION_NMOVES\n");

      append_sql_buffer(arg_db,
                        "UPDATE moves "
                        "SET flags = flags | %d WHERE id = %lld;",
                        FLAG_MOVE_NMOVES_LIMIT,
                        move_ids[imove]);
    }
    else
    {
      int jmove;

      HARDBUG((jmove = search_move(&moves_list, bmoves[imove])) == INVALID)

      PRINTF("expanding move and adding position after move..\n");

      append_sql_buffer(arg_db,
                        "UPDATE moves "
                        "SET flags = flags | %d WHERE id = %lld;",
                        FLAG_MOVE_EXPANDED,
                        move_ids[imove]);

      do_move(&(arg_search->S_board), jmove, &moves_list);

      add_position(arg_db, arg_search);

      undo_move(&(arg_search->S_board), jmove, &moves_list);

      nmoves++;
    }
  }

  PRINTF("nmoves=%d\n", nmoves);

  append_sql_buffer(arg_db,
                    "UPDATE positions "
                    "SET flags = flags | %d WHERE id = %lld;",
                    FLAG_POSITION_MOVES_EVALUATED,
                    arg_position_id);

  flush_sql_buffer(arg_db, 0);

  for (int imove = 0; imove < NMOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)
}

void expand_book(my_sqlite3_t *arg_db, char *arg_fen, int arg_centi_seconds)
{
  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  BSTRING(bfen)

  // slave 0 adds all positions to the book

  if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_lead_slave)
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

      add_position(arg_db, &search);
    }

    destroy_my_bstream(&my_bstream);

    flush_sql_buffer(arg_db, 0);
  }

  my_mpi_barrier_v3("after adding positions",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_add_positions_barrier,
                    FALSE);

  // all slaves gather the positions

  query_positions(arg_db->MSQL_db);

  my_mpi_barrier_v3("after querying positions",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_query_positions_barrier,
                    FALSE);

  // now we analyze the positions in parallel

  progress_t progress;

  construct_progress(&progress, "expand_book", npositions, 10);

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    if (!make_progress(&progress, iposition))
      continue;

    HARDBUG(query_position_fen(arg_db->MSQL_db,
                               position_ids[iposition],
                               bfen,
                               FLAG_NULL) == INVALID)

    fen2board(&(search.S_board), bdata(bfen));

    expand_book_worker(arg_db,
                       position_ids[iposition],
                       &search,
                       arg_centi_seconds);
  }

  finalize_progress(&progress);

  destroy_progress(&progress);

  flush_sql_buffer(arg_db, 0);
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

  construct_board(&board, STDOUT, NETWORK0_ID);

  board_t *with = &board;

  if (arg_fen == NULL)
    string2board(with, STARTING_POSITION);
  else
    fen2board(with, arg_fen);

  print_board(&board);

  BSTRING(bempty)

  help_walk_book(db, with, 0, bempty);

  BDESTROY(bempty)

  HARDBUG(my_sqlite3_close(db) != SQLITE_OK)
}
