// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef NetworksH
#define NetworksH

#include "common.h"
#include "patterns.h"

// networks.c

enum
{
  NETWORKS_MAX = 2,
  NETWORK0_ID = 0,
  NETWORK1_ID = 1
};

enum
{
  NETWORK_ARCHITECTURE_NULL = 0,
  NETWORK_ARCHITECTURE_ADD = 1,
  NETWORK_ARCHITECTURE_CONCAT = 2
};

enum
{
  NETWORK_ACTIVATION_RELU = 1,
  NETWORK_ACTIVATION_TANH = 2,
  NETWORK_ACTIVATION_RSQRT = 3,
  NETWORK_ACTIVATION_GELU = 4,
  NETWORK_ACTIVATION_GELUV2 = 5,
  NETWORK_ACTIVATION_LINEAR = 6,
  NETWORK_ACTIVATION_LOGITS = 7,
  NETWORK_ACTIVATION_2HEADS = 8
};

enum
{
  NINPUTS_MAX = 2048,
  NLAYERS_MAX = 8
};

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
  int DNS_input_activation;
  int DNS_activation;
  int DNS_final_activation;
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
  int N_pattern_architecture;
  int N_material_architecture;
  int N_input_activation;
  int N_activation;
  int N_final_activation;
  int N_nmen_min;
  int N_nmen_max;

  patterns_shared_t N_shared_patterns;
  materials_shared_t N_shared_materials;

  dense_network_shared_t N_shared_pattern_dense_network;
  int N_pattern_dense_network_16x8_kernel;
  int N_pattern_dense_network_64x16_kernel; // embedding summation only
  dense_network_shared_t N_shared_material_dense_network;
  int N_material_dense_network_16x8_kernel;
  int N_material_dense_network_64x16_kernel; // embedding summation only
  scaled_double_t *N_material_cache; // immutable final material output vectors
  dense_network_shared_t N_shared_dense_network;
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

void construct_network_state(network_state_t *arg_with_network_state,
                             int arg_network_id,
                             int arg_verbose);
int base3_index_v2(const board_t *arg_board, my_uint64_t arg_pattern_bb);
int base3_index_v3(const board_t *arg_board, my_uint64_t arg_pattern_bb);
int base3_index_v4(const board_t *arg_board, my_uint64_t arg_pattern_bb);

void vcopy_a2b(int arg_nvalues,
               const scaled_double_t *arg_a,
               scaled_double_t *arg_b);
void vadd_ab2a(int arg_nvalues,
               scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b);
void vmul_ab2a(int arg_nvalues,
               scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b);
void vmul_ab2c(int arg_nvalues,
               const scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b,
               scaled_double_t *restrict arg_c);
double return_dense_network_score_scaled(
  const dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state);
double return_dense_network_score_double(
  const dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state);
void evaluate_material_network(const network_t *arg_with_network,
                               dense_network_state_t *arg_state,
                               int arg_nwhite_men,
                               int arg_nblack_men,
                               int arg_nwhite_kings,
                               int arg_nblack_kings,
                               int arg_inputs);
void fen2network(const char *arg_name, my_int64_t arg_npositions);
void fen2csv(const char *arg_name, int arg_npieces_min, int arg_npieces_max);
void init_networks(void);
void test_networks(void);

#endif
