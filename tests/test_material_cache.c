/* Exercise material cache construction and lookup using aligned test storage. */
#include "../networks.c"
#include <assert.h>

int zzzzzz_invocation;
my_mpi_globals_t my_mpi_globals;

_Noreturn void zzzzzz(char *file, const char *func, long line,
                     char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

void *my_malloc(char *name, char *file, const char *func, int line, size_t size)
{
  (void)name; (void)file; (void)func; (void)line;
  void *pointer = NULL;
  assert(posix_memalign(&pointer, 64, size) == 0);
  return pointer;
}
void my_free(void *pointer) { free(pointer); }
void register_heap(char *name, char *file, const char *func, int line,
                   void *pointer, size_t size, int allocated, int readonly)
{
  (void)name; (void)file; (void)func; (void)line;
  (void)pointer; (void)size; (void)allocated; (void)readonly;
}
void my_mpi_allocate_shared(MPI_Comm comm, size_t size, size_t count,
                            void **pointer, MPI_Win *win)
{
  (void)comm;
  *pointer = malloc(size * count);
  assert(*pointer != NULL);
  memset(*pointer, 0xa5, size * count); /* Detect missing zero filling. */
  *win = 0;
}
void my_mpi_sync_shared(MPI_Comm comm, MPI_Win win, char *name)
{
  (void)comm; (void)win; (void)name;
}

void *STDOUT;
void my_printf(my_printf_t *out, char *format, ...)
{
  (void)out; (void)format;
}

static float *make_values(size_t count, int seed)
{
  float *values = my_malloc(NULL, NULL, NULL, 0, count * sizeof(float));
  for (size_t icell = 0; icell < count; icell++)
    values[icell] = (float)((int)((icell * 13 + seed) % 31) - 15) / 64.0f;
  return values;
}

static void test_cache(int nembed, int concat)
{
  network_t network = {0};
  network.N_network_id = NETWORK0_ID;
  network.N_architecture_material = concat ? NETWORK_ARCHITECTURE_CONCAT :
                                            NETWORK_ARCHITECTURE_ADD;
  network.N_material_dense_network_16x8_kernel = !concat && nembed == 16;
  network.N_material_dense_network_64x16_kernel = !concat && nembed == 64;
  network.N_materials_shared.MSS_nmaterials = concat ? 4 : 5;
  dense_network_shared_t *dense = &network.N_material_dense_network_shared;
  dense->DNS_ninputs = concat ? 4 * nembed : nembed;
  dense->DNS_nlayers = 2;
  dense->DNS_compute_layer0 = !concat;
  dense->DNS_activate_layer0 = !concat;
  dense->DNS_activation_inputs = NETWORK_ACTIVATION_GELUV2;
  dense->DNS_activation = NETWORK_ACTIVATION_GELUV2;
  dense->DNS_activation_last = NETWORK_ACTIVATION_LINEAR;
  dense->DNS_16x8_kernel = dense->DNS_ninputs == 16;
  const int ids[] = {MATERIAL_NWHITE_MAN_ID, MATERIAL_NBLACK_MAN_ID,
                     MATERIAL_NWHITE_KING_ID, MATERIAL_NBLACK_KING20_ID,
                     MATERIAL_MAN_KING_ID};
  const int counts[] = {21, 21, MATERIAL_KING_MAX + 1, 21,
                        21 * 21 * (MATERIAL_KING_MAX + 1) *
                        (MATERIAL_KING_MAX + 1)};
  for (int ilayer = 0; ilayer < 2; ilayer++)
  {
    layer_shared_t *layer = dense->DNS_layers + ilayer;
    layer->LS_ninputs = ilayer == 0 ? dense->DNS_ninputs :
                                                   dense->DNS_layers[0].LS_noutputs;
    layer->LS_noutputs = ilayer == 0 && nembed == 64 && !concat ? 16 : 8;
    layer->LS_weights_noutputsxninputs =
      make_values(layer->LS_ninputs * layer->LS_noutputs, 3);
    layer->LS_bias = make_values(layer->LS_noutputs, 1);
  }
  for (int imaterial = 0; imaterial < network.N_materials_shared.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *material = network.N_materials_shared.MSS_materials + imaterial;
    material->MS_id = ids[imaterial];
    material->MS_nstates = counts[imaterial];
    material->MS_nembed = nembed;
    material->MS_offset = concat ? imaterial * nembed : 0;
    material->MS_weights_nstatesxnembed = make_values(counts[imaterial] * nembed,
                                                       imaterial);
    if (concat)
    {
      material->MS_sum_nstatesxnoutputs = malloc(counts[imaterial] * sizeof(float *));
      for (int istate = 0; istate < counts[imaterial]; istate++)
      {
        material->MS_sum_nstatesxnoutputs[istate] = make_values(8, 0);
        for (int ioutput = 0; ioutput < 8; ioutput++)
          material->MS_sum_nstatesxnoutputs[istate][ioutput] = dot_scaled(
            nembed, material->MS_weights_nstatesxnembed + istate * nembed,
            dense->DNS_layers[0].LS_weights_noutputsxninputs +
              ioutput * dense->DNS_ninputs + material->MS_offset);
      }
    }
  }
  construct_material_cache(&network);
  dense_network_state_t state = {0};
  construct_dense_network_state(dense, &state);
  float reference[8];
  for (int iwhite_man = 0; iwhite_man <= 20; iwhite_man++)
    for (int iblack_man = 0; iblack_man <= 20; iblack_man++)
      for (int iwhite_king = 0; iwhite_king <= 20; iwhite_king++)
        for (int iblack_king = 0; iblack_king <= 20; iblack_king++)
        {
          evaluate_material_network(&network, &state, iwhite_man, iblack_man,
                                     iwhite_king, iblack_king, TRUE);
          memcpy(reference, state.DNS_layers[1].LT_outputs, sizeof(reference));
          for (int ioutput = 0; ioutput < 8; ioutput++)
            state.DNS_layers[1].LT_outputs[ioutput] = NAN;
          evaluate_material_network(&network, &state, iwhite_man, iblack_man,
                                     iwhite_king, iblack_king, FALSE);
          assert(memcmp(reference, state.DNS_layers[1].LT_outputs,
                        sizeof(reference)) == 0);
        }
  // An out-of-table king count uses the original clamping/evaluation path.
  evaluate_material_network(&network, &state, 0, 0, 21, 22, TRUE);
  memcpy(reference, state.DNS_layers[1].LT_outputs, sizeof(reference));
  evaluate_material_network(&network, &state, 0, 0, 21, 22, FALSE);
  assert(memcmp(reference, state.DNS_layers[1].LT_outputs, sizeof(reference)) == 0);
  for (int ilayer = 0; ilayer < 2; ilayer++)
  {
    free(state.DNS_layers[ilayer].LT_dot);
    free(state.DNS_layers[ilayer].LT_sum);
    free(state.DNS_layers[ilayer].LT_outputs);
    free(dense->DNS_layers[ilayer].LS_weights_noutputsxninputs);
    free(dense->DNS_layers[ilayer].LS_bias);
  }
  for (int imaterial = 0; imaterial < network.N_materials_shared.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *material = network.N_materials_shared.MSS_materials + imaterial;
    if (concat)
    {
      for (int istate = 0; istate < material->MS_nstates; istate++)
        free(material->MS_sum_nstatesxnoutputs[istate]);
      free(material->MS_sum_nstatesxnoutputs);
    }
    free(material->MS_weights_nstatesxnembed);
  }
  free(network.N_material_cache);
  printf("%s embedding width %d: all 194481 tuples bit-identical PASS\n",
         concat ? "CONCAT" : "ADD", nembed);
}

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  my_mpi_globals.MMG_is_master = TRUE;
  test_cache(16, FALSE);
  test_cache(64, FALSE);
  test_cache(4, TRUE);
  return 0;
}
