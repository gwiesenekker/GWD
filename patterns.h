// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#ifndef PatternsH
#define PatternsH

#define NPATTERNS_MAX 32
#define NMATERIAL_MAX 20
#define NSIGNALS_MAX 8
#define NLINEAR_MAX 16
#define NMASK2INPUTS_MAX (1 << NLINEAR_MAX)

#define EMBED_EMPTY 0
#define EMBED_WHITE_MAN 1
#define EMBED_BLACK_MAN 2

#define PATTERN_NKING_MAX 5

#define PATTERN_BASE3_INDEX_CNAME "base3_index"
#define PATTERN_NWHITE_MAN_CNAME "nwhite_man"
#define PATTERN_NBLACK_MAN_CNAME "nblack_man"
#define PATTERN_NWHITE_KING_CNAME "nwhite_king"
#define PATTERN_NBLACK_KING_CNAME "nblack_king"
#define PATTERN_WHITE_MOBILITY_CNAME "white_mobility"
#define PATTERN_BLACK_MOBILITY_CNAME "black_mobility"
#define PATTERN_WHITE_IMMOBILITY_CNAME "white_immobility"
#define PATTERN_BLACK_IMMOBILITY_CNAME "black_immobility"
#define PATTERN_NWHITE_KING_DIAGONAL_5_46_CNAME "nwhite_king_diagonal_5_46"
#define PATTERN_NBLACK_KING_DIAGONAL_5_46_CNAME "nblack_king_diagonal_5_46"

#define PATTERN_NULL_ID 0
#define PATTERN_BASE3_INDEX_ID 1
#define PATTERN_NWHITE_MAN_ID 2
#define PATTERN_NBLACK_MAN_ID 3
#define PATTERN_NWHITE_KING_ID 4
#define PATTERN_NBLACK_KING_ID 5
#define PATTERN_WHITE_MOBILITY_ID 6
#define PATTERN_BLACK_MOBILITY_ID 7
#define PATTERN_WHITE_IMMOBILITY_ID 8
#define PATTERN_BLACK_IMMOBILITY_ID 9
#define PATTERN_NWHITE_KING_DIAGONAL_5_46_ID 10
#define PATTERN_NBLACK_KING_DIAGONAL_5_46_ID 11

#define MATERIAL_KING_MAX 5

#define MATERIAL_NWHITE_MAN_CNAME "nwhite_man"
#define MATERIAL_NBLACK_MAN_CNAME "nblack_man"
#define MATERIAL_NWHITE_KING_CNAME "nwhite_king"
#define MATERIAL_NBLACK_KING_CNAME "nblack_king"
#define MATERIAL_MAN_KING_CNAME "man_king"
#define MATERIAL_WHITE_MOBILITY_CNAME "white_mobility"
#define MATERIAL_BLACK_MOBILITY_CNAME "black_mobility"
#define MATERIAL_WHITE_IMMOBILITY_CNAME "white_immobility"
#define MATERIAL_BLACK_IMMOBILITY_CNAME "black_immobility"
#define MATERIAL_WHITE_UNSUPPORTED_CNAME "white_unsupported"
#define MATERIAL_BLACK_UNSUPPORTED_CNAME "black_unsupported"

#define MATERIAL_NULL_ID 0
#define MATERIAL_NWHITE_MAN_ID 1
#define MATERIAL_NBLACK_MAN_ID 2
#define MATERIAL_NWHITE_KING_ID 3
#define MATERIAL_NBLACK_KING_ID 4
#define MATERIAL_MAN_KING_ID 5
#define MATERIAL_WHITE_MOBILITY_ID 6
#define MATERIAL_BLACK_MOBILITY_ID 7
#define MATERIAL_WHITE_IMMOBILITY_ID 8
#define MATERIAL_BLACK_IMMOBILITY_ID 9
#define MATERIAL_WHITE_UNSUPPORTED_ID 10
#define MATERIAL_BLACK_UNSUPPORTED_ID 11

#define SIGNAL_NULL_ID 0
#define SIGNAL_NWHITE_MAN_ID 1
#define SIGNAL_NBLACK_MAN_ID 2
#define SIGNAL_NWHITE_KING_ID 3
#define SIGNAL_NBLACK_KING_ID 4
#define SIGNAL_NPIECES_ID 5
#define SIGNAL_NWHITE_KING_DIAGONAL_5_46_ID 6
#define SIGNAL_NBLACK_KING_DIAGONAL_5_46_ID 7
#define SIGNAL_24_ID 8
#define SIGNAL_27_ID 9

typedef struct
{
  bstring PS_bname;
  int PS_id;
  ui64_t PS_bb;
  int PS_nlinear;
  int PS_nstates;
  int PS_nembed;
  int PS_offset;
  scaled_double_t **PS_weights_nstatesxnembed;
  scaled_double_t **PS_sum_nstatesxnoutputs;
} pattern_shared_t;

typedef struct
{
  bstring MS_bname;
  int MS_id;
  int MS_nstates;
  int MS_nembed;
  int MS_offset;
  scaled_double_t **MS_weights_nembedxnstates;
  scaled_double_t **MS_weights_nstatesxnembed;
  scaled_double_t **MS_sum_nstatesxnoutputs;
} material_shared_t;

typedef struct
{
  bstring SS_bname;
  int SS_id;
  int SS_offset;
} signal_shared_t;

// plural

typedef struct
{
  int PSS_npatterns;
  pattern_shared_t PSS_patterns[NPATTERNS_MAX];
} patterns_shared_t;

typedef struct
{
  int MSS_nmaterials;
  material_shared_t MSS_materials[NMATERIAL_MAX];
} materials_shared_t;

typedef struct
{
  int SS_nsignals;
  signal_shared_t SS_signals[NSIGNALS_MAX];
} signals_shared_t;

extern ui64_t row_bb[10];
extern int row_bb_ctz[10];

extern ui64_t diagonal_5_46_bb;

float return_signal_24(board_t *);
float return_signal_27(board_t *);

void construct_patterns_shared(patterns_shared_t *, char *);

#endif
