//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef MovesH
#define MovesH

#define NMOVES_MAX       128
#define MOVE_STRING_MAX 128

#define MOVE_DO_NOT_REDUCE_BIT        BIT(0)
#define MOVE_EXTEND_IN_QUIESCENCE_BIT BIT(1)
#define MOVE_KING_BIT                 BIT(2)

#define MOVE_DO_NOT_REDUCE(F)        ((F) & MOVE_DO_NOT_REDUCE_BIT)
#define MOVE_EXTEND_IN_QUIESCENCE(F) ((F) & MOVE_EXTEND_IN_QUIESCENCE_BIT)
#define MOVE_KING(F)                 ((F) & MOVE_KING_BIT)

typedef struct
{
  int M_from;
  int M_move_to;
  ui64_t M_captures_bb;
} move_t;

typedef struct
{
  int ML_nmoves;
  int ML_ncaptx;

  int ML_nblocked;
  int ML_nsafe;
  move_t ML_moves[NMOVES_MAX];

  int ML_move_weights[NMOVES_MAX];
  int ML_move_flags[NMOVES_MAX];
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
void do_white_move(board_t *, int, moves_list_t *, int);
void do_black_move(board_t *, int, moves_list_t *, int);
void undo_white_move(board_t *, int, moves_list_t *, int);
void undo_black_move(board_t *, int, moves_list_t *, int);
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

