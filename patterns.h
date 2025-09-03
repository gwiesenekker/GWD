//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef PatternsH
#define PatternsH

#define NPATTERNS_MAX    256
#define NLINEAR_MAX      16
#define NMASK2INPUTS_MAX (1 << NLINEAR_MAX)

#define EMBED_EMPTY     0
#define EMBED_WHITE_MAN 1
#define EMBED_BLACK_MAN 2

typedef struct
{
  int PS_nlinear;
  int PS_field2linear[BOARD_MAX];
  int PS_nstates;
  int PS_nembed;
  int PS_offset;
  scaled_double_t **PS_weights_nstatesxnembed;
  scaled_double_t **PS_sum_nstatesxnoutputs;
} pattern_shared_t;

typedef struct
{
  i32_t PT_embed[NLINEAR_MAX];
} pattern_thread_t;

typedef struct
{
  int MS_nstates;
  int MS_nembed; 
  int MS_offset;
  scaled_double_t **MS_weights_nembedxnstates;
  scaled_double_t **MS_weights_nstatesxnembed;
  scaled_double_t **MS_sum;
} material_shared_t;

typedef struct
{
  int PS_npatterns;
  int *PS_patterns_map[BOARD_MAX];
  pattern_shared_t PS_patterns[NPATTERNS_MAX];
} patterns_shared_t;

typedef struct
{
  pattern_thread_t PT_patterns[NPATTERNS_MAX];
} patterns_thread_t;

extern patterns_shared_t patterns_shared;

void construct_patterns_shared(patterns_shared_t *, char *);

void board2patterns_thread(board_t *);

void check_board_patterns_thread(board_t *, char *where, int);

#endif

