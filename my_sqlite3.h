// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef DbaseH
#define DbaseH

#ifdef USE_SQLITE3

#if COMPAT_OS == COMPAT_OS_WINDOWS
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif

#else

#define SQLITE_OPEN_READONLY 1

#define SQLITE_OK 2
#define SQLITE_ROW 3
#define SQLITE_DONE 4

typedef struct sqlite3_ sqlite3;
typedef struct sqlite3_stmt_ sqlite3_stmt;

#endif

#define FLAG_NULL NULL

#define FLAG_POSITION_MOVES_EVALUATED BIT(0)

#define FLAG_MOVE_EVALUATION_SIZE BIT(0)
#define FLAG_MOVE_NMOVES_LIMIT BIT(1)
#define FLAG_MOVE_EXPANDED BIT(2)

typedef struct
{
  sqlite3 *MSQL_db;
  MPI_Comm MSQL_comm;
  bstring MSQL_bbuffer;
  int MSQL_verbose;
} my_sqlite3_t;

const char *my_sqlite3_errmsg(sqlite3 *arg_db);

int my_sqlite3_open(const char *arg_db_name, sqlite3 **arg_db);

int my_sqlite3_open_v2(const char *arg_db_name,
                       sqlite3 **arg_db,
                       int arg_flags,
                       const char *arg_zvfs);

int my_sqlite3_bind_int(sqlite3_stmt *arg_stmt, int arg_index, int arg_value);
int my_sqlite3_bind_int64(sqlite3_stmt *arg_stmt,
                          int arg_index,
                          my_int64_t arg_value);

int my_sqlite3_step(sqlite3_stmt *arg_stmt);

int my_sqlite3_exec(sqlite3 *arg_db,
                    const char *arg_sql,
                    int (*arg_callback)(void *, int, char **, char **),
                    void *arg_void,
                    char **arg_errmsg);

int my_sqlite3_column_int(sqlite3_stmt *arg_stmt, int arg_icol);

my_int64_t my_sqlite3_column_int64(sqlite3_stmt *arg_stmt, int arg_icol);

const unsigned char *my_sqlite3_column_text(sqlite3_stmt *arg_stmt,
                                            int arg_icol);

int my_sqlite3_finalize(sqlite3_stmt *arg_stmt);

int my_sqlite3_close(sqlite3 *arg_db);

void my_sqlite3_prepare_v2(sqlite3 *arg_db,
                           const char *arg_sql,
                           sqlite3_stmt **arg_stmt);

int execute_sql(sqlite3 *arg_db, const void *arg_sql, int arg_step);

void create_tables(sqlite3 *arg_db);

int query_position_fen(sqlite3 *arg_db,
                       my_int64_t arg_position_id,
                       bstring arg_bfen,
                       int *arg_flags);

my_int64_t query_position_id(sqlite3 *arg_db,
                             const char *arg_fen,
                             int arg_frequency,
                             int *arg_flags);

my_int64_t query_move(sqlite3 *arg_db,
                      my_int64_t arg_position_id,
                      const char *arg_move,
                      int arg_frequency,
                      int *arg_flags);

score_t
query_evaluation(sqlite3 *arg_db, my_int64_t arg_move_id, int *arg_flags);

void backup_db(sqlite3 *arg_in_memory_db, const char *arg_file_name);

void append_sql_buffer(my_sqlite3_t *arg_with_my_sqlite3,
                       const char *arg_fmt,
                       ...);

void flush_sql_buffer(my_sqlite3_t *arg_with_my_sqlite3, int arg_nsql_buffer);

void construct_my_sqlite3(my_sqlite3_t *arg_with_my_sqlite3,
                          const char *arg_db_name,
                          MPI_Comm arg_comm);

void close_my_sqlite3(my_sqlite3_t *arg_with_my_sqlite3);

void test_dbase(void);

#endif
