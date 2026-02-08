// SCU REVISION 8.1178 zo  8 feb 2026  6:06:03 CET
#ifndef PatternsH
#define PatternsH

#define NPATTERNS_MAX 32
#define NMATERIAL_MAX 8
#define NLINEAR_MAX 16
#define NMASK2INPUTS_MAX (1 << NLINEAR_MAX)

#define EMBED_EMPTY 0
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
  int MG_nstates;
  int MG_nembed;
  int MG_offset;
  scaled_double_t **MG_weights_nembedxnstates;
  scaled_double_t **MG_weights_nstatesxnembed;
  scaled_double_t **MG_sum_nstatesxnoutputs;
} material_shared_t;

// plural

typedef struct
{
  int PSS_npatterns;
  int *PSS_patterns_map[BOARD_MAX];
  pattern_shared_t PSS_patterns[NPATTERNS_MAX];
} patterns_shared_t;

typedef struct
{
  pattern_thread_t PT_patterns[NPATTERNS_MAX];
} patterns_thread_t;

extern patterns_shared_t patterns_shared;

typedef struct
{
  int MSS_nmaterial;
  material_shared_t MSS_materials[NMATERIAL_MAX];
} materials_global_t;

extern materials_global_t materials_global;

void construct_patterns_shared(patterns_shared_t *, char *);

void board2patterns_thread(board_t *);

void check_board_patterns_thread(board_t *, char *, int);

#endif
