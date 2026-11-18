// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef MovesH
#define MovesH

enum
{
  NMOVES_MAX = 128
};

#define MOVE_DO_NOT_REDUCE_BIT BIT(0)
#define MOVE_EXTEND_IN_QUIESCENCE_BIT BIT(1)

static inline uint32_t move_do_not_reduce(uint32_t arg_flags)
{
  return (arg_flags & MOVE_DO_NOT_REDUCE_BIT);
}

static inline uint32_t move_extend_in_quiescence(uint32_t arg_flags)
{
  return (arg_flags & MOVE_EXTEND_IN_QUIESCENCE_BIT);
}

typedef struct
{
  int M_move_from;
  int M_move_to;
  my_uint64_t M_captures_bb;
} move_t;

typedef struct
{
  int ML_nmoves;
  int ML_ncaptx;

  move_t ML_moves[NMOVES_MAX];

  uint32_t ML_move_flags[NMOVES_MAX];
} moves_list_t;

void gen_moves(board_t *arg_with_board, moves_list_t *arg_moves_list);
void gen_moves_legacy(board_t *arg_with_board, moves_list_t *arg_moves_list);
int can_capture(const board_t *arg_with_board, int arg_kings);
void do_move(board_t *arg_with_board,
             int arg_imove,
             const moves_list_t *arg_moves_list);
void undo_move(board_t *arg_with_board,
               int arg_imove,
               const moves_list_t *arg_moves_list);
void check_moves(board_t *arg_with_board, moves_list_t *arg_moves_list);

void move2bstring(const moves_list_t *arg_with_moves_list,
                  int arg_imove,
                  bstring arg_bmove_string);
int search_move(const moves_list_t *arg_with_moves_list,
                const_bstring arg_bmove);

void construct_moves_list(moves_list_t *arg_with_moves_list);

void fprintf_moves_list(const moves_list_t *arg_with_moves_list,
                        my_printf_t *arg_my_printf,
                        int arg_verbose);

#endif
