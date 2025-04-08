//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#ifndef PatternsH
#define PatternsH

#define MASK_WHITE_MAN 01
#define MASK_BLACK_MAN 02
#define MASK_EMPTY     03

#define NPATTERNS_MAX    256
#define NLINEAR_MAX      5
#define NMASK2INPUTS_MAX (1 << (2 * NLINEAR_MAX))

typedef struct
{
  int P_root_square;
  int P_nlinear;
  int P_valid_mask;
  ui64_t P_square2linear;
  int P_mask2inputs[NMASK2INPUTS_MAX];
} pattern_t;

typedef struct
{
  int PMS_mask[NPATTERNS_MAX];
} pattern_mask_state_t;

typedef struct
{
  int PM_npatterns;
  int PM_mask[NPATTERNS_MAX];
  int PM_nstates;
  pattern_mask_state_t PM_states[NODE_MAX];
} pattern_mask_t;

typedef struct
{
  int npatterns;
  int *patterns_map[BOARD_MAX];
  pattern_t patterns[NPATTERNS_MAX];
} patterns_t;

int construct_patterns(patterns_t *, char *);

void push_pattern_mask_state(pattern_mask_t *self);
void pop_pattern_mask_state(pattern_mask_t *self);

void board2patterns(board_t *);

void check_board_patterns(board_t *, char *where);

#endif
