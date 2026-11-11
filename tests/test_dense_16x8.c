/* Exercise the real implementation; discard unrelated engine code at link time. */
#ifndef NETWORKS_TEST_SOURCE
#define NETWORKS_TEST_SOURCE "../networks.c"
#endif
#include NETWORKS_TEST_SOURCE
#include <assert.h>

int zzzzzz_invocation;
_Noreturn void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

static uint32_t rng = 1;
static float sample(void)
{
  rng = rng * 1664525u + 1013904223u;
  return ldexpf((float)((int)(rng >> 16) - 32768), -(int)(rng % 20));
}

static void select_kernel(dense_network_shared_t *dense, int enabled)
{
  int ninputs = dense->DNS_layers[0].LS_ninputs;
  int noutputs = dense->DNS_layers[0].LS_noutputs;
  dense->DNS_16x8_kernel = enabled && ninputs == 16 && noutputs == 8;
  dense->DNS_32x8_kernel = enabled && ninputs == 32 && noutputs == 8;
}

static void test_shape(int ninputs, int noutputs)
{
  dense_network_shared_t dense = {0};
  network_state_t state = {0};
  dense_network_state_t *thread = &state.NS_pattern_dense_network_state;
  ALIGN64(float weights_storage[16 * 72 + 8]);
  ALIGN64(float input_storage[72]);
  ALIGN64(float dot[16]);
  ALIGN64(float sum[16]);
  ALIGN64(float output[16]);
  ALIGN64(float bias[16]);
  float reference[16];
  size_t output_bytes = noutputs * sizeof(float);
  dense.DNS_layers[0].LS_ninputs = ninputs;
  dense.DNS_layers[0].LS_noutputs = noutputs;
  dense.DNS_layers[0].LS_bias = bias;
  dense.DNS_nlayers = 1;
  dense.DNS_activation = NETWORK_ACTIVATION_LINEAR;
  thread->DNS_layers[0].LT_dot = dot;
  thread->DNS_layers[0].LT_sum = sum;
  thread->DNS_layers[0].LT_outputs = output;

  for (int iteration = 0; iteration < 10000; iteration++)
  {
    /* Vary alignment of the inputs and contiguous weight matrix. */
    float *inputs = input_storage + iteration % 8;
    float *weights = weights_storage + iteration % 8;
    dense.DNS_layers[0].LS_weights_noutputsxninputs = weights;
    for (int i = 0; i < ninputs; i++) inputs[i] = sample();
    for (int o = 0; o < noutputs; o++)
    {
      for (int i = 0; i < ninputs; i++) weights[o * ninputs + i] = sample();
      bias[o] = sample();
    }
    select_kernel(&dense, FALSE);
    dense_layer0_dot(&dense, inputs, reference);
    select_kernel(&dense, TRUE);
    dense_layer0_dot(&dense, inputs, dot);
    assert(memcmp(reference, dot, output_bytes) == 0);

    memcpy(thread->DNS_inputs, inputs, ninputs * sizeof(float));
    for (int activate = 0; activate <= 1; activate++)
    {
      dense.DNS_activate_layer0 = activate;
      dense.DNS_activation_inputs = NETWORK_ACTIVATION_GELUV2;
      dense.DNS_compute_layer0 = TRUE;
      select_kernel(&dense, FALSE);
      (void)return_dense_network_score_scaled(&dense, thread);
      memcpy(reference, output, output_bytes);
      select_kernel(&dense, TRUE);
      (void)return_dense_network_score_scaled(&dense, thread);
      assert(memcmp(reference, output, output_bytes) == 0);
    }
    /* CONCAT can supply precomputed dots: never overwrite those. */
    dense.DNS_activate_layer0 = FALSE;
    dense.DNS_compute_layer0 = FALSE;
    for (int o = 0; o < noutputs; o++) reference[o] = dot[o] = sample();
    (void)return_dense_network_score_scaled(&dense, thread);
    assert(memcmp(reference, dot, output_bytes) == 0);
  }
  printf("%dx%d: 10000 bitwise comparisons, activated/plain dispatch and precomputed dots PASS\n",
         ninputs, noutputs);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  test_shape(16, 8);
  test_shape(32, 8);
  return 0;
}
