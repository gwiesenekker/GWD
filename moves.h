//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef MovesH
#define MovesH

#define NMOVES_MAX 128
#define MOVE_STRING_MAX 128

#define MOVE_DO_NOT_REDUCE_BIT BIT(0)
#define MOVE_EXTEND_IN_QUIESCENCE_BIT BIT(1)
#define MOVE_KING_BIT BIT(2)

#define MOVE_DO_NOT_REDUCE(F) ((F) & MOVE_DO_NOT_REDUCE_BIT)
#define MOVE_EXTEND_IN_QUIESCENCE(F) ((F) & MOVE_EXTEND_IN_QUIESCENCE_BIT)
#define MOVE_KING(F) ((F) & MOVE_KING_BIT)

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

void gen_moves(board_t *, moves_list_t *);
int can_capture(board_t *, int);
void do_move(board_t *, int, moves_list_t *, int);
void undo_move(board_t *, int, moves_list_t *, int);
void check_moves(board_t *, moves_list_t *);

void move2bstring(void *, int, bstring);
int search_move(void *, bstring);

void construct_moves_list(void *);

void fprintf_moves_list(void *, my_printf_t *, int);

#endif
