// SCU REVISION 8.1178 zo  8 feb 2026  6:06:03 CET
#ifndef NetworksH
#define NetworksH

// network.c

#define NETWORK_ARCHITECTURE_NULL 0
#define NETWORK_ARCHITECTURE_ADD 1
#define NETWORK_ARCHITECTURE_CONCAT 2

#define NETWORK_ACTIVATION_RELU6 1
#define NETWORK_ACTIVATION_TANH 2
#define NETWORK_ACTIVATION_RSQRT 3
#define NETWORK_ACTIVATION_GELU 4
#define NETWORK_ACTIVATION_GELUV2 5
#define NETWORK_ACTIVATION_LINEAR 6
#define NETWORK_ACTIVATION_SIGMOID 7

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
  int DNS_ninputs;
  int DNS_nlayers;
  int DNS_activate_layer0;
  int DNS_compute_layer0;
  layer_shared_t DNS_layers[NLAYERS_MAX];
} dense_network_shared_t;

typedef struct
{
  ALIGN64(scaled_double_t DNT_inputs[NINPUTS_MAX]);
  layer_thread_t DNT_layers[NLAYERS_MAX];
} dense_network_thread_t;

typedef struct
{
  bstring NG_bshape;

  int NG_loaded;

  double NG_network2material_score;
  int NG_architecture;
  int NG_activation_inputs;
  int NG_activation;
  int NG_activation_last;
  int NG_nman_min;
  int NG_nman_max;

  dense_network_shared_t NG_pattern_dense_network_shared;
  dense_network_shared_t NG_material_dense_network_shared;
  dense_network_shared_t NG_dense_network_shared;
} network_global_t;

typedef struct
{
  patterns_thread_t NT_patterns;
  dense_network_thread_t NT_pattern_dense_network_thread;
  dense_network_thread_t NT_material_dense_network_thread;
  dense_network_thread_t NT_dense_network_thread;
} network_thread_t;

extern network_global_t network_global;

void construct_network_shared(int);
void construct_network_thread(network_thread_t *, int);
int base3_index(const int32_t *, int);
void vcopy_a2b(int n, scaled_double_t *, scaled_double_t *);
void vadd_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2c(int,
               scaled_double_t *restrict,
               scaled_double_t *restrict,
               scaled_double_t *restrict);
double return_network_score_scaled(dense_network_shared_t *,
                                   dense_network_thread_t *);
double return_network_score_double(dense_network_shared_t *,
                                   dense_network_thread_t *);
void fen2network(char *, i64_t);
void fen2csv(char *, int, int);
void init_networks(void);

#endif
