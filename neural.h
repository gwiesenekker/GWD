//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef NeuralH
#define NeuralH

//neural.c

#define NEURAL_CROWN_WEIGHT 3

#define NEURAL_INPUT_MAP_V1            1
#define NEURAL_INPUT_MAP_V2            2
#define NEURAL_INPUT_MAP_V5            5
#define NEURAL_INPUT_MAP_V6            6

#define NEURAL_ACTIVATION_RELU         10
#define NEURAL_ACTIVATION_CLIPPED_RELU 11
#define NEURAL_ACTIVATION_LINEAR       12
#define NEURAL_ACTIVATION_SIGMOID      13

#define NEURAL_LABEL_C2M               20
#define NEURAL_LABEL_W2M               21

#define NEURAL_COLOUR_BITS_0           30

#define NEURAL_OUTPUT_C2M              42
#define NEURAL_OUTPUT_W2M              43

#define SCALED_DOUBLE_I16_T 1
#define SCALED_DOUBLE_I32_T 2
#define SCALED_DOUBLE_I64_T 3

#define SCALED_DOUBLE_T SCALED_DOUBLE_I16_T

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
typedef i16_t scaled_double_t;

#define SCALED_DOUBLE_FACTOR 1000
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
typedef i32_t scaled_double_t;

#define SCALED_DOUBLE_FACTOR 10000
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
typedef i64_t scaled_double_t;

#define SCALED_DOUBLE_FACTOR 100000
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
  int white_crown_input_map[BOARD_MAX];
  int black_man_input_map[BOARD_MAX];
  int black_crown_input_map[BOARD_MAX];
  int empty_input_map[BOARD_MAX];
  int c2m_input_map;

  double neural2material_score;
  int neural_input_map;
  int neural_activation;
  int neural_activation_last;
  int neural_label;
  int neural_colour_bits;
  int neural_output;
  int neural_npieces_min;
  int neural_npieces_max;

  int neural_nlayers;

  ALIGN64(scaled_double_t neural_inputs[NINPUTS_MAX]);

  int nlayer0_sum;
  scaled_double_t **layer0_sum;

  layer_t neural_layers[NLAYERS_MAX];
} neural_t;
//neural.c

void check_first_layer_sum(neural_t *);
double return_neural_score_scaled(neural_t *, int, int);
double return_neural_score_double(neural_t *, int);
void copy_first_layer_sum(neural_t *);
void restore_first_layer_sum(neural_t *);
void update_first_layer(neural_t *, int, scaled_double_t);
void load_neural(char *, neural_t *, int);
void board2neural(board_t *, neural_t *, int);
void fen2neural(char *);
void fen2search(char *);
void fen2csv(char *);
void test_neural(void);
void egtb2neural(void);

#endif

