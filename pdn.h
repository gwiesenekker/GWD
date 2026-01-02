//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef PdnH
#define PdnH

void read_games(my_sqlite3_t *, char *);
void gen_db(my_sqlite3_t *, i64_t, int);
void update_db(my_sqlite3_t *, int);
void update_db_v2(my_sqlite3_t *, int, int);

#endif

