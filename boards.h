//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#ifndef BoardsH
#define BoardsH

#define wO_hub "w"
#define wX_hub "W"
#define bO_hub "b"
#define bX_hub "B"
#define nn_hub "e"
#define NN_hub "E"

#define wO "o"
#define wX "x"
#define bO "O"
#define bX "X"
#define nn "."
#define NN "E"

#define STARTING_POSITION "wOOOOOOOOOOOOOOOOOOOO..........oooooooooooooooooooo"
#define STARTING_POSITION2FEN "W:W31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50:B01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20"

#define MAN_BIT   BIT(0)
#define KING_BIT BIT(1)
#define SIDE_BIT  BIT(2)
#define WHITE_BIT BIT(3)
#define BLACK_BIT BIT(4)

#define BOARD_INTERRUPT_TIME     BIT(0)
#define BOARD_INTERRUPT_MESSAGE  BIT(1)

#define IS_WHITE(X) ((X) & WHITE_BIT)
#define IS_BLACK(X) ((X) & BLACK_BIT)

#define IS_EMPTY(X) ((X) == 0)
#define IS_MAN(X)   ((X) & MAN_BIT)
#define IS_KING(X) ((X) & KING_BIT)

#define IS_MINE(X) ((X) & MY_BIT)
#define IS_YOURS(X) ((X) & YOUR_BIT)

#define the_man_bb(X) cat3(board_, X, _man_bb)
#define my_man_bb     the_man_bb(my_colour)
#define your_man_bb   the_man_bb(your_colour)

#define the_king_bb(X) cat3(board_, X, _king_bb)
#define my_king_bb     the_king_bb(my_colour)
#define your_king_bb   the_king_bb(your_colour)

#define the_row(X) cat2(X, _row)
#define my_row     the_row(my_colour)
#define your_row   the_row(your_colour)

#define the_man_key(X) cat2(X, _man_key)
#define my_man_key     the_man_key(my_colour)
#define your_man_key   the_man_key(your_colour)

#define the_king_key(X) cat2(X, _king_key)
#define my_king_key     the_king_key(my_colour)
#define your_king_key   the_king_key(your_colour)

#define PV_MAX 256

typedef i8_t pv_t;

typedef struct
{
  int node_ncaptx;
  hash_key_t node_key;
  hash_key_t node_move_key;
  ui64_t node_white_man_bb;
  ui64_t node_white_king_bb;
  ui64_t node_black_man_bb;
  ui64_t node_black_king_bb;
  int node_move_tactical;
} node_t;

typedef struct board
{
  int board_id;
  my_printf_t *board_my_printf;
  thread_t *board_thread;

  ui64_t board_empty_bb;
  ui64_t board_white_man_bb;
  ui64_t board_white_king_bb;
  ui64_t board_black_man_bb;
  ui64_t board_black_king_bb;

  int board_colour2move;
  hash_key_t board_key;

  int board_inode;
  int board_root_inode;

  node_t board_nodes[NODE_MAX];

  my_timer_t board_timer;

  neural_t board_neural0;
  int board_neural1_not_null;
  neural_t board_neural1;

  //endgame

  cache_t board_endgame_entry_cache;

  //dxp

  int board_dxp_game_colour;
  int board_dxp_game_time;
  int board_dxp_game_moves;
  char board_dxp_move_string[MY_LINE_MAX];
  int board_dxp_game_time_used;
  int board_dxp_move_number;
  char board_dxp_game_code;
 
  //search

  int board_search_root_simple_score;
  int board_search_root_score;
  int board_search_best_score;
  int board_search_best_score_kind;
  int board_search_best_move;
  int board_search_best_depth;
  pv_t board_search_best_pv[PV_MAX];

  int board_interrupt;

  i64_t total_move_repetitions;

  i64_t total_quiescence_nodes;
  i64_t total_quiescence_all_moves_captures_only;
  i64_t total_quiescence_all_moves_le2_moves;
  i64_t total_quiescence_all_moves_tactical;

  i64_t total_nodes;
  i64_t total_alpha_beta_nodes;
  i64_t total_minimal_window_nodes;
  i64_t total_pv_nodes;

  i64_t total_reductions_delta;
  i64_t total_reductions_delta_lost;
  i64_t total_reductions_delta_le_alpha;
  i64_t total_reductions_delta_ge_beta;

  i64_t total_reductions;
  i64_t total_reductions_le_alpha;
  i64_t total_reductions_ge_beta;

  i64_t total_single_reply_extensions;

  i64_t total_evaluations;
  i64_t total_material_only_evaluations;
  i64_t total_neural_evaluations;

  i64_t total_alpha_beta_cache_hits;
  i64_t total_alpha_beta_cache_depth_hits;
  i64_t total_alpha_beta_cache_le_alpha_hits;
  i64_t total_alpha_beta_cache_le_alpha_cutoffs;
  i64_t total_alpha_beta_cache_ge_beta_hits;
  i64_t total_alpha_beta_cache_ge_beta_cutoffs;
  i64_t total_alpha_beta_cache_true_score_hits;
  i64_t total_alpha_beta_cache_le_alpha_stored;
  i64_t total_alpha_beta_cache_ge_beta_stored;
  i64_t total_alpha_beta_cache_true_score_stored;
  i64_t total_alpha_beta_cache_nmoves_errors;
  i64_t total_alpha_beta_cache_crc32_errors;

  char board_string[MY_LINE_MAX];

  char * (*board2string)(struct board *, int);
  hash_key_t board_board2string_key;
} board_t;

extern int map[1 + 50];

extern int inverse_map[BOARD_MAX];

extern char *nota[BOARD_MAX];

extern int white_row[BOARD_MAX];
extern int black_row[BOARD_MAX];

extern hash_key_t white_man_key[BOARD_MAX];
extern hash_key_t white_king_key[BOARD_MAX];

extern hash_key_t black_man_key[BOARD_MAX];
extern hash_key_t black_king_key[BOARD_MAX];

board_t *return_with_board(int);
int create_board(my_printf_t *, thread_t *);
void destroy_board(int);
hash_key_t return_key_from_bb(board_t *);
hash_key_t return_key_from_inputs(neural_t *);
void string2board(char *, int);
void fen2board(char *, int);
//TODO make board2fen member of board_t
void board2fen(int, char[MY_LINE_MAX], int);
void print_board(int);
void init_boards();
void state2board(board_t *, state_t *);

//boards.d

void check_white_list(board_t *);
void check_black_list(board_t *);

#endif

