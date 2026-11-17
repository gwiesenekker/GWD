/* Exercise the real implementation; discard unrelated engine code at link time.
 */
#ifndef NETWORKS_TEST_SOURCE
#define NETWORKS_TEST_SOURCE "../networks.c"
#endif
#include NETWORKS_TEST_SOURCE
#include <assert.h>

int zzzzzz_invocation;
_Noreturn void zzzzzz(const char *arg_file,
                      const char *arg_func,
                      long arg_line,
                      const char *arg_message,
                      int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

static uint32_t rng = 1;
static float sample(void)
{
  rng = rng * 1664525u + 1013904223u;

  return ldexpf((float)((int)(rng >> 16) - 32768), -(int)(rng % 20));
}

static void select_kernel(dense_network_shared_t *arg_dense, int arg_enabled)
{
  int ninputs = arg_dense->DNS_layers[0].LS_ninputs;
  int noutputs = arg_dense->DNS_layers[0].LS_noutputs;

  arg_dense->DNS_16x8_kernel = arg_enabled && ninputs == 16 && noutputs == 8;
  arg_dense->DNS_32x8_kernel = arg_enabled && ninputs == 32 && noutputs == 8;
}

static void test_shape(int arg_ninputs, int arg_noutputs)
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
  size_t output_bytes = arg_noutputs * sizeof(float);

  dense.DNS_layers[0].LS_ninputs = arg_ninputs;
  dense.DNS_layers[0].LS_noutputs = arg_noutputs;
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

    for (int iinput = 0; iinput < arg_ninputs; iinput++)
      inputs[iinput] = sample();

    for (int ioutput = 0; ioutput < arg_noutputs; ioutput++)
    {
      for (int iinput = 0; iinput < arg_ninputs; iinput++)
        weights[ioutput * arg_ninputs + iinput] = sample();
      bias[ioutput] = sample();
    }
    select_kernel(&dense, FALSE);
    dense_layer0_dot(&dense, inputs, reference);
    select_kernel(&dense, TRUE);
    dense_layer0_dot(&dense, inputs, dot);

    assert(memcmp(reference, dot, output_bytes) == 0);

    memcpy(thread->DNS_inputs, inputs, arg_ninputs * sizeof(float));

    for (int activate = 0; activate <= 1; activate++)
    {
      dense.DNS_activate_layer0 = activate;
      dense.DNS_input_activation = NETWORK_ACTIVATION_GELUV2;
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

    for (int ioutput = 0; ioutput < arg_noutputs; ioutput++)
      reference[ioutput] = dot[ioutput] = sample();
    (void)return_dense_network_score_scaled(&dense, thread);

    assert(memcmp(reference, dot, output_bytes) == 0);
  }
  printf("%dx%d: 10000 bitwise comparisons, activated/plain dispatch and "
         "precomputed dots PASS\n",
         arg_ninputs,
         arg_noutputs);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  test_shape(16, 8);
  test_shape(32, 8);

  return 0;
}
