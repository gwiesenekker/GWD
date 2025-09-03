//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define NRETRIES 1000000

#define NTRIES_MAX 10

#define NSQL_BUFFER (MBYTE)
#define SPLIT_SQL_BUFFER

#define SQL_PRINTF_WIDTH 1024

const char *my_sqlite3_errmsg(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  return(sqlite3_errmsg(arg_db));
#else
  return(NULL);
#endif
}

int my_sqlite3_open(const char *arg_db_name, sqlite3 **arg_db)
{
#ifdef USE_SQLITE3
  return(sqlite3_open(arg_db_name, arg_db));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_open_v2(const char *arg_db_name, sqlite3 **arg_db, int flags,
  const char *arg_zvfs)
{
#ifdef USE_SQLITE3
  return(sqlite3_open_v2(arg_db_name, arg_db, flags, arg_zvfs));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_bind_int(sqlite3_stmt *arg_stmt, int arg_index, 
  int arg_value)
{
#ifdef USE_SQLITE3
  return(sqlite3_bind_int(arg_stmt, arg_index, arg_value));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_bind_int64(sqlite3_stmt *arg_stmt, int arg_index,
  i64_t arg_value)
{
#ifdef USE_SQLITE3
  return(sqlite3_bind_int64(arg_stmt, arg_index, arg_value));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_step(sqlite3_stmt *arg_stmt)
{
#ifdef USE_SQLITE3
  return(sqlite3_step(arg_stmt));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_exec(sqlite3 *arg_db, const char *arg_sql,
  int (*arg_callback)(void*,int,char**,char**), void *arg_void,
  char **arg_errmsg)
{
#ifdef USE_SQLITE3
  return(sqlite3_exec(arg_db, arg_sql, arg_callback, arg_void,
         arg_errmsg));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_column_int(sqlite3_stmt *arg_stmt, int arg_icol)
{
#ifdef USE_SQLITE3
  return(sqlite3_column_int(arg_stmt, arg_icol));
#else
  return(INVALID);
#endif
}

i64_t my_sqlite3_column_int64(sqlite3_stmt *arg_stmt, int arg_icol)
{
#ifdef USE_SQLITE3
  return(sqlite3_column_int64(arg_stmt, arg_icol));
#else
  return(INVALID);
#endif
}

const unsigned char *my_sqlite3_column_text(sqlite3_stmt *arg_stmt,
  int arg_icol)
{
#ifdef USE_SQLITE3
  return(sqlite3_column_text(arg_stmt, arg_icol));
#else
  return(NULL);
#endif
}

int my_sqlite3_finalize(sqlite3_stmt *arg_stmt)
{
#ifdef USE_SQLITE3
  return(sqlite3_finalize(arg_stmt));
#else
  return(SQLITE_OK);
#endif
}

int my_sqlite3_close(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  return(sqlite3_close(arg_db));
#else
  return(SQLITE_OK);
#endif
}

void my_sqlite3_prepare_v2(sqlite3 *arg_db, const char *arg_sql,
  sqlite3_stmt **arg_stmt)
{
#ifdef USE_SQLITE3
  int rc = sqlite3_prepare_v2(arg_db, arg_sql, -1, arg_stmt, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement sql=%s rc=%d err_msg=%s\n",
           arg_sql, rc, sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }
#endif
}

int execute_sql(sqlite3 *arg_db, const void *arg_sql, int arg_step)
{
#ifdef USE_SQLITE3
  static my_random_t retry_random;
  static int retry_random_init = FALSE;

  BSTRING(bsql)

  int nsemicolons = 0;

  if (arg_step)
    HARDBUG(bassigncstr(bsql, sqlite3_sql((sqlite3_stmt *)arg_sql)) == BSTR_ERR)
  else
  {
    HARDBUG(bassigncstr(bsql, (char *) arg_sql) == BSTR_ERR)
    
    for (int ichar = 0; ichar < blength(bsql); ichar++)
      if (bchar(bsql, ichar) == ';') nsemicolons++;
  }

  if (!retry_random_init)
  {
    construct_my_random(&retry_random, 0);

    retry_random_init = TRUE;
  }

  double delay = 0.0;

  int rc = SQLITE_OK;

  char *err_msg = NULL;

  int iretry = 0;

  while(iretry <= NRETRIES)
  {
    if (arg_step)
      rc = sqlite3_step((sqlite3_stmt *) arg_sql);
    else
      rc = sqlite3_exec(arg_db, (const char *) arg_sql, 0, 0, &err_msg);

    if ((rc != SQLITE_BUSY) and (rc != SQLITE_LOCKED)) break;

    if (arg_step)
    {
      if ((iretry % 1000) == 0)
      {
        PRINTF("busy or locked step=%s iretry=%d delay=%.6f\n", 
               bdata(bsql), iretry, delay);
      }
    }
    else
    {
      sqlite3_free(err_msg);

      err_msg = NULL;

      if ((iretry % 1000) == 0)
      {
        if (blength(bsql) <= (2 * SQL_PRINTF_WIDTH))
        {
          PRINTF("busy or locked sql=%s iretry=%d delay=%.6f\n", 
                 bdata(bsql), iretry, delay);
        }
        else
        {
          PRINTF("busy or locked sql=%.*s...%.*s iretry=%d delay=%.6f\n", 
                 SQL_PRINTF_WIDTH,
                 bdata(bsql),
                 SQL_PRINTF_WIDTH,
                 bdata(bsql) + blength(bsql) - SQL_PRINTF_WIDTH,
                 iretry, delay);
        }
      }
    }

    if ((nsemicolons > 1) or (iretry >= 1000))
      delay = (1.0 + return_my_random(&retry_random) % 1000) * MILLI_SECOND;
    else
      delay = (1.0 + return_my_random(&retry_random) % 1000) * MICRO_SECOND;

    if ((iretry % 1000) == 0) PRINTF("delay=%.6f\n", delay);

    compat_sleep(delay);

    ++iretry;
  }

  if ((rc == SQLITE_OK) or (rc == SQLITE_DONE) or (rc == SQLITE_ROW))
  {
    if (iretry > 0)
    {
      if (arg_step)
      {
        PRINTF("OK or DONE or ROW step=%s iretry=%d delay=%.6f\n", 
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
    PRINTF("FAILED step=%s rc=%d\n", bdata(bsql), rc);
  }
  else
  {
    if (blength(bsql) <= (2 * SQL_PRINTF_WIDTH))
    {
      PRINTF("FAILED sql=%s rc=%d err_msg=%s iretry=%d delay=%.6f\n", 
             bdata(bsql), rc, sqlite3_errmsg(arg_db), iretry, delay);
    }
    else
    {
      PRINTF("FAILED sql=%.*s...%.*s rc=%d err_msg=%s iretry=%d delay=%.6f\n", 
             SQL_PRINTF_WIDTH,
             bdata(bsql),
             SQL_PRINTF_WIDTH,
             bdata(bsql) + blength(bsql) - SQL_PRINTF_WIDTH,
             rc, sqlite3_errmsg(arg_db), iretry, delay);
    }
  }

  FATAL("sqlite3", EXIT_FAILURE)

  return(INVALID);
#else
  return(INVALID);
#endif
}

void create_tables(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  const char *create_positions_table_sql =
    "CREATE TABLE IF NOT EXISTS positions"
    " (id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " fen TEXT UNIQUE,"
    " frequency INTEGER DEFAULT 0,"
    " nwon INTEGER DEFAULT 0,"
    " ndraw INTEGER DEFAULT 0,"
    " nlost INTEGER DEFAULT 0);";

  const char *create_moves_table_sql =
    "CREATE TABLE IF NOT EXISTS moves (id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " position_id INTEGER,"
    " move TEXT,"
    " centi_seconds INTEGER DEFAULT 1,"
    " FOREIGN KEY (position_id) REFERENCES positions (id),"
    " UNIQUE (position_id, move));";

  const char *create_evaluations_table_sql =
    "CREATE TABLE IF NOT EXISTS evaluations ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " move_id INTEGER,"
    " centi_seconds INTEGER,"
    " evaluation INTEGER,"
    " FOREIGN KEY (move_id) REFERENCES moves (id),"
    " UNIQUE (move_id, centi_seconds));";

  HARDBUG(execute_sql(arg_db, create_positions_table_sql, FALSE) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_moves_table_sql, FALSE) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_evaluations_table_sql, FALSE) !=
          SQLITE_OK)
#endif
}

i64_t query_position(sqlite3 *arg_db, const char *arg_fen, int *arg_nmoves)
{
#ifdef USE_SQLITE3
  const char *sql = "SELECT id FROM positions WHERE fen = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_text(stmt, 1, arg_fen, -1, SQLITE_STATIC) != SQLITE_OK)

  int rc;

  if ((rc = execute_sql(arg_db, stmt, TRUE)) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(INVALID);
  }

  i64_t id = sqlite3_column_int64(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  if (arg_nmoves != NULL)
  {
    sql = "SELECT COUNT(*) FROM moves WHERE position_id = ?;";

    my_sqlite3_prepare_v2(arg_db, sql, &stmt);
  
    HARDBUG(sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK)
  
    if ((rc = execute_sql(arg_db, stmt, TRUE)) != SQLITE_ROW)
      *arg_nmoves = 0;
    else
      *arg_nmoves = sqlite3_column_int64(stmt, 0);
  
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)
  }

  return(id);
#else
  return(INVALID);
#endif
}

i64_t insert_position(sqlite3 *arg_db, const char *arg_fen)
{
#ifdef USE_SQLITE3
  const char *sql =
    "INSERT OR IGNORE INTO positions (fen) VALUES (?);";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_text(stmt, 1, arg_fen, -1, SQLITE_STATIC) !=
          SQLITE_OK)

  HARDBUG(execute_sql(arg_db, stmt, TRUE) != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  i64_t id = query_position(arg_db, arg_fen, NULL);

  HARDBUG(id == INVALID)

  return(id);
#else
  return(INVALID);
#endif
}

i64_t query_move(sqlite3 *arg_db, i64_t arg_position_id, const char *arg_move)
{
#ifdef USE_SQLITE3
  const char *sql =
    "SELECT id FROM moves WHERE position_id = ? AND move = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_text(stmt, 2, arg_move, -1, SQLITE_STATIC) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(INVALID);
  }

  i64_t id = sqlite3_column_int64(stmt, 0);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return(id);
#else
  return(INVALID);
#endif
}

i64_t insert_move(sqlite3 *arg_db, i64_t arg_position_id, const char *arg_move,
                  int arg_centi_seconds)
{
#ifdef USE_SQLITE3
  const char *sql =
    "INSERT INTO moves (position_id, move, centi_seconds)"
    " VALUES (?, ?, ?)"
    " ON CONFLICT(position_id, move) DO UPDATE SET"
    "  centi_seconds = excluded.centi_seconds" 
    " WHERE excluded.centi_seconds > moves.centi_seconds;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_text(stmt, 2, arg_move, -1, SQLITE_STATIC) != SQLITE_OK)

  HARDBUG(sqlite3_bind_int(stmt, 3, arg_centi_seconds) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, stmt, TRUE) != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  i64_t id = query_move(arg_db, arg_position_id, arg_move);

  HARDBUG(id == INVALID)

  return(id);
#else
  return(INVALID);
#endif
}

int query_evaluation(sqlite3 *arg_db,
  i64_t arg_move_id, int *arg_centi_seconds)
{
#ifdef USE_SQLITE3
  const char *sql =
    "SELECT centi_seconds, evaluation FROM evaluations"
    " WHERE move_id = ? AND centi_seconds = ("
    "  SELECT MAX(centi_seconds)"
    "  FROM evaluations"
    "  WHERE move_id = ?"
    " );";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_move_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_int64(stmt, 2, arg_move_id) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return(SCORE_PLUS_INFINITY);
  }

  *arg_centi_seconds = sqlite3_column_int(stmt, 0);

  int evaluation = sqlite3_column_int(stmt, 1);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return(evaluation);
#else
  return(INVALID);
#endif
}

// Function to query moves for a given position

void query_moves(sqlite3 *arg_db, i64_t arg_position_id)
{
#ifdef USE_SQLITE3
  const char *sql =
   "SELECT move from moves WHERE position_id = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  int rc;

  while ((rc = execute_sql(arg_db, stmt, TRUE)) == SQLITE_ROW)
  {
    const unsigned char *move = sqlite3_column_text(stmt, 0);

    PRINTF("move=%s\n", move);
  }

  HARDBUG(rc != SQLITE_DONE)

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)
#endif
}

void backup_db(sqlite3 *arg_in_memory_db, const char *arg_file_name)
{
#ifdef USE_SQLITE3
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
#endif
}

void flush_sql_buffer(my_sqlite3_t *self, int nsql_buffer)
{
  my_sqlite3_t *object = self;

  if (blength(object->MS_bbuffer) > nsql_buffer)
  {
    PRINTF("blength(object->MS_bbuffer)=%d\n$", blength(object->MS_bbuffer));

    if (object->MS_comm != MPI_COMM_NULL)
    {
      PRINTF("acquiring semaphore..\n$");

      my_mpi_acquire_semaphore_v3(object->MS_comm, SEMKEY_FLUSH_SQL_BUFFER);
    }

    PRINTF("executing BEGIN TRANSACTION..\n$");

    HARDBUG(execute_sql(object->MS_db, "BEGIN TRANSACTION;", FALSE) !=
            SQLITE_OK)

#ifdef SPLIT_SQL_BUFFER
    BSTRING(bsplit)

    bassigncstr(bsplit, ";");
  
    struct bstrList *bsql;
    
    HARDBUG((bsql = bsplits(object->MS_bbuffer, bsplit)) == NULL)

    for (int isql = 0; isql < bsql->qty; isql++)
    {
      HARDBUG(execute_sql(object->MS_db, bdata(bsql->entry[isql]),
                          FALSE) != SQLITE_OK)
    }

    HARDBUG(bstrListDestroy(bsql) == BSTR_ERR)

    BDESTROY(bsplit)
#else
    PRINTF("executing SQL\n$");

    HARDBUG(execute_sql(object->MS_db, bdata(object->MS_bbuffer), FALSE) !=
            SQLITE_OK)
#endif

    PRINTF("executing COMMIT..\n$");

    HARDBUG(execute_sql(object->MS_db, "COMMIT;", FALSE) != SQLITE_OK)

    if (object->MS_comm != MPI_COMM_NULL)
    {
      PRINTF("releasing semaphore..\n$");

      my_mpi_release_semaphore_v3(object->MS_comm, SEMKEY_FLUSH_SQL_BUFFER);
    }

    PRINTF("done\n$");

    HARDBUG(bassigncstr(object->MS_bbuffer, "") == BSTR_ERR)
  }
}

void append_sql_buffer(my_sqlite3_t *self, const char *arg_fmt, ...)
{
  my_sqlite3_t *object = self;

  int ret;

  bvformata(ret, object->MS_bbuffer, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  flush_sql_buffer(object, NSQL_BUFFER);
}

void construct_my_sqlite3(my_sqlite3_t *self, char *arg_db_name,
  MPI_Comm arg_comm)
{
  my_sqlite3_t *object = self;

  object->MS_comm = arg_comm;

  int iproc = INVALID;

  if (object->MS_comm != MPI_COMM_NULL)
    my_mpi_comm_rank(object->MS_comm, &iproc);

  if (iproc <= 0)
  {
    int rc = my_sqlite3_open(arg_db_name, &(object->MS_db));
  
    if (rc != SQLITE_OK)
    {
      if (object->MS_db == NULL)
      {
        PRINTF("Cannot open database: %s err_msg=NULL\n", arg_db_name);
      }
      else
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
               arg_db_name, my_sqlite3_errmsg(object->MS_db));
      }
  
      FATAL("sqlite3", EXIT_FAILURE)
    }

    create_tables(object->MS_db);

    HARDBUG(execute_sql(object->MS_db, "PRAGMA journal_mode = WAL;", FALSE) !=
            SQLITE_OK)

    HARDBUG(execute_sql(object->MS_db, "PRAGMA cache_size=-131072;", FALSE) !=
            SQLITE_OK)
  }

  my_mpi_barrier_v3("after open database", arg_comm,
                    SEMKEY_CONSTRUCT_MY_SQLITE3_BARRIER, TRUE);

  if (iproc > 0)
  {
    int rc = my_sqlite3_open(arg_db_name, &(object->MS_db));
  
    if (rc != SQLITE_OK)
    {
      if (object->MS_db == NULL)   
      {
        PRINTF("Cannot open database: %s err_msg=NULL\n", arg_db_name);
      }
      else
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
                arg_db_name, my_sqlite3_errmsg(object->MS_db));
      }
      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(execute_sql(object->MS_db, "PRAGMA cache_size=-131072;", FALSE) !=
            SQLITE_OK)
  }
  
  HARDBUG((object->MS_bbuffer = bfromcstr("")) == NULL)
}

void close_my_sqlite3(my_sqlite3_t *self)
{
#ifdef USE_SQLITE3
  my_sqlite3_t *object = self;

  int iproc = INVALID;

  if (object->MS_comm != MPI_COMM_NULL)
    my_mpi_comm_rank(object->MS_comm, &iproc);

  if (iproc > 0)
  {
    PRINTF("%s::closing db iproc=%d\n$", __FUNC__, iproc);

    my_mpi_acquire_semaphore_v3(object->MS_comm, SEMKEY_CLOSE_MY_SQLITE3);

    HARDBUG(my_sqlite3_close(object->MS_db) != SQLITE_OK)

    my_mpi_release_semaphore_v3(object->MS_comm, SEMKEY_CLOSE_MY_SQLITE3);

    PRINTF("%s::closed db iproc=%d\n$", __FUNC__, iproc);
  }

  my_mpi_barrier_v3("after my_sqlite3_close", object->MS_comm,
                    SEMKEY_CLOSE_MY_SQLITE3_BARRIER, TRUE);

  if (iproc <= 0)
  {
    int ntries = 0;

    while(TRUE)
    {
      int frames_in_wal;
      int frames_checkpointed;
  
      int rc = sqlite3_wal_checkpoint_v2(object->MS_db,
                                         NULL,
                                         SQLITE_CHECKPOINT_TRUNCATE,
                                         &frames_in_wal,
                                         &frames_checkpointed);
      if (rc != SQLITE_OK)
      {
        PRINTF("Failed wal_checkpoint_v2: %s\n", sqlite3_errmsg(object->MS_db));
    
        FATAL("sqlite3", EXIT_FAILURE);
      }
    
      PRINTF("ntries=%d frames_in_wal=%d frames_checkpointed=%d\n$",
             ntries, frames_in_wal, frames_checkpointed);
  
      if (frames_in_wal <= 0) break;
  
      ++ntries;

      HARDBUG(ntries > NTRIES_MAX)
    }

    if (ntries > 0) PRINTF("needed %d tries to flush wal\n$", ntries);

    HARDBUG(sqlite3_close(object->MS_db) != SQLITE_OK)
  }
#endif
}

#define TEST_DB "test.db"

#define NTEST_POSITIONS 1000
#define NTEST_MOVES     100

void test_dbase(void)
{
  PRINTF("test_dbase\n");

  my_sqlite3_t db;

  remove(TEST_DB);

  construct_my_sqlite3(&db, TEST_DB, MPI_COMM_NULL);

  create_tables(db.MS_db);

  // Insert position and moves

  const char *position = "initial_position";

  i64_t position_id = query_position(db.MS_db, position, NULL);

  if (position_id == INVALID)
    position_id = insert_position(db.MS_db, position);

  i64_t move_id = query_move(db.MS_db, position_id, "move1");

  if (move_id == INVALID)
    move_id = insert_move(db.MS_db, position_id, "move1", 1);

  move_id = query_move(db.MS_db, position_id, "move2");

  if (move_id == INVALID)
    move_id = insert_move(db.MS_db, position_id, "move2", 2);

  move_id = query_move(db.MS_db, position_id, "move3");

  if (move_id == INVALID)
    move_id = insert_move(db.MS_db, position_id, "move3", 3);

  PRINTF("first query\n");

  query_moves(db.MS_db, position_id);

  HARDBUG((move_id = query_move(db.MS_db, position_id, "move1")) == INVALID)

  HARDBUG((move_id = query_move(db.MS_db, position_id, "move2")) == INVALID)

  HARDBUG((move_id = query_move(db.MS_db, position_id, "move3")) == INVALID)

  PRINTF("second query\n");

  query_moves(db.MS_db, position_id);

  PRINTF("multiple queries\n");

  for (i64_t iposition = 0; iposition < NTEST_POSITIONS; iposition++)
  {
    ++position_id;

    append_sql_buffer(&db,
                     "INSERT INTO positions (id, fen)"
                     " VALUES (%d, 'position%d');",
                     position_id, position_id);
   
    for (i64_t imove = 0; imove < NTEST_MOVES; imove++)
    {
      ++move_id;

      append_sql_buffer(&db,
                        "INSERT INTO moves"
                        " (position_id, id, move)"
                        " VALUES (%d, %d, 'move%d');",
                        position_id,
                        move_id, move_id);
    }
  }

  flush_sql_buffer(&db, 0);
  
  HARDBUG(my_sqlite3_close(db.MS_db) != SQLITE_OK)

  PRINTF("test_dbase OK\n");
}

