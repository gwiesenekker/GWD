//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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

#define the_man_bb(X) cat3(B_, X, _man_bb)
#define my_man_bb     the_man_bb(my_colour)
#define your_man_bb   the_man_bb(your_colour)

#define the_king_bb(X) cat3(B_, X, _king_bb)
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

#define the_row_empty_bb(X) cat2(X, _row_empty_bb)
#define my_row_empty_bb     the_row_empty_bb(my_colour)
#define your_row_empty_bb   the_row_empty_bb(your_colour)

#define PV_MAX 256

typedef i8_t pv_t;

typedef struct
{
  int node_ncaptx;
  hash_key_t node_move_key;
} node_t;

typedef struct 
{
  ui64_t BS_key;
  ui64_t BS_white_man_bb;
  ui64_t BS_white_king_bb;
  ui64_t BS_black_man_bb;
  ui64_t BS_black_king_bb;
  int BS_npieces;
} board_state_t;

typedef struct board
{
  my_printf_t *B_my_printf;

  ui64_t B_valid_bb;
  ui64_t B_white_man_bb;
  ui64_t B_white_king_bb;
  ui64_t B_black_man_bb;
  ui64_t B_black_king_bb;

  int B_colour2move;
  hash_key_t B_key;

  int B_inode;
  int B_root_inode;

  node_t B_nodes[NODE_MAX];

  pattern_mask_t *B_pattern_mask;

  network_t B_network;

  char B_string[MY_LINE_MAX];

  int B_nstate;
  board_state_t B_states[NODE_MAX];
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

extern ui64_t left_wing_bb;
extern ui64_t center_bb;
extern ui64_t right_wing_bb;

extern ui64_t left_half_bb;
extern ui64_t right_half_bb;

extern ui64_t white_row_empty_bb;
extern ui64_t black_row_empty_bb;

void construct_board(void *, my_printf_t *, int);
void push_board_state(void *self);
void pop_board_state(void *self);

hash_key_t return_key_from_bb(void *);
void string2board(board_t *, char *, int);
void fen2board(board_t *, char *, int);
void board2fen(board_t *, bstring, int);
void print_board(board_t *);
void init_boards();
void state2board(board_t *, state_t *);
char *board2string(board_t *, int);

//boards.d

void check_white_list(board_t *);
void check_black_list(board_t *);

#endif

