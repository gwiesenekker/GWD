//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef PdnH
#define PdnH

void read_games(my_sqlite3_t *, char *);
void gen_db(my_sqlite3_t *, i64_t, int);
void update_db(my_sqlite3_t *, int);
void update_db_v2(my_sqlite3_t *, int, int);

#endif
