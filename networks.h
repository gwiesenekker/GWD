//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#ifndef NetworksH
#define NetworksH

//network.c

#define NETWORK_ACTIVATION_RELU         10
#define NETWORK_ACTIVATION_CLIPPED_RELU 11
#define NETWORK_ACTIVATION_LINEAR       12
#define NETWORK_ACTIVATION_SIGMOID      13

#define NETWORK_LABEL_C2M               20
#define NETWORK_LABEL_W2M               21

#define NETWORK_COLOUR_BITS_0           30

#define NETWORK_OUTPUT_C2M              42
#define NETWORK_OUTPUT_W2M              43

#define SCALED_DOUBLE_FACTOR 10000
#define SCALED_DOUBLE_MAX L_MAX
#define SCALED_DOUBLE_MIN L_MIN

typedef i32_t scaled_double_t;

#define DOUBLE2SCALED(D) (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2DOUBLE(S) ((double) (S) / SCALED_DOUBLE_FACTOR)

#define NINPUTS_MAX  8192
#define NLAYERS_MAX  8

#define SCORE2TANH(S) tanh((S) * 0.6 / SCORE_MAN)

typedef struct
{
  scaled_double_t *NS_layer0_inputs;
  i32_t *NS_layer0_sum;
} network_state_t;

typedef struct
{
  int layer_ninputs;
  int layer_noutputs;

  scaled_double_t **layer_weights;
  scaled_double_t **layer_weights_transposed;
  scaled_double_t *layer_bias;
  i64_t *layer_bias64;
  i64_t *layer_dot64;
  scaled_double_t *layer_sum;
  scaled_double_t *layer_outputs;

  double **layer_weights_double;
  double **layer_weights_transposed_double;
  double *layer_bias_double;
  double *layer_sum_double;
  double *layer_outputs_double;
} layer_t;

typedef struct
{  
  bstring network_bshape;

  int nwhite_king_input_map;
  int nblack_king_input_map;
   
  int network_ninputs;
  int network_loaded;

  double network2material_score;
  int network_clip;
  int network_activation_last;
  int network_npieces_min;
  int network_npieces_max;

  int network_nlayers;

  scaled_double_t *network_inputs;

  layer_t network_layers[NLAYERS_MAX];

  int network_nstate;
  network_state_t network_states[NODE_MAX];

  patterns_t *network_patterns;
} network_t;

void check_layer0(network_t *);
double return_network_score_scaled(network_t *, int, int);
double return_network_score_double(network_t *, int);
void push_network_state(network_t *);
void pop_network_state(network_t *);
void update_layer0(network_t *, int, scaled_double_t);
void construct_network(network_t *, char *, int, int);
void board2network(board_t *, int);
void fen2network(char *, i64_t);
void fen2csv(char *);
void test_network(void);

#endif

