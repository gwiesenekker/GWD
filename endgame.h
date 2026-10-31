// SCU REVISION 8.1255 za 31 okt 2026  9:15:42 CET
#ifndef EndgameH
#define EndgameH

#define USE_COMPRESSED_WDL

#define ENDGAME_UNKNOWN 32767

extern GwdegtbWdlProbe *wdl_probe_alpha_beta_master;

void init_endgame(void);
int read_endgame(search_t *);
score_t return_endgame_score(search_t *);
void fin_endgame(void);

#endif
