//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef SearchH
#define SearchH

#define DEPTH_MAX 127

#define MINIMAL_WINDOW_BIT BIT(0)
#define PV_BIT             BIT(1)
#define GE_BETA_BIT        BIT(2)
#define LE_ALPHA_BIT       BIT(3)
#define TRUE_SCORE_BIT     BIT(4)

#define IS_MINIMAL_WINDOW(X) ((X) & MINIMAL_WINDOW_BIT)
#define IS_PV(X)             ((X) & PV_BIT)

//search.c

#define SCORE_WON            10000
#define SCORE_LOST           (-SCORE_WON)
#define SCORE_PLUS_INFINITY  20000
#define SCORE_MINUS_INFINITY (-SCORE_PLUS_INFINITY)

#define SEARCH_BEST_SCORE_EGTB 0
#define SEARCH_BEST_SCORE_AB   1

//#define NODE_MAX

int draw_by_repetition(board_t *with, int);
void clear_totals(board_t *with);
void print_totals(board_t *with);
void clear_caches(void);
void init_search(void);
void fin_search(void);
void search(board_t *with, moves_list_t *, int, int, int, my_random_t *);

#endif

