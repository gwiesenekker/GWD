// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#ifndef PatternsH
#define PatternsH

#define NPATTERNS_MAX 32
#define NMATERIAL_MAX 20
#define NLINEAR_MAX 16

#define PATTERN_BASE3_INDEX_CNAME "base3_index"

#define PATTERN_BASE3_INDEX_ID 1

#define MATERIAL_KING_MAX 5

#define MATERIAL_NWHITE_MAN_CNAME "nwhite_man"
#define MATERIAL_NBLACK_MAN_CNAME "nblack_man"
#define MATERIAL_NWHITE_KING_CNAME "nwhite_king"
#define MATERIAL_NBLACK_KING_CNAME "nblack_king"
#define MATERIAL_NWHITE_KING20_CNAME "nwhite_king20"
#define MATERIAL_NBLACK_KING20_CNAME "nblack_king20"
#define MATERIAL_MAN_KING_CNAME "man_king"

#define MATERIAL_NWHITE_MAN_ID 1
#define MATERIAL_NBLACK_MAN_ID 2
#define MATERIAL_NWHITE_KING_ID 3
#define MATERIAL_NBLACK_KING_ID 4
#define MATERIAL_NWHITE_KING20_ID 5
#define MATERIAL_NBLACK_KING20_ID 6
#define MATERIAL_MAN_KING_ID 7

typedef struct
{
  bstring PS_bname;
  int PS_id;
  ui64_t PS_bb;
  int PS_nlinear;
  int PS_nstates;
  int PS_nembed;
  int PS_offset;
  scaled_double_t *PS_weights_nstatesxnembed;
  scaled_double_t **PS_sum_nstatesxnoutputs;
} pattern_shared_t;

typedef struct
{
  bstring MS_bname;
  int MS_id;
  int MS_nstates;
  int MS_nembed;
  int MS_offset;
  scaled_double_t *MS_weights_nstatesxnembed;
  scaled_double_t **MS_sum_nstatesxnoutputs;
} material_shared_t;

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

void construct_patterns_shared(patterns_shared_t *, char *);

#endif
