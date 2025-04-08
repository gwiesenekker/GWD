//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

#define NPOSITIONS_MAX 100000000
#define FULL_WIDTH

local my_random_t book_random;

local sqlite3 *book;

void init_book(void)
{
  construct_my_random(&book_random, INVALID);
}

local int help_walk_book_alpha_beta(sqlite3 *arg_db, board_t *object,
  int arg_depth)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(object, &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0) return(SCORE_LOST + arg_depth);

  int position_id = INVALID;
 
  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    position_id = query_position(arg_db, bdata(bfen), 0);
  
    BDESTROY(bfen)
   
    if (position_id == INVALID) return(SCORE_PLUS_INFINITY);
  }

  int best_score = SCORE_MINUS_INFINITY;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int temp_score = SCORE_MINUS_INFINITY;

    if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      do_move(object, imove, &moves_list);
  
      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1);
    
      undo_move(object, imove, &moves_list);

      if (temp_score == SCORE_MINUS_INFINITY) return(SCORE_PLUS_INFINITY);
    }
    else
    {
      BSTRING(bmove)

      move2bstring(&moves_list, imove, bmove);

      int move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove), 0);

      BDESTROY(bmove)

      HARDBUG(move_id == INVALID)

      int centi_seconds;

      int evaluation = query_evaluation(arg_db, move_id, &centi_seconds, 0);

      do_move(object, imove, &moves_list);

      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1);

      undo_move(object, imove, &moves_list);

      //if temp_score == SCORE_MINUS_INFINITY
      //the evaluation of the position after the move is unknown

      if (temp_score == SCORE_MINUS_INFINITY) temp_score = evaluation;
    }

    if (temp_score > best_score) best_score = temp_score;
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

  int position_id = INVALID;

  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);
  
    position_id = query_position(arg_db, bdata(bfen), 0);
  
    BDESTROY(bfen)

    if (position_id == INVALID)
    {
      PRINTF("%s (position_id == INVALID)\n", bdata(arg_bpv));
  
      return;
    }
  }

  int nmoves_ge0 = 0;

  BSTRING(bmove)

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    move2bstring(&moves_list, imove, bmove);

    if ((moves_list.ML_nmoves == 1) or (moves_list.ML_ncaptx > 0))
    {
      HARDBUG(position_id != INVALID)

      BSTRING(bpv)

      HARDBUG(bformata(bpv, "%s%s(--, --) ", bdata(arg_bpv), bdata(bmove)) ==
              BSTR_ERR)

      do_move(object, imove, &moves_list);

      help_walk_book(arg_db, object, arg_depth + 1, bpv);
  
      undo_move(object, imove, &moves_list);

      BDESTROY(bpv)
    }
    else
    {
      HARDBUG(position_id == INVALID)

      int move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove), 0);

      if (move_id == INVALID) continue;  
  
      nmoves_ge0++;

      int centi_seconds;

      int evaluation = query_evaluation(arg_db, move_id, &centi_seconds, 0);

      do_move(object, imove, &moves_list);

      int best_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth);

      BSTRING(bpv)

      HARDBUG(bformata(bpv, "%s%s[%d(%.2fs), %d] ",
              bdata(arg_bpv), bdata(bmove),
              evaluation, centi_seconds / 100.0, best_score) == BSTR_ERR)

      help_walk_book(arg_db, object, arg_depth + 1, bpv);

      undo_move(object, imove, &moves_list);

      BDESTROY(bpv)
    }
  }

  BDESTROY(bmove)

  if (nmoves_ge0 == 0) PRINTF("%s (nmoves_ge0 == 0)\n", bdata(arg_bpv));
}

void open_book(void)
{
  PRINTF("opening book..\n");

  sqlite3 *disk;

  //int rc = sqlite3_open(options.book_name, &disk);

  int rc = sqlite3_open_v2(options.book_name, &disk,
                           SQLITE_OPEN_READONLY, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database %s: %s\n", options.book_name,
           sqlite3_errmsg(disk));

    FATAL("sqlite3", EXIT_FAILURE)
  }
  
  rc = sqlite3_open(":memory:", &book);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s\n", sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  char attach_sql[MY_LINE_MAX];

  snprintf(attach_sql, MY_LINE_MAX,
           "ATTACH DATABASE '%s' AS disk;", options.book_name);

  rc = sqlite3_exec(book, attach_sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s err_msg=%s\n",
           attach_sql, sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  const char *sql = "SELECT name, sql FROM sqlite_master WHERE type='table'";

  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(disk, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s err_msg=%s\n",
           sql, sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *table_name = (const char *) sqlite3_column_text(stmt, 0);
    const char *create_table_sql = (const char *) sqlite3_column_text(stmt, 1);

    if (strcmp(table_name, "sqlite_sequence") == 0) continue;

    rc = sqlite3_exec(book, create_table_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s err_msg=%s\n",
             create_table_sql, sqlite3_errmsg(book));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    char create_sql[MY_LINE_MAX];

    snprintf(create_sql, MY_LINE_MAX,
      "INSERT INTO %s SELECT * FROM disk.%s", table_name, table_name);

    rc = sqlite3_exec(book, create_sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to exec statement: %s err_msg=%s\n",
             create_sql, sqlite3_errmsg(book));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }
  }

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  sql = "DETACH DATABASE disk;";

  rc = sqlite3_exec(book, sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s err_msg=%s\n",
           sql, sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_close(disk);

  if (FALSE)
  {
    board_t board;
    
    construct_board(&board, STDOUT, FALSE);

    board_t *with = &board;
  
    string2board(with, STARTING_POSITION, TRUE);
  
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

  int position_id = INVALID;
 
  if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
  {
    BSTRING(bfen)

    board2fen(object, bfen, FALSE);

    position_id = query_position(book, bdata(bfen), 0);
  
    BDESTROY(bfen)
   
    if (position_id == INVALID) return;
  }

  int sort[MOVES_MAX];

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

      int move_id;
    
      move_id = query_move(book, position_id, bdata(bmove), 0);
  
      HARDBUG(move_id == INVALID)
  
      evaluation = query_evaluation(book, move_id, &centi_seconds, 0);
  
      HARDBUG(evaluation == SCORE_PLUS_INFINITY)
    }

    int minimax_score = SCORE_MINUS_INFINITY;

    if (options.book_minimax or
        (moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves == 1))
    {
      do_move(object, jmove, &moves_list);

      minimax_score = -help_walk_book_alpha_beta(book, object, 0);
  
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
    }
  }//for (int imove

  BDESTROY(bmove)

  HARDBUG(best_move == INVALID)

  HARDBUG(best_score == SCORE_MINUS_INFINITY)

  move2bstring(&moves_list, best_move, arg_bbook_move);

  PRINTF("book_move=%s best_score=%d\n", bdata(arg_bbook_move), best_score);
}

local int *semaphore;
local MPI_Win win;
local int npositions;
local int *position_ids;

local void query_position_ids(sqlite3 *arg_db)
{
  char *sql = "SELECT id FROM positions ORDER BY id ASC;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s\n", sqlite3_errmsg(arg_db));
  
    FATAL("sqlite3", EXIT_FAILURE)
  }

  npositions = 0;

  while ((rc = execute_sql(arg_db, stmt, TRUE, INVALID)) == SQLITE_ROW)
  {
    int position_id = sqlite3_column_int(stmt, 0);

    HARDBUG(npositions >= NPOSITIONS_MAX)

    position_ids[npositions++] = position_id;
  }

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  PRINTF("npositions=%d\n", npositions);
}

local void return_position_moves(sqlite3 *arg_db, int arg_position_id,
  board_t *arg_board, moves_list_t *arg_moves_list)
{
  char *sql = "SELECT fen FROM positions WHERE id = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));
 
    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_int(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG((rc = execute_sql(arg_db, stmt, TRUE, INVALID)) != SQLITE_ROW)

  const unsigned char *fen = sqlite3_column_text(stmt, 0);

  BSTRING(bfen)

  HARDBUG(bassigncstr(bfen, (char *) fen) == BSTR_ERR)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  fen2board(arg_board, bdata(bfen), FALSE);

  BDESTROY(bfen)

  gen_moves(arg_board, arg_moves_list, FALSE);

  HARDBUG(arg_moves_list->ML_nmoves == 0)
  HARDBUG(arg_moves_list->ML_nmoves == 1)
  HARDBUG(arg_moves_list->ML_ncaptx > 0)
}

local void help_gen_book_controller(sqlite3 *arg_db, search_t *arg_search,
  int arg_centi_seconds, sql_buffer_t *arg_sql_buffer,
  int *npositions_added, int *nmoves_added)
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

      help_gen_book_controller(arg_db, arg_search,
        arg_centi_seconds, arg_sql_buffer,
        npositions_added, nmoves_added);

      undo_move(&(arg_search->S_board), imove, &moves_list);
    }
  }
  else
  {
    BSTRING(bfen)

    board2fen(&(arg_search->S_board), bfen, FALSE);

    int position_id = query_position(arg_db, bdata(bfen), 0);

    if (position_id == INVALID)
    {
      if ((*npositions_added < 100) or ((*npositions_added % 10000) == 0))
        PRINTF("adding position *npositions_added=%d position=%s\n",
               *npositions_added, bdata(bfen));
  
      append_sql_buffer(arg_sql_buffer,
        "INSERT OR IGNORE INTO positions (fen)"  
        " VALUES ('%s');", bdata(bfen));
  
      BSTRING(bmove)

      for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
      {
        move2bstring(&moves_list, imove, bmove);
        
        if ((*npositions_added < 100) or ((*npositions_added % 10000) == 0))
        {
          PRINTF("adding move *npositions_added=%d move=%s arg_centi_seconds=%d\n",
                 *npositions_added, bdata(bmove), arg_centi_seconds);
        }

        *nmoves_added += 1;
  
        append_sql_buffer(arg_sql_buffer,
          "INSERT INTO moves"  
          " (position_id, move, nwon, ndraw, nlost, centi_seconds)"
          " VALUES ((SELECT id FROM positions WHERE fen = '%s'), '%s',"
          " 0, 0, 0, %d)"
          " ON CONFLICT(position_id, move) DO UPDATE SET"
          "  centi_seconds = excluded.centi_seconds"
          " WHERE excluded.centi_seconds > moves.centi_seconds;",
          bdata(bfen), bdata(bmove), arg_centi_seconds);
      }

      BDESTROY(bmove)
    }

    *npositions_added += 1;

    BDESTROY(bfen)
  }
}

local void gen_book_controller(sqlite3 *arg_db, search_t *arg_search, int arg_limit)
{
  sql_buffer_t sql_buffer;

  construct_sql_buffer(&sql_buffer, arg_db,
                       my_mpi_globals.MY_MPIG_comm_slaves, win, INVALID);

  int nrows = 0;
  int npositions_added = 0;
  int nmoves_added = 0;

  bstring bmoves[MOVES_MAX];
  int evaluations[MOVES_MAX];

  BSTRING(bfen)

  for (int imove = 0; imove < MOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)
  
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  for (int iposition = 0; iposition < npositions; iposition++)
  {
    int position_id = position_ids[iposition];

    HARDBUG(position_id < 1)

    if (my_mpi_globals.MY_MPIG_nslaves > 0)
    {
      if (((position_id - 1) % my_mpi_globals.MY_MPIG_nslaves) !=
          my_mpi_globals.MY_MPIG_id_slave) continue;
    }

    return_position_moves(arg_db, position_id,
                          &(arg_search->S_board), &moves_list);
    
    board2fen(&(arg_search->S_board), bfen, FALSE);

    char *sql =
      "SELECT m.move, e.evaluation"
      " FROM moves m"
      " JOIN ("
      "  SELECT move_id, evaluation, centi_seconds"
      "  FROM evaluations"
      "  WHERE (move_id, centi_seconds) IN ("
      "   SELECT move_id, MAX(centi_seconds)"
      "   FROM evaluations"
      "   GROUP BY move_id"
      "  )"
      " ) e ON m.id = e.move_id"
      " WHERE m.position_id = ?"
      " ORDER BY e.evaluation DESC"
      " LIMIT ?;";

    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to prepare statement: %s\n", sqlite3_errmsg(arg_db));
    
      FATAL("sqlite3", EXIT_FAILURE)
    }
  
    HARDBUG(sqlite3_bind_int(stmt, 1, position_id) != SQLITE_OK)

    HARDBUG(sqlite3_bind_int(stmt, 2, arg_limit) != SQLITE_OK)

    int nmoves = 0;

    while ((rc = execute_sql(arg_db, stmt, TRUE, INVALID)) == SQLITE_ROW)
    {
      ++nrows;

      HARDBUG(nmoves >= MOVES_MAX)

      const unsigned char *move = sqlite3_column_text(stmt, 0);

      HARDBUG(bassigncstr(bmoves[nmoves], (char *) move) == BSTR_ERR)

      evaluations[nmoves] = sqlite3_column_int(stmt, 1);

      if ((iposition < 100) or ((iposition % 10000) == 0))
      {
        PRINTF("iposition=%d(%.2f%%) position=%s nmoves=%d moves=%s evaluations=%d\n",
               iposition, iposition * 100.0 / npositions,
               bdata(bfen), nmoves, bdata(bmoves[nmoves]),
               evaluations[nmoves]);
      }

      nmoves++;
    }

    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    if ((iposition < 100) or ((iposition % 10000) == 0))
      PRINTF("nmoves=%d\n", nmoves);

    for (int imove = 0; imove < nmoves; imove++)
    {
      int jmove;

      HARDBUG((jmove = search_move(&moves_list, bmoves[imove])) == INVALID)
  
      int centi_seconds = 1;

      if (abs(evaluations[imove]) > 10)
        centi_seconds = 10;
      else
        centi_seconds = 100;
     
      do_move(&(arg_search->S_board), jmove, &moves_list);
  
      help_gen_book_controller(arg_db, arg_search, centi_seconds, 
        &sql_buffer, &npositions_added, &nmoves_added);
  
      undo_move(&(arg_search->S_board), jmove, &moves_list);
    }
  }//iposition

  flush_sql_buffer(&sql_buffer, 0);

  for (int imove = 0; imove < MOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)

  BDESTROY(bfen)

  PRINTF("nrows=%d npositions_added=%d nmoves_added=%d\n",
         nrows, npositions_added, nmoves_added);
}

local void gen_book_worker(sqlite3 *arg_db, search_t *arg_search)
{
  sql_buffer_t sql_buffer;

  construct_sql_buffer(&sql_buffer, arg_db,
                       my_mpi_globals.MY_MPIG_comm_slaves, win, INVALID);

  BSTRING(bfen)

  int move_ids[MOVES_MAX];
  bstring bmoves[MOVES_MAX];
  int centi_seconds[MOVES_MAX];

  for (int imove = 0; imove < MOVES_MAX; imove++)
    HARDBUG((bmoves[imove] = bfromcstr("")) == NULL)

  moves_list_t moves_list;
 
  for (int iposition = 0; iposition < npositions; iposition++)
  {
    int position_id = position_ids[iposition];

    HARDBUG(position_id < 1)

    if (my_mpi_globals.MY_MPIG_nslaves > 0)
    {
      if (((position_id - 1) % my_mpi_globals.MY_MPIG_nslaves) !=
          my_mpi_globals.MY_MPIG_id_slave) continue;
    }

    PRINTF("iposition=%d position_id=%d\n", iposition, position_id);

    return_position_moves(arg_db, position_id,
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

    int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
      PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
             sql, rc, sqlite3_errmsg(arg_db));

      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(sqlite3_bind_int(stmt, 1, position_id) != SQLITE_OK)

    int nmoves = 0;

    while ((rc = execute_sql(arg_db, stmt, TRUE, INVALID)) == SQLITE_ROW)
    {
      HARDBUG(nmoves >= MOVES_MAX)

      move_ids[nmoves] = sqlite3_column_int(stmt, 0);

      const unsigned char *move = sqlite3_column_text(stmt, 1);

      HARDBUG(bassigncstr(bmoves[nmoves], (char *) move) == BSTR_ERR)

      centi_seconds[nmoves] = sqlite3_column_int(stmt, 2);

      nmoves++;
    }

    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

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

      PRINTF("adding move_id=%d move=%s"
             " centi_seconds=%d evaluation=%d\n",
             move_ids[imove], bdata(bmoves[imove]),
             centi_seconds[imove], evaluation);
  
      append_sql_buffer(&sql_buffer,
        "INSERT INTO evaluations (move_id, centi_seconds, evaluation)"
        " VALUES (%d, %d, %d);",
        move_ids[imove], centi_seconds[imove], evaluation);
    }
  }//for (int iposition

  flush_sql_buffer(&sql_buffer, 0);

  for (int imove = 0; imove < MOVES_MAX; imove++)
    HARDBUG(bdestroy(bmoves[imove]) == BSTR_ERR)

  BDESTROY(bfen)
}

local void add_position(sqlite3 *arg_db, board_t *arg_board,
  int arg_centi_seconds)
{
  BSTRING(bfen)

  board2fen(arg_board, bfen, FALSE);

  moves_list_t moves_list;
    
  construct_moves_list(&moves_list);
  
  gen_moves(arg_board, &moves_list, FALSE);

  if ((moves_list.ML_ncaptx > 0) or (moves_list.ML_nmoves <= 1))
  {
    print_board(arg_board);

    PRINTF("FUNNY POSITION\n");

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(arg_board, imove, &moves_list);

      add_position(arg_db, arg_board, arg_centi_seconds);

      undo_move(arg_board, imove, &moves_list);
    }
  }
  else
  {
    int position_id = query_position(arg_db, bdata(bfen), 0);

    if (position_id == INVALID)
      position_id = insert_position(arg_db, bdata(bfen), 0);

    BSTRING(bmove)

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      move2bstring(&moves_list, imove, bmove);
    
      (void) insert_move(arg_db, position_id, bdata(bmove),
                         arg_centi_seconds, 0);
    }

    BDESTROY(bmove)
  }

  BDESTROY(bfen)
}

void gen_book(char *arg_db_name, char *arg_positions_name)
{
  ui64_t seed = return_my_random(&book_random);

  PRINTF("seed=%llX\n", seed);

  my_mpi_win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MY_MPIG_comm_slaves, &semaphore, &win);

  if (my_mpi_globals.MY_MPIG_id_slave == 0) *semaphore = 0;

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_slaves, 0, win);

  sqlite3 *db = NULL;

  //open database

  if (my_mpi_globals.MY_MPIG_id_slave <= 0)
  {
    int rc = sqlite3_open(arg_db_name, &db);
  
    if (rc != SQLITE_OK)
    {
      PRINTF("Cannot open database: %s err_msg=%s\n",
             arg_db_name, sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    create_tables(db, 0);

    HARDBUG(execute_sql(db, "PRAGMA journal_mode = WAL;", FALSE, 0) !=
            SQLITE_OK)

    HARDBUG(execute_sql(db, "PRAGMA cache_size=-131072;", FALSE, 0) !=
            SQLITE_OK)
  }

  my_mpi_barrier("after open database", my_mpi_globals.MY_MPIG_comm_slaves,
                 TRUE);

  if (my_mpi_globals.MY_MPIG_id_slave > 0)
  {
    HARDBUG(my_mpi_globals.MY_MPIG_nslaves == 0)

    int rc = sqlite3_open(arg_db_name, &db);
  
    if (rc != SQLITE_OK)
    {
      PRINTF("Cannot open database: %s err_msg=%s\n",
             arg_db_name, sqlite3_errmsg(db));
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(execute_sql(db, "PRAGMA cache_size=-131072;", FALSE, INVALID) !=
            SQLITE_OK)
  }

  search_t search;

  construct_search(&search, STDOUT, NULL);

  string2board(&(search.S_board), STARTING_POSITION, TRUE);

  MY_MALLOC(position_ids, int, NPOSITIONS_MAX)

  if (arg_positions_name != NULL)
  {
    if (my_mpi_globals.MY_MPIG_id_slave <= 0)
    {
      PRINTF("adding positions..\n");

      HARDBUG(execute_sql(db, "BEGIN TRANSACTION;", FALSE, 0) !=
              SQLITE_OK)
  
      my_bstream_t my_bstream;
     
      construct_my_bstream(&my_bstream, arg_positions_name);
  
      BSTRING(bmove)
      
      while(my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
      {
        bstring bfen = my_bstream.BS_bline;
  
        if (strncmp(bdata(bfen), "//", 2) == 0) continue;
  
        fen2board(&(search.S_board), bdata(bfen), TRUE);
  
        add_position(db, &(search.S_board), 100);
      }

      BDESTROY(bmove)
  
      destroy_my_bstream(&my_bstream);
  
      HARDBUG(execute_sql(db, "COMMIT;", FALSE, INVALID) != SQLITE_OK)
    }

    my_mpi_barrier("after adding positions", my_mpi_globals.MY_MPIG_comm_slaves,
                   FALSE);
  
    PRINTF("adding evaluations to moves..\n");
  
    query_position_ids(db);
  
    my_mpi_barrier("after query position ids",
                   my_mpi_globals.MY_MPIG_comm_slaves, FALSE);

    gen_book_worker(db, &search);
  }
  else
  {
    PRINTF("adding positions after moves..\n");
  
    query_position_ids(db);

    my_mpi_barrier("after query position ids",
                   my_mpi_globals.MY_MPIG_comm_slaves, FALSE);

    gen_book_controller(db, &search, 3);
  
    my_mpi_barrier("after controller", my_mpi_globals.MY_MPIG_comm_slaves,
                   FALSE);
  
    PRINTF("adding evaluations to moves..\n");
  
    query_position_ids(db);
  
    my_mpi_barrier("after query position ids",
                   my_mpi_globals.MY_MPIG_comm_slaves, FALSE);

    gen_book_worker(db, &search);
  }

  my_mpi_barrier("before sqlite3_close", my_mpi_globals.MY_MPIG_comm_slaves,
                 FALSE);
  
  //close database

  if (my_mpi_globals.MY_MPIG_id_slave > 0)
    HARDBUG(sqlite3_close(db) != SQLITE_OK)

  my_mpi_barrier("before wal_checkpoint",
                 my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  if (my_mpi_globals.MY_MPIG_id_slave <= 0)
  {
    wal_checkpoint(db);

    HARDBUG(sqlite3_close(db) != SQLITE_OK)

    HARDBUG(sqlite3_open(arg_db_name, &db) != SQLITE_OK)

    wal_checkpoint(db);

    HARDBUG(sqlite3_close(db) != SQLITE_OK)
  }

  my_mpi_barrier("after wal_checkpoint",
                 my_mpi_globals.MY_MPIG_comm_slaves, TRUE);
}

void walk_book(char *arg_db_name)
{
  sqlite3 *db;

  HARDBUG(!fexists(arg_db_name))

  int rc = sqlite3_open(arg_db_name, &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s err_msg=%s\n",
           arg_db_name, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  //we start with the starting position

  board_t board;

  construct_board(&board, STDOUT, FALSE);

  board_t *with = &board;

  string2board(with, STARTING_POSITION, TRUE);

  BSTRING(bempty)

  help_walk_book(db, with, 0, bempty);

  BDESTROY(bempty)

  HARDBUG(sqlite3_close(db) != SQLITE_OK)
}

