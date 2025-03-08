//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#define NRETRIES 5

local my_random_t book_random;

local sqlite3 *book;

void init_book(void)
{
  construct_my_random(&book_random, INVALID);
}

local int help_walk_book_alpha_beta(sqlite3 *arg_db, board_t *object,
  int arg_depth, int arg_eval_time, int arg_alpha, int arg_beta)
{
  moves_list_t my_moves_list;

  construct_moves_list(&my_moves_list);

  gen_moves(object, &my_moves_list, FALSE);

  if (my_moves_list.nmoves == 0) return(SCORE_LOST + arg_depth);

  char *position = board2string(object, FALSE);

  int position_id = query_position(arg_db, position, NRETRIES);
 
  if (position_id == INVALID) return(SCORE_PLUS_INFINITY);

  int best_score = arg_alpha;

  for (int imove = 0; imove < my_moves_list.nmoves; imove++)
  {
    int temp_score = SCORE_MINUS_INFINITY;

    if (my_moves_list.nmoves == 1)
    {
      do_move(object, imove, &my_moves_list);
  
      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1, arg_eval_time,
                                              -arg_beta, -best_score);
    
      undo_move(object, imove, &my_moves_list);
    }
    else
    {
      BSTRING(bmove)

      move2bstring(&my_moves_list, imove, bmove);

      int move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove), NRETRIES);

      BDESTROY(bmove)

      if (move_id == INVALID) return(SCORE_PLUS_INFINITY);

      int evaluation = query_evaluation(arg_db, move_id, arg_eval_time, NRETRIES);

      do_move(object, imove, &my_moves_list);

      temp_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth + 1, arg_eval_time,
                                              -arg_beta, -best_score);

      undo_move(object, imove, &my_moves_list);

      if (temp_score == SCORE_MINUS_INFINITY) temp_score = evaluation;

    }
    if (temp_score > best_score) best_score = temp_score;
  }
  return(best_score);
}

local void help_walk_book(sqlite3 *arg_db, board_t *object, int arg_depth,
  int arg_eval_time, bstring arg_bpv)
{
  char *position = board2string(object, FALSE);

  int position_id = query_position(arg_db, position, NRETRIES);

  if (position_id == INVALID)
  {
    PRINTF("%s\n", bdata(arg_bpv));

    return;
  }

  moves_list_t my_moves_list;

  construct_moves_list(&my_moves_list);

  gen_moves(object, &my_moves_list, FALSE);

  if (my_moves_list.nmoves == 0) return;

  for (int imove = 0; imove < my_moves_list.nmoves; imove++)
  {
    BSTRING(bmove)

    move2bstring(&my_moves_list, imove, bmove);

    int evaluation = SCORE_PLUS_INFINITY;

    int best_score = SCORE_PLUS_INFINITY;

    if (my_moves_list.nmoves > 1)
    {
      int move_id;
  
      move_id = query_move(arg_db, position_id, bdata(bmove), NRETRIES);

      if (move_id != INVALID)
      {
         evaluation = query_evaluation(arg_db, move_id, arg_eval_time, NRETRIES);

         do_move(object, imove, &my_moves_list);

         best_score = -help_walk_book_alpha_beta(arg_db, object, arg_depth, arg_eval_time,
                                                 SCORE_MINUS_INFINITY,
                                                 SCORE_PLUS_INFINITY);

         undo_move(object, imove, &my_moves_list);
      }
    }

    BSTRING(bpv)

    HARDBUG(bformata(bpv, "%s%s(%d, %d) ",
                     bdata(arg_bpv), bdata(bmove),
                     evaluation, best_score) == BSTR_ERR)

    do_move(object, imove, &my_moves_list);

    help_walk_book(arg_db, object, arg_depth + 1, arg_eval_time, bpv);

    undo_move(object, imove, &my_moves_list);

    BDESTROY(bpv)

    BDESTROY(bmove)
  }
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

  sqlite3_finalize(stmt);

  sql = "DETACH DATABASE disk;";

  rc = sqlite3_exec(book, sql, 0, 0, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to exec statement: %s err_msg=%s\n",
           sql, sqlite3_errmsg(book));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_close(disk);

  if (TRUE)
  {
    board_t board;
    
    construct_board(&board, STDOUT, FALSE);

    board_t *with = &board;
  
    string2board(with, STARTING_POSITION, TRUE);
  
    BSTRING(bempty)

    help_walk_book(book, with, 0, 30, bempty);

    BDESTROY(bempty)
  }
}

void return_book_move(board_t *object, moves_list_t *arg_moves_list,
  bstring arg_bbook_move)
{
  int eval_time = 30;

  HARDBUG(bassigncstr(arg_bbook_move, "NULL") == BSTR_ERR)

  moves_list_t my_moves_list;

  construct_moves_list(&my_moves_list);

  gen_moves(object, &my_moves_list, FALSE);

  if (my_moves_list.nmoves == 0) return;

  if (my_moves_list.nmoves == 1)
  {
    move2bstring(arg_moves_list, 0, arg_bbook_move);

    return;
  }

  char *position = board2string(object, FALSE);

  int position_id = query_position(book, position, NRETRIES);

  if (position_id == INVALID) return;

  int sort[MOVES_MAX];

  for (int imove = 0; imove < arg_moves_list->nmoves; imove++)
    sort[imove] = imove;

  if (options.book_randomness > 1)
  {
    for (int imove = arg_moves_list->nmoves - 1; imove >= 1; --imove)
    {
      int jmove = return_my_random(&book_random) % (imove + 1);

      if (jmove != imove)
      {
        int temp = sort[imove];
        sort[imove] = sort[jmove];
        sort[jmove] = temp;
      }
    }

    for (int idebug = 0; idebug < arg_moves_list->nmoves; idebug++)
    {
      int ndebug = 0;

      for (int jdebug = 0; jdebug < arg_moves_list->nmoves; jdebug++)
        if (sort[jdebug] == idebug) ++ndebug;

      HARDBUG(ndebug != 1)
    }
  }

  int best_move = INVALID;

  int best_score = SCORE_MINUS_INFINITY;

  for (int imove = 0; imove < my_moves_list.nmoves; imove++)
  {
    int jmove = sort[imove];

    BSTRING(bmove)

    move2bstring(&my_moves_list, jmove, bmove);

    int evaluation = SCORE_PLUS_INFINITY;

    int temp_score = SCORE_PLUS_INFINITY;

    int move_id;
  
    move_id = query_move(book, position_id, bdata(bmove), NRETRIES);

    if (move_id == INVALID)
    {
      PRINTF("WARNING: no evaluation for move %s in positions %s!\n",
             bdata(bmove), position);

      //if we have no evaluation for one move
      //we do not have a best_score
      FATAL("YOU STILL HAVE TO THINK ABOUT THIS", EXIT_FAILURE)
      return;
    }

    evaluation = query_evaluation(book, move_id, eval_time, NRETRIES);

    HARDBUG(evaluation == SCORE_PLUS_INFINITY)

    do_move(object, jmove, &my_moves_list);

    temp_score = -help_walk_book_alpha_beta(book, object, 0, eval_time,
                                            SCORE_MINUS_INFINITY,
                                            SCORE_PLUS_INFINITY);

    undo_move(object, jmove, &my_moves_list);

    if (temp_score == SCORE_MINUS_INFINITY) temp_score = evaluation;

    int random_score = temp_score;

    if (options.book_randomness > 0)
      random_score = temp_score / options.book_randomness;

    PRINTF("move=%s evaluation=%d temp_score=%d random_score=%d\n",
      bdata(bmove), evaluation, temp_score, random_score);

    if (random_score > best_score)
    {
      best_move = jmove;

      best_score = temp_score;
    }
    BDESTROY(bmove)
  }

  HARDBUG(best_move == INVALID)

  HARDBUG(best_score == SCORE_MINUS_INFINITY)

  move2bstring(&my_moves_list, best_move, arg_bbook_move);

  PRINTF("book_move=%s best_score=%d\n", bdata(arg_bbook_move), best_score);
}

local int *semaphore;
local MPI_Win win;
local i64_t npositions;

local void drop_out(sqlite3 *arg_db, search_t *object, int arg_depth, int arg_depth_max,
  int arg_eval_time)
{
  if (arg_depth > arg_depth_max) return;

  PRINTF("checking position depth=%d depth_max=%d\n", arg_depth, arg_depth_max);

  //we query the position and add all moves to the database if needed

  char *position = board2string(&(object->S_board), FALSE);

  my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

  int position_id = query_position(arg_db, position, NRETRIES);

  if (position_id == INVALID)
    position_id = insert_position(arg_db, position, NRETRIES);

  my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

  moves_list_t my_moves_list;

  construct_moves_list(&my_moves_list);

  gen_moves(&(object->S_board), &my_moves_list, FALSE);

  //for each move we check if we have a result

  if (my_moves_list.nmoves > 1)
  {
    for (int imove = 0; imove < my_moves_list.nmoves; imove++)
    {
      if ((imove % my_mpi_globals.MY_MPIG_nslaves) !=
          my_mpi_globals.MY_MPIG_id_slave) continue;

      BSTRING(bmove)

      move2bstring(&my_moves_list, imove, bmove);
  
      my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

      
      int move_id = query_move(arg_db, position_id, bdata(bmove), NRETRIES);
  
      if (move_id == INVALID)
        move_id = insert_move(arg_db, position_id, bdata(bmove), NRETRIES);
  
      int evaluation = query_evaluation(arg_db, move_id, arg_eval_time, NRETRIES);

      my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);
  
      PRINTF("db move=%s eval_time=%d evaluation=%d\n",
        bdata(bmove), arg_eval_time, evaluation);
  
      if (evaluation == SCORE_PLUS_INFINITY)
      {
        //evaluate the position

        options.time_limit = arg_eval_time;
        options.time_ntrouble = 0;

        do_move(&(object->S_board), imove, &my_moves_list);

        moves_list_t your_moves_list;

        construct_moves_list(&your_moves_list);

        gen_moves(&(object->S_board), &your_moves_list, FALSE);

        if (your_moves_list.nmoves == 0)
        {
          evaluation = SCORE_WON;
        }
        else
        {
          do_search(object, &your_moves_list,
            INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

          evaluation = -object->S_best_score;
        }

        undo_move(&(object->S_board), imove, &my_moves_list);

        PRINTF("search move=%s eval_time=%d evaluation=%d\n",
          bdata(bmove), arg_eval_time, evaluation);

        //update the evaluation
  
        const char *sql =
         "INSERT INTO evaluations (move_id, eval_time, evaluation)"
         " VALUES (?, ?, ?);";

        sqlite3_stmt *stmt;
  
        int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);
  
        if (rc != SQLITE_OK)
        {
          PRINTF("Failed to prepare update statement: %s\n",
                 sqlite3_errmsg(arg_db));
  
          FATAL("sqlite3", EXIT_FAILURE);
        }
  
        sqlite3_bind_int(stmt, 1, move_id);

        sqlite3_bind_int(stmt, 2, arg_eval_time);
  
        sqlite3_bind_int(stmt, 3, evaluation);
    
        my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

        HARDBUG(execute_sql(arg_db, stmt, TRUE, NRETRIES) != SQLITE_DONE)

        my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

        sqlite3_finalize(stmt);
      }
      BDESTROY(bmove)
    }
    my_mpi_barrier("after imove", my_mpi_globals.MY_MPIG_comm_slaves, FALSE);
  }

  //expand all moves with an evaluation >= 0

  int depth_next = arg_depth + 1;

  int nexpand = 0;

  for (int imove = 0; imove < my_moves_list.nmoves; imove++)
  {
    BSTRING(bmove)

    move2bstring(&my_moves_list, imove, bmove);

    int evaluation = SCORE_PLUS_INFINITY;

    if (my_moves_list.nmoves > 1)
    {
      int move_id;
  
      my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

      HARDBUG((move_id = query_move(arg_db, position_id, bdata(bmove),
                                    NRETRIES)) == INVALID)

      evaluation = query_evaluation(arg_db, move_id, arg_eval_time, NRETRIES);

      my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_slaves, win);

      HARDBUG(evaluation == SCORE_PLUS_INFINITY)
    }

    if ((arg_depth <= 2) or (evaluation >= 0))
    {
      PRINTF("expanding depth=%d depth_next=%d depth_max=%d"
             " move=%s evaluation=%d\n",
             arg_depth, depth_next, arg_depth_max, bdata(bmove), evaluation);

      do_move(&(object->S_board), imove, &my_moves_list);

      ++nexpand;

      ++npositions;

      drop_out(arg_db, object, depth_next, arg_depth_max, arg_eval_time);

      undo_move(&(object->S_board), imove, &my_moves_list); 
    }
    BDESTROY(bmove)
  }
  if (nexpand == 0) PRINTF("nothing to expand\n");
}

void gen_book(int arg_eval_time, int arg_depth_limit)
{
  ui64_t seed = return_my_random(&book_random);

  PRINTF("seed=%llX\n", seed);

  sqlite3 *db;

  int rc = sqlite3_open("gen.db", &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: gen.db err_msg=%s\n",
           sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  if (my_mpi_globals.MY_MPIG_id_slave == 0) create_tables(db, NRETRIES);

  my_mpi_barrier("after create_tables", my_mpi_globals.MY_MPIG_comm_slaves,
                 TRUE);

  my_mpi_win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MY_MPIG_comm_slaves, &semaphore, &win);

  if (my_mpi_globals.MY_MPIG_id_slave == 0) *semaphore = 0;

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_slaves, 0, win);

  //we start with the starting position

  search_t search;

  construct_search(&search, STDOUT, NULL);

  search_t *with = &search;

  string2board(&(with->S_board), STARTING_POSITION, TRUE);

  //query the position and the moves

  for (int depth_max = 1; depth_max <= arg_depth_limit; ++depth_max)
  {
    npositions = 0;

    drop_out(db, with, 0, depth_max, arg_eval_time);

    PRINTF("depth_max=%d npositions=%lld\n", depth_max, npositions);
  }
}


void walk_book(int arg_eval_time)
{
  sqlite3 *db;

  int rc = sqlite3_open("gen.db", &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: gen.db err_msg=%s\n",
           sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  //we start with the starting position

  board_t board;

  construct_board(&board, STDOUT, FALSE);

  board_t *with = &board;

  string2board(with, STARTING_POSITION, TRUE);

  BSTRING(bempty)

  help_walk_book(db, with, 0, arg_eval_time, bempty);

  BDESTROY(bempty)
}

local void help_count_book(sqlite3 *arg_db, board_t *object, int arg_depth,
  int arg_depth_max)
{
  if (arg_depth > arg_depth_max) return;

  char *position = board2string(object, FALSE);

  int position_id = query_position(arg_db, position, NRETRIES);

  if (position_id == INVALID)
    position_id = insert_position(arg_db, position, NRETRIES);

  moves_list_t my_moves_list;

  construct_moves_list(&my_moves_list);

  gen_moves(object, &my_moves_list, FALSE);

  //for each move we check if we have a result

  if (my_moves_list.nmoves > 1)
  {
    for (int imove = 0; imove < my_moves_list.nmoves; imove++)
    {
      BSTRING(bmove)

      move2bstring(&my_moves_list, imove, bmove);
  
      int move_id = query_move(arg_db, position_id, bdata(bmove), NRETRIES);
  
      if (move_id == INVALID)
      {
        move_id = insert_move(arg_db, position_id, bdata(bmove), NRETRIES);

        npositions++;
      }

      BDESTROY(bmove)
    }
  }

  //expand all moves

  for (int imove = 0; imove < my_moves_list.nmoves; imove++)
  {
    do_move(object, imove, &my_moves_list);

    if (arg_depth <= 3) help_count_book(arg_db, object, arg_depth + 1, arg_depth_max);

    undo_move(object, imove, &my_moves_list); 
  }
}

void count_book(int arg_depth_limit)
{
  sqlite3 *db;

  int rc = sqlite3_open("gen.db", &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: gen.db err_msg=%s\n",
           sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  create_tables(db, NRETRIES);

  //we start with the starting position

  board_t board;

  construct_board(&board, STDOUT, FALSE);

  board_t *with = &board;

  string2board(with, STARTING_POSITION, TRUE);

  //query the position and the moves

  for (int depth_max = 1; depth_max <= arg_depth_limit; ++depth_max)
  {
    npositions = 0;

    help_count_book(db, with, 0, depth_max);

    PRINTF("depth_max=%d npositions=%lld\n", depth_max, npositions);
  }
}

