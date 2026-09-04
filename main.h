// SCU REVISION 8.12462 vr  4 sep 2026  7:33:55 CEST
#ifndef MainH
#define MainH

// options

#define GAME_MODE 1

#define NTROUBLE_MAX 1024

typedef struct
{
  char hub_version[MY_LINE_MAX];

  char overrides[MY_LINE_MAX];

  int verbose;

  char network0[MY_LINE_MAX];
  char network1[MY_LINE_MAX];

  int network_evaluation_min;
  int network_evaluation_max;
  int network_evaluation_time;

  double time_limit;
  int time_control_ntrouble;
  int time_control_mean;
  int time_control_sigma;
  int time_control_method;

  // TO DO this should be moved out of options

  int time_ntrouble;
  double time_trouble[NTROUBLE_MAX];

  int wall_time_threshold;

  int depth_limit;

  int use_book;
  char book_name[MY_LINE_MAX];
  int book_randomness;
  int book_position_frequency;
  int book_move_frequency;
  int book_evaluation_time;
  int book_minimax;

  int ponder;

  int material_only;

  int mobility_moves_threshold;

  int pv_combination_length;

  int best_score_margin;
  int aspiration_margin;
  int research_margin;

  int use_quiescence;
  int use_reductions;
  int reduction_moves_threshold;
  int reduction_depth_root;
  int reduction_depth_leaf;
  int reduction_combination_length;
  int reduction_full_min;
  int reduction_percentage;
  int reduction_margin;

  int singular_probe_depth;
  int singular_moves_threshold;
  int singular_probe_percentage;
  int singular_probe_margin;

  int sync_clock;

  i64_t alpha_beta_cache_size;
  int cache_draw_results;

  i64_t score_cache_size;

  int nthreads_alpha_beta;

  int lazy_smp_policy;

  char egtb_dir[MY_LINE_MAX];
  char egtb_dir_wdl[MY_LINE_MAX];
  char egtb_ram[MY_LINE_MAX];
  char egtb_ram_wdl[MY_LINE_MAX];

  i64_t egtb_entry_cache_size;

  char dxp_server_ip[MY_LINE_MAX];
  int dxp_port;
  int dxp_initiator;
  int dxp_game_time;
  int dxp_game_moves;
  int dxp_games;
  char dxp_ballot[MY_LINE_MAX];
  int dxp_annotate_level;
  int dxp_strict_gameend;
  char dxp_tag[MY_LINE_MAX];

  int hub_server_game_moves;
  int hub_server_game_time;
  int hub_server_games;
  char hub_server_ballot[MY_LINE_MAX];
  char hub_server_client[MY_LINE_MAX];
  int hub_annotate_level;

  int nthreads;
  int mode;

  int hub;
} options_t;

extern options_t options;

extern my_random_t main_random;

extern queue_t main_queue;

extern cJSON *gwd_json;

void results2csv(int, int, int, int);
int main(int argc, char **argv);

#endif
