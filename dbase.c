//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#define NSQL_BUFFER (MBYTE)
#undef SPLIT_SQL 

int execute_sql(sqlite3 *arg_db, const void *arg_sql, int arg_step, int arg_nretries)
{
  static my_random_t retry_random;
  static int retry_random_init = FALSE;

  BSTRING(bsql)

  if (arg_step)
    HARDBUG(bassigncstr(bsql, sqlite3_sql((sqlite3_stmt *)arg_sql)) == BSTR_ERR)
  else
    HARDBUG(bassigncstr(bsql, (char *) arg_sql) == BSTR_ERR)

  if (!retry_random_init)
  {
    construct_my_random(&retry_random, 0);

    retry_random_init = TRUE;
  }

  double delay = 0.0;

  double backoff = 0.0;

  int rc = SQLITE_OK;

  char *err_msg = NULL;

  int iretry = 0;

  while((arg_nretries == INVALID) or (iretry <= arg_nretries))
  {
    if (arg_step)
      rc = sqlite3_step((sqlite3_stmt *) arg_sql);
    else
      rc = sqlite3_exec(arg_db, (const char *) arg_sql, 0, 0, &err_msg);

    if ((rc != SQLITE_BUSY) and (rc != SQLITE_LOCKED)) break;

    if (arg_step)
    {
      PRINTF("busy or locked step=%s iretry=%d delay=%.6f\n", 
             bdata(bsql), iretry, delay);
    }
    else
    {
      sqlite3_free(err_msg);

      err_msg = NULL;

      PRINTF("busy or locked sql=%s iretry=%d delay=%.6f\n", 
             bdata(bsql), iretry, delay);
    }

    if (arg_nretries > 1)
    {
      if ((iretry % arg_nretries) == 0)
      {
        delay = (1.0 + return_my_random(&retry_random) % arg_nretries) /
                arg_nretries;

        backoff = pow(1.0 / delay, 1.0 / arg_nretries);

        delay *= CENTI_SECOND;
      }
      else
      {
        delay *= backoff;
      }

      PRINTF("delay=%.6f\n", delay);

      compat_sleep(delay);
    }

    ++iretry;
  }

  if ((rc == SQLITE_OK) or (rc == SQLITE_DONE) or (rc == SQLITE_ROW))
  {
    if (iretry > 0)
    {
      if (arg_step)
      {
        PRINTF("OK or DONE or ROW step=%s iretry=%d delay=%.2f\n", 
               bdata(bsql), iretry, delay);
      }
      else
      {
        PRINTF("OK or DONE or ROW sql=%s iretry=%d delay=%.2f\n", 
               bdata(bsql), iretry, delay);
      }
    }

    BDESTROY(bsql);

    return(rc);
  }

  if (arg_step)
  {
    PRINTF("FAILED step=%s rc=%d err_msg=%s\n",
           bdata(bsql), rc, sqlite3_errmsg(arg_db));
  }
  else
  {
    PRINTF("FAILED sql=%s rc=%d err_msg=%s\n",
           bdata(bsql), rc, err_msg);
  }

  FATAL("sqlite3", EXIT_FAILURE)

  return(INVALID);
}

void create_tables(sqlite3 *arg_db, int arg_nretries)
{
  const char *create_positions_table_sql =
    "CREATE TABLE IF NOT EXISTS positions"
    " (id INTEGER PRIMARY KEY AUTOINCREMENT, position TEXT UNIQUE);";

  const char *create_moves_table_sql =
    "CREATE TABLE IF NOT EXISTS moves (id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " position_id INTEGER,"
    " move TEXT,"
    " nwon INTEGER DEFAULT 0,"
    " ndraw INTEGER DEFAULT 0,"
    " nlost INTEGER DEFAULT 0,"
    " FOREIGN KEY (position_id) REFERENCES positions (id),"
    " UNIQUE (position_id, move));";

  const char *create_evaluations_table_sql =
    "CREATE TABLE IF NOT EXISTS evaluations ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " move_id INTEGER,"
    " eval_time INTEGER,"
    " evaluation INTEGER,"
    " FOREIGN KEY (move_id) REFERENCES moves (id),"
    " UNIQUE (move_id, eval_time));";

  HARDBUG(execute_sql(arg_db, create_positions_table_sql, FALSE, arg_nretries) !=
          SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_moves_table_sql, FALSE, arg_nretries) !=
          SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_evaluations_table_sql, FALSE, arg_nretries) !=
          SQLITE_OK)
}

int query_position(sqlite3 *arg_db, const char *arg_position, int arg_nretries)
{
  const char *sql = "SELECT id FROM positions WHERE position = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_text(stmt, 1, arg_position, -1, SQLITE_STATIC) != SQLITE_OK)

  if ((rc = execute_sql(arg_db, stmt, TRUE, arg_nretries)) == SQLITE_DONE)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(INVALID);
  }

  HARDBUG(rc != SQLITE_ROW)

  int id = sqlite3_column_int(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return(id);
}

int insert_position(sqlite3 *arg_db, const char *arg_position, int arg_nreties)
{
  const char *sql =
    "INSERT OR IGNORE INTO positions (position) VALUES (?);";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_text(stmt, 1, arg_position, -1, SQLITE_STATIC) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, stmt, TRUE, arg_nreties) != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  int id = query_position(arg_db, arg_position, arg_nreties);

  HARDBUG(id == INVALID)

  return(id);
}

int query_move(sqlite3 *arg_db, int arg_position_id, const char *arg_move, int arg_nretries)
{
  const char *sql =
    "SELECT id FROM moves WHERE position_id = ? and move = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_int(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_text(stmt, 2, arg_move, -1, SQLITE_STATIC) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE, arg_nretries) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(INVALID);
  }

  int id = sqlite3_column_int(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return(id);
}

int insert_move(sqlite3 *arg_db, int arg_position_id, const char *arg_move, int arg_nretries)
{
  const char *sql =
   "INSERT INTO moves"
   " (position_id, move, nwon, ndraw, nlost)"
   " VALUES (?, ?, 0, 0, 0);";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_int(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_text(stmt, 2, arg_move, -1, SQLITE_STATIC) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, stmt, TRUE, arg_nretries) != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  int id = query_move(arg_db, arg_position_id, arg_move, arg_nretries);

  HARDBUG(id == INVALID)

  return(id);
}

int query_evaluation(sqlite3 *arg_db, int arg_move_id, int arg_eval_time, int arg_nretries)
{
  const char *sql =
   "SELECT evaluation FROM evaluations WHERE move_id = ? and eval_time = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_int(stmt, 1, arg_move_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_int(stmt, 2, arg_eval_time) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE, arg_nretries) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(SCORE_PLUS_INFINITY);
  }

  int evaluation = sqlite3_column_int(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return(evaluation);
}

void increment_nwon_ndraw_nlost(sqlite3 *arg_db, int arg_position_id,
  int arg_id, int arg_nwon, int arg_ndraw, int arg_nlost, int arg_nretries)
{
  const char *sql;

  sql =
    "UPDATE moves "
    "SET nwon = nwon + ?, "
    "    ndraw = ndraw + ?, "
    "    nlost = nlost + ? "
    "  WHERE position_id = ? AND id = ?;";
  
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
   }

   HARDBUG(sqlite3_bind_int(stmt, 1, arg_nwon) != SQLITE_OK)

   HARDBUG(sqlite3_bind_int(stmt, 2, arg_ndraw) != SQLITE_OK)

   HARDBUG(sqlite3_bind_int(stmt, 3, arg_nlost) != SQLITE_OK)

   HARDBUG(sqlite3_bind_int(stmt, 4, arg_position_id) != SQLITE_OK)

   HARDBUG(sqlite3_bind_int(stmt, 5, arg_id) != SQLITE_OK)

   HARDBUG(execute_sql(arg_db, stmt, TRUE, arg_nretries) != SQLITE_DONE)

   HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)
}

// Function to query moves for a given position

void query_moves(sqlite3 *arg_db, int arg_position_id, int arg_nretries)
{
  const char *sql =
   "SELECT move, nwon, ndraw, nlost from moves WHERE position_id = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s\n", sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  HARDBUG(sqlite3_bind_int(stmt, 1, arg_position_id) != SQLITE_OK)

  while ((rc = execute_sql(arg_db, stmt, TRUE, arg_nretries)) == SQLITE_ROW)
  {
    const unsigned char *move = sqlite3_column_text(stmt, 0);

    int nwon = sqlite3_column_int(stmt, 1);
    int ndraw = sqlite3_column_int(stmt, 2);
    int nlost = sqlite3_column_int(stmt, 3);

    PRINTF("move=%s nwon=%d ndraw=%d nlost=%d\n", 
      move, nwon, ndraw, nlost);
  }

  HARDBUG(rc != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)
}

void backup_db(sqlite3 *arg_in_memory_db, const char *arg_file_name)
{
  sqlite3 *disk_db;
  sqlite3_backup *backup;
  int rc;

  rc = sqlite3_open(arg_file_name, &disk_db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open disk database: %s err_mes=%s\n",
           arg_file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  backup = sqlite3_backup_init(disk_db, "main", arg_in_memory_db, "main");

  if (!backup)
  {
    PRINTF("Failed to initialize backup: %s err_msg=%s\n", 
           arg_file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_step(backup, -1);

  if ((rc != SQLITE_DONE) and (rc != SQLITE_OK))
  {
    PRINTF("Backup step failed: %s err_msg=%s\n",
           arg_file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_finish(backup);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup finish failed: %s err_msg=%s\n",
           arg_file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_errcode(disk_db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup failed: %s err_msg=%s\n",
           arg_file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_close(disk_db);
}

void wal_checkpoint(sqlite3 *arg_db)
{
  sqlite3_stmt *stmt;
  
  const char *sql = "PRAGMA wal_checkpoint(FULL);";

  int rc = sqlite3_prepare_v2(arg_db, sql, -1, &stmt, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(arg_db));
   
    FATAL("sqlite3", EXIT_FAILURE)
  }
  
  HARDBUG((rc = execute_sql(arg_db, stmt, TRUE, INVALID)) != SQLITE_ROW)
  
  int checkpoint_result = sqlite3_column_int(stmt, 0);
  int pages_checkpointed = sqlite3_column_int(stmt, 1);
  int pages_remaining = sqlite3_column_int(stmt, 2);
  
  PRINTF("checkpoint_result=%d\n", checkpoint_result);
  PRINTF("pages_checkpointed=%d\n", pages_checkpointed);
  PRINTF("pages_remaining=%d\n", pages_remaining);
  
  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)
}

void flush_sql_buffer(sql_buffer_t *self, int nsql_buffer)
{
  sql_buffer_t *object = self;

  if (blength(object->SB_bbuffer) > nsql_buffer)
  {
    my_mpi_acquire_semaphore(object->SB_comm, object->SB_win);

    HARDBUG(execute_sql(object->SB_db, "BEGIN TRANSACTION;", FALSE,
                        object->SB_nretries) != SQLITE_OK)

#ifdef SPLIT_SQL
    BSTRING(bsplit)

    bassigncstr(bsplit, ";");
  
    struct bstrList *bsql;
    
    HARDBUG((bsql = bsplits(object->SB_bbuffer, bsplit)) == NULL)

    for (int isql = 0; isql < bsql->qty; isql++)
    {
      HARDBUG(execute_sql(object->SB_db, bdata(bsql->entry[isql]),
                          FALSE, object->SB_nretries) != SQLITE_OK)
    }

    HARDBUG(bstrListDestroy(bsql) == BSTR_ERR)

    BDESTROY(bsplit)
#else
    HARDBUG(execute_sql(object->SB_db, bdata(object->SB_bbuffer),
                        FALSE, object->SB_nretries) != SQLITE_OK)
#endif

    HARDBUG(execute_sql(object->SB_db, "COMMIT;",
                        FALSE, object->SB_nretries) != SQLITE_OK)

    my_mpi_release_semaphore(object->SB_comm, object->SB_win);

    HARDBUG(bassigncstr(object->SB_bbuffer, "") == BSTR_ERR)
  }
}

void append_sql_buffer(sql_buffer_t *self, const char *arg_fmt, ...)
{
  sql_buffer_t *object = self;

  int ret;

  bvformata(ret, object->SB_bbuffer, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  flush_sql_buffer(object, NSQL_BUFFER);
}

void construct_sql_buffer(sql_buffer_t *self,
  sqlite3 *arg_db, MPI_Comm arg_comm, MPI_Win arg_win, int arg_nretries)
{
  sql_buffer_t *object = self;

  object->SB_db = arg_db;
  object->SB_comm = arg_comm;
  object->SB_win = arg_win;
  object->SB_nretries = arg_nretries;

  HARDBUG((object->SB_bbuffer = bfromcstr("")) == NULL)
}

#define NRETRIES 5
#define NTEST_POSITIONS 1000
#define NTEST_MOVES     100

void test_dbase(void)
{
  PRINTF("test_dbase\n");
  sqlite3 *db;

  remove("test_dbase.db");

  int rc = sqlite3_open("test_dbase.db", &db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open database: %s\n", sqlite3_errmsg(db));
    FATAL("sqlite3", EXIT_FAILURE)
  }

  create_tables(db, NRETRIES);

  // Insert position and moves

  const char *position = "initial_position";

  int position_id = query_position(db, position, 0);

  if (position_id == INVALID)
    position_id = insert_position(db, position, NRETRIES);

  int move_id = query_move(db, position_id, "move1", NRETRIES);

  if (move_id == INVALID)
    move_id = insert_move(db, position_id, "move1", NRETRIES);

  move_id = query_move(db, position_id, "move2", NRETRIES);

  if (move_id == INVALID)
    move_id = insert_move(db, position_id, "move2", NRETRIES);

  move_id = query_move(db, position_id, "move3", NRETRIES);

  if (move_id == INVALID)
    move_id = insert_move(db, position_id, "move3", NRETRIES);

  PRINTF("first query\n");

  query_moves(db, position_id, NRETRIES);

  HARDBUG((move_id = query_move(db, position_id, "move1", NRETRIES)) ==
          INVALID)

  increment_nwon_ndraw_nlost(db, position_id, move_id, 1, 0, 0, NRETRIES);

  HARDBUG((move_id = query_move(db, position_id, "move2", NRETRIES)) ==
          INVALID)

  increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 1, 0, NRETRIES);

  HARDBUG((move_id = query_move(db, position_id, "move3", NRETRIES)) ==
          INVALID)

  increment_nwon_ndraw_nlost(db, position_id, move_id, 0, 0, 1, NRETRIES);

  PRINTF("second query\n");

  query_moves(db, position_id, NRETRIES);

  PRINTF("multiple queries\n");

  sql_buffer_t sql_buffer;

  construct_sql_buffer(&sql_buffer, db, MPI_COMM_NULL, MPI_WIN_NULL, INVALID);

  for (int iposition = 0; iposition < NTEST_POSITIONS; iposition++)
  {
    ++position_id;

    append_sql_buffer(&sql_buffer,
                     "INSERT INTO positions (id, position)"
                     " VALUES (%d, 'position%d');",
                     position_id, position_id);
   
    for (int imove = 0; imove < NTEST_MOVES; imove++)
    {
      ++move_id;

      append_sql_buffer(&sql_buffer,
                        "INSERT INTO moves"
                        " (position_id, id, move, nwon, ndraw, nlost)"
                        " VALUES (%d, %d, 'move%d', %d, %d, %d);",
                        position_id,
                        move_id, move_id, move_id, move_id, move_id);
    }
  }

  flush_sql_buffer(&sql_buffer, 0);
  
  HARDBUG(sqlite3_close(db) != SQLITE_OK)

  PRINTF("test_dbase OK\n");
}

