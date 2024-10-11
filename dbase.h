//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef DbaseH
#define DbaseH

#if COMPAT_OS == COMPAT_OS_WINDOWS
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif

int execute_sql(sqlite3 *, const void *, int, int);
void create_tables(sqlite3 *, int);

int query_position(sqlite3 *, const char *, int);
int insert_position(sqlite3 *, const char *, int);

int query_move(sqlite3 *, int, const char *, int);
int insert_move(sqlite3 *, int, const char *, int);
int query_evaluation(sqlite3 *, int, int, int);
void increment_nwon_ndraw_nlost(sqlite3 *, int, int, int, int, int, int);

void query_moves(sqlite3 *, int, int);

void backup_db(sqlite3 *, const char *);

void test_dbase(void);

#endif
