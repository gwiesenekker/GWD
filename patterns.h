// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef PatternsH
#define PatternsH

enum
{
  NPATTERNS_MAX = 32,
  NMATERIALS_MAX = 20,
  NLINEAR_MAX = 16
};

#define PATTERN_BASE3_INDEX_CNAME "base3_index"

enum
{
  PATTERN_BASE3_INDEX_ID = 1
};

enum
{
  MATERIAL_KING_MAX = 5
};

#define MATERIAL_NWHITE_MEN_CNAME "nwhite_man"
#define MATERIAL_NBLACK_MEN_CNAME "nblack_man"
#define MATERIAL_NWHITE_KINGS_CNAME "nwhite_king"
#define MATERIAL_NBLACK_KINGS_CNAME "nblack_king"
#define MATERIAL_NWHITE_KINGS20_CNAME "nwhite_king20"
#define MATERIAL_NBLACK_KINGS20_CNAME "nblack_king20"
#define MATERIAL_MAN_KING_CNAME "man_king"

enum
{
  MATERIAL_NWHITE_MEN_ID = 1,
  MATERIAL_NBLACK_MEN_ID = 2,
  MATERIAL_NWHITE_KINGS_ID = 3,
  MATERIAL_NBLACK_KINGS_ID = 4,
  MATERIAL_NWHITE_KINGS20_ID = 5,
  MATERIAL_NBLACK_KINGS20_ID = 6,
  MATERIAL_MAN_KING_ID = 7
};

typedef struct
{
  bstring PS_bname;
  int PS_id;
  my_uint64_t PS_bb;
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
  material_shared_t MSS_materials[NMATERIALS_MAX];
} materials_shared_t;

void construct_patterns_shared(patterns_shared_t *arg_with_patterns_shared,
                               const char *arg_shape);

#endif
