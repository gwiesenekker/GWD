//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef SearchH
#define SearchH

#define DEPTH_MAX 127

#define MINIMAL_WINDOW_BIT BIT(0)
#define PV_BIT BIT(1)
#define GE_BETA_BIT BIT(2)
#define LE_ALPHA_BIT BIT(3)
#define TRUE_SCORE_BIT BIT(4)

#define IS_MINIMAL_WINDOW(X) ((X) & MINIMAL_WINDOW_BIT)
#define IS_PV(X) ((X) & PV_BIT)

#define SEARCH_BEST_SCORE_EGTB 0
#define SEARCH_BEST_SCORE_AB 1

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
  pv_t S_best_pv[PV_MAX];

  int S_best_score_by_depth[DEPTH_MAX];

  int S_interrupt;

  i64_t S_total_move_repetitions;

  i64_t S_total_quiescence_nodes;
  i64_t S_total_quiescence_all_moves_captures_only;
  i64_t S_total_quiescence_all_moves_le2_moves;
  i64_t S_total_quiescence_all_moves_combination;
  i64_t S_total_quiescence_all_moves_extended;

  i64_t S_total_nodes;
  i64_t S_total_alpha_beta_nodes;
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
