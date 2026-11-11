// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#include "globals.h"

#define NRETRIES 1000000

#define NTRIES_MAX 10

#define NSQL_BUFFER (MBYTE)
#define SPLIT_SQL_BUFFER

#define SQL_PRINTF_WIDTH 1024

const char *my_sqlite3_errmsg(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  return (sqlite3_errmsg(arg_db));
#else
  return (NULL);
#endif
}

int my_sqlite3_open(const char *arg_db_name, sqlite3 **arg_db)
{
#ifdef USE_SQLITE3
  return (sqlite3_open(arg_db_name, arg_db));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_open_v2(const char *arg_db_name,
                       sqlite3 **arg_db,
                       int arg_flags,
                       const char *arg_zvfs)
{
#ifdef USE_SQLITE3
  return (sqlite3_open_v2(arg_db_name, arg_db, arg_flags, arg_zvfs));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_bind_int(sqlite3_stmt *arg_stmt, int arg_index, int arg_value)
{
#ifdef USE_SQLITE3
  return (sqlite3_bind_int(arg_stmt, arg_index, arg_value));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_bind_int64(sqlite3_stmt *arg_stmt,
                          int arg_index,
                          i64_t arg_value)
{
#ifdef USE_SQLITE3
  return (sqlite3_bind_int64(arg_stmt, arg_index, arg_value));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_step(sqlite3_stmt *arg_stmt)
{
#ifdef USE_SQLITE3
  return (sqlite3_step(arg_stmt));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_exec(sqlite3 *arg_db,
                    const char *arg_sql,
                    int (*arg_callback)(void *, int, char **, char **),
                    void *arg_void,
                    char **arg_errmsg)
{
#ifdef USE_SQLITE3
  return (sqlite3_exec(arg_db, arg_sql, arg_callback, arg_void, arg_errmsg));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_column_int(sqlite3_stmt *arg_stmt, int arg_icol)
{
#ifdef USE_SQLITE3
  return (sqlite3_column_int(arg_stmt, arg_icol));
#else
  return (INVALID);
#endif
}

i64_t my_sqlite3_column_int64(sqlite3_stmt *arg_stmt, int arg_icol)
{
#ifdef USE_SQLITE3
  return (sqlite3_column_int64(arg_stmt, arg_icol));
#else
  return (INVALID);
#endif
}

const unsigned char *my_sqlite3_column_text(sqlite3_stmt *arg_stmt,
                                            int arg_icol)
{
#ifdef USE_SQLITE3
  return (sqlite3_column_text(arg_stmt, arg_icol));
#else
  return (NULL);
#endif
}

int my_sqlite3_finalize(sqlite3_stmt *arg_stmt)
{
#ifdef USE_SQLITE3
  return (sqlite3_finalize(arg_stmt));
#else
  return (SQLITE_OK);
#endif
}

int my_sqlite3_close(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  return (sqlite3_close(arg_db));
#else
  return (SQLITE_OK);
#endif
}

void my_sqlite3_prepare_v2(sqlite3 *arg_db,
                           const char *arg_sql,
                           sqlite3_stmt **arg_stmt)
{
#ifdef USE_SQLITE3
  int rc = sqlite3_prepare_v2(arg_db, arg_sql, -1, arg_stmt, NULL);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement sql=%s rc=%d err_msg=%s\n",
           arg_sql,
           rc,
           sqlite3_errmsg(arg_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }
#endif
}

int execute_sql(sqlite3 *arg_db, const void *arg_sql, int arg_step)
{
#ifdef USE_SQLITE3
  static _Thread_local my_random_t retry_random;
  static _Thread_local int retry_random_init = FALSE;

  BSTRING(bsql)

  if (arg_step)
    HARDBUG(bassigncstr(bsql, sqlite3_sql((sqlite3_stmt *)arg_sql)) == BSTR_ERR)
  else
    HARDBUG(bassigncstr(bsql, (char *)arg_sql) == BSTR_ERR)

  if (!retry_random_init)
  {
    ui64_t seed;

    construct_my_random(&retry_random, &seed);

    PRINTF("execute_sql seed=%llX\n", seed);

    retry_random_init = TRUE;
  }

  int rc = SQLITE_OK;

  char *err_msg = NULL;

  progress_t progress;

  construct_progress(&progress, "execute_sql", 0, 10);

  while (return_ndone(&progress) <= NRETRIES)
  {
    (void)make_progress(&progress, 0);

    if (arg_step)
      rc = sqlite3_step((sqlite3_stmt *)arg_sql);
    else
      rc = sqlite3_exec(arg_db, (const char *)arg_sql, 0, 0, &err_msg);

    if ((rc != SQLITE_BUSY) and (rc != SQLITE_LOCKED))
      break;

    if (arg_step)
    {
      if ((return_ndone(&progress) % 1000) == 0)
      {
        PRINTF("busy or locked step=%s"
               " return_ndone(&progress)=%lld\n",
               bdata(bsql),
               return_ndone(&progress));
      }
    }
    else
    {
      sqlite3_free(err_msg);

      err_msg = NULL;

      if ((return_ndone(&progress) % 1000) == 0)
      {
        if (blength(bsql) <= (2 * SQL_PRINTF_WIDTH))
        {
          PRINTF("busy or locked sql=%s"
                 " return_ndone(&progress)=%lld\n",
                 bdata(bsql),
                 return_ndone(&progress));
        }
        else
        {
          PRINTF("busy or locked sql=%.*s...%.*s"
                 " return_ndone(&progress)=%lld\n",
                 SQL_PRINTF_WIDTH,
                 bdata(bsql),
                 SQL_PRINTF_WIDTH,
                 bdata(bsql) + blength(bsql) - SQL_PRINTF_WIDTH,
                 return_ndone(&progress));
        }
      }
    }

    double delay = delay =
      (1.0 + return_my_random(&retry_random) % 1000) * MICRO_SECOND;

    if ((return_ndone(&progress) % 1000) == 0)
      PRINTF("return_ndone(&progress)=%lld delay=%.6f\n",
             return_ndone(&progress),
             delay);

    HARDBUG(compat_sleep(delay) != 0)
  }

  if ((rc == SQLITE_OK) or (rc == SQLITE_DONE) or (rc == SQLITE_ROW))
  {
    // 1 step is expected

    if (return_ndone(&progress) > 1)
    {
      finalize_progress(&progress);

      if (arg_step)
      {
        PRINTF("OK or DONE or ROW step=%s"
               " return_ndone(&progress)=%lld\n",
               bdata(bsql),
               return_ndone(&progress));
      }
      else
      {
        PRINTF("OK or DONE or ROW sql=%s"
               " return_ndone(&progress)=%lld\n",
               bdata(bsql),
               return_ndone(&progress));
      }
    }

    destroy_progress(&progress);

    BDESTROY(bsql);

    return (rc);
  }

  finalize_progress(&progress);

  destroy_progress(&progress);

  if (arg_step)
  {
    PRINTF("FAILED step=%s rc=%d\n", bdata(bsql), rc);
  }
  else
  {
    if (blength(bsql) <= (2 * SQL_PRINTF_WIDTH))
    {
      PRINTF("FAILED sql=%s"
             " rc=%d"
             " sqlite3_errmsg(arg_db)=%s"
             " return_ndone(&progress)=%lld\n",
             bdata(bsql),
             rc,
             sqlite3_errmsg(arg_db),
             return_ndone(&progress));
    }
    else
    {
      PRINTF("FAILED sql=%.*s...%.*s"
             " rc=%d"
             " sqlite3_errmsg(arg_db)=%s"
             " return_ndone(&progress)=%lld\n",
             SQL_PRINTF_WIDTH,
             bdata(bsql),
             SQL_PRINTF_WIDTH,
             bdata(bsql) + blength(bsql) - SQL_PRINTF_WIDTH,
             rc,
             sqlite3_errmsg(arg_db),
             return_ndone(&progress));
    }
  }

  FATAL("sqlite3", EXIT_FAILURE)

  return (INVALID);
#else
  return (INVALID);
#endif
}

void create_tables(sqlite3 *arg_db)
{
#ifdef USE_SQLITE3
  const char *create_positions_table_sql =
    "CREATE TABLE IF NOT EXISTS positions "
    "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "fen TEXT UNIQUE,"
    "material INTEGER,"
    "frequency INTEGER DEFAULT 0,"
    "nwon INTEGER DEFAULT 0,"
    "ndraw INTEGER DEFAULT 0,"
    "nlost INTEGER DEFAULT 0,"
    "flags INTEGER DEFAULT 0);";

  const char *create_moves_table_sql =
    "CREATE TABLE IF NOT EXISTS moves (id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "position_id INTEGER NOT NULL,"
    "move TEXT,"
    "frequency INTEGER DEFAULT 0,"
    "nwon INTEGER DEFAULT 0,"
    "ndraw INTEGER DEFAULT 0,"
    "nlost INTEGER DEFAULT 0,"
    "flags INTEGER DEFAULT 0,"
    "FOREIGN KEY (position_id) REFERENCES positions (id) ON DELETE CASCADE,"
    "UNIQUE (position_id, move));";

  const char *create_evaluations_table_sql =
    "CREATE TABLE IF NOT EXISTS evaluations ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "move_id INTEGER NOT NULL,"
    "evaluation INTEGER,"
    "lcb REAL,"
    "ucb REAL,"
    "flags INTEGER DEFAULT 0,"
    "FOREIGN KEY (move_id) REFERENCES moves (id) ON DELETE CASCADE,"
    "UNIQUE (move_id));";

  HARDBUG(execute_sql(arg_db, create_positions_table_sql, FALSE) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_moves_table_sql, FALSE) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db, create_evaluations_table_sql, FALSE) != SQLITE_OK)
#endif
}

int query_position_fen(sqlite3 *arg_db,
                       i64_t arg_position_id,
                       bstring arg_bfen,
                       int *arg_flags)
{
#ifdef USE_SQLITE3
  const char *sql = "SELECT fen, flags FROM positions WHERE id = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return (INVALID);
  }

  const unsigned char *fen = my_sqlite3_column_text(stmt, 0);

  HARDBUG(bassigncstr(arg_bfen, (char *)fen) == BSTR_ERR)

  if (arg_flags != NULL)
    *arg_flags = sqlite3_column_int(stmt, 1);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return (0);
#else
  return (INVALID);
#endif
}

i64_t query_position_id(sqlite3 *arg_db,
                        const char *arg_fen,
                        int arg_frequency,
                        int *arg_flags)
{
#ifdef USE_SQLITE3
  const char *sql = "SELECT id, flags FROM positions WHERE "
                    " fen = ? AND frequency >= ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_text(stmt, 1, arg_fen, -1, SQLITE_STATIC) != SQLITE_OK)

  HARDBUG(sqlite3_bind_int(stmt, 2, arg_frequency) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return (INVALID);
  }

  i64_t id = sqlite3_column_int64(stmt, 0);

  if (arg_flags != NULL)
    *arg_flags = sqlite3_column_int(stmt, 1);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return (id);
#else
  return (INVALID);
#endif
}

i64_t query_move(sqlite3 *arg_db,
                 i64_t arg_position_id,
                 const char *arg_move,
                 int arg_frequency,
                 int *arg_flags)
{
#ifdef USE_SQLITE3
  const char *sql = "SELECT id, flags FROM moves WHERE "
                    "position_id = ? AND move = ? AND frequency >= ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_position_id) != SQLITE_OK)

  HARDBUG(sqlite3_bind_text(stmt, 2, arg_move, -1, SQLITE_STATIC) != SQLITE_OK)

  HARDBUG(sqlite3_bind_int(stmt, 3, arg_frequency) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return (INVALID);
  }

  i64_t id = sqlite3_column_int64(stmt, 0);

  if (arg_flags != NULL)
    *arg_flags = sqlite3_column_int(stmt, 1);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return (id);
#else
  return (INVALID);
#endif
}

score_t query_evaluation(sqlite3 *arg_db, i64_t arg_move_id, int *arg_flags)
{
#ifdef USE_SQLITE3
  const char *sql =
    "SELECT evaluation, flags FROM evaluations WHERE move_id = ?;";

  sqlite3_stmt *stmt;

  my_sqlite3_prepare_v2(arg_db, sql, &stmt);

  HARDBUG(sqlite3_bind_int64(stmt, 1, arg_move_id) != SQLITE_OK)

  if (execute_sql(arg_db, stmt, TRUE) != SQLITE_ROW)
  {
    HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

    return (SCORE_NOT_A_NUMBER);
  }

  int evaluation = sqlite3_column_int(stmt, 0);

  if (arg_flags != NULL)
    *arg_flags = sqlite3_column_int(stmt, 1);

  HARDBUG(sqlite3_finalize(stmt) != SQLITE_OK)

  return (evaluation);
#else
  return (INVALID);
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
           arg_file_name,
           sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  backup = sqlite3_backup_init(disk_db, "main", arg_in_memory_db, "main");

  if (!backup)
  {
    PRINTF("Failed to initialize backup: %s err_msg=%s\n",
           arg_file_name,
           sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_step(backup, -1);

  if ((rc != SQLITE_DONE) and (rc != SQLITE_OK))
  {
    PRINTF("Backup step failed: %s err_msg=%s\n",
           arg_file_name,
           sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_finish(backup);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup finish failed: %s err_msg=%s\n",
           arg_file_name,
           sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_errcode(disk_db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup failed: %s err_msg=%s\n",
           arg_file_name,
           sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_close(disk_db);
#endif
}

void flush_sql_buffer(my_sqlite3_t *arg_with_my_sqlite3, int nsql_buffer)
{
  if (blength(arg_with_my_sqlite3->MSQL_bbuffer) > nsql_buffer)
  {
    if (arg_with_my_sqlite3->MSQL_verbose > 0)
      PRINTF("blength(arg_with_my_sqlite3->MSQL_bbuffer)=%d\n$",
             blength(arg_with_my_sqlite3->MSQL_bbuffer));

    if (arg_with_my_sqlite3->MSQL_comm != MPI_COMM_NULL)
    {
      PRINTF("acquiring semaphore..\n$");

      my_mpi_acquire_semaphore_v3(arg_with_my_sqlite3->MSQL_comm,
                                  my_mpi_globals.MMG_semkey_flush_sql_buffer);
    }

    if (arg_with_my_sqlite3->MSQL_verbose > 0)
      PRINTF("executing BEGIN TRANSACTION..\n$");

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        "BEGIN TRANSACTION;",
                        FALSE) != SQLITE_OK)

#ifdef SPLIT_SQL_BUFFER
    BSTRING(b)

    bassigncstr(b, ";");

    struct bstrList *bsql;

    HARDBUG((bsql = bsplits(arg_with_my_sqlite3->MSQL_bbuffer, b)) == NULL)

    for (int isql = 0; isql < bsql->qty; isql++)
    {
      HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                          bdata(bsql->entry[isql]),
                          FALSE) != SQLITE_OK)
    }

    HARDBUG(bstrListDestroy(bsql) == BSTR_ERR)

    BDESTROY(b)
#else
    PRINTF("executing SQL\n$");

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        bdata(arg_with_my_sqlite3->MSQL_bbuffer),
                        FALSE) != SQLITE_OK)
#endif

    if (arg_with_my_sqlite3->MSQL_verbose > 0)
      PRINTF("executing COMMIT..\n$");

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db, "COMMIT;", FALSE) !=
            SQLITE_OK)

    if (arg_with_my_sqlite3->MSQL_comm != MPI_COMM_NULL)
    {
      PRINTF("releasing semaphore..\n$");

      my_mpi_release_semaphore_v3(arg_with_my_sqlite3->MSQL_comm,
                                  my_mpi_globals.MMG_semkey_flush_sql_buffer);
    }

    if (arg_with_my_sqlite3->MSQL_verbose > 0)
      PRINTF("done\n$");

    HARDBUG(bassigncstr(arg_with_my_sqlite3->MSQL_bbuffer, "") == BSTR_ERR)
  }
}

void append_sql_buffer(my_sqlite3_t *arg_with_my_sqlite3,
                       const char *arg_fmt,
                       ...)
{
  int ret;

  bvformata(ret, arg_with_my_sqlite3->MSQL_bbuffer, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  flush_sql_buffer(arg_with_my_sqlite3, NSQL_BUFFER);
}

void construct_my_sqlite3(my_sqlite3_t *arg_with_my_sqlite3,
                          char *arg_db_name,
                          MPI_Comm arg_comm)
{
  arg_with_my_sqlite3->MSQL_comm = arg_comm;

  int iproc = INVALID;

  if (arg_with_my_sqlite3->MSQL_comm != MPI_COMM_NULL)
    my_mpi_comm_rank(arg_with_my_sqlite3->MSQL_comm, &iproc);

  if (iproc <= 0)
  {
    int rc = my_sqlite3_open(arg_db_name, &(arg_with_my_sqlite3->MSQL_db));

    if (rc != SQLITE_OK)
    {
      if (arg_with_my_sqlite3->MSQL_db == NULL)
      {
        PRINTF("Cannot open database: %s err_msg=NULL\n", arg_db_name);
      }
      else
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
               arg_db_name,
               my_sqlite3_errmsg(arg_with_my_sqlite3->MSQL_db));
      }

      FATAL("sqlite3", EXIT_FAILURE)
    }

    create_tables(arg_with_my_sqlite3->MSQL_db);

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        "PRAGMA foreign_keys = ON;",
                        FALSE) != SQLITE_OK)

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        "PRAGMA journal_mode = WAL;",
                        FALSE) != SQLITE_OK)

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        "PRAGMA cache_size=-131072;",
                        FALSE) != SQLITE_OK)
  }

  my_mpi_barrier_v3("after open database",
                    arg_comm,
                    my_mpi_globals.MMG_semkey_construct_my_sqlite3_barrier,
                    TRUE);

  if (iproc > 0)
  {
    int rc = my_sqlite3_open(arg_db_name, &(arg_with_my_sqlite3->MSQL_db));

    if (rc != SQLITE_OK)
    {
      if (arg_with_my_sqlite3->MSQL_db == NULL)
      {
        PRINTF("Cannot open database: %s err_msg=NULL\n", arg_db_name);
      }
      else
      {
        PRINTF("Cannot open database: %s err_msg=%s\n",
               arg_db_name,
               my_sqlite3_errmsg(arg_with_my_sqlite3->MSQL_db));
      }
      FATAL("sqlite3", EXIT_FAILURE)
    }

    HARDBUG(execute_sql(arg_with_my_sqlite3->MSQL_db,
                        "PRAGMA cache_size=-131072;",
                        FALSE) != SQLITE_OK)
  }

  HARDBUG((arg_with_my_sqlite3->MSQL_bbuffer = bfromcstr("")) == NULL)

  arg_with_my_sqlite3->MSQL_verbose = 1;
}

void close_my_sqlite3(my_sqlite3_t *arg_with_my_sqlite3)
{
#ifdef USE_SQLITE3
  int iproc = INVALID;

  if (arg_with_my_sqlite3->MSQL_comm != MPI_COMM_NULL)
    my_mpi_comm_rank(arg_with_my_sqlite3->MSQL_comm, &iproc);

  if (iproc > 0)
  {
    PRINTF("%s::closing db iproc=%d\n$", __FUNC__, iproc);

    my_mpi_acquire_semaphore_v3(arg_with_my_sqlite3->MSQL_comm,
                                my_mpi_globals.MMG_semkey_close_my_sqlite3);

    HARDBUG(my_sqlite3_close(arg_with_my_sqlite3->MSQL_db) != SQLITE_OK)

    my_mpi_release_semaphore_v3(arg_with_my_sqlite3->MSQL_comm,
                                my_mpi_globals.MMG_semkey_close_my_sqlite3);

    PRINTF("%s::closed db iproc=%d\n$", __FUNC__, iproc);
  }

  my_mpi_barrier_v3("after my_sqlite3_close",
                    arg_with_my_sqlite3->MSQL_comm,
                    my_mpi_globals.MMG_semkey_close_my_sqlite3_barrier,
                    TRUE);

  if (iproc <= 0)
  {
    int ntries = 0;

    while (TRUE)
    {
      int frames_in_wal;
      int frames_checkpointed;

      int rc = sqlite3_wal_checkpoint_v2(arg_with_my_sqlite3->MSQL_db,
                                         NULL,
                                         SQLITE_CHECKPOINT_TRUNCATE,
                                         &frames_in_wal,
                                         &frames_checkpointed);
      if (rc != SQLITE_OK)
      {
        PRINTF("Failed wal_checkpoint_v2: %s\n",
               sqlite3_errmsg(arg_with_my_sqlite3->MSQL_db));

        FATAL("sqlite3", EXIT_FAILURE);
      }

      PRINTF("ntries=%d frames_in_wal=%d frames_checkpointed=%d\n$",
             ntries,
             frames_in_wal,
             frames_checkpointed);

      if (frames_in_wal <= 0)
        break;

      ++ntries;

      HARDBUG(ntries > NTRIES_MAX)
    }

    if (ntries > 0)
      PRINTF("needed %d tries to flush wal\n$", ntries);

    HARDBUG(sqlite3_close(arg_with_my_sqlite3->MSQL_db) != SQLITE_OK)
  }
#endif
}

#define TEST_DB "test.db"

#define NTEST_POSITIONS 1000
#define NTEST_MOVES 100

void test_dbase(void)
{
  PRINTF("test_dbase\n");

  my_sqlite3_t db;

  remove(TEST_DB);

  construct_my_sqlite3(&db, TEST_DB, MPI_COMM_NULL);

  create_tables(db.MSQL_db);

  i64_t position_id = 0;
  i64_t move_id = 0;

  for (i64_t iposition = 0; iposition < NTEST_POSITIONS; iposition++)
  {
    ++position_id;

    append_sql_buffer(&db,
                      "INSERT INTO positions (id, fen) "
                      "VALUES (%d, 'position%d');",
                      position_id,
                      position_id);

    for (i64_t imove = 0; imove < NTEST_MOVES; imove++)
    {
      ++move_id;

      append_sql_buffer(&db,
                        "INSERT INTO moves (position_id, id, move) "
                        "VALUES (%d, %d, 'move%d');",
                        position_id,
                        move_id,
                        move_id);
    }
  }

  flush_sql_buffer(&db, 0);

  HARDBUG(my_sqlite3_close(db.MSQL_db) != SQLITE_OK)

  PRINTF("test_dbase OK\n");
}
