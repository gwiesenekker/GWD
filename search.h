// SCU REVISION 8.1181 zo  8 feb 2026 10:04:41 CET
#ifndef SearchH
#define SearchH

#define DEPTH_MAX 127

#define NPV_SLOTS 4
#define NPV_ENTRIES 64

#define MINIMAL_WINDOW_BIT BIT(0)
#define PV_BIT BIT(1)
#define GE_BETA_BIT BIT(2)
#define LE_ALPHA_BIT BIT(3)
#define TRUE_SCORE_BIT BIT(4)

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
#define IS_SR_EGTB_DRAW(X) ((X) & SR_EGTB_DRAW_BIT)
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
  int SR_score;
  int SR_flags;
  int SR_depth;
  pv_entries_t SR_pv;
} search_result_t;

typedef struct
{
  my_printf_t *S_my_printf;
  void *S_thread;

  board_t S_board;

  my_timer_t S_timer;

  my_random_t S_random;

  cache_t S_endgame_entry_cache;
  cache_t S_endgame_wdl_entry_cache;

  int S_root_simple_score;
  int S_root_score;
  int S_best_score;
  int S_best_score_kind;
  int S_best_move;
  int S_best_depth;

  int S_best_score_by_depth[DEPTH_MAX];

  i64_t S_total_move_repetitions;

  i64_t S_total_quiescence_nodes;
  i64_t S_total_quiescence_all_moves_captures_only;
  i64_t S_total_quiescence_all_moves_le2_moves;
  i64_t S_total_quiescence_all_moves_combination;
  i64_t S_total_quiescence_all_moves_extended;

  i64_t S_total_nodes;
  i64_t S_total_alpha_beta_nodes;
  i64_t S_total_capture_nodes;
  i64_t S_total_minimal_window_nodes;
  i64_t S_total_pv_nodes;

  i64_t S_total_quiescence_extension_searches;
  i64_t S_total_quiescence_extension_searches_combination;
  i64_t S_total_quiescence_extension_searches_le_alpha;
  i64_t S_total_quiescence_extension_searches_ge_beta;
  i64_t S_total_quiescence_capture_extensions;

  i64_t S_total_pv_extension_searches;
  i64_t S_total_pv_extension_searches_combination;
  i64_t S_total_pv_extension_searches_le_alpha;
  i64_t S_total_pv_extension_searches_ge_beta;

  i64_t S_total_reductions_combinations;
  i64_t S_total_reductions;
  i64_t S_total_reductions_lost;
  i64_t S_total_reductions_le_alpha;
  i64_t S_total_reductions_ge_beta;

  i64_t S_total_single_reply_extensions;

  i64_t S_total_evaluations;
  i64_t S_total_lazy_alpha_evaluations;
  i64_t S_total_lazy_beta_evaluations;
  i64_t S_total_material_only_evaluations;
  i64_t S_total_network_evaluations;

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
  i64_t S_total_alpha_beta_cache_crc64_errors;

  i64_t S_total_score_cache_hits;
  i64_t S_total_score_cache_crc32_errors;
} search_t;

int draw_by_repetition(board_t *, int);
void clear_totals(search_t *);
void print_totals(search_t *);
void do_search(search_t *, moves_list_t *, int, int, int, my_random_t *);
void clear_caches(void);
void construct_search(void *, my_printf_t *, void *);
void init_search(void);
void fin_search(void);

#endif
