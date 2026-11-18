// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef SearchH
#define SearchH

enum
{
  DEPTH_MAX = 127
};

enum
{
  NPV_SLOTS = 4,
  NPV_ENTRIES = 64
};

_Static_assert((NPV_ENTRIES > 0) and ((NPV_ENTRIES & (NPV_ENTRIES - 1)) == 0),
               "PV entry count must be a power of two");

#define MINIMAL_WINDOW_BIT BIT(0)
#define PV_BIT BIT(1)

static inline unsigned int is_minimal_window(unsigned int arg_flags)
{
  return (arg_flags & MINIMAL_WINDOW_BIT);
}

static inline unsigned int is_pv(unsigned int arg_flags)
{
  return (arg_flags & PV_BIT);
}

enum
{
  SEARCH_BEST_SCORE_EGTB = 0,
  SEARCH_BEST_SCORE_AB = 1
};

#define SR_NULL 0
#define SR_DRAW_BY_REP_BIT BIT(0)
#define SR_LE_ALPHA_BIT BIT(1)
#define SR_GE_BETA_BIT BIT(2)
#define SR_TRUE_SCORE_BIT BIT(3)
#define SR_EGTB_DRAW_BIT BIT(4)
#define SR_WON_BIT BIT(5)
#define SR_LOST_BIT BIT(6)

static inline unsigned int is_sr_draw_by_rep(unsigned int arg_flags)
{
  return (arg_flags & SR_DRAW_BY_REP_BIT);
}

static inline unsigned int is_sr_le_alpha(unsigned int arg_flags)
{
  return (arg_flags & SR_LE_ALPHA_BIT);
}

static inline unsigned int is_sr_ge_beta(unsigned int arg_flags)
{
  return (arg_flags & SR_GE_BETA_BIT);
}

static inline unsigned int is_sr_true_score(unsigned int arg_flags)
{
  return (arg_flags & SR_TRUE_SCORE_BIT);
}

static inline unsigned int is_sr_won(unsigned int arg_flags)
{
  return (arg_flags & SR_WON_BIT);
}

static inline unsigned int is_sr_lost(unsigned int arg_flags)
{
  return (arg_flags & SR_LOST_BIT);
}

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
  my_uint64_t PVE_slots[NPV_SLOTS];
} pv_entry_t;

typedef struct
{
  pv_entry_t PVE_entries[NPV_ENTRIES];
} pv_entries_t;

typedef struct
{
  score_t SR_raw_score;
  score_t SR_score;
  uint32_t SR_flags;
  int SR_depth;
  pv_entries_t SR_pv;
} search_result_t;

typedef enum
{
  DTM_ROOT_AND_PV,
  DTM_ROOT_ONLY,
  DTM_ROOT_IF_OPEN
} dtm_policy_t;

typedef struct
{
  my_printf_t *S_my_printf;
  void *S_thread;
  int S_network_id;
  dtm_policy_t S_dtm_policy;

  board_t S_board;

  my_timer_t S_timer;

  my_random_t S_random;

  // history heuristic: [colour][from][to], cleared at the start of a search
  int S_history[2][HISTORY_NFIELDS][HISTORY_NFIELDS];

  score_t S_root_simple_score;
  score_t S_root_score;
  score_t S_best_score;
  int S_best_score_kind;
  int S_best_move;
  int S_best_depth;
  pv_entries_t S_best_pv;

  my_int64_t S_ntotal_move_repetitions;

  my_int64_t S_ntotal_nodes;
  my_int64_t S_ntotal_alpha_beta_nodes;
  my_int64_t S_ntotal_alpha_beta_single_move_nodes;
  my_int64_t S_ntotal_capture_nodes;
  my_int64_t S_ntotal_minimal_window_nodes;
  my_int64_t S_ntotal_pv_nodes;
  my_int64_t S_ntotal_quiescence_nodes;
  my_int64_t S_ntotal_quiescence_single_move_nodes;

  my_int64_t S_ntotal_reductions;
  my_int64_t S_ntotal_lost_reductions;
  my_int64_t S_ntotal_le_alpha_reductions;
  my_int64_t S_ntotal_ge_beta_reductions;

  my_int64_t S_ntotal_prob_cut_probes;
  my_int64_t S_ntotal_prob_cut_le_alpha_probes;
  my_int64_t S_ntotal_prob_cut_ge_beta_probes;
  my_int64_t S_ntotal_prob_cut_nodes;
  my_int64_t S_ntotal_prob_cut_le_alpha_nodes;
  my_int64_t S_ntotal_prob_cut_ge_beta_nodes;
  my_int64_t S_ntotal_prob_cut_aborted_probes;
  my_int64_t S_ntotal_prob_cut_aborted_nodes;

  my_int64_t S_ntotal_singular_extension_probes;
  my_int64_t S_ntotal_singular_extension_le_alpha_probes;
  my_int64_t S_ntotal_singular_extension_ge_beta_probes;
  my_int64_t S_ntotal_singular_extensions;
  my_int64_t S_ntotal_singular_reductions;

  my_int64_t S_ntotal_evaluations;
  my_int64_t S_ntotal_lazy_alpha_evaluations;
  my_int64_t S_ntotal_lazy_beta_evaluations;
  my_int64_t S_ntotal_material_only_evaluations;
  my_int64_t S_ntotal_network_evaluations;

  my_int64_t S_ntotal_alpha_beta_cache_updates;
  my_int64_t S_ntotal_alpha_beta_cache_aged_entries;
  my_int64_t S_ntotal_alpha_beta_cache_hits;
  my_int64_t S_ntotal_alpha_beta_cache_depth_hits;
  my_int64_t S_ntotal_alpha_beta_cache_le_alpha_hits;
  my_int64_t S_ntotal_alpha_beta_cache_le_alpha_cutoffs;
  my_int64_t S_ntotal_alpha_beta_cache_ge_beta_hits;
  my_int64_t S_ntotal_alpha_beta_cache_ge_beta_cutoffs;
  my_int64_t S_ntotal_alpha_beta_cache_true_score_hits;
  my_int64_t S_ntotal_alpha_beta_cache_le_alpha_stored_entries;
  my_int64_t S_ntotal_alpha_beta_cache_ge_beta_stored_entries;
  my_int64_t S_ntotal_alpha_beta_cache_true_score_stored_entries;
  my_int64_t S_ntotal_alpha_beta_cache_nmoves_errors;
  my_int64_t S_ntotal_alpha_beta_cache_probes;
  my_int64_t S_ntotal_alpha_beta_cache_memory_hits;
  my_int64_t S_ntotal_alpha_beta_cache_local_hits;
  my_int64_t S_ntotal_alpha_beta_cache_crc_errors;

  my_int64_t S_ntotal_score_cache_hits;
  my_int64_t S_ntotal_score_cache_crc_errors;

  GwdegtbWdlProbe *S_wdl_probe;
} search_t;

int draw_by_repetition(board_t *, int);
int move_repetition(board_t *);
void clear_totals(search_t *);
void print_totals(search_t *);
int probe_pv(pv_entries_t *, search_t *);
void do_search(search_t *, moves_list_t *, int, int, score_t, my_random_t *);
void clear_caches(void);
void construct_search(search_t *, my_printf_t *, void *, int);
void init_search(void);
void fin_search(void);

#endif
