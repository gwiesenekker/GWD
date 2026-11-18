// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef MainH
#define MainH

#include "common.h"
#include "cJSON.h"
#include "bstrlib.h"
#include "my_random.h"
#include "queues.h"

// options

enum
{
  GAME_MODE = 1
};

enum
{
  NTROUBLE_MAX = 1024
};

typedef struct
{
  bstring hub_version;

  bstring overrides;

  int verbose;

  bstring network0;
  bstring network1;

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
  bstring book_name;
  int book_randomness;
  int book_position_frequency;
  int book_move_frequency;
  int book_evaluation_time;
  int book_minimax;

  int ponder;

  int material_only;

  int mobility_moves_threshold;

  int best_score_margin;
  int aspiration_margin;
  int research_margin;

  int use_quiescence;
  int use_reductions;
  int reduction_moves_threshold;
  int reduction_depth_root;
  int reduction_depth_leaf;
  int reduction_full_min;
  int reduction_percentage;
  int reduction_margin;

  int prob_cut_depth;
  int prob_cut_percentage;
  int prob_cut_margin;

  int singular_probe_depth;
  int singular_moves_threshold;
  int singular_probe_percentage;
  int singular_probe_margin;
  int singular_reduction;

  int sync_clock;

  my_int64_t alpha_beta_cache_size;
  int cache_draw_results;

  my_int64_t score_cache_size;

  int nthreads_alpha_beta;

  int lazy_smp_policy;

  bstring egtb_dir;
  bstring egtb_dir_wdl;
  bstring egtb_ram;
  bstring egtb_ram_wdl;

  my_int64_t egtb_entry_cache_size;

  bstring dxp_server_ip;
  int dxp_port;
  int dxp_initiator;
  int dxp_game_time;
  double dxp_game_increment;
  int dxp_game_moves;
  int dxp_games;
  bstring dxp_ballot;
  int dxp_annotate_level;
  int dxp_strict_gameend;
  bstring dxp_tag;

  int nthreads;
  int mode;

  int hub;
} options_t;

extern options_t options;

extern my_random_t main_random;

extern queue_t main_queue;

extern cJSON *gwd_json;

void results2csv(int arg_nwon, int arg_ndraw, int arg_nlost, int arg_nunknown);
int main(int argc, char **argv);

#endif
