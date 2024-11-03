//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
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

  char neural0_name[MY_LINE_MAX];
  char neural1_name[MY_LINE_MAX];

  int neural_evaluation_min;
  int neural_evaluation_max;
  int neural_evaluation_time;

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

  int ponder;

  int egtb_level;

  int material_only;

  int captures_are_transparent;
  int returned_depth_includes_captures;

  int use_reductions;

  int reduction_depth_root;
  int reduction_depth_leaf;

  int reduction_mean;
  int reduction_sigma;

  int reduction_delta;
  int reduction_max;
  int reduction_strong;
  int reduction_weak;
  int reduction_min;

  int row9_captures_are_tactical;
  int promotions_are_tactical;

  int use_single_reply_extensions;

  int sync_clock;

  i64_t alpha_beta_cache_size;
  int pv_cache_fraction;

  int nthreads_alpha_beta;

  int lazy_smp_policy;

  int nslaves;

  char ipc_dir[MY_LINE_MAX];
 
  char egtb_dir[MY_LINE_MAX];
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
  int dxp_book;
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

