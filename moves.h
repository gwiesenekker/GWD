//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#ifndef MovesH
#define MovesH

#define MOVES_MAX       128
#define MOVE_STRING_MAX 128

#define ROW9_CAPTURE_BIT BIT(0)
#define PROMOTION_BIT    BIT(1)

typedef struct
{
  int move_from;
  int move_to;
  ui64_t move_captures_bb;
} move_t;

typedef struct moves_list
{
  int nmoves;

  move_t moves[MOVES_MAX];

  int moves_weight[MOVES_MAX];
  i8_t moves_tactical[MOVES_MAX];

  int ncaptx;

  char move_string[MOVE_STRING_MAX];

  void (*new_moves_list)(struct moves_list *);

  //TODO change order of moves_list and imove?
  char * (*move2string)(struct moves_list *, int);
  void (*fprintf_moves)(my_printf_t *, struct moves_list *, int);
  void (*printf_moves)(struct moves_list *, int);
} moves_list_t;

//moves.c

#define gen_the_moves(X) cat3(gen_, X, _moves)
#define gen_my_moves     gen_the_moves(my_colour)
#define gen_your_moves   gen_the_moves(your_colour)

#define the_can_capture(X) cat2(X, _can_capture)
#define i_can_capture     the_can_capture(my_colour)
#define you_can_capture   the_can_capture(your_colour)

#define do_the_move(X) cat3(do_, X, _move)
#define do_my_move     do_the_move(my_colour)
#define do_your_move   do_the_move(your_colour)

#define undo_the_move(X) cat3(undo_, X, _move)
#define undo_my_move     undo_the_move(my_colour)
#define undo_your_move   undo_the_move(your_colour)

//typedef i8_t move_t;

//moves.d
void gen_white_moves(board_t *, moves_list_t *, int);
void gen_black_moves(board_t *, moves_list_t *, int);
int white_can_capture(board_t *, int);
int black_can_capture(board_t *, int);
void do_white_move(board_t *, int, moves_list_t *);
void do_black_move(board_t *, int, moves_list_t *);
void undo_white_move(board_t *, int, moves_list_t *);
void undo_black_move(board_t *, int, moves_list_t *);
void check_white_moves(board_t *, moves_list_t *);
void check_black_moves(board_t *, moves_list_t *);

void gen_moves(board_t *, moves_list_t *, int);
void do_move(board_t *, int, moves_list_t *);
void undo_move(board_t *, int, moves_list_t *);
void check_moves(board_t *, moves_list_t *);
//TODO move search_move to moves_list
int search_move(moves_list_t *, char *);
void create_moves_list(moves_list_t *);

#endif

