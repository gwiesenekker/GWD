//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#ifndef NetworksH
#define NetworksH

//network.c

#define NETWORK_KING_WEIGHT_ARES        -2
#define NETWORK_KING_WEIGHT_GWD         -3

#define NETWORK_ACTIVATION_RELU         4
#define NETWORK_ACTIVATION_CLIPPED_RELU 5
#define NETWORK_ACTIVATION_LINEAR       6
#define NETWORK_ACTIVATION_SIGMOID      7

#define SCALED_DOUBLE_FACTOR 10000
#define SCALED_DOUBLE_MAX    L_MAX
#define SCALED_DOUBLE_MIN    L_MIN

typedef i32_t scaled_double_t;

#define DOUBLE2SCALED(D) (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2DOUBLE(S) ((double) (S) / SCALED_DOUBLE_FACTOR)

#define NINPUTS_MAX 8192
#define NLAYERS_MAX 8

typedef struct
{
  scaled_double_t *NS_layer0_inputs;
  i32_t *NS_layer0_sum;
} network_state_t;

typedef struct
{
  int L_ninputs;
  int L_noutputs;

  scaled_double_t **L_weights;
  scaled_double_t **L_weights_transposed;
  scaled_double_t *L_bias;
  i64_t *L_bias64;
  i64_t *L_dot64;
  scaled_double_t *L_sum;
  scaled_double_t *L_outputs;

  double **L_weights_double;
  double **L_weights_transposed_double;
  double *L_bias_double;
  double *L_sum_double;
  double *L_outputs_double;
} layer_t;

typedef struct
{  
  bstring N_bshape;

  int N_nwhite_king_input_map;
  int N_nblack_king_input_map;
   
  int N_ninputs;
  int N_loaded;

  double N_network2material_score;
  int N__king_weight;
  int N_clip_value;
  int N_activation_last;

  int N_nlayers;

  scaled_double_t *N_inputs;

  layer_t N_layers[NLAYERS_MAX];

  int N_nstate;
  network_state_t N_states[NODE_MAX];

  patterns_t *N_patterns;
} network_t;

void check_layer0(network_t *);
double return_network_score_scaled(network_t *, int, int);
double return_network_score_double(network_t *, int);
void push_network_state(network_t *);
void pop_network_state(network_t *);
void update_layer0(network_t *, int, scaled_double_t);
void construct_network(network_t *, int, int);
void board2network(board_t *, int);
void fen2network(char *, i64_t);
void fen2csv(char *, int, int, int);
void test_network(void);

#endif

