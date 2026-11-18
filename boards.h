// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef BoardsH
#define BoardsH

enum
{
  BOARD_MAX = 66,
  NPIECES_MAX = 20
};

#define wO_hub "w"
#define wX_hub "W"
#define bO_hub "b"
#define bX_hub "B"
#define nn_hub "e"

#define wO "o"
#define wX "x"
#define bO "O"
#define bX "X"
#define nn "."

#define STARTING_POSITION "wOOOOOOOOOOOOOOOOOOOO..........oooooooooooooooooooo"
#define STARTING_POSITION2FEN                                                 \
  "W:W31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50:B01,02,03," \
  "04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20"

#define B_white_men_bb B_bit_boards[WHITE_ENUM][MAN_ENUM]
#define B_black_men_bb B_bit_boards[BLACK_ENUM][MAN_ENUM]
#define B_white_kings_bb B_bit_boards[WHITE_ENUM][KING_ENUM]
#define B_black_kings_bb B_bit_boards[BLACK_ENUM][KING_ENUM]

#define my_men_bb B_bit_boards[my_colour][MAN_ENUM]
#define your_men_bb B_bit_boards[your_colour][MAN_ENUM]

#define my_kings_bb B_bit_boards[my_colour][KING_ENUM]
#define your_kings_bb B_bit_boards[your_colour][KING_ENUM]

#define my_man_key hash_keys[my_colour][MAN_ENUM]
#define your_man_key hash_keys[your_colour][MAN_ENUM]

#define my_king_key hash_keys[my_colour][KING_ENUM]
#define your_king_key hash_keys[your_colour][KING_ENUM]

typedef enum
{
  WHITE_ENUM = 0,
  BLACK_ENUM = 1,
  NCOLOUR_ENUM
} colour_enum;
typedef enum
{
  MAN_ENUM = 0,
  KING_ENUM = 1,
  NPIECE_ENUM
} piece_enum;

static inline int is_white(int arg_colour)
{
  return (arg_colour == WHITE_ENUM);
}

static inline int is_black(int arg_colour)
{
  return (arg_colour == BLACK_ENUM);
}

typedef struct
{
  int node_ncaptx;
  hash_key_t node_move_key;
} node_t;

typedef struct
{
  my_uint64_t BS_key;
  my_uint64_t BS_bit_boards[NCOLOUR_ENUM][NPIECE_ENUM];
  int BS_npieces;
} board_state_t;

typedef struct board
{
  my_printf_t *B_my_printf;

  int B_network_id;

  my_uint64_t B_valid_bb;

  colour_enum B_colour2move;

  my_uint64_t B_bit_boards[NCOLOUR_ENUM][NPIECE_ENUM];

  hash_key_t B_key;

  int B_inode;
  int B_root_inode;

  node_t B_nodes[NODE_MAX];

  network_state_t B_network_state;

  int B_nstates;
  board_state_t B_states[NODE_MAX];
} board_t;

extern const int square2field[1 + 50];

extern const int field2square[BOARD_MAX];

extern const char *const nota[BOARD_MAX];

extern const int white_row[BOARD_MAX];
extern const int black_row[BOARD_MAX];

extern hash_key_t hash_keys[NCOLOUR_ENUM][NPIECE_ENUM][BOARD_MAX];

void construct_board(board_t *arg_with_board,
                     my_printf_t *arg_my_printf,
                     int arg_network_id);
void push_board_state(board_t *arg_with_board);
void pop_board_state(board_t *arg_with_board);

hash_key_t return_key_from_bb(const board_t *arg_with_board);
void string2board(board_t *arg_with_board, const char *arg_position);
void fen2board(board_t *arg_with_board, const char *arg_fen);
void board2fen(const board_t *arg_with_board,
               bstring arg_bfen,
               int arg_brackets);
void print_board(const board_t *arg_with_board);
void init_boards(void);
void state2board(board_t *arg_with_board, const game_state_t *arg_game);
char *
board2string(const board_t *arg_with_board, int arg_hub, bstring arg_string);

#endif
