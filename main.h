//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef MainH
#define MainH

//options

#define GAME_MODE  1

#define NTROUBLE_MAX 1024

typedef struct
{
  char hub_version[MY_LINE_MAX];

  char overrides[MY_LINE_MAX];

  int verbose;

  char networks[MY_LINE_MAX];
  char network_name[MY_LINE_MAX];

  int network_evaluation_min;
  int network_evaluation_max;
  int network_evaluation_time;

  double time_limit;
  int time_control_ntrouble;
  int time_control_mean;
  int time_control_sigma;
  int time_control_method;

  //TO DO this should be moved out of options

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

  int egtb_level;

  int material_only;

  int captures_are_transparent;
  int returned_depth_includes_captures;

  int quiescence_evaluation_policy;
  int quiescence_combination_length;
  int quiescence_extension_search_window;
  int quiescence_capture_extensions;

  int pv_combination_length;
  int pv_extension_search_window;

  int aspiration_window;
  int best_score_margin;

  int use_reductions;

  int reduction_depth_root;
  int reduction_depth_leaf;
  int reduction_moves_min;
  int reduction_combination_length;

  int reduction_full_min;
  int reduction_strength;
  int reduction_probes;
  int reduction_probe_window;
  int reduction_research_margin;

  int use_single_reply_extensions;

  int sync_clock;

  i64_t alpha_beta_cache_size;
  int alpha_beta_cache_depth_margin;
  int pv_cache_fraction;
  int pv2cache;

  i64_t score_cache_size;

  int nthreads_alpha_beta;

  int lazy_smp_policy;

  char ipc_dir[MY_LINE_MAX];
 
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

