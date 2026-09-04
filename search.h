// SCU REVISION 8.12464 vr  4 sep 2026  8:14:53 CEST
#ifndef SearchH
#define SearchH

#define DEPTH_MAX 127

#define NPV_SLOTS 4
#define NPV_ENTRIES 64

#define MINIMAL_WINDOW_BIT BIT(0)
#define PV_BIT BIT(1)

#define IS_MINIMAL_WINDOW(X) ((X) & MINIMAL_WINDOW_BIT)
#define IS_PV(X) ((X) & PV_BIT)

#define SEARCH_BEST_SCORE_EGTB 0
#define SEARCH_BEST_SCORE_AB 1

#define SR_NULL 0
#define SR_DRAW_BY_REP_BIT BIT(0)
#define SR_LE_ALPHA_BIT BIT(1)
#define SR_GE_BETA_BIT BIT(2)
#define SR_TRUE_SCORE_BIT BIT(3)
#define SR_EGTB_DRAW_BIT BIT(4)
#define SR_WON_BIT BIT(5)
#define SR_LOST_BIT BIT(6)

#define IS_SR_DRAW_BY_REP(X) ((X) & SR_DRAW_BY_REP_BIT)
#define IS_SR_LE_ALPHA(X) ((X) & SR_LE_ALPHA_BIT)
#define IS_SR_GE_BETA(X) ((X) & SR_GE_BETA_BIT)
#define IS_SR_TRUE_SCORE(X) ((X) & SR_TRUE_SCORE_BIT)
#define IS_SR_WON(X) ((X) & SR_WON_BIT)
#define IS_SR_LOST(X) ((X) & SR_LOST_BIT)

#define PV_SLOT_WHITE_MOVE_MASK 0xFFULL
#define PV_SLOT_BLACK_MOVE_MASK 0xFF00ULL
#define PV_SLOT_MOVES_MASK (PV_SLOT_WHITE_MOVE_MASK | PV_SLOT_BLACK_MOVE_MASK)

typedef enum
{
  SR_OPERATOR_LT = 0,
  SR_OPERATOR_LE,
  SR_OPERATOR_EQ,
  SR_OPERATOR_GE,
  SR_OPERATOR_GT
} search_result_operator_enum;

typedef struct
{
  ui64_t PVE_slots[NPV_SLOTS];
} pv_entry_t;

typedef struct
{
  pv_entry_t PVE_entries[NPV_ENTRIES];
} pv_entries_t;

typedef struct
{
  score_t SR_raw_score;
  score_t SR_score;
  ui32_t SR_flags;
  int SR_depth;
  pv_entries_t SR_pv;
} search_result_t;

typedef struct
{
  my_printf_t *S_my_printf;
  void *S_thread;
  int S_network_id;

  board_t S_board;

  my_timer_t S_timer;

  my_random_t S_random;

  score_t S_root_simple_score;
  score_t S_root_score;
  score_t S_best_score;
  int S_best_score_kind;
  int S_best_move;
  int S_best_depth;
  pv_entries_t S_best_pv;

  i64_t S_total_move_repetitions;

  i64_t S_total_nodes;
  i64_t S_total_alpha_beta_nodes;
  i64_t S_total_alpha_beta_1move;
  i64_t S_total_capture_nodes;
  i64_t S_total_minimal_window_nodes;
  i64_t S_total_pv_nodes;
  i64_t S_total_quiescence_nodes;
  i64_t S_total_quiescence_1move;

  i64_t S_total_pv_extension_searches_combination;

  i64_t S_total_reductions_combinations;
  i64_t S_total_reductions;
  i64_t S_total_reductions_lost;
  i64_t S_total_reductions_le_alpha;
  i64_t S_total_reductions_ge_beta;

  i64_t S_total_singular_extension_probes;
  i64_t S_total_singular_extension_probes_le_alpha;
  i64_t S_total_singular_extension_probes_ge_beta;
  i64_t S_total_singular_extensions;

  i64_t S_total_evaluations;
  i64_t S_total_lazy_alpha_evaluations;
  i64_t S_total_lazy_beta_evaluations;
  i64_t S_total_material_only_evaluations;
  i64_t S_total_network_evaluations;

  i64_t S_total_alpha_beta_cache_updates;
  i64_t S_total_alpha_beta_cache_aged;
  i64_t S_total_alpha_beta_cache_hits;
  i64_t S_total_alpha_beta_cache_depth_hits;
  i64_t S_total_alpha_beta_cache_le_alpha_hits;
  i64_t S_total_alpha_beta_cache_le_alpha_cutoffs;
  i64_t S_total_alpha_beta_cache_ge_beta_hits;
  i64_t S_total_alpha_beta_cache_ge_beta_cutoffs;
  i64_t S_total_alpha_beta_cache_true_score_hits;
  i64_t S_total_alpha_beta_cache_le_alpha_stored;
  i64_t S_total_alpha_beta_cache_ge_beta_stored;
  i64_t S_total_alpha_beta_cache_true_score_stored;
  i64_t S_total_alpha_beta_cache_nmoves_errors;
  i64_t S_total_alpha_beta_cache_nprobes;
  i64_t S_total_alpha_beta_cache_memory_hits;
  i64_t S_total_alpha_beta_cache_local_hits;
  i64_t S_total_alpha_beta_cache_crc_errors;

  i64_t S_total_score_cache_hits;
  i64_t S_total_score_cache_crc_errors;

  GwdegtbWdlProbe *S_wdl_probe;
} search_t;

int draw_by_repetition(board_t *, int);
int move_repetition(board_t *);
void clear_totals(search_t *);
void print_totals(search_t *);
void do_search(search_t *, moves_list_t *, int, int, score_t, my_random_t *);
void clear_caches(void);
void construct_search(search_t *, my_printf_t *, void *, int);
void init_search(void);
void fin_search(void);

#endif
