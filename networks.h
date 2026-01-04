//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef NetworksH
#define NetworksH

// network.c

#define NETWORK_EMBEDDING_SUM 1
#define NETWORK_EMBEDDING_SUM2 2
#define NETWORK_EMBEDDING_SUM2PRODUCT 4
#define NETWORK_EMBEDDING_SUM2PRODUCTCONCAT 5
#define NETWORK_EMBEDDING_CONCAT 6

#define NETWORK_ACTIVATION_RELU6 7
#define NETWORK_ACTIVATION_TANH 8
#define NETWORK_ACTIVATION_RSQRT 9
#define NETWORK_ACTIVATION_GELU 10
#define NETWORK_ACTIVATION_GELUV2 11
#define NETWORK_ACTIVATION_LINEAR 12
#define NETWORK_ACTIVATION_SIGMOID 13

#define NINPUTS_MAX 2048
#define NMATERIAL_MAX 8
#define NLAYERS_MAX 8

#define COMBINED_KING_MAX 5
#define DELTA_MAN_MAX 10
#define DELTA_KING_MAX 5

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
  int NS_nmaterial;
  int NS_embedding;
  int NS_activation_inputs;
  int NS_activation;
  int NS_activation_last;
  int NS_nman_min;
  int NS_nman_max;

  material_shared_t NS_material[NMATERIAL_MAX];

  int NS_nlayers;

  layer_shared_t NS_layers[NLAYERS_MAX];
} network_shared_t;

typedef struct
{
  patterns_thread_t NT_patterns;
  ALIGN64(scaled_double_t NT_inputs[NINPUTS_MAX]);
  layer_thread_t NT_layers[NLAYERS_MAX];
} network_thread_t;

extern network_shared_t network_shared;

void construct_network_shared(network_shared_t *, int);
void construct_network_thread(network_thread_t *, int);
int base3_index(const int32_t *, int);
void vcopy_a2b(int n, scaled_double_t *, scaled_double_t *);
void vadd_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2c(int,
               scaled_double_t *restrict,
               scaled_double_t *restrict,
               scaled_double_t *restrict);
double return_network_score_scaled(network_thread_t *);
double return_network_score_double(network_thread_t *);
void fen2network(char *, i64_t);
void fen2csv(char *, int, int, int, int);
void init_networks(void);

#endif
