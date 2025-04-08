//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#ifndef DbaseH
#define DbaseH

#if COMPAT_OS == COMPAT_OS_WINDOWS
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif

typedef struct
{
  //needed for flush
  sqlite3 *SB_db;
  MPI_Comm SB_comm;
  MPI_Win SB_win;
  int SB_nretries;
  bstring SB_bbuffer;
} sql_buffer_t;

int execute_sql(sqlite3 *, const void *, int, int);

void create_tables(sqlite3 *, int);

int query_position(sqlite3 *, const char *, int);

int insert_position(sqlite3 *, const char *, int);

int query_move(sqlite3 *, int, const char *, int);

int insert_move(sqlite3 *, int, const char *, int, int);

int query_evaluation(sqlite3 *, int, int *, int);

void increment_nwon_ndraw_nlost(sqlite3 *, int, int, int, int, int, int);

void query_moves(sqlite3 *, int, int);

void backup_db(sqlite3 *, const char *);
 
void wal_checkpoint(sqlite3 *);

void construct_sql_buffer(sql_buffer_t *, sqlite3 *, MPI_Comm, MPI_Win, int);

void append_sql_buffer(sql_buffer_t *, const char *fmt, ...);

void flush_sql_buffer(sql_buffer_t *, int);

void test_dbase(void);

#endif
