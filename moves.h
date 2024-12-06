//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef MovesH
#define MovesH

#define MOVES_MAX       128
#define MOVE_STRING_MAX 128

#define MOVE_DO_NOT_REDUCE_BIT        BIT(0)
#define MOVE_EXTEND_IN_QUIESCENCE_BIT BIT(1)

#define MOVE_DO_NOT_REDUCE(F)        ((F) & MOVE_DO_NOT_REDUCE_BIT)
#define MOVE_EXTEND_IN_QUIESCENCE(F) ((F) & MOVE_EXTEND_IN_QUIESCENCE_BIT)

typedef struct
{
  int move_from;
  int move_to;
  ui64_t move_captures_bb;
} move_t;

typedef struct
{
  int nmoves;
  int nblocked;
  int ncaptx;

  move_t moves[MOVES_MAX];

  int moves_weight[MOVES_MAX];
  int moves_flag[MOVES_MAX];
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

void move2bstring(void *, int, bstring);
int search_move(void *, bstring);

void gen_moves(board_t *, moves_list_t *, int);
void do_move(board_t *, int, moves_list_t *);
void undo_move(board_t *, int, moves_list_t *);
void check_moves(board_t *, moves_list_t *);

void construct_moves_list(void *);

void fprintf_moves_list(void *, my_printf_t *, int);

#endif

