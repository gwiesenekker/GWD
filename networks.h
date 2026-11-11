// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#ifndef NetworksH
#define NetworksH

// networks.c

#define NETWORKS_MAX 2
#define NETWORK0_ID 0
#define NETWORK1_ID 1

#define NETWORK_ARCHITECTURE_NULL 0
#define NETWORK_ARCHITECTURE_ADD 1
#define NETWORK_ARCHITECTURE_CONCAT 2

#define NETWORK_ACTIVATION_RELU 1
#define NETWORK_ACTIVATION_TANH 2
#define NETWORK_ACTIVATION_RSQRT 3
#define NETWORK_ACTIVATION_GELU 4
#define NETWORK_ACTIVATION_GELUV2 5
#define NETWORK_ACTIVATION_LINEAR 6
#define NETWORK_ACTIVATION_LOGITS 7
#define NETWORK_ACTIVATION_2HEADS 8

#define NINPUTS_MAX 2048
#define NLAYERS_MAX 8

typedef struct
{
  int LS_ninputs;
  int LS_noutputs;

  scaled_double_t *LS_weights_noutputsxninputs;

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
  int DNS_activation_inputs;
  int DNS_activation;
  int DNS_activation_last;
  int DNS_ninputs;
  int DNS_nlayers;
  int DNS_activate_layer0;
  int DNS_compute_layer0;
  int DNS_16x8_kernel; // first dense layer has 16 inputs and 8 outputs
  int DNS_32x8_kernel; // first dense layer has 32 inputs and 8 outputs
  layer_shared_t DNS_layers[NLAYERS_MAX];
} dense_network_shared_t;

typedef struct
{
  ALIGN64(scaled_double_t DNS_inputs[NINPUTS_MAX]);
  layer_thread_t DNS_layers[NLAYERS_MAX];
} dense_network_state_t;

_Static_assert(_Alignof(dense_network_state_t) == 64,
               "dense network state must retain 64-byte alignment");
_Static_assert(offsetof(dense_network_state_t, DNS_inputs) % 64 == 0,
               "network inputs must be 64-byte aligned");

typedef struct
{
  int N_network_id;

  bstring N_bshape;

  int N_network2material_score;
  int N_architecture_patterns;
  int N_architecture_material;
  int N_activation_inputs;
  int N_activation;
  int N_activation_last;
  int N_nman_min;
  int N_nman_max;

  patterns_shared_t N_patterns_shared;
  materials_shared_t N_materials_shared;

  dense_network_shared_t N_pattern_dense_network_shared;
  int N_pattern_dense_network_16x8_kernel;
  int N_pattern_dense_network_64x16_kernel; // embedding summation only
  dense_network_shared_t N_material_dense_network_shared;
  int N_material_dense_network_16x8_kernel;
  int N_material_dense_network_64x16_kernel; // embedding summation only
  scaled_double_t *N_material_cache; // immutable final material output vectors
  dense_network_shared_t N_dense_network_shared;
} network_t;

typedef struct
{
  int NS_network_id;
  dense_network_state_t NS_pattern_dense_network_state;
  dense_network_state_t NS_material_dense_network_state;
  dense_network_state_t NS_dense_network_state;
} network_state_t;

_Static_assert(_Alignof(network_state_t) == 64,
               "network state must retain 64-byte alignment");

extern network_t networks[NETWORKS_MAX];

void construct_network_state(network_state_t *, int, int);
int base3_index(const int32_t *, int);
int base3_index_v2(board_t *, ui64_t);
int base3_index_v3(board_t *, ui64_t);
int base3_index_v4(board_t *, ui64_t);

void vcopy_a2b(int n, scaled_double_t *, scaled_double_t *);
void vadd_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2a(int, scaled_double_t *restrict, scaled_double_t *restrict);
void vmul_ab2c(int,
               scaled_double_t *restrict,
               scaled_double_t *restrict,
               scaled_double_t *restrict);
double return_dense_network_score_scaled(dense_network_shared_t *,
                                         dense_network_state_t *);
double return_dense_network_score_double(dense_network_shared_t *,
                                         dense_network_state_t *);
void evaluate_material_network(network_t *,
                               dense_network_state_t *,
                               int,
                               int,
                               int,
                               int,
                               int);
void fen2network(char *, i64_t);
void fen2csv(char *, int, int);
void init_networks(void);
void test_networks(void);

#endif
