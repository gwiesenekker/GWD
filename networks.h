//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
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

#define SCALED_DOUBLE_I32_T 1
#define SCALED_DOUBLE_I64_T 2

#define SCALED_DOUBLE_T SCALED_DOUBLE_I32_T

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
#define SCALED_DOUBLE_FACTOR 10000
#define SCALED_DOUBLE_MAX L_MAX
#define SCALED_DOUBLE_MIN L_MIN

typedef i32_t scaled_double_t;

#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
#define SCALED_DOUBLE_FACTOR 100000

#define SCALED_DOUBLE_MAX L_MAX
#define SCALED_DOUBLE_MIN L_MIN

typedef i64_t scaled_double_t;

#else
#error unknown SCALED_DOUBLE_T
#endif

#define DOUBLE2SCALED(D) (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2DOUBLE(S) ((double) (S) / SCALED_DOUBLE_FACTOR)

#define NINPUTS_MAX  384
#define NLAYERS_MAX  8

#define SCORE2TANH(S) tanh((S) * 0.6 / SCORE_MAN)

typedef struct
{
  scaled_double_t *NS_layer0_inputs;
  scaled_double_t *NS_layer0_sum;
} network_state_t;

typedef struct
{
  int layer_ninputs;
  int layer_noutputs;

  scaled_double_t **layer_weights;
  scaled_double_t **layer_weights_transposed;
  scaled_double_t *layer_bias;
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
  int white_man_input_map[BOARD_MAX];
  int black_man_input_map[BOARD_MAX];
  int empty_input_map[BOARD_MAX];

  int colour2move_input_map;

  int nmy_king_input_map;
  int nyour_king_input_map;

  int nleft_wing_input_map;
  int ncenter_input_map;
  int nright_wing_input_map;

  int nleft_half_input_map;
  int nright_half_input_map;

  int nblocked_input_map;

  double network2material_score;
  int network_input_map;
  int network_activation;
  int network_activation_last;
  int network_wings;
  int network_half;
  int network_blocked;
  int network_npieces_min;
  int network_npieces_max;

  int network_nlayers;

  scaled_double_t *network_inputs;

  layer_t network_layers[NLAYERS_MAX];

  int network_nstate;
  network_state_t network_states[NODE_MAX];
} network_t;

void check_layer0(network_t *);
double return_network_score_scaled(network_t *, int, int);
double return_network_score_double(network_t *, int);
void push_network_state(network_t *);
void pop_network_state(network_t *);
void update_layer0(network_t *, int, scaled_double_t);
void load_network(char *, network_t *, int);
void board2network(board_t *, network_t *, int);
void fen2network(char *);
void fen2bar(char *);
void test_network(void);

#endif

