// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#ifndef MovesH
#define MovesH

#define NMOVES_MAX 128

#define MOVE_DO_NOT_REDUCE_BIT BIT(0)
#define MOVE_EXTEND_IN_QUIESCENCE_BIT BIT(1)

#define MOVE_DO_NOT_REDUCE(F) ((F) & MOVE_DO_NOT_REDUCE_BIT)
#define MOVE_EXTEND_IN_QUIESCENCE(F) ((F) & MOVE_EXTEND_IN_QUIESCENCE_BIT)

typedef struct
{
  int M_move_from;
  int M_move_to;
  ui64_t M_captures_bb;
} move_t;

typedef struct
{
  int ML_nmoves;
  int ML_ncaptx;

  move_t ML_moves[NMOVES_MAX];

  ui32_t ML_move_flags[NMOVES_MAX];
} moves_list_t;

void gen_moves(board_t *, moves_list_t *);
void gen_moves_legacy(board_t *, moves_list_t *);
int can_capture(board_t *, int);
void do_move(board_t *, int, moves_list_t *);
void undo_move(board_t *, int, moves_list_t *);
void check_moves(board_t *, moves_list_t *);

void move2bstring(moves_list_t *, int, bstring);
int search_move(moves_list_t *, bstring);

void construct_moves_list(moves_list_t *);

void fprintf_moves_list(moves_list_t *, my_printf_t *, int);

#endif
