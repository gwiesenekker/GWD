//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#include "globals.h"

int execute_sql(sqlite3 *db, const void *sql, int step, int nretries)
{
  int rc = SQLITE_OK;

  char *err_msg = NULL;

  int iretry = INVALID;

  if (nretries == 0)
  {
    if (step)
      rc = sqlite3_step((sqlite3_stmt *) sql);
    else
      rc = sqlite3_exec(db, (const char *) sql, 0, 0, &err_msg);
  }
  else
  {
    for (iretry = 0; iretry < nretries; iretry++)
    {
      static my_random_t retry_random;
      static int retry_random_init = FALSE;

      if (!retry_random_init)
      {
        construct_my_random(&retry_random, 0);

        retry_random_init = TRUE;
      }


      if (step)
        rc = sqlite3_step((sqlite3_stmt *) sql);
      else
        rc = sqlite3_exec(db, (const char *) sql, 0, 0, &err_msg);

      double sleep =
        0.1 + 0.1 * (return_my_random(&retry_random) % nretries / nretries);

      if ((rc == SQLITE_BUSY) or (rc == SQLITE_LOCKED))
      {
        if (step)
        {
          sqlite3_reset((sqlite3_stmt *)sql);

          PRINTF("Busy or locked: %s iretry=%d sleep=%.2f\n", 
                 sqlite3_sql((sqlite3_stmt *) sql), iretry, sleep);
        }
        else
        {
          sqlite3_free(err_msg);

          err_msg = NULL;

          PRINTF("Busy or locked: %s iretry=%d sleep=%.2f\n", 
                 (const char *) sql, iretry, sleep);
        }

        compat_sleep(sleep);

        continue;
      }
      break;
    }
  }

  if ((rc == SQLITE_OK) or (rc == SQLITE_DONE) or (rc == SQLITE_ROW))
  {
    if (iretry > 0) PRINTF("needed %d retries\n", iretry);

    return(rc);
  }


  if (step)
  {
    PRINTF("Failed to execute: %s rc=%d err_msg=%s\n",
           sqlite3_sql((sqlite3_stmt *) sql), rc, sqlite3_errmsg(db));
  }
  else
  {
    PRINTF("Failed to execute: %s rc=%d err_msg=%s\n",
           (const char *) sql, rc, err_msg);
  }

  FATAL("sqlite3", EXIT_FAILURE)

  return(INVALID);
}

void create_tables(sqlite3 *db, int nretries)
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

  HARDBUG(execute_sql(db, create_positions_table_sql, FALSE, nretries) !=
          SQLITE_OK)

  HARDBUG(execute_sql(db, create_moves_table_sql, FALSE, nretries) !=
          SQLITE_OK)

  HARDBUG(execute_sql(db, create_evaluations_table_sql, FALSE, nretries) !=
          SQLITE_OK)
}

int query_position(sqlite3 *db, const char *position, int nretries)
{
  const char *sql = "SELECT id FROM positions WHERE position = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_text(stmt, 1, position, -1, SQLITE_STATIC);

  if ((rc = execute_sql(db, stmt, TRUE, nretries)) == SQLITE_DONE)
  {
    sqlite3_finalize(stmt);

    return(INVALID);
  }

  HARDBUG(rc != SQLITE_ROW)

  int id = sqlite3_column_int(stmt, 0);

  sqlite3_finalize(stmt);

  return(id);
}

int insert_position(sqlite3 *db, const char *position, int nretries)
{
  const char *sql =
    "INSERT OR IGNORE INTO positions (position) VALUES (?);";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_text(stmt, 1, position, -1, SQLITE_STATIC);

  HARDBUG(execute_sql(db, stmt, TRUE, nretries) != SQLITE_DONE)

  sqlite3_finalize(stmt);

  int id = query_position(db, position, nretries);

  HARDBUG(id == INVALID)

  return(id);
}

int query_move(sqlite3 *db, int position_id, const char *move, int nretries)
{
  const char *sql =
    "SELECT id FROM moves WHERE position_id = ? and move = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_int(stmt, 1, position_id);

  sqlite3_bind_text(stmt, 2, move, -1, SQLITE_STATIC);

  if (execute_sql(db, stmt, TRUE, nretries) != SQLITE_ROW)
  {
    sqlite3_finalize(stmt);

    return(INVALID);
  }

  int id = sqlite3_column_int(stmt, 0);

  sqlite3_finalize(stmt);

  return(id);
}

int insert_move(sqlite3 *db, int position_id, const char *move, int nretries)
{
  const char *sql =
   "INSERT INTO moves"
   " (position_id, move, nwon, ndraw, nlost)"
   " VALUES (?, ?, 0, 0, 0);";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_int(stmt, 1, position_id);

  sqlite3_bind_text(stmt, 2, move, -1, SQLITE_STATIC);

  HARDBUG(execute_sql(db, stmt, TRUE, nretries) != SQLITE_DONE)

  sqlite3_finalize(stmt);

  int id = query_move(db, position_id, move, nretries);

  HARDBUG(id == INVALID)

  return(id);
}

int query_evaluation(sqlite3 *db, int move_id, int eval_time, int nretries)
{
  const char *sql =
   "SELECT evaluation FROM evaluations WHERE move_id = ? and eval_time = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_int(stmt, 1, move_id);

  sqlite3_bind_int(stmt, 2, eval_time);

  if (execute_sql(db, stmt, TRUE, nretries) != SQLITE_ROW)
  {
    sqlite3_finalize(stmt);

    return(SCORE_PLUS_INFINITY);
  }

  int evaluation = sqlite3_column_int(stmt, 0);

  sqlite3_finalize(stmt);

  return(evaluation);
}

void increment_nwon_ndraw_nlost(sqlite3 *db, int position_id,
  int id, int nwon, int ndraw, int nlost, int nretries)
{
  const char *sql;

  if (nwon > 0)
  {
    HARDBUG(ndraw != 0)
    HARDBUG(nlost != 0)

    sql =
      "UPDATE moves SET nwon = nwon + 1 WHERE position_id = ? AND id = ?;";
  }
  if (ndraw > 0)
  {
    HARDBUG(nwon != 0)
    HARDBUG(nlost != 0)

    sql =
      "UPDATE moves SET ndraw = ndraw + 1 WHERE position_id = ? AND id = ?;";
  }
  if (nlost > 0)
  {
    HARDBUG(nwon != 0)
    HARDBUG(ndraw != 0)

    sql =
      "UPDATE moves SET nlost = nlost + 1 WHERE position_id = ? AND id = ?;";
  }
  
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s rc=%d err_msg=%s\n",
           sql, rc, sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
   }

   sqlite3_bind_int(stmt, 1, position_id);

   sqlite3_bind_int(stmt, 2, id);

   HARDBUG(execute_sql(db, stmt, TRUE, nretries) != SQLITE_DONE)

   sqlite3_finalize(stmt);
}

// Function to query moves for a given position

void query_moves(sqlite3 *db, int position_id, int nretries)
{
  const char *sql =
   "SELECT move, nwon, ndraw, nlost from moves WHERE position_id = ?;";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK)
  {
    PRINTF("Failed to prepare statement: %s\n", sqlite3_errmsg(db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_bind_int(stmt, 1, position_id);

  while ((rc = execute_sql(db, stmt, TRUE, nretries)) == SQLITE_ROW)
  {
    const unsigned char *move = sqlite3_column_text(stmt, 0);

    int nwon = sqlite3_column_int(stmt, 1);
    int ndraw = sqlite3_column_int(stmt, 2);
    int nlost = sqlite3_column_int(stmt, 3);

    PRINTF("move=%s nwon=%d ndraw=%d nlost=%d\n", 
      move, nwon, ndraw, nlost);
  }

  HARDBUG(rc != SQLITE_DONE)

  sqlite3_finalize(stmt);
}

void backup_db(sqlite3 *in_memory_db, const char *file_name)
{
  sqlite3 *disk_db;
  sqlite3_backup *backup;
  int rc;

  rc = sqlite3_open(file_name, &disk_db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Cannot open disk database: %s err_mes=%s\n",
           file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  backup = sqlite3_backup_init(disk_db, "main", in_memory_db, "main");

  if (!backup)
  {
    PRINTF("Failed to initialize backup: %s err_msg=%s\n", 
           file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_step(backup, -1);

  if ((rc != SQLITE_DONE) and (rc != SQLITE_OK))
  {
    PRINTF("Backup step failed: %s err_msg=%s\n",
           file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_backup_finish(backup);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup finish failed: %s err_msg=%s\n",
           file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  rc = sqlite3_errcode(disk_db);

  if (rc != SQLITE_OK)
  {
    PRINTF("Backup failed: %s err_msg=%s\n",
           file_name, sqlite3_errmsg(disk_db));

    FATAL("sqlite3", EXIT_FAILURE)
  }

  sqlite3_close(disk_db);
}

#define NRETRIES 5

void test_dbase(void)
{
  PRINTF("test_dbase\n");
  sqlite3 *db;

  int rc = sqlite3_open("draughts.db", &db);

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

  sqlite3_close(db);

  PRINTF("test_dbase OK\n");
}

