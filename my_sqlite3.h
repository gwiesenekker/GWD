//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
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

typedef struct
{
  sqlite3 *MS_db;
  MPI_Comm MS_comm;
  bstring MS_bbuffer;
  int MS_verbose;
} my_sqlite3_t;

const char *my_sqlite3_errmsg(sqlite3 *);

int my_sqlite3_open(const char *, sqlite3 **);

int my_sqlite3_open_v2(const char *, sqlite3 **, int, const char *);

int my_sqlite3_bind_int(sqlite3_stmt *, int, int);
int my_sqlite3_bind_int64(sqlite3_stmt *, int, i64_t);

int my_sqlite3_step(sqlite3_stmt *);

int my_sqlite3_exec(sqlite3 *,
                    const char *,
                    int (*)(void *, int, char **, char **),
                    void *,
                    char **);

int my_sqlite3_column_int(sqlite3_stmt *, int);

i64_t my_sqlite3_column_int64(sqlite3_stmt *, int);

const unsigned char *my_sqlite3_column_text(sqlite3_stmt *, int);

int my_sqlite3_finalize(sqlite3_stmt *);

int my_sqlite3_close(sqlite3 *);

void my_sqlite3_prepare_v2(sqlite3 *, const char *, sqlite3_stmt **);

int execute_sql(sqlite3 *, const void *, int);

void create_tables(sqlite3 *);

int query_position_fen(sqlite3 *, i64_t, bstring);

i64_t query_position_id(sqlite3 *, const char *, int);

i64_t query_move(sqlite3 *, i64_t, const char *, int);

int query_evaluation(sqlite3 *, i64_t);

void backup_db(sqlite3 *, const char *);

void append_sql_buffer(my_sqlite3_t *, const char *, ...);

void flush_sql_buffer(my_sqlite3_t *, int);

void construct_my_sqlite3(my_sqlite3_t *, char *, MPI_Comm);

void close_my_sqlite3(my_sqlite3_t *);

void test_dbase(void);

#endif
