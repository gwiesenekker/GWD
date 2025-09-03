//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef NetworksH
#define NetworksH

//network.c

#define NETWORK_EMBEDDING_SUM           1
#define NETWORK_EMBEDDING_SUM2          2
#define NETWORK_EMBEDDING_CONCAT        3

#define NETWORK_ACTIVATION_RELU6        3
#define NETWORK_ACTIVATION_TANH         4
#define NETWORK_ACTIVATION_RSQRT        5
#define NETWORK_ACTIVATION_LINEAR       6
#define NETWORK_ACTIVATION_SIGMOID      7

#define NINPUTS_MAX 2048
#define NLAYERS_MAX 8

typedef struct
{
  int LS_ninputs;
  int LS_noutputs;

  scaled_double_t **LS_weights_noutputsxninputs;
  scaled_double_t **LS_weights_ninputsxnoutputs;
  scaled_double_t *LS_bias;
} layer_shared_t;

typedef struct
{
  scaled_double_t *LT_dot;
  scaled_double_t *LT_sum;
  scaled_double_t *LT_outputs;
} layer_thread_t;

typedef struct
{  
  bstring NS_bshape;

  int NS_ninputs_patterns;
  int NS_ninputs_material;
  int NS_ninputs;
  int NS_loaded;

  double NS_network2material_score;
  int NS_embedding;
  int NS_activation;
  int NS_activation_last;
  int NS_nman_min;  
  int NS_nman_max;

  material_shared_t NS_material[4];

  int NS_nlayers;

  layer_shared_t NS_layers[NLAYERS_MAX];
} network_shared_t;

typedef struct
{
  patterns_thread_t NT_patterns;
  scaled_double_t *NT_inputs;
  layer_thread_t NT_layers[NLAYERS_MAX];
} network_thread_t;

extern int load_network;
extern network_shared_t network_shared;

void construct_network_shared(network_shared_t *, int);
void construct_network_thread(network_thread_t *, int);
int base3_index(const int32_t *, int);
void vadd_aba(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vcopy_ab(int n, scaled_double_t *, scaled_double_t *b);
double return_network_score_scaled(network_thread_t *);
double return_network_score_double(network_thread_t *);
void board2network(board_t *);
void fen2network(char *, i64_t);
void fen2csv(char *, int, int, int, int);
void init_networks(void);

#endif

