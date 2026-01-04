//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
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
#define STARTING_POSITION2FEN                                                 \
  "W:W31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50:B01,02,03," \
  "04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20"

#define BOARD_INTERRUPT_TIME BIT(0)
#define BOARD_INTERRUPT_MESSAGE BIT(1)

#define IS_WHITE(X) ((X) == WHITE_ENUM)
#define IS_BLACK(X) ((X) == BLACK_ENUM)

#define IS_EMPTY(X) ((X) == 0)
#define IS_MAN(X) ((X) == MAN_ENUM)
#define IS_KING(X) ((X) == KING_ENUM)

#define B_white_man_bb B_bit_boards[WHITE_ENUM][MAN_ENUM]
#define B_black_man_bb B_bit_boards[BLACK_ENUM][MAN_ENUM]
#define B_white_king_bb B_bit_boards[WHITE_ENUM][KING_ENUM]
#define B_black_king_bb B_bit_boards[BLACK_ENUM][KING_ENUM]

#define my_man_bb B_bit_boards[my_colour][MAN_ENUM]
#define your_man_bb B_bit_boards[your_colour][MAN_ENUM]

#define my_king_bb B_bit_boards[my_colour][KING_ENUM]
#define your_king_bb B_bit_boards[your_colour][KING_ENUM]

#define my_man_key hash_keys[my_colour][MAN_ENUM]
#define your_man_key hash_keys[your_colour][MAN_ENUM]

#define my_king_key hash_keys[my_colour][KING_ENUM]
#define your_king_key hash_keys[your_colour][KING_ENUM]

#define PV_MAX 256

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

typedef i8_t pv_t;

typedef struct
{
  int node_ncaptx;
  hash_key_t node_move_key;
} node_t;

typedef struct
{
  ui64_t BS_key;
  ui64_t BS_bit_boards[NCOLOUR_ENUM][NPIECE_ENUM];
  int BS_npieces;
} board_state_t;

typedef struct board
{
  my_printf_t *B_my_printf;

  ui64_t B_valid_bb;

  colour_enum B_colour2move;

  ui64_t B_bit_boards[NCOLOUR_ENUM][NPIECE_ENUM];

  hash_key_t B_key;

  int B_inode;
  int B_root_inode;

  node_t B_nodes[NODE_MAX];

  network_thread_t B_network_thread;

  char B_string[MY_LINE_MAX];

  int B_nstate;
  board_state_t B_states[NODE_MAX];
} board_t;

extern int square2field[1 + 50];

extern int field2square[BOARD_MAX];

extern char *nota[BOARD_MAX];

extern int white_row[BOARD_MAX];
extern int black_row[BOARD_MAX];

extern hash_key_t hash_keys[NCOLOUR_ENUM][NPIECE_ENUM][BOARD_MAX];

void construct_board(void *, my_printf_t *);
void push_board_state(void *self);
void pop_board_state(void *self);

hash_key_t return_key_from_bb(void *);
void string2board(board_t *, char *);
void fen2board(board_t *, char *);
void board2fen(board_t *, bstring, int);
void print_board(board_t *);
void init_boards();
void state2board(board_t *, game_state_t *);
char *board2string(board_t *, int);

#endif
