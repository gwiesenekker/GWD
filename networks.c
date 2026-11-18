// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "globals.h"
#include <float.h>

_Static_assert((sizeof(float) == 4) and (FLT_RADIX == 2) and
                 (FLT_MANT_DIG == 24) and (FLT_MIN_EXP == -125) and
                 (FLT_MAX_EXP == 128),
               "network files require binary32 float characteristics");

// MPI shared memory does not guarantee 32-byte alignment

#define USE_FMA
#define USE_RSQRT
#define MM256_LOAD_PS _mm256_loadu_ps

#ifdef USE_FMA
#define MM256_MADD_PS(a, b, c) _mm256_fmadd_ps((a), (b), (c))
#else
#define MM256_MADD_PS(a, b, c) _mm256_add_ps(_mm256_mul_ps((a), (b)), (c))
#endif

#define NPIECES_PERCENTILE (INVALID)

#undef USE_RECIPROCAL
#define USE_ERF

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT

local double wdl_score, mat_score;

#define DOUBLE2SCALED(D) ((scaled_double_t)(D))
#define SCALED2DOUBLE(S) ((double)(S))

#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32

#define SCALED_DOUBLE_FACTOR 10000

#define FLOAT2SCALED(D) (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2FLOAT(S) ((float)(S) / SCALED_DOUBLE_FACTOR)

#define DOUBLE2SCALED(D) (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2DOUBLE(S) ((double)(S) / SCALED_DOUBLE_FACTOR)

#else

#error unknown SCALED_DOUBLE_T

#endif

network_t networks[NETWORKS_MAX];

// for binary fen

typedef i32_t bin_t;

local i8_t base64_table[256];
local ui16_t base3_table[65536];

local void init_base64_table(void)
{
  const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  for (int ichar = 0; ichar < 256; ichar++)
    base64_table[ichar] = INVALID;

  for (int ichar = 0; base64_chars[ichar]; ichar++)
    base64_table[(unsigned char)base64_chars[ichar]] = ichar;
}

local void init_base3_table(void)
{
  for (int white_mask = 0; white_mask < 256; white_mask++)
  {
    for (int black_mask = 0; black_mask < 256; black_mask++)
    {
      int index = 0;
      int power = 1;

      for (int ibit = 0; ibit < 8; ibit++)
      {
        int white_bit = (white_mask >> ibit) & 1;
        int black_bit = (black_mask >> ibit) & 1;

        int digit = white_bit + 2 * black_bit;

        index += digit * power;
        power *= 3;
      }

      base3_table[white_mask | (black_mask << 8)] = (uint16_t)index;
    }
  }
}

#define BASE3_PATTERN_BITS 16
#define BASE3_MASK_TABLE_SIZE (1u << BASE3_PATTERN_BITS)

local uint32_t base3_mask_table[BASE3_MASK_TABLE_SIZE];

local void init_base3_mask_table(void)
{
  for (uint32_t mask = 0; mask < BASE3_MASK_TABLE_SIZE; mask++)
  {
    uint32_t index = 0;
    uint32_t power = 1;

    for (int ibit = 0; ibit < BASE3_PATTERN_BITS; ibit++)
    {
      if ((mask >> ibit) & 1u)
      {
        index += power;
      }

      power *= 3u;
    }

    base3_mask_table[mask] = index;
  }
}

void decode_base64(size_t arg_nsource,
                   const unsigned char *arg_source,
                   size_t arg_ntarget,
                   unsigned char *arg_target)
{
  ui32_t bits = 0;
  int nbits = -8;

  size_t itarget = 0;

  for (size_t isource = 0;
       (isource < arg_nsource) and (arg_source[isource] != '=');
       isource++)
  {
    i8_t digit = base64_table[(unsigned char)arg_source[isource]];

    HARDBUG(digit == INVALID)

    bits = (bits << 6) | digit;

    nbits += 6;

    if (nbits >= 0)
    {
      HARDBUG(itarget >= arg_ntarget)

      arg_target[itarget++] = (bits >> nbits) & 0xFF;

      nbits -= 8;
    }
  }

  HARDBUG(itarget != arg_ntarget)
}

local cJSON *open_cjson_file(const cJSON *arg_directory, const char *arg_name)
{
  cJSON *files = cJSON_GetObjectItem(arg_directory, CJSON_FILES_ID);

  HARDBUG(!cJSON_IsArray(files))

  cJSON *result = NULL;

  cJSON *file;

  cJSON_ArrayForEach(file, files)
  {
    cJSON *name_cjson = cJSON_GetObjectItem(file, CJSON_FILE_NAME_ID);

    char *name = cJSON_GetStringValue(name_cjson);

    HARDBUG(name == NULL)

    if (strcmp(name, arg_name) == 0)
    {
      result = file;

      break;
    }
  }

  return (result);
}

local void read_1d_csv_base64(const cJSON *arg_csv,
                              int *arg_nrows,
                              scaled_double_t **arg_cells)
{
  *arg_cells = NULL;

  cJSON *cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_ROWS_ID);

  HARDBUG(!cJSON_IsNumber(cjson_value))

  int nrows = round(cJSON_GetNumberValue(cjson_value));

  HARDBUG(nrows < 1)

  cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_DATA_ID);

  HARDBUG(!cJSON_IsString(cjson_value))

  char *data = cJSON_GetStringValue(cjson_value);

  float *cells_float;

  MY_MALLOC_BY_TYPE(cells_float, float, nrows)

  decode_base64(strlen(data),
                (const unsigned char *)data,
                nrows * sizeof(float),
                (unsigned char *)cells_float);

  scaled_double_t *cells;

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
  cells = cells_float;
#else
  MY_MALLOC_BY_TYPE(cells, scaled_double_t, nrows)

  for (int irow = 0; irow < nrows; irow++)
    cells[irow] = FLOAT2SCALED(cells_float[irow]);

  my_free(cells_float);
#endif

  if (*arg_nrows != INVALID)
    HARDBUG(nrows != *arg_nrows)
  else
    *arg_nrows = nrows;

  *arg_cells = cells;
}

local void read_2d_csv_base64(const cJSON *arg_csv,
                              int *arg_nrows,
                              int *arg_ncols,
                              scaled_double_t **arg_cells_nrowsxncols,
                              scaled_double_t **arg_cells_ncolsxnrows)
{
  HARDBUG((arg_cells_nrowsxncols == NULL) and (arg_cells_ncolsxnrows == NULL))

  cJSON *cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_ROWS_ID);

  HARDBUG(!cJSON_IsNumber(cjson_value))

  int nrows = round(cJSON_GetNumberValue(cjson_value));

  HARDBUG(nrows < 1)

  cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_COLS_ID);

  HARDBUG(!cJSON_IsNumber(cjson_value))

  int ncols = round(cJSON_GetNumberValue(cjson_value));

  HARDBUG(ncols < 1)

  cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_DATA_ID);

  HARDBUG(!cJSON_IsString(cjson_value))

  char *data = cJSON_GetStringValue(cjson_value);

  float *cells_float;

  MY_MALLOC_BY_TYPE(cells_float, float, nrows *ncols);

  decode_base64(strlen(data),
                (const unsigned char *)data,
                nrows * ncols * sizeof(float),
                (unsigned char *)cells_float);

  if (arg_cells_nrowsxncols != NULL)
  {
    void *void_cells_nrowsxncols;

    MPI_Win win_cells_nrowsxncols;

    my_mpi_allocate_shared(my_mpi_globals.MMG_global_comm,
                           sizeof(scaled_double_t),
                           nrows * ncols,
                           &void_cells_nrowsxncols,
                           &win_cells_nrowsxncols);

    scaled_double_t *cells_nrowsxncols =
      (scaled_double_t *)void_cells_nrowsxncols;

    *arg_cells_nrowsxncols = cells_nrowsxncols;

    for (int irow = 0; irow < nrows; irow++)
    {
      for (int icol = 0; icol < ncols; icol++)
      {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
        cells_nrowsxncols[irow * ncols + icol] =
          cells_float[irow * ncols + icol];
#else
        cells_nrowsxncols[irow * ncols + icol] =
          FLOAT2SCALED(cells_float[irow * ncols + icol]);
#endif
      }
    }

    my_mpi_sync_shared(my_mpi_globals.MMG_global_comm,
                       win_cells_nrowsxncols,
                       "win_cells_nrowsxncols");

    MARK_ARRAY_READ_ONLY(cells_nrowsxncols, nrows * ncols);
  }

  if (arg_cells_ncolsxnrows != NULL)
  {
    scaled_double_t *cells_ncolsxnrows;

    void *void_cells_ncolsxnrows;

    MPI_Win win_cells_ncolsxnrows;

    my_mpi_allocate_shared(my_mpi_globals.MMG_global_comm,
                           sizeof(scaled_double_t),
                           ncols * nrows,
                           &void_cells_ncolsxnrows,
                           &win_cells_ncolsxnrows);

    cells_ncolsxnrows = (scaled_double_t *)void_cells_ncolsxnrows;

    *arg_cells_ncolsxnrows = cells_ncolsxnrows;

    for (int irow = 0; irow < nrows; irow++)
    {
      for (int icol = 0; icol < ncols; icol++)
      {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
        cells_ncolsxnrows[icol * nrows + irow] =
          cells_float[irow * ncols + icol];
#else
        cells_ncolsxnrows[icol * nrows + irow] =
          FLOAT2SCALED(cells_float[irow * ncols + icol]);
#endif
      }
    }

    my_mpi_sync_shared(my_mpi_globals.MMG_global_comm,
                       win_cells_ncolsxnrows,
                       "win_cells_ncolsxnrows");

    MARK_ARRAY_READ_ONLY(cells_ncolsxnrows, ncols * nrows);
  }

  my_free(cells_float);

  if (*arg_nrows != INVALID)
    HARDBUG(nrows != *arg_nrows)
  else
    *arg_nrows = nrows;

  if (*arg_ncols != INVALID)
    HARDBUG(ncols != *arg_ncols)
  else
    *arg_ncols = ncols;
}

local scaled_double_t dot_scaled(int arg_nvalues,
                                 const scaled_double_t *restrict arg_a,
                                 const scaled_double_t *restrict arg_b)
{
  BEGIN_BLOCK(__FUNC__)

  scaled_double_t sum = 0;

#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      sum += arg_a[ivalue] * arg_b[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    __m256 vector_sum = _mm256_setzero_ps();

    for (const float *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      __m256 vector_a = MM256_LOAD_PS(arg_a);
      __m256 vector_b = MM256_LOAD_PS(arg_b);

      vector_sum = MM256_MADD_PS(vector_a, vector_b, vector_sum);
    }

    ALIGN64(float lanes[8]);

    _mm256_store_ps(lanes, vector_sum);

    sum = lanes[0] + lanes[1] + lanes[2] + lanes[3] + lanes[4] + lanes[5] +
          lanes[6] + lanes[7];
#else

    __m256i s64_lo = _mm256_setzero_si256();
    __m256i s64_hi = _mm256_setzero_si256();

    for (const i32_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      __m256i ta = _mm256_load_si256((const __m256i *)arg_a);
      __m256i tb = _mm256_load_si256((const __m256i *)arg_b);

      __m256i ta_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(ta));
      __m256i ta_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(ta, 1));

      __m256i tb_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(tb));
      __m256i tb_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(tb, 1));

      s64_lo = _mm256_add_epi64(s64_lo, _mm256_mul_epi32(ta_lo, tb_lo));
      s64_hi = _mm256_add_epi64(s64_hi, _mm256_mul_epi32(ta_hi, tb_hi));
    }

    ALIGN64(i64_t r64_lo[4]);
    ALIGN64(i64_t r64_hi[4]);

    _mm256_store_si256((__m256i *)r64_lo, s64_lo);
    _mm256_store_si256((__m256i *)r64_hi, s64_hi);

    i64_t s64 = r64_lo[0] + r64_lo[1] + r64_lo[2] + r64_lo[3] + r64_hi[0] +
                r64_hi[1] + r64_hi[2] + r64_hi[3];

    sum = s64 / SCALED_DOUBLE_FACTOR;
#endif
  }
#endif

  END_BLOCK

  return (sum);
}

#if defined(USE_AVX2_INTRINSICS) && SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
local inline void dense_fixed_x8(const float *arg_inputs,
                                 const scaled_double_t *arg_weights,
                                 float *arg_dot,
                                 int arg_nchunks)
{
  // Callers supply 2 or 4 chunks. Preserve dot_scaled's accumulation order,
  // including the initial FMA with zero and left-to-right scalar reduction.
  __m256 inputs[4];

  for (int ichunk = 0; ichunk < arg_nchunks; ichunk++)
    inputs[ichunk] = _mm256_loadu_ps(arg_inputs + 8 * ichunk);

  for (int ioutput = 0; ioutput < 8; ioutput += 4)
  {
    __m256 sums[4] = {_mm256_setzero_ps(),
                      _mm256_setzero_ps(),
                      _mm256_setzero_ps(),
                      _mm256_setzero_ps()};

    for (int ichunk = 0; ichunk < arg_nchunks; ichunk++)
    {
      for (int joutput = 0; joutput < 4; joutput++)
      {
        // Shared MPI allocations need not be 32-byte aligned.
        __m256 weights = _mm256_loadu_ps(
          arg_weights + (size_t)(ioutput + joutput) * (8 * arg_nchunks) +
          8 * ichunk);
        sums[joutput] = MM256_MADD_PS(inputs[ichunk], weights, sums[joutput]);
      }
    }

    for (int joutput = 0; joutput < 4; joutput++)
    {
      ALIGN64(float lanes[8]);
      _mm256_store_ps(lanes, sums[joutput]);
      arg_dot[ioutput + joutput] = lanes[0] + lanes[1] + lanes[2] + lanes[3] +
                                   lanes[4] + lanes[5] + lanes[6] + lanes[7];
    }
  }
}

local void dense_16x8(const float *arg_inputs,
                      const scaled_double_t *arg_weights,
                      float *arg_dot)
{
  dense_fixed_x8(arg_inputs, arg_weights, arg_dot, 2);
}

local void dense_32x8(const float *arg_inputs,
                      const scaled_double_t *arg_weights,
                      float *arg_dot)
{
  dense_fixed_x8(arg_inputs, arg_weights, arg_dot, 4);
}
#endif

local void dense_layer0_dot(const dense_network_shared_t *arg_dense,
                            const scaled_double_t *arg_inputs,
                            scaled_double_t *arg_dot)
{
  const layer_shared_t *with_layer = arg_dense->DNS_layers;

#if defined(USE_AVX2_INTRINSICS) && SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
  if (arg_dense->DNS_16x8_kernel)
  {
    HARDBUG((with_layer->LS_ninputs != 16) or (with_layer->LS_noutputs != 8))

    dense_16x8(arg_inputs, with_layer->LS_weights_noutputsxninputs, arg_dot);

    return;
  }

  if (arg_dense->DNS_32x8_kernel)
  {
    HARDBUG((with_layer->LS_ninputs != 32) or (with_layer->LS_noutputs != 8))

    dense_32x8(arg_inputs, with_layer->LS_weights_noutputsxninputs, arg_dot);

    return;
  }
#endif

  for (int ioutput = 0; ioutput < with_layer->LS_noutputs; ioutput++)
    arg_dot[ioutput] = dot_scaled(with_layer->LS_ninputs,
                                  arg_inputs,
                                  with_layer->LS_weights_noutputsxninputs +
                                    (size_t)ioutput * with_layer->LS_ninputs);
}

local void construct_dense_network_shared(network_t *arg_network,
                                          const cJSON *arg_directory,
                                          const char *arg_prefix,
                                          int arg_verbose)
{
  dense_network_shared_t *with_dense_network_shared = NULL;

  if (compat_strcasecmp(arg_prefix, "pattern") == 0)
    with_dense_network_shared = &(arg_network->N_shared_pattern_dense_network);
  else if (compat_strcasecmp(arg_prefix, "material") == 0)
    with_dense_network_shared = &(arg_network->N_shared_material_dense_network);
  else if (compat_strcasecmp(arg_prefix, "final") == 0)
    with_dense_network_shared = &(arg_network->N_shared_dense_network);
  else
    FATAL("unknown arg_prefix", EXIT_FAILURE)

  HARDBUG(with_dense_network_shared == NULL)

  with_dense_network_shared->DNS_input_activation =
    arg_network->N_input_activation;
  with_dense_network_shared->DNS_activation = arg_network->N_activation;
  with_dense_network_shared->DNS_final_activation =
    arg_network->N_final_activation;

  if (arg_verbose)
    PRINTF("arg_prefix=%s\n", arg_prefix);

  for (int idense = 0; idense < 4; idense++)
  {
    BSTRING(bweights)

    BSTRING(bbias)

    if (arg_prefix == NULL)
    {
      HARDBUG(bformata(bweights, "dense%d_weights.csv", idense) == BSTR_ERR)
      HARDBUG(bformata(bbias, "dense%d_bias.csv", idense) == BSTR_ERR)
    }
    else
    {
      HARDBUG(
        bformata(bweights, "%s_dense%d_weights.csv", arg_prefix, idense) ==
        BSTR_ERR)
      HARDBUG(bformata(bbias, "%s_dense%d_bias.csv", arg_prefix, idense) ==
              BSTR_ERR)
    }

    cJSON *file = open_cjson_file(arg_directory, bdata(bweights));

    if (file != NULL)
    {
      HARDBUG(with_dense_network_shared->DNS_nlayers >= NLAYERS_MAX)

      layer_shared_t *with_layer_shared =
        with_dense_network_shared->DNS_layers +
        with_dense_network_shared->DNS_nlayers;

      with_layer_shared->LS_ninputs = with_layer_shared->LS_noutputs = INVALID;

      read_2d_csv_base64(file,
                         &(with_layer_shared->LS_ninputs),
                         &(with_layer_shared->LS_noutputs),
                         NULL,
                         &(with_layer_shared->LS_weights_noutputsxninputs));

      file = open_cjson_file(arg_directory, bdata(bbias));

      HARDBUG(file == NULL)

      read_1d_csv_base64(file,
                         &(with_layer_shared->LS_noutputs),
                         &(with_layer_shared->LS_bias));

      mark_pointer_read_only(with_layer_shared->LS_bias);

      with_dense_network_shared->DNS_nlayers++;
    }

    BDESTROY(bbias)

    BDESTROY(bweights)
  }

  HARDBUG(with_dense_network_shared->DNS_nlayers < 1)

  with_dense_network_shared->DNS_16x8_kernel =
    (with_dense_network_shared->DNS_layers[0].LS_ninputs == 16) and
    (with_dense_network_shared->DNS_layers[0].LS_noutputs == 8);
  with_dense_network_shared->DNS_32x8_kernel =
    (with_dense_network_shared->DNS_layers[0].LS_ninputs == 32) and
    (with_dense_network_shared->DNS_layers[0].LS_noutputs == 8);

  if (arg_verbose)
  {
    PRINTF("nlayers=%d\n", with_dense_network_shared->DNS_nlayers);

    for (int ilayer = 0; ilayer < with_dense_network_shared->DNS_nlayers;
         ilayer++)
    {
      layer_shared_t *with_layer_shared =
        with_dense_network_shared->DNS_layers + ilayer;

      PRINTF("ilayer=%d (inputs, outputs)=(%d, %d)\n",
             ilayer,
             with_layer_shared->LS_ninputs,
             with_layer_shared->LS_noutputs);
    }
  }

  // check topology

  for (int ilayer = 0; ilayer < with_dense_network_shared->DNS_nlayers - 1;
       ilayer++)
  {
    layer_shared_t *with_current =
      with_dense_network_shared->DNS_layers + ilayer;

    layer_shared_t *with_next =
      with_dense_network_shared->DNS_layers + ilayer + 1;

    HARDBUG(with_current->LS_noutputs != with_next->LS_ninputs)
  }
}

local char *activation_last2string(int arg_activation_last, int arg_lc)
{
  char *result = NULL;

  if (arg_activation_last == NETWORK_ACTIVATION_LINEAR)
  {
    if (arg_lc)
      result = "linear";
    else
      result = "LINEAR";
  }
  else if (arg_activation_last == NETWORK_ACTIVATION_TANH)
  {
    if (arg_lc)
      result = "tanh";
    else
      result = "TANH";
  }
  else if (arg_activation_last == NETWORK_ACTIVATION_LOGITS)
  {
    if (arg_lc)
      result = "logits";
    else
      result = "LOGITS";
  }
  else if (arg_activation_last == NETWORK_ACTIVATION_2HEADS)
  {
    if (arg_lc)
      result = "2heads";
    else
      result = "2HEADS";
  }
  else
    FATAL("unknown arg_activation_last", EXIT_FAILURE)

  return (result);
}

local void construct_network_shared(const char *arg_network_name,
                                    int arg_network_id,
                                    int arg_verbose)
{
  HARDBUG((arg_network_id < 0) or (arg_network_id >= NETWORKS_MAX))

  network_t *with_network = networks + arg_network_id;

  with_network->N_network_id = arg_network_id;

  cJSON *networks_json;

  file2cjson(arg_network_name, &networks_json);

  // find the directory

  cJSON *directories = cJSON_GetObjectItem(networks_json, CJSON_DIRECTORIES_ID);

  HARDBUG(!cJSON_IsArray(directories))

  cJSON *directory = cJSON_GetArrayItem(directories, 0);

  HARDBUG(directory == NULL)

  cJSON *name_cjson = cJSON_GetObjectItem(directory, CJSON_DIRECTORY_NAME_ID);

  char *directory_name = cJSON_GetStringValue(name_cjson);

  HARDBUG(directory_name == NULL)

  PRINTF("directory_name=%s\n", directory_name);

  // shape

  cJSON *value_cjson = cJSON_GetObjectItem(directory, CJSON_SHAPE_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  char *svalue = cJSON_GetStringValue(value_cjson);

  HARDBUG((with_network->N_bshape = bfromcstr(svalue)) == NULL)

  if (arg_verbose)
    PRINTF("shape=%s\n", bdata(with_network->N_bshape));

  construct_patterns_shared(&(with_network->N_shared_patterns),
                            bdata(with_network->N_bshape));

  // network2material_score

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NETWORK2MATERIAL_SCORE_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson))

  with_network->N_network2material_score =
    round(cJSON_GetNumberValue(value_cjson));

  if (arg_verbose)
    PRINTF("with_network->N_network2material_score=%d\n",
           with_network->N_network2material_score);

  // nmaterials

  cJSON *cjson_materials = cJSON_GetObjectItem(directory, CJSON_MATERIALS_ID);

  if (!cJSON_IsArray(cjson_materials))
    with_network->N_shared_materials.MSS_nmaterials = 0;
  else
    with_network->N_shared_materials.MSS_nmaterials =
      cJSON_GetArraySize(cjson_materials);

  HARDBUG(with_network->N_shared_materials.MSS_nmaterials > NMATERIALS_MAX)

  BSTRING(bmaterial_name)

  for (int imaterial = 0;
       imaterial < with_network->N_shared_materials.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *with_material_shared =
      with_network->N_shared_materials.MSS_materials + imaterial;

    cJSON *cjson_material = cJSON_GetArrayItem(cjson_materials, imaterial);

    HARDBUG(!cJSON_IsObject(cjson_material))

    cJSON *cjson_material_name =
      cJSON_GetObjectItem(cjson_material, CJSON_MATERIAL_NAME_ID);

    HARDBUG(!cJSON_IsString(cjson_material_name))

    const char *material_name = cJSON_GetStringValue(cjson_material_name);

    HARDBUG(material_name == NULL)

    HARDBUG((bmaterial_name = bfromcstr(material_name)) == NULL)

    if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_MEN_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_MEN_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_MEN_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_MEN_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_MEN_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_MEN_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_KINGS_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_KINGS_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_KINGS_ID;

      with_material_shared->MS_nstates = (MATERIAL_KING_MAX + 1);
      // with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_KINGS_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_KINGS_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_KINGS_ID;

      with_material_shared->MS_nstates = (MATERIAL_KING_MAX + 1);
      // with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_KINGS20_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_KINGS20_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_KINGS20_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_KINGS20_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_KINGS20_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_KINGS20_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_MAN_KING_CNAME) == 1)
    {
      with_material_shared->MS_id = MATERIAL_MAN_KING_ID;

      with_material_shared->MS_nstates =
        (NPIECES_MAX + 1) * (MATERIAL_KING_MAX + 1) * (NPIECES_MAX + 1) *
        (MATERIAL_KING_MAX + 1);
    }
    else
      FATAL("unknown material_name", EXIT_FAILURE)
  }

  BDESTROY(bmaterial_name)

  if (arg_verbose)
  {
    PRINTF("with_network->N_shared_materials.MSS_nmaterials=%d\n",
           with_network->N_shared_materials.MSS_nmaterials);

    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      PRINTF("imaterial=%d"
             " with_material_shared->MS_bname=%s"
             " with_material_shared->MS_id=%d"
             " with_material_shared->MS_nstates=%d\n",
             imaterial,
             bdata(with_material_shared->MS_bname),
             with_material_shared->MS_id,
             with_material_shared->MS_nstates);
    }
  }

  // architecture patterns

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ARCHITECTURE_PATTERNS_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "add") == 0)
    with_network->N_pattern_architecture = NETWORK_ARCHITECTURE_ADD;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    with_network->N_pattern_architecture = NETWORK_ARCHITECTURE_CONCAT;
  else
    FATAL("unknown architecture_patterns option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_ADD)
      PRINTF("architecture_patterns=ADD\n");
    else if (with_network->N_pattern_architecture ==
             NETWORK_ARCHITECTURE_CONCAT)
      PRINTF("architecture_patterns=CONCAT\n");
    else
      FATAL("unknown architecture_patterns option", EXIT_FAILURE)
  }

  // architecture material

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ARCHITECTURE_MATERIAL_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "NULL") == 0)
    with_network->N_material_architecture = NETWORK_ARCHITECTURE_NULL;
  else if (compat_strcasecmp(svalue, "add") == 0)
    with_network->N_material_architecture = NETWORK_ARCHITECTURE_ADD;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    with_network->N_material_architecture = NETWORK_ARCHITECTURE_CONCAT;
  else
    FATAL("unknown architecture_material option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_material_architecture == NETWORK_ARCHITECTURE_NULL)
      PRINTF("architecture_material=NULL\n");
    else if (with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD)
      PRINTF("architecture_material=ADD\n");
    else if (with_network->N_material_architecture ==
             NETWORK_ARCHITECTURE_CONCAT)
      PRINTF("architecture_material=CONCAT\n");
    else
      FATAL("unknown architecture_material option", EXIT_FAILURE)
  }

  // activation_inputs

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_INPUTS_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_RELU;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_RSQRT;
  else if (compat_strcasecmp(svalue, "gelu") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_GELU;
  else if (compat_strcasecmp(svalue, "geluv2") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_GELUV2;
  else if (compat_strcasecmp(svalue, "linear") == 0)
    with_network->N_input_activation = NETWORK_ACTIVATION_LINEAR;
  else
    FATAL("unknown activation_inputs option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_input_activation == NETWORK_ACTIVATION_RELU)
      PRINTF("activation_inputs=RELU\n");
    else if (with_network->N_input_activation == NETWORK_ACTIVATION_TANH)
      PRINTF("activation_inputs=TANH\n");
    else if (with_network->N_input_activation == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation_inputs=RSQRT\n");
    else if (with_network->N_input_activation == NETWORK_ACTIVATION_GELU)
      PRINTF("activation_inputs=GELU\n");
    else if (with_network->N_input_activation == NETWORK_ACTIVATION_GELUV2)
      PRINTF("activation_inputs=GELUV2\n");
    else if (with_network->N_input_activation == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_inputs=LINEAR\n");
    else
      FATAL("unknown activation_inputs option", EXIT_FAILURE)
  }

  // activation

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu") == 0)
    with_network->N_activation = NETWORK_ACTIVATION_RELU;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    with_network->N_activation = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    with_network->N_activation = NETWORK_ACTIVATION_RSQRT;
  else if (compat_strcasecmp(svalue, "gelu") == 0)
    with_network->N_activation = NETWORK_ACTIVATION_GELU;
  else if (compat_strcasecmp(svalue, "geluv2") == 0)
    with_network->N_activation = NETWORK_ACTIVATION_GELUV2;
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_activation == NETWORK_ACTIVATION_RELU)
      PRINTF("activation=RELU\n");
    else if (with_network->N_activation == NETWORK_ACTIVATION_TANH)
      PRINTF("activation=TANH\n");
    else if (with_network->N_activation == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation=RSQRT\n");
    else if (with_network->N_activation == NETWORK_ACTIVATION_GELU)
      PRINTF("activation=GELU\n");
    else if (with_network->N_activation == NETWORK_ACTIVATION_GELUV2)
      PRINTF("activation=GELUV2\n");
    else
      FATAL("unknown activation option", EXIT_FAILURE)
  }

  // activation_last

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_LAST_ID);

  HARDBUG(!cJSON_IsString(value_cjson))

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "linear") == 0)
    with_network->N_final_activation = NETWORK_ACTIVATION_LINEAR;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    with_network->N_final_activation = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "logits") == 0)
    with_network->N_final_activation = NETWORK_ACTIVATION_LOGITS;
  else if (compat_strcasecmp(svalue, "2heads") == 0)
    with_network->N_final_activation = NETWORK_ACTIVATION_2HEADS;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (arg_verbose)
  {
    PRINTF("with_network->N_final_activation=%s\n",
           activation_last2string(with_network->N_final_activation, TRUE));
  }

  // nman_min

  with_network->N_nmen_min = 0;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MIN_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    with_network->N_nmen_min = ivalue;

    if (arg_verbose)
    {
      PRINTF("N_nmen_min=%d\n", with_network->N_nmen_min);
    }
  }

  // nman_max

  with_network->N_nmen_max = 40;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MAX_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    with_network->N_nmen_max = ivalue;

    if (arg_verbose)
    {
      PRINTF("N_nmen_max=%d\n", with_network->N_nmen_max);
    }
  }

  HARDBUG(with_network->N_nmen_min > with_network->N_nmen_max)

  value_cjson = cJSON_GetObjectItem(directory, CJSON_PRAGMA_ID);

  if (cJSON_IsString(value_cjson))
  {
    svalue = cJSON_GetStringValue(value_cjson);

    PRINTF("pragma=%s\n", svalue);

    if (compat_strcasecmp(svalue, "dummy") == 0)
      return;
  }

  with_network->N_shared_pattern_dense_network.DNS_ninputs = 0;
  with_network->N_shared_pattern_dense_network.DNS_nlayers = 0;
  with_network->N_shared_pattern_dense_network.DNS_activate_layer0 = FALSE;
  with_network->N_shared_pattern_dense_network.DNS_compute_layer0 = TRUE;

  with_network->N_shared_material_dense_network.DNS_ninputs = 0;
  with_network->N_shared_material_dense_network.DNS_nlayers = 0;
  with_network->N_shared_material_dense_network.DNS_activate_layer0 = FALSE;
  with_network->N_shared_material_dense_network.DNS_compute_layer0 = TRUE;

  with_network->N_shared_dense_network.DNS_ninputs = 0;
  with_network->N_shared_dense_network.DNS_nlayers = 0;
  with_network->N_shared_dense_network.DNS_activate_layer0 = FALSE;
  with_network->N_shared_dense_network.DNS_compute_layer0 = TRUE;

  // load the embedding vectors for the patterns

  for (int ipattern = 0;
       ipattern < with_network->N_shared_patterns.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_shared_patterns.PSS_patterns + ipattern;

    BSTRING(bindex)

    HARDBUG(bformata(bindex, "pattern_embedding%d_index.csv", ipattern) ==
            BSTR_ERR)

    cJSON *file = open_cjson_file(directory, bdata(bindex));

    BDESTROY(bindex)

    if (file != NULL)
      FATAL("compact embedding index files are not supported", EXIT_FAILURE)

    BSTRING(bweights)

    HARDBUG(bformata(bweights, "pattern_embedding%d_weights.csv", ipattern) ==
            BSTR_ERR)

    file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    read_2d_csv_base64(file,
                       &(with_pattern_shared->PS_nstates),
                       &(with_pattern_shared->PS_nembed),
                       &(with_pattern_shared->PS_weights_nstatesxnembed),
                       NULL);

    if (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_ADD)
    {
      with_pattern_shared->PS_offset = 0;

      if (with_network->N_shared_pattern_dense_network.DNS_ninputs == 0)
      {
        with_network->N_shared_pattern_dense_network.DNS_ninputs =
          with_pattern_shared->PS_nembed; // first
      }
      else
        HARDBUG(with_pattern_shared->PS_nembed !=
                with_network->N_shared_pattern_dense_network.DNS_ninputs)
    }
    else
    {
      with_pattern_shared->PS_offset =
        with_network->N_shared_pattern_dense_network.DNS_ninputs;

      with_network->N_shared_pattern_dense_network.DNS_ninputs +=
        with_pattern_shared->PS_nembed;
    }

    PRINTF("ipattern=%d with_pattern_shared->PS_nstates=%d "
           "with_pattern_shared->PS_nembed=%d"
           " with_pattern_shared->PS_offset=%d\n",
           ipattern,
           with_pattern_shared->PS_nstates,
           with_pattern_shared->PS_nembed,
           with_pattern_shared->PS_offset);

    HARDBUG(with_network->N_shared_pattern_dense_network.DNS_ninputs >=
            NINPUTS_MAX)
  }

  // load the material embedding vectors

  if (with_network->N_material_architecture != NETWORK_ARCHITECTURE_NULL)
  {
    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      BSTRING(bweights)

      HARDBUG(bformata(bweights,
                       "material_embedding%d_weights.csv",
                       imaterial) == BSTR_ERR)

      cJSON *file = open_cjson_file(directory, bdata(bweights));

      BDESTROY(bweights)

      HARDBUG(file == NULL)

      with_material_shared->MS_nembed = INVALID;

      read_2d_csv_base64(file,
                         &(with_material_shared->MS_nstates),
                         &(with_material_shared->MS_nembed),
                         &(with_material_shared->MS_weights_nstatesxnembed),
                         NULL);

      if (with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD)
      {
        with_material_shared->MS_offset = 0;

        if (with_network->N_shared_material_dense_network.DNS_ninputs == 0)
        {
          with_network->N_shared_material_dense_network.DNS_ninputs =
            with_material_shared->MS_nembed; // first
        }
        else
          HARDBUG(with_material_shared->MS_nembed !=
                  with_network->N_shared_material_dense_network.DNS_ninputs)
      }
      else
      {
        with_material_shared->MS_offset =
          with_network->N_shared_material_dense_network.DNS_ninputs;

        with_network->N_shared_material_dense_network.DNS_ninputs +=
          with_material_shared->MS_nembed;
      }

      PRINTF("imaterial=%d with_material_shared->MS_bname=%s"
             " with_material_shared->MS_id=%d"
             " with_material_shared->MS_nstates=%d"
             " with_material_shared->MS_nembed=%d"
             " with_material_shared->MS_offset=%d\n",
             imaterial,
             bdata(with_material_shared->MS_bname),
             with_material_shared->MS_id,
             with_material_shared->MS_nstates,
             with_material_shared->MS_nembed,
             with_material_shared->MS_offset);

      HARDBUG(with_network->N_shared_material_dense_network.DNS_ninputs >=
              NINPUTS_MAX)
    }
  }

  // read the dense networks

  construct_dense_network_shared(with_network, directory, "pattern", TRUE);

  // topology check

  HARDBUG(with_network->N_shared_pattern_dense_network.DNS_ninputs !=
          with_network->N_shared_pattern_dense_network.DNS_layers[0].LS_ninputs)

  // The specialized path sums 16-element embeddings for a 16x8 first layer.
  with_network->N_pattern_dense_network_16x8_kernel =
    (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_ADD) and
    (with_network->N_shared_pattern_dense_network.DNS_layers[0].LS_ninputs ==
     16) and
    (with_network->N_shared_pattern_dense_network.DNS_layers[0].LS_noutputs ==
     8);

  with_network->N_pattern_dense_network_64x16_kernel =
    (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_ADD) and
    (with_network->N_shared_pattern_dense_network.DNS_layers[0].LS_ninputs ==
     64) and
    (with_network->N_shared_pattern_dense_network.DNS_layers[0].LS_noutputs ==
     16);

  for (int ipattern = 0;
       ipattern < with_network->N_shared_patterns.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_shared_patterns.PSS_patterns + ipattern;

    if ((with_pattern_shared->PS_nembed != 16) or
        (with_pattern_shared->PS_offset != 0))
      with_network->N_pattern_dense_network_16x8_kernel = FALSE;

    if ((with_pattern_shared->PS_nembed != 64) or
        (with_pattern_shared->PS_offset != 0))
      with_network->N_pattern_dense_network_64x16_kernel = FALSE;
  }

  with_network->N_material_dense_network_16x8_kernel = FALSE;
  with_network->N_material_dense_network_64x16_kernel = FALSE;

  if (with_network->N_material_architecture != NETWORK_ARCHITECTURE_NULL)
  {
    construct_dense_network_shared(with_network, directory, "material", TRUE);

    // topology check

    HARDBUG(
      with_network->N_shared_material_dense_network.DNS_ninputs !=
      with_network->N_shared_material_dense_network.DNS_layers[0].LS_ninputs)

    with_network->N_material_dense_network_16x8_kernel =
      (with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD) and
      (with_network->N_shared_material_dense_network.DNS_layers[0].LS_ninputs ==
       16) and
      (with_network->N_shared_material_dense_network.DNS_layers[0]
         .LS_noutputs == 8);

    with_network->N_material_dense_network_64x16_kernel =
      (with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD) and
      (with_network->N_shared_material_dense_network.DNS_layers[0].LS_ninputs ==
       64) and
      (with_network->N_shared_material_dense_network.DNS_layers[0]
         .LS_noutputs == 16);

    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      if ((with_material_shared->MS_nembed != 16) or
          (with_material_shared->MS_offset != 0))
        with_network->N_material_dense_network_16x8_kernel = FALSE;

      if ((with_material_shared->MS_nembed != 64) or
          (with_material_shared->MS_offset != 0))
        with_network->N_material_dense_network_64x16_kernel = FALSE;
    }
  }

  PRINTF("N_pattern_dense_network_16x8_kernel=%d\n",
         with_network->N_pattern_dense_network_16x8_kernel);
  PRINTF("N_material_dense_network_16x8_kernel=%d\n",
         with_network->N_material_dense_network_16x8_kernel);
  PRINTF("N_pattern_dense_network_64x16_kernel=%d\n",
         with_network->N_pattern_dense_network_64x16_kernel);
  PRINTF("N_material_dense_network_64x16_kernel=%d\n",
         with_network->N_material_dense_network_64x16_kernel);

  // set expected inputs for dense network

  {
    dense_network_shared_t *with_pattern_dense_network_shared =
      &(with_network->N_shared_pattern_dense_network);

    with_network->N_shared_dense_network.DNS_ninputs =
      with_pattern_dense_network_shared
        ->DNS_layers[with_pattern_dense_network_shared->DNS_nlayers - 1]
        .LS_noutputs;

    dense_network_shared_t *with_material_dense_network_shared =
      &(with_network->N_shared_material_dense_network);

    with_network->N_shared_dense_network.DNS_ninputs +=
      with_material_dense_network_shared
        ->DNS_layers[with_material_dense_network_shared->DNS_nlayers - 1]
        .LS_noutputs;
  }

  PRINTF("with_network->N_shared_dense_network.DNS_ninputs=%d\n",
         with_network->N_shared_dense_network.DNS_ninputs);

  HARDBUG(with_network->N_shared_dense_network.DNS_ninputs >= NINPUTS_MAX)

  construct_dense_network_shared(with_network, directory, "final", TRUE);

  if (with_network->N_shared_dense_network.DNS_ninputs !=
      with_network->N_shared_dense_network.DNS_layers[0].LS_ninputs)
  {
    PRINTF("with_network->N_shared_dense_network.DNS_ninputs=%d\n",
           with_network->N_shared_dense_network.DNS_ninputs);
    PRINTF("with_network->N_shared_dense_network.DNS_layers[0].LS_ninputs=%d\n",
           with_network->N_shared_dense_network.DNS_layers[0].LS_ninputs);

    FATAL("topology error", EXIT_FAILURE)
  }

  if (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_ADD)
  {
    for (int ipattern = 0;
         ipattern < with_network->N_shared_patterns.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_shared_patterns.PSS_patterns + ipattern;

      with_pattern_shared->PS_sum_nstatesxnoutputs = NULL;
    }
    with_network->N_shared_pattern_dense_network.DNS_activate_layer0 = TRUE;
    with_network->N_shared_pattern_dense_network.DNS_compute_layer0 = TRUE;
  }
  else if (with_network->N_pattern_architecture == NETWORK_ARCHITECTURE_CONCAT)
  {
    // precompute layer0 contribution

    with_network->N_shared_pattern_dense_network.DNS_activate_layer0 = FALSE;
    with_network->N_shared_pattern_dense_network.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      with_network->N_shared_pattern_dense_network.DNS_layers;

    for (int ipattern = 0;
         ipattern < with_network->N_shared_patterns.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_shared_patterns.PSS_patterns + ipattern;

      void *void_sum_nstatesxnoutputs;

      MPI_Win win_sum_nstatesxnoutputs;

      my_mpi_allocate_shared(my_mpi_globals.MMG_global_comm,
                             sizeof(scaled_double_t),
                             with_pattern_shared->PS_nstates *
                               with_current->LS_noutputs,
                             &void_sum_nstatesxnoutputs,
                             &win_sum_nstatesxnoutputs);

      scaled_double_t *sum_nstatesxnoutputs =
        (scaled_double_t *)void_sum_nstatesxnoutputs;

      MY_MALLOC_BY_TYPE(with_pattern_shared->PS_sum_nstatesxnoutputs,
                        scaled_double_t *,
                        with_pattern_shared->PS_nstates);

      for (int istate = 0; istate < with_pattern_shared->PS_nstates; istate++)
      {
        with_pattern_shared->PS_sum_nstatesxnoutputs[istate] =
          sum_nstatesxnoutputs + istate * with_current->LS_noutputs;

        if (my_mpi_globals.MMG_is_master)
        {
          for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
          {
            with_pattern_shared->PS_sum_nstatesxnoutputs[istate][ioutput] =
              dot_scaled(with_pattern_shared->PS_nembed,
                         with_pattern_shared->PS_weights_nstatesxnembed +
                           (size_t)istate * with_pattern_shared->PS_nembed,
                         with_current->LS_weights_noutputsxninputs +
                           (size_t)ioutput * with_current->LS_ninputs +
                           with_pattern_shared->PS_offset);
          }
        }
      }

      my_mpi_sync_shared(my_mpi_globals.MMG_global_comm,
                         win_sum_nstatesxnoutputs,
                         "sum_nstatesxnoutputs");

      MARK_ARRAY_READ_ONLY(sum_nstatesxnoutputs,
                           with_pattern_shared->PS_nstates *
                             with_current->LS_noutputs);
    }
  }
  else
    FATAL("unknown with_network->N_architecur_pattern", EXIT_FAILURE)

  if (with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD)
  {
    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      with_material_shared->MS_sum_nstatesxnoutputs = NULL;
    }

    with_network->N_shared_material_dense_network.DNS_activate_layer0 = TRUE;
    with_network->N_shared_material_dense_network.DNS_compute_layer0 = TRUE;
  }
  else if (with_network->N_material_architecture == NETWORK_ARCHITECTURE_CONCAT)
  {
    // precompute layer0 contribution

    with_network->N_shared_material_dense_network.DNS_activate_layer0 = FALSE;
    with_network->N_shared_material_dense_network.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      with_network->N_shared_material_dense_network.DNS_layers;

    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      scaled_double_t *sum;

      MY_MALLOC_BY_TYPE(sum,
                        scaled_double_t,
                        with_material_shared->MS_nstates *
                          with_current->LS_noutputs);

      MY_MALLOC_BY_TYPE(with_material_shared->MS_sum_nstatesxnoutputs,
                        scaled_double_t *,
                        with_material_shared->MS_nstates);

      for (int istate = 0; istate < with_material_shared->MS_nstates; istate++)
      {
        with_material_shared->MS_sum_nstatesxnoutputs[istate] =
          sum + istate * with_current->LS_noutputs;

        for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
        {
          with_material_shared->MS_sum_nstatesxnoutputs[istate][ioutput] =
            dot_scaled(with_material_shared->MS_nembed,
                       with_material_shared->MS_weights_nstatesxnembed +
                         (size_t)istate * with_material_shared->MS_nembed,
                       with_current->LS_weights_noutputsxninputs +
                         (size_t)ioutput * with_current->LS_ninputs +
                         with_material_shared->MS_offset);
        }
      }

      mark_pointer_read_only(sum);
    }
  }
}

local void construct_dense_network_state(
  const dense_network_shared_t *arg_with_dense_network_shared,
  dense_network_state_t *arg_with_dense_network_state)
{
  for (int ilayer = 0; ilayer < arg_with_dense_network_shared->DNS_nlayers;
       ilayer++)
  {
    const layer_shared_t *with_layer_shared =
      arg_with_dense_network_shared->DNS_layers + ilayer;

    layer_thread_t *with_thread =
      arg_with_dense_network_state->DNS_layers + ilayer;

    MY_MALLOC_BY_TYPE(with_thread->LT_dot,
                      scaled_double_t,
                      with_layer_shared->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_sum,
                      scaled_double_t,
                      with_layer_shared->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_outputs,
                      scaled_double_t,
                      with_layer_shared->LS_noutputs);
  }
}

void construct_network_state(network_state_t *arg_with_network_state,
                             int arg_network_id,
                             int arg_verbose)
{
  HARDBUG((arg_network_id < 0) or (arg_network_id >= NETWORKS_MAX))

  arg_with_network_state->NS_network_id = arg_network_id;

  network_t *with_network = networks + arg_with_network_state->NS_network_id;

  construct_dense_network_state(
    &(with_network->N_shared_pattern_dense_network),
    &(arg_with_network_state->NS_pattern_dense_network_state));

  construct_dense_network_state(
    &(with_network->N_shared_material_dense_network),
    &(arg_with_network_state->NS_material_dense_network_state));

  construct_dense_network_state(
    &(with_network->N_shared_dense_network),
    &(arg_with_network_state->NS_dense_network_state));
}

local scaled_double_t dot_double(int arg_nvalues,
                                 const scaled_double_t *restrict arg_a,
                                 const scaled_double_t *restrict arg_b)
{
  double sum = 0;

  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    sum += SCALED2DOUBLE(arg_a[ivalue]) * SCALED2DOUBLE(arg_b[ivalue]);

  return (DOUBLE2SCALED(sum));
}

void vcopy_a2b(int arg_nvalues,
               const scaled_double_t *restrict arg_a,
               scaled_double_t *restrict arg_b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_b[ivalue] = arg_a[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      __m256 vector_a = MM256_LOAD_PS(arg_a);

      _mm256_store_ps(arg_b, vector_a);
    }
#else
    for (const i32_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      __m256i vector_a = _mm256_load_si256((const __m256i *)arg_a);

      _mm256_store_si256((__m256i *)arg_b, vector_a);
    }
#endif
  }
#endif
}

local void vadd_ab2c(int arg_nvalues,
                     const scaled_double_t *restrict arg_a,
                     const scaled_double_t *restrict arg_b,
                     scaled_double_t *restrict arg_c)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_c[ivalue] = arg_a[ivalue] + arg_b[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8, arg_c += 8)
    {
      __m256 vector_a = MM256_LOAD_PS(arg_a);
      __m256 vector_b = MM256_LOAD_PS(arg_b);

      __m256 vector_result = _mm256_add_ps(vector_a, vector_b);

      _mm256_store_ps(arg_c, vector_result);
    }
#else
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8, arg_c += 8)
    {
      __m256i vector_a = _mm256_load_si256((const __m256i *)arg_a);
      __m256i vector_b = _mm256_load_si256((const __m256i *)arg_b);

      __m256i vector_result = _mm256_add_epi32(vector_a, vector_b);

      _mm256_store_si256((__m256i *)arg_c, vector_result);
    }
#endif
  }
#endif

  END_BLOCK
}

void vmul_ab2c(int arg_nvalues,
               const scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b,
               scaled_double_t *restrict arg_c)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_c[ivalue] = arg_a[ivalue] * arg_b[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8, arg_c += 8)
    {
      __m256 vector_a = MM256_LOAD_PS(arg_a);
      __m256 vector_b = MM256_LOAD_PS(arg_b);

      __m256 vector_result = _mm256_mul_ps(vector_a, vector_b);

      _mm256_store_ps(arg_c, vector_result);
    }
#else
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8, arg_c += 8)
    {
      __m256i vector_a = _mm256_load_si256((const __m256i *)arg_a);
      __m256i vector_b = _mm256_load_si256((const __m256i *)arg_b);

      __m256i vector_result = _mm256_mullo_epi32(vector_a, vector_b);

      _mm256_store_si256((__m256i *)arg_c, vector_result);
    }
#endif
  }
#endif

  END_BLOCK
}

void vadd_ab2a(int arg_nvalues,
               scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_a[ivalue] += arg_b[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      const __m256 vector_a = MM256_LOAD_PS(arg_a);
      const __m256 vector_b = MM256_LOAD_PS(arg_b);

      const __m256 vector_sum = _mm256_add_ps(vector_a, vector_b);

      _mm256_store_ps(arg_a, vector_sum);
    }
#else
    for (const i32_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      const __m256i vector_a = _mm256_load_si256((const __m256i *)arg_a);
      const __m256i vector_b = _mm256_load_si256((const __m256i *)arg_b);

      const __m256i vector_sum = _mm256_add_epi32(vector_a, vector_b);

      _mm256_store_si256((__m256i *)arg_a, vector_sum);
    }
#endif
  }
#endif

  END_BLOCK
}

void vmul_ab2a(int arg_nvalues,
               scaled_double_t *restrict arg_a,
               const scaled_double_t *restrict arg_b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_a[ivalue] *= arg_b[ivalue];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      const __m256 vector_a = MM256_LOAD_PS(arg_a);
      const __m256 vector_b = MM256_LOAD_PS(arg_b);

      const __m256 vector_sum = _mm256_mul_ps(vector_a, vector_b);

      _mm256_store_ps(arg_a, vector_sum);
    }
#else
    for (const i32_t *end_a = arg_a + arg_nvalues; arg_a < end_a;
         arg_a += 8, arg_b += 8)
    {
      const __m256i vector_a = _mm256_load_si256((const __m256i *)arg_a);
      const __m256i vector_b = _mm256_load_si256((const __m256i *)arg_b);

      const __m256i vector_sum = _mm256_mullo_epi32(vector_a, vector_b);

      _mm256_store_si256((__m256i *)arg_a, vector_sum);
    }
#endif
  }
#endif

  END_BLOCK
}

local void activation_relu_scaled(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    {
      if (arg_inputs[ivalue] <= 0)
        arg_outputs[ivalue] = 0;
      else
        arg_outputs[ivalue] = arg_inputs[ivalue];
    }
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((arg_nvalues % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 vector_zero = _mm256_setzero_ps();

    for (const scaled_double_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256 vector_input = MM256_LOAD_PS(arg_inputs);

      vector_input = _mm256_max_ps(vector_input, vector_zero);

      _mm256_store_ps(arg_outputs, vector_input);
    }
#else
    __m256i zero = _mm256_setzero_si256();

    for (const int32_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256i vector_input = _mm256_load_si256((const __m256i *)arg_inputs);

      vector_input = _mm256_max_epi32(vector_input, zero);

      _mm256_store_si256((__m256i *)arg_outputs, vector_input);
    }
#endif
  }
#endif
}

local void activation_relu_double(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
  {
    if (arg_inputs[ivalue] <= 0.0)
      arg_outputs[ivalue] = 0.0;
    else
      arg_outputs[ivalue] = arg_inputs[ivalue];
  }
}

local void activation_tanh_scaled(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_outputs[ivalue] = tanhf(arg_inputs[ivalue]);
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 three_halfs = _mm256_set1_ps(1.5f);
    const __m256 p = _mm256_set1_ps(2.425f);
    const __m256 q = _mm256_set1_ps(1.400f);

    for (const scaled_double_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256 vector_input = MM256_LOAD_PS(arg_inputs);

      __m256 va2p = _mm256_add_ps(p, _mm256_mul_ps(vector_input, vector_input));

      __m256 rsqrt = _mm256_rsqrt_ps(va2p);

      // needed for Newton-Raphson

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      rsqrt = _mm256_mul_ps(
        rsqrt,
        _mm256_sub_ps(three_halfs,
                      _mm256_mul_ps(half, _mm256_mul_ps(va2p, rsqrt2))));

      // needed for correction

      rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      __m256 vq = _mm256_add_ps(one, _mm256_mul_ps(q, rsqrt2));

      _mm256_store_ps(arg_outputs,
                      _mm256_mul_ps(vector_input, _mm256_mul_ps(rsqrt, vq)));
    }
#else
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_outputs[ivalue] =
        FLOAT2SCALED(tanhf(SCALED2FLOAT(arg_inputs[ivalue])));
#endif
  }
#endif
}

local void activation_tanh_double(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    arg_outputs[ivalue] =
      DOUBLE2SCALED(tanh(SCALED2DOUBLE(arg_inputs[ivalue])));
}

local void activation_rsqrt_scaled(int arg_nvalues,
                                   const scaled_double_t *restrict arg_inputs,
                                   scaled_double_t *restrict arg_outputs)
{
#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
      arg_outputs[ivalue] =
        arg_inputs[ivalue] /
        sqrtf(1.0f + arg_inputs[ivalue] * arg_inputs[ivalue]);
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 three_halfs = _mm256_set1_ps(1.5f);

    for (const scaled_double_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256 vector_input = MM256_LOAD_PS(arg_inputs);

      __m256 va2p1 =
        _mm256_add_ps(one, _mm256_mul_ps(vector_input, vector_input));

      __m256 rsqrt = _mm256_rsqrt_ps(va2p1);

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      rsqrt = _mm256_mul_ps(
        rsqrt,
        _mm256_sub_ps(three_halfs,
                      _mm256_mul_ps(half, _mm256_mul_ps(va2p1, rsqrt2))));

      _mm256_store_ps(arg_outputs, _mm256_mul_ps(vector_input, rsqrt));
    }
#else
    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    {
      float input = SCALED2FLOAT(arg_inputs[ivalue]);

      arg_outputs[ivalue] = input / sqrtf(1.0f + input * input);
    }
#endif
  }
#endif
}

local void activation_rsqrt_double(int arg_nvalues,
                                   const scaled_double_t *restrict arg_inputs,
                                   scaled_double_t *restrict arg_outputs)
{
  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    arg_outputs[ivalue] = DOUBLE2SCALED(arg_inputs[ivalue]) /
                          sqrt(1.0 + SCALED2DOUBLE(arg_inputs[ivalue]) *
                                       SCALED2DOUBLE(arg_inputs[ivalue]));
}

local void activation_gelu_scaled(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
#ifdef USE_ERF
  const float c = 0.707107f;
#else
  const float k = 0.797884f;
  const float c = 0.044715f;
#endif

  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
  {
    float input = arg_inputs[ivalue];

#ifdef USE_ERF
    arg_outputs[ivalue] = 0.5f * input * (1.0f + erff(c * input));
#else
    float af2 = input * input;

    arg_outputs[ivalue] =
      0.5f * input * (1.0f + tanhf(k * (input + c * input * af2)));
#endif
  }
#else
#error not implemented
#endif
}

local void activation_gelu_double(int arg_nvalues,
                                  const scaled_double_t *restrict arg_inputs,
                                  scaled_double_t *restrict arg_outputs)
{
#ifdef USE_ERF
  const float c = 0.707107f;
#else
  const double k = 0.797884;
  const double c = 0.044715;
#endif

  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
  {
    double input = arg_inputs[ivalue];
#ifdef USE_ERF
    arg_outputs[ivalue] = 0.5f * input * (1.0f + erf(c * input));
#else
    double ad2 = input * input;

    arg_outputs[ivalue] =
      DOUBLE2SCALED(0.5 * input * (1.0 + tanhf(k * (input + c * input * ad2))));
#endif
  }
}

local void activation_geluv2_scaled(int arg_nvalues,
                                    const scaled_double_t *restrict arg_inputs,
                                    scaled_double_t *restrict arg_outputs)
{
#ifdef USE_AVX2_INTRINSICS
  if ((arg_nvalues < 8) or ((arg_nvalues % 8) != 0))
#endif
  {
    const float p = 0.544790f;

    for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
    {
      float input = arg_inputs[ivalue];
      float af2 = input * input;

      arg_outputs[ivalue] = 0.5f * input * (1.0f + input / sqrtf(p + af2));
    }
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    /*
        const __m256 p = _mm256_set1_ps(0.544790f);
        const __m256 half = _mm256_set1_ps(0.5f);
        const __m256 one = _mm256_set1_ps(1.0f);
        const __m256 three_halfs = _mm256_set1_ps(1.5f);

        for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
        {
          __m256 va = MM256_LOAD_PS(a);

          __m256 va2p = _mm256_add_ps(p, _mm256_mul_ps(va, va));

          __m256 rsqrt = _mm256_rsqrt_ps(va2p);

          // needed for Newton-Raphson

          __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

          rsqrt = _mm256_mul_ps(
            rsqrt,
            _mm256_sub_ps(three_halfs,
                          _mm256_mul_ps(half, _mm256_mul_ps(va2p, rsqrt2))));

          __m256 vp = _mm256_add_ps(one, _mm256_mul_ps(va, rsqrt));

          _mm256_store_ps(b, _mm256_mul_ps(_mm256_mul_ps(half, va), vp));
        }
    */
#ifdef USE_RSQRT
    const __m256 qp = _mm256_set1_ps(0.25f * 0.544790f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 three_halfs = _mm256_set1_ps(1.5f);

    for (const scaled_double_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256 vector_input = MM256_LOAD_PS(arg_inputs);

      __m256 vy = _mm256_mul_ps(half, vector_input);

      __m256 vy2 = _mm256_mul_ps(vy, vy);

      __m256 vy2qp = MM256_MADD_PS(vy, vy, qp);

      __m256 rsqrt = _mm256_rsqrt_ps(vy2qp);

      // needed for Newton-Raphson

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

#ifdef USE_FMA
      rsqrt = _mm256_mul_ps(
        rsqrt,
        _mm256_fnmadd_ps(_mm256_mul_ps(half, vy2qp), rsqrt2, three_halfs));
#else
      rsqrt = _mm256_mul_ps(
        rsqrt,
        _mm256_sub_ps(three_halfs,
                      _mm256_mul_ps(_mm256_mul_ps(half, vy2qp), rsqrt2)));
#endif

      __m256 vg = MM256_MADD_PS(vy2, rsqrt, vy);

      _mm256_store_ps(arg_outputs, vg);
    }
#else
    const __m256 p = _mm256_set1_ps(0.544790f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 one = _mm256_set1_ps(1.0f);

    for (const scaled_double_t *inputs_end = arg_inputs + arg_nvalues;
         arg_inputs < inputs_end;
         arg_inputs += 8, arg_outputs += 8)
    {
      __m256 vector_input = MM256_LOAD_PS(arg_inputs);

      __m256 va2 = _mm256_mul_ps(vector_input, vector_input);

      __m256 vsqrt = _mm256_sqrt_ps(_mm256_add_ps(p, va2));

      __m256 rsqrt = _mm256_add_ps(one, _mm256_div_ps(vector_input, vsqrt));

      __m256 vg = _mm256_mul_ps(_mm256_mul_ps(half, vector_input), rsqrt);

      _mm256_store_ps(arg_outputs, vg);
    }
#endif
#else
#error not implemented
#endif
  }
#endif
}

local void activation_geluv2_double(int arg_nvalues,
                                    const scaled_double_t *restrict arg_inputs,
                                    scaled_double_t *restrict arg_outputs)
{
  const double p = 0.544790;

  for (int ivalue = 0; ivalue < arg_nvalues; ivalue++)
  {
    double input = arg_inputs[ivalue];
    double ad2 = input * input;

    arg_outputs[ivalue] =
      DOUBLE2SCALED(0.5 * input * (1.0 + input / sqrt(p + ad2)));
  }
}

local void activation_scaled(int arg_activation,
                             int arg_nvalues,
                             const scaled_double_t *restrict arg_inputs,
                             scaled_double_t *restrict arg_outputs)
{
  if (arg_activation == NETWORK_ACTIVATION_RELU)
  {
    activation_relu_scaled(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_scaled(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_scaled(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELU)
  {
    activation_gelu_scaled(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELUV2)
  {
    activation_geluv2_scaled(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_LINEAR)
  {
    vcopy_a2b(arg_nvalues, arg_inputs, arg_outputs);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local void activation_double(int arg_activation,
                             int arg_nvalues,
                             const scaled_double_t *restrict arg_inputs,
                             scaled_double_t *restrict arg_outputs)
{
  if (arg_activation == NETWORK_ACTIVATION_RELU)
  {
    activation_relu_double(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_double(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_double(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELU)
  {
    activation_gelu_double(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELUV2)
  {
    activation_geluv2_double(arg_nvalues, arg_inputs, arg_outputs);
  }
  else if (arg_activation == NETWORK_ACTIVATION_LINEAR)
  {
    vcopy_a2b(arg_nvalues, arg_inputs, arg_outputs);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local double return_sigmoid(double arg_input)
{
  return (2.0 / (1.0 + exp(-arg_input)) - 1.0);
}

int base3_index_v2(const board_t *arg_board, ui64_t arg_pattern_bb)
{
  BEGIN_BLOCK(__func__)

  int result = 0;
  int power = 1;

  ui64_t white_man_bb = arg_board->B_white_men_bb;
  ui64_t black_man_bb = arg_board->B_black_men_bb;

  while (arg_pattern_bb != 0)
  {
    int ifield = BIT_CTZ(arg_pattern_bb);

    arg_pattern_bb &= ~BITULL(ifield);

    int value =
      !!(white_man_bb & BITULL(ifield)) + 2 * !!(black_man_bb & BITULL(ifield));

    result += value * power;

    power *= 3;
  }
  END_BLOCK
  return (result);
}

int base3_index_v3(const board_t *arg_board, ui64_t arg_pattern_bb)
{
  uint64_t white_mask = _pext_u64(arg_board->B_white_men_bb, arg_pattern_bb);
  uint64_t black_mask = _pext_u64(arg_board->B_black_men_bb, arg_pattern_bb);

  return (base3_table[white_mask | (black_mask << 8)]);
}

int base3_index_v4(const board_t *arg_board, ui64_t arg_pattern_bb)
{
  // HARDBUG(BIT_COUNT(arg_pattern_bb) > BASE3_PATTERN_BITS)

  uint32_t white_mask =
    (uint32_t)_pext_u64(arg_board->B_white_men_bb, arg_pattern_bb);
  uint32_t black_mask =
    (uint32_t)_pext_u64(arg_board->B_black_men_bb, arg_pattern_bb);

  HARDBUG((white_mask & black_mask) != 0)

  return (
    (int)(base3_mask_table[white_mask] + 2u * base3_mask_table[black_mask]));
}

// assumes inputs have been set

double return_dense_network_score_scaled(
  const dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state)
{
  BEGIN_BLOCK(__func__)

  const layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_state->DNS_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    ALIGN64(scaled_double_t activated_inputs[NINPUTS_MAX]);

    activation_scaled(arg_dense_network_shared->DNS_input_activation,
                      with_current->LS_ninputs,
                      arg_dense_network_state->DNS_inputs,
                      activated_inputs);

    // preserve inputs for return_dense_network_score_double

    HARDBUG(arg_dense_network_shared->DNS_compute_layer0 == FALSE)

    dense_layer0_dot(arg_dense_network_shared,
                     activated_inputs,
                     with_current_thread->LT_dot);
  }
  else
  {
    if (arg_dense_network_shared->DNS_compute_layer0)
    {
      dense_layer0_dot(arg_dense_network_shared,
                       arg_dense_network_state->DNS_inputs,
                       with_current_thread->LT_dot);
    }
  }

  // first layer

  BEGIN_BLOCK("Ax+b-and-activation-first-layer")

  vadd_ab2c(with_current->LS_noutputs,
            with_current_thread->LT_dot,
            with_current->LS_bias,
            with_current_thread->LT_sum);

  BEGIN_BLOCK("Ax+b-and-activation-first-layer-activation")

  activation_scaled(arg_dense_network_shared->DNS_activation,
                    with_current->LS_noutputs,
                    with_current_thread->LT_sum,
                    with_current_thread->LT_outputs);

  END_BLOCK

  END_BLOCK

  for (int ilayer = 1; ilayer < arg_dense_network_shared->DNS_nlayers; ilayer++)
  {
    with_current = arg_dense_network_shared->DNS_layers + ilayer;
    with_current_thread = arg_dense_network_state->DNS_layers + ilayer;

    layer_thread_t *with_previous_thread =
      arg_dense_network_state->DNS_layers + ilayer - 1;

    BEGIN_BLOCK("Ax+b-and-activation-other-layers")

    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    {
      with_current_thread->LT_dot[ioutput] =
        dot_scaled(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs +
                     (size_t)ioutput * with_current->LS_ninputs);
    }

    vadd_ab2c(with_current->LS_noutputs,
              with_current_thread->LT_dot,
              with_current->LS_bias,
              with_current_thread->LT_sum);

    if (with_current->LS_noutputs > 3)
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-activation")

      activation_scaled(arg_dense_network_shared->DNS_activation,
                        with_current->LS_noutputs,
                        with_current_thread->LT_sum,
                        with_current_thread->LT_outputs);

      END_BLOCK
    }
    else
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-last-layer")

      if (arg_dense_network_shared->DNS_final_activation ==
          NETWORK_ACTIVATION_LINEAR)
      {
        HARDBUG(with_current->LS_noutputs != 1)

        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_TANH)
      {
        HARDBUG(with_current->LS_noutputs != 1)

        with_current_thread->LT_outputs[0] =
          tanhf(with_current_thread->LT_sum[0]);
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_LOGITS)
      {
        HARDBUG(with_current->LS_noutputs != 3)

        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
        with_current_thread->LT_outputs[1] = with_current_thread->LT_sum[1];
        with_current_thread->LT_outputs[2] = with_current_thread->LT_sum[2];
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_2HEADS)
      {
        HARDBUG(with_current->LS_noutputs != 2)

        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
        with_current_thread->LT_outputs[1] = with_current_thread->LT_sum[1];
      }
      else
        FATAL("unknown activation_last option", EXIT_FAILURE)

      END_BLOCK
    }

    END_BLOCK
  }

  END_BLOCK

  layer_thread_t *with_thread = arg_dense_network_state->DNS_layers +
                                arg_dense_network_shared->DNS_nlayers - 1;

  if (with_current->LS_noutputs == 1)
  {
    return (SCALED2DOUBLE(with_thread->LT_outputs[0]));
  }
  else if (with_current->LS_noutputs == 2)
  {
    wdl_score = SCALED2DOUBLE(with_thread->LT_outputs[0]);
    mat_score = SCALED2DOUBLE(with_thread->LT_outputs[1]);

    /*
        PRINTF("wdl_score=%.6f mat_score=%.6f\n",
               wdl_score, mat_score);
    */

    return (wdl_score + 50 * mat_score);
    // return((wdl_score + mat_score) / 2.0);
    // return(wdl_score);
    // return(mat_score);
  }
  else if (with_current->LS_noutputs == 3)
  {
    double logits0 = SCALED2DOUBLE(with_thread->LT_outputs[0]);
    double logits1 = SCALED2DOUBLE(with_thread->LT_outputs[1]);
    double logits2 = SCALED2DOUBLE(with_thread->LT_outputs[2]);

    return (logits2 - logits0);

    // Mild drawish boost
    double v = logits2 - logits0;
    double g = logits1 - 0.5 * (logits2 + logits0);
    // double g = logits1 - fmax(logits2, logits0);
    double drawish = 1.0 / (1.0 + exp(-g));

    return (v * (1.0 + 1.0 * drawish));

    double logitsm = logits0;

    if (logits1 > logitsm)
      logitsm = logits1;

    if (logits2 > logitsm)
      logitsm = logits2;

    double exp0 = exp(logits0 - logitsm);
    double exp1 = exp(logits1 - logitsm);
    double exp2 = exp(logits2 - logitsm);

    double sum = exp0 + exp1 + exp2;

    double ploss = exp0 / sum;
    double pwin = exp2 / sum;

    // return (pwin - ploss);

    return ((logits2 - logits0) * (pwin + ploss));
  }
  else
  {
    return (0.0);
  }
}

double return_dense_network_score_double(
  const dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state)
{
  const layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_state->DNS_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    scaled_double_t activated_inputs[NINPUTS_MAX];

    activation_double(arg_dense_network_shared->DNS_input_activation,
                      with_current->LS_ninputs,
                      arg_dense_network_state->DNS_inputs,
                      activated_inputs);

    // preserve inputs for return_dense_network_score_double

    // always compute layer0

    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    {
      with_current_thread->LT_dot[ioutput] =
        dot_double(with_current->LS_ninputs,
                   activated_inputs,
                   with_current->LS_weights_noutputsxninputs +
                     (size_t)ioutput * with_current->LS_ninputs);
    }
  }
  else
  {
    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    {
      with_current_thread->LT_dot[ioutput] =
        dot_double(with_current->LS_ninputs,
                   arg_dense_network_state->DNS_inputs,
                   with_current->LS_weights_noutputsxninputs +
                     (size_t)ioutput * with_current->LS_ninputs);
    }
  }

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
  {
    with_current_thread->LT_sum[ioutput] =
      with_current_thread->LT_dot[ioutput] + with_current->LS_bias[ioutput];
  }

  activation_double(arg_dense_network_shared->DNS_activation,
                    with_current->LS_noutputs,
                    with_current_thread->LT_sum,
                    with_current_thread->LT_outputs);

  for (int ilayer = 1; ilayer < arg_dense_network_shared->DNS_nlayers; ilayer++)
  {
    with_current = arg_dense_network_shared->DNS_layers + ilayer;

    with_current_thread = arg_dense_network_state->DNS_layers + ilayer;

    layer_thread_t *with_previous_thread =
      arg_dense_network_state->DNS_layers + ilayer - 1;

    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    {
      with_current_thread->LT_sum[ioutput] =
        dot_double(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs +
                     (size_t)ioutput * with_current->LS_ninputs) +
        with_current->LS_bias[ioutput];
    }

    if (with_current->LS_noutputs > 1)
    {
      activation_double(arg_dense_network_shared->DNS_activation,
                        with_current->LS_noutputs,
                        with_current_thread->LT_sum,
                        with_current_thread->LT_outputs);
    }
    else
    {
      if (arg_dense_network_shared->DNS_final_activation ==
          NETWORK_ACTIVATION_LINEAR)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_TANH)
      {
        with_current_thread->LT_outputs[0] =
          tanh(with_current_thread->LT_sum[0]);
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_LOGITS)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_final_activation ==
               NETWORK_ACTIVATION_2HEADS)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else
        FATAL("unknown activation_last option", EXIT_FAILURE)
    }
  }

  layer_thread_t *with_thread = arg_dense_network_state->DNS_layers +
                                arg_dense_network_shared->DNS_nlayers - 1;

  return (SCALED2DOUBLE(with_thread->LT_outputs[0]));
}

local void
heap_sort_double(i64_t arg_nvalues, i64_t *arg_order, double *arg_values)
{
  i64_t iparent, ichild, iroot, saved_index;

  if (arg_nvalues < 2)
    return;

  for (iparent = 0; iparent < arg_nvalues; iparent++)
    arg_order[iparent] = iparent;

  iroot = arg_nvalues / 2;
  --arg_nvalues;

  while (TRUE)
  {
    if (iroot > 0)
    {
      saved_index = arg_order[--iroot];
    }
    else
    {
      saved_index = arg_order[arg_nvalues];
      arg_order[arg_nvalues] = arg_order[0];

      if (--arg_nvalues == 0)
      {
        arg_order[0] = saved_index;

        break;
      }
    }
    iparent = iroot;
    ichild = 2 * iroot + 1;

    while (ichild <= arg_nvalues)
    {
      if ((ichild < arg_nvalues) and
          (arg_values[arg_order[ichild]] < arg_values[arg_order[ichild + 1]]))
        ichild++;

      if (arg_values[saved_index] < arg_values[arg_order[ichild]])
      {
        arg_order[iparent] = arg_order[ichild];
        iparent = ichild;
        ichild = 2 * ichild + 1;
      }
      else
      {
        ichild = arg_nvalues + 1;
      }
    }
    arg_order[iparent] = saved_index;
  }

  for (iparent = 0; iparent < arg_nvalues; iparent++)
    HARDBUG(arg_values[arg_order[iparent]] > arg_values[arg_order[iparent + 1]])
}

local void update_mean_sigma(long long arg_nsamples,
                             double arg_sample,
                             double *arg_maximum,
                             double *arg_mean,
                             double *arg_squared_deviations)
{
  double previous_mean = *arg_mean;
  double previous_squared_deviations = *arg_squared_deviations;

  if (arg_sample > *arg_maximum)
    *arg_maximum = arg_sample;

  *arg_mean = previous_mean + (arg_sample - previous_mean) / arg_nsamples;

  *arg_squared_deviations =
    previous_squared_deviations +
    (arg_sample - previous_mean) * (arg_sample - *arg_mean);
}

static inline uint64_t mirror_bb(uint64_t arg_bb)
{
  ui64_t result;

  result = (arg_bb >> 6);

  result = BIT_REVERSE(result) >> (64 - 54);

  result = (result << 6);

  ui64_t debug = 0;

  while (arg_bb)
  {
    int ifield = BIT_CTZ(arg_bb);

    arg_bb &= ~BITULL(ifield);

    debug |= 1ULL << (65 - ifield);
  }

  HARDBUG(debug != result)

  return (result);
}

void fen2network(const char *arg_name, i64_t arg_npositions)
{
  board_t board;

  construct_board(&board, STDOUT, NETWORK0_ID);

  board_t *with_board = &board;

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  i64_t nfen = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    nfen++;

    if ((arg_npositions > 0) and (nfen >= arg_npositions))
      break;
  }

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen=%lld\n", nfen);

  i64_t *npieces_fen;

  MY_MALLOC_BY_TYPE(npieces_fen, i64_t, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    npieces_fen[ifen] = 0;

  double *score_scaled, *score_double;

  MY_MALLOC_BY_TYPE(score_scaled, double, nfen)
  MY_MALLOC_BY_TYPE(score_double, double, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    score_scaled[ifen] = score_double[ifen] = 0.0;

  double *delta_scaled, *delta_double;

  MY_MALLOC_BY_TYPE(delta_scaled, double, nfen)
  MY_MALLOC_BY_TYPE(delta_double, double, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    delta_scaled[ifen] = delta_double[ifen] = 0.0;

  double mse_vs_draw_scaled = 0.0;
  double mse_vs_draw_double = 0.0;
  double mse_scaled = 0.0;
  double mse_double = 0.0;

  double error_max_scaled_vs_double = 0.0;

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  progress_t progress;

  construct_progress(&progress, "fen2network:stats", nfen, 10);

  FILE *fcsv;

  fcsv = fopen("xyz.csv", "w");

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    if (!make_progress(&progress, ifen))
      continue;

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with_board, bdata(bfen));

    HARDBUG(IS_BLACK(with_board->B_colour2move))

    int nwhite_men = BIT_COUNT(with_board->B_white_men_bb);
    int nblack_men = BIT_COUNT(with_board->B_black_men_bb);
    int nwhite_kings = BIT_COUNT(with_board->B_white_kings_bb);
    int nblack_kings = BIT_COUNT(with_board->B_black_kings_bb);

    int npieces = nwhite_men + nblack_men + nwhite_kings + nblack_kings;

    if ((NPIECES_PERCENTILE != INVALID) and (npieces != NPIECES_PERCENTILE))
      continue;

    npieces_fen[ifen] = npieces;

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with_board, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    // HARDBUG(fabs(result) >= 0.999999)

    // check_moves(with, &moves_list);

    int material_score = return_material_score(with_board);

    return_score_from_board(with_board, score_scaled + ifen);

    if (ifen < 100000)
      fprintf(fcsv, "%.3f,%.3f,%.3f\n", wdl_score, mat_score, result);

    network_t *with_network = networks + with_board->B_network_id;

    score_double[ifen] = return_dense_network_score_double(
      &(with_network->N_shared_dense_network),
      &(with_board->B_network_state.NS_dense_network_state));

    if (ifen <= 100)
      PRINTF("%s {%.6f} ms=%d ss=%.6f sd=%.6f r=%.6f\n",
             bdata(bfen),
             result,
             material_score,
             score_scaled[ifen],
             score_double[ifen],
             result);

    delta_scaled[ifen] = fabs(score_scaled[ifen] - result);
    delta_double[ifen] = fabs(score_double[ifen] - result);

    mse_vs_draw_scaled += score_scaled[ifen] * score_scaled[ifen];

    mse_vs_draw_double += score_double[ifen] * score_double[ifen];

    mse_scaled += delta_scaled[ifen] * delta_scaled[ifen];

    mse_double += delta_double[ifen] * delta_double[ifen];

    double error = fabs(score_scaled[ifen] - score_double[ifen]);

    if (error > error_max_scaled_vs_double)
    {
      error_max_scaled_vs_double = error;

      PRINTF("%s {%.6f} ms=%d nss=%.6f nsd=%.6f r=%.6f\n",
             bdata(bfen),
             result,
             material_score,
             score_scaled[ifen],
             score_double[ifen],
             result);

      PRINTF("error_max_scaled_vs_double=%.6f\n", error_max_scaled_vs_double);
    }
  }
  fclose(fcsv);
  finalize_progress(&progress);

  destroy_progress(&progress);

  destroy_my_bstream(&my_bstream);

  my_mpi_allreduce(MPI_IN_PLACE,
                   npieces_fen,
                   nfen,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   score_scaled,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   score_double,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   delta_scaled,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   delta_double,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_vs_draw_scaled,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_vs_draw_double,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_scaled,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_double,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &error_max_scaled_vs_double,
                   1,
                   MPI_DOUBLE,
                   MPI_MAX,
                   my_mpi_globals.MMG_slave_comm);

  PRINTF("delta npieces\n");

  for (int ipiece = 1; ipiece <= 2 * NPIECES_MAX + 1; ipiece++)
  {
    i64_t ndelta = 0;
    double delta_scaled_max = 0.0;
    double delta_scaled_mean = 0.0;
    double delta_scaled_sigma = 0.0;
    double delta_double_max = 0.0;
    double delta_double_mean = 0.0;
    double delta_double_sigma = 0.0;

    for (i64_t ifen = 0; ifen < nfen; ifen++)
    {
      if (npieces_fen[ifen] != ipiece)
        continue;

      ndelta++;

      update_mean_sigma(ndelta,
                        delta_scaled[ifen],
                        &delta_scaled_max,
                        &delta_scaled_mean,
                        &delta_scaled_sigma);

      update_mean_sigma(ndelta,
                        delta_double[ifen],
                        &delta_double_max,
                        &delta_double_mean,
                        &delta_double_sigma);
    }
    PRINTF("ipiece=%d nd=%lld ds_max=%.6f ds_mean=%.6f ds_sigma=%.6f"
           " dd_max=%.6f dd_mean=%.6f dd_sigma=%.6f\n",
           ipiece,
           ndelta,
           delta_scaled_max,
           delta_scaled_mean,
           sqrt(delta_scaled_sigma / ndelta),
           delta_double_max,
           delta_double_mean,
           sqrt(delta_double_sigma / ndelta));
  }

  i64_t *pdelta_scaled;

  MY_MALLOC_BY_TYPE(pdelta_scaled, i64_t, nfen)

  heap_sort_double(nfen, pdelta_scaled, delta_scaled);

  PRINTF("ds_max=%.6f ss_max=%.6f dd_max=%.6f ds_max=%.6f\n",
         delta_scaled[pdelta_scaled[nfen - 1]],
         score_scaled[pdelta_scaled[nfen - 1]],
         delta_double[pdelta_scaled[nfen - 1]],
         score_double[pdelta_scaled[nfen - 1]]);

  PRINTF("delta percentiles\n");

  for (int ipercentile = 1; ipercentile <= 99; ++ipercentile)
  {
    i64_t ifen = round(ipercentile / 100.0 * nfen);

    HARDBUG(ifen >= nfen)

    PRINTF("ipercentile=%d delta_scaled=%.6f score_scaled=%.6f"
           " delta_double=%.6f score_double=%.6f\n",
           ipercentile,
           delta_scaled[pdelta_scaled[ifen]],
           score_scaled[pdelta_scaled[ifen]],
           delta_double[pdelta_scaled[ifen]],
           score_double[pdelta_scaled[ifen]]);
  }

  PRINTF("mse_vs_draw_scaled=%.6f\n"
         "rmse_vs_draw_scaled=%.6f\n"
         "mse_scaled=%.6f\n"
         "rmse_scaled=%.6f\n",
         mse_vs_draw_scaled / nfen,
         sqrt(mse_vs_draw_scaled / nfen),
         mse_scaled / nfen,
         sqrt(mse_scaled / nfen));

  PRINTF("mse_vs_draw_double=%.6f\n"
         "rmse_vs_draw_double=%.6f\n"
         "mse_double=%.6f\n"
         "rmse_double=%.6f\n",
         mse_vs_draw_double / nfen,
         sqrt(mse_vs_draw_double / nfen),
         mse_double / nfen,
         sqrt(mse_double / nfen));

  PRINTF("error_max_scaled_vs_double=%.6f\n", error_max_scaled_vs_double);

  i64_t ipercentile = round(90 / 100.0 * nfen);
  double percentile = delta_scaled[pdelta_scaled[ipercentile]];

  PRINTF("percentile=%.6f\n", percentile);

  construct_my_bstream(&my_bstream, arg_name);

  construct_progress(&progress, "fen2network:factor", nfen, 10);

  i64_t nfactor[NPIECES_MAX + 1];
  double factor[NPIECES_MAX + 1];
  double score[NPIECES_MAX + 1];

  for (int ipiece = 0; ipiece <= NPIECES_MAX; ++ipiece)
  {
    nfactor[ipiece] = 0;
    factor[ipiece] = 0;
    score[ipiece] = 0;
  }

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    if (!make_progress(&progress, ifen))
      continue;

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with_board, bdata(bfen));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with_board, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    // HARDBUG(fabs(result) >= 0.999999)

    // check_moves(with, &moves_list);

    int nwhite_kings = BIT_COUNT(with_board->B_white_kings_bb);
    int nblack_kings = BIT_COUNT(with_board->B_black_kings_bb);

    if ((nwhite_kings > 0) or (nblack_kings > 0))
      continue;

    int nwhite_men = BIT_COUNT(with_board->B_white_men_bb);
    int nblack_men = BIT_COUNT(with_board->B_black_men_bb);

    if ((nwhite_men == 0) or (nblack_men == 0))
      continue;

    int delta_men = nwhite_men - nblack_men;

    if (delta_men < 0)
      continue;

    // if (fabs(score_scaled[ifen] - result) > percentile)
    //   continue;

    nfactor[delta_men]++;
    factor[delta_men] += fabs(score_scaled[ifen]);
    score[delta_men] += ABS_SCORE_T(return_score_from_board(with_board, NULL));
  }

  finalize_progress(&progress);

  destroy_progress(&progress);

  destroy_my_bstream(&my_bstream);

  BDESTROY(bfen)

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfactor,
                   NPIECES_MAX + 1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &factor,
                   NPIECES_MAX + 1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &score,
                   NPIECES_MAX + 1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  for (int ipiece = 0; ipiece <= NPIECES_MAX; ++ipiece)
  {
    if (nfactor[ipiece] == 0)
      continue;

    factor[ipiece] /= nfactor[ipiece];
    score[ipiece] /= nfactor[ipiece];

    double factor2man_score = (ipiece * SCORE_MAN) / factor[ipiece];

    PRINTF("ipiece=%d"
           " nfactor[ipiece]=%lld"
           " factor[ipiece]=%.6f"
           " score[ipiece]=%.6f"
           " factor2man_score=%.6f\n",
           ipiece,
           nfactor[ipiece],
           factor[ipiece],
           score[ipiece],
           factor2man_score);
  }
}

local void append_fen(const board_t *arg_with_board,
                      const_bstring arg_bfen,
                      double arg_result,
                      fbuffer_t *arg_fcsv,
                      fbuffer_t *arg_ffen)
{
  /*
    static int n = 0;

    if (n++ < 100)
    {
      print_board(arg_with_board);

      PRINTF("mobility=[%d,%d] immobility=[%d,%d]\n",
             return_mobility(arg_with_board, WHITE_ENUM),
             return_mobility(arg_with_board, BLACK_ENUM),
             return_immobility(arg_with_board, WHITE_ENUM),
             return_immobility(arg_with_board, BLACK_ENUM));
    }
  */

  // always WTM

  HARDBUG(IS_BLACK(arg_with_board->B_colour2move))

  int nwhite_men = BIT_COUNT(arg_with_board->B_white_men_bb);
  int nblack_men = BIT_COUNT(arg_with_board->B_black_men_bb);
  int nwhite_kings = BIT_COUNT(arg_with_board->B_white_kings_bb);
  int nblack_kings = BIT_COUNT(arg_with_board->B_black_kings_bb);

  network_t *with_network = networks + arg_with_board->B_network_id;

  for (int ipattern = 0;
       ipattern < with_network->N_shared_patterns.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_shared_patterns.PSS_patterns + ipattern;

    if (with_pattern_shared->PS_id == PATTERN_BASE3_INDEX_ID)
    {
      int input = base3_index_v2(arg_with_board, with_pattern_shared->PS_bb);

      bin_t ibin = input;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else
      FATAL("unknown with_pattern_shared->PS_id", EXIT_FAILURE)
  }

  for (int imaterial = 0;
       imaterial < with_network->N_shared_materials.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *with_material_shared =
      with_network->N_shared_materials.MSS_materials + imaterial;

    if (with_material_shared->MS_id == MATERIAL_NWHITE_MEN_ID)
    {
      bin_t ibin = nwhite_men;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_MEN_ID)
    {
      bin_t ibin = nblack_men;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NWHITE_KINGS_ID)
    {
      int ncapped_white_kings = nwhite_kings;

      if (ncapped_white_kings > MATERIAL_KING_MAX)
        ncapped_white_kings = MATERIAL_KING_MAX;

      bin_t ibin = ncapped_white_kings;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_KINGS_ID)
    {
      int ncapped_black_kings = nblack_kings;

      if (ncapped_black_kings > MATERIAL_KING_MAX)
        ncapped_black_kings = MATERIAL_KING_MAX;

      bin_t ibin = ncapped_black_kings;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NWHITE_KINGS20_ID)
    {
      bin_t ibin = nwhite_kings;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_KINGS20_ID)
    {
      bin_t ibin = nblack_kings;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_MAN_KING_ID)
    {
      int ncapped_white_kings = nwhite_kings;

      if (ncapped_white_kings > MATERIAL_KING_MAX)
        ncapped_white_kings = MATERIAL_KING_MAX;

      int ncapped_black_kings = nblack_kings;

      if (ncapped_black_kings > MATERIAL_KING_MAX)
        ncapped_black_kings = MATERIAL_KING_MAX;

      bin_t ibin =
        (nwhite_men * (MATERIAL_KING_MAX + 1) + ncapped_white_kings) *
          (NPIECES_MAX + 1) * (MATERIAL_KING_MAX + 1) +
        nblack_men * (MATERIAL_KING_MAX + 1) + ncapped_black_kings;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else
      FATAL("unknown with_material_shared->MS_id", EXIT_FAILURE)
  }

  if (with_network->N_final_activation == NETWORK_ACTIVATION_LINEAR)
  {
    float result = arg_result;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));
  }
  else if (with_network->N_final_activation == NETWORK_ACTIVATION_LOGITS)
  {
    i32_t result = INVALID;

    if (fabs(arg_result - (-1.0)) < 1.0e-3)
      result = 0;
    else if (fabs(arg_result) < 1.0e-3)
      result = 1;
    else if (fabs(arg_result - 1.0) < 1.0e-3)
      result = 2;
    else
      FATAL("unknown arg_result", EXIT_FAILURE)

    append_fbuffer_bin(arg_fcsv, &result, sizeof(i32_t));
  }
  else if (with_network->N_final_activation == NETWORK_ACTIVATION_2HEADS)
  {
    float result = arg_result;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));

    result = (float)return_material_score(arg_with_board) / 1000000.0f;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));
  }
  else
    FATAL("unknown with_network->N_final_activation", EXIT_FAILURE)

  append_fbuffer_bin(arg_fcsv, NULL, 0);

  if (arg_ffen != NULL)
    append_fbuffer_fmt(arg_ffen, "%s {%.6f}\n", bdata(arg_bfen), arg_result);
}

void fen2csv(const char *arg_name, int arg_npieces_min, int arg_npieces_max)
{
  my_random_t fen2csv_random;

  options.material_only = TRUE;

  if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_lead_slave)
    construct_my_random(&fen2csv_random, NULL);
  else
  {
    ui64_t seed;

    construct_my_random(&fen2csv_random, &seed);

    PRINTF("fen2csv seed=%llX\n", seed);
  }

  board_t board;

  construct_board(&board, STDOUT, NETWORK0_ID);

  board_t *with_board = &board;

  BSTRING(barg_name)

  HARDBUG(bassigncstr(barg_name, arg_name) == BSTR_ERR)

  BSTRING(btag)

  network_t *with_network = networks + with_board->B_network_id;

  HARDBUG(
    bformata(btag,
             "%s-nm%d-%s-np%d-%d-mirror",
             bdata(with_network->N_bshape),
             with_network->N_shared_materials.MSS_nmaterials,
             activation_last2string(with_network->N_final_activation, TRUE),
             arg_npieces_min,
             arg_npieces_max) == BSTR_ERR)

  // training

  BSTRING(bsuffix)

  HARDBUG(bformata(bsuffix, "-%s-training.bin", bdata(btag)) == BSTR_ERR)

  fbuffer_t ftraining;

  // add size of embddings

  if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_lead_slave)
  {
    PRINTF("adding states..\n");

    construct_fbuffer(&ftraining, barg_name, bdata(bsuffix), TRUE);

    for (int ipattern = 0;
         ipattern < with_network->N_shared_patterns.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_shared_patterns.PSS_patterns + ipattern;

      int nstates = with_pattern_shared->PS_nstates;

      PRINTF("ipattern=%d nstates=%d\n", ipattern, nstates);

      bin_t ibin = nstates;

      append_fbuffer_bin(&ftraining, &ibin, sizeof(bin_t));
    }

    for (int imaterial = 0;
         imaterial < with_network->N_shared_materials.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_shared_materials.MSS_materials + imaterial;

      int nstates = with_material_shared->MS_nstates;

      PRINTF("imaterial=%d nstates=%d\n", imaterial, nstates);

      bin_t ibin = nstates;

      append_fbuffer_bin(&ftraining, &ibin, sizeof(bin_t));
    }

    flush_fbuffer(&ftraining, 0);
  }
  else
  {
    construct_fbuffer(&ftraining, barg_name, bdata(bsuffix), FALSE);
  }

  my_mpi_barrier_v3("after adding states",
                    my_mpi_globals.MMG_slave_comm,
                    my_mpi_globals.MMG_fen2csv_barrier_semkey,
                    TRUE);

  // validation

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s-validation.bin", bdata(btag)) == BSTR_ERR)

  fbuffer_t fvalidation;

  construct_fbuffer(&fvalidation, barg_name, bdata(bsuffix), TRUE);

  // fen

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s.fen", bdata(btag)) == BSTR_ERR)

  fbuffer_t ffen;

  construct_fbuffer(&ffen, barg_name, bdata(bsuffix), TRUE);

  // done

  PRINTF("training_name=%s\n", bdata(return_fbuffer_bname(&ftraining)));
  PRINTF("validation_name=%s\n", bdata(return_fbuffer_bname(&fvalidation)));
  PRINTF("fen_name=%s\n", bdata(return_fbuffer_bname(&ffen)));

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  i64_t nfen = 0;

  int label_white2move = FALSE;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
    {
      if (biseqcaselessStatic(bline, "#pragma label_white2move"))
      {
        PRINTF("WARNING: #PRAGMA label_white2move\n");

        label_white2move = TRUE;
      }
    }

    nfen++;
  }

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen=%lld\n", nfen);

  stats_t stats_original_labels;

  stats_t stats_training_labels;

  stats_t stats_validation_labels;

  construct_stats(&stats_original_labels, "stats_original_labels");

  construct_stats(&stats_training_labels, "stats_training_labels");

  construct_stats(&stats_validation_labels, "stats_validation_labels");

  bucket_t bucket_original_labels;

  construct_bucket(&bucket_original_labels,
                   "bucket_original_labels",
                   0.1,
                   -1.2,
                   1.2,
                   BUCKET_LINEAR);

  bucket_t bucket_training_labels;

  construct_bucket(&bucket_training_labels,
                   "bucket_training_labels",
                   0.1,
                   -1.2,
                   1.2,
                   BUCKET_LINEAR);

  bucket_t bucket_validation_labels;

  construct_bucket(&bucket_validation_labels,
                   "bucket_validation_labelS",
                   0.1,
                   -1.2,
                   1.2,
                   BUCKET_LINEAR);

  bucket_t bucket_material_score;

  construct_bucket(&bucket_material_score,
                   "bucket_material_score",
                   SCORE_MAN,
                   SCORE_LOST,
                   SCORE_WON,
                   BUCKET_LINEAR);

  bucket_t bucket_nwhite_men;

  construct_bucket(&bucket_nwhite_men,
                   "bucket_nwhite_man",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nblack_men;

  construct_bucket(&bucket_nblack_men,
                   "bucket_nblack_man",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nwhite_kings;

  construct_bucket(&bucket_nwhite_kings,
                   "bucket_nwhite_kings",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nblack_kings;

  construct_bucket(&bucket_nblack_kings,
                   "bucket_nblack_kings",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nmen;

  construct_bucket(&bucket_nmen,
                   "bucket_nman",
                   1,
                   0,
                   2 * NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nkings;

  construct_bucket(&bucket_nkings,
                   "bucket_nkings",
                   1,
                   0,
                   2 * NPIECES_MAX,
                   BUCKET_LINEAR);

  BSTRING(bdata_name)

  HARDBUG(bassigncstr(bdata_name, "/tmp5/gwies/data.csv") == BSTR_ERR)

  fbuffer_t fdata;

  construct_fbuffer(&fdata, bdata_name, NULL, TRUE);

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  progress_t progress;

  construct_progress(&progress, "fen2csv", nfen, 10);

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    if (!make_progress(&progress, ifen))
      continue;

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    double result;

    CSTRING(cfen, blength(bline))

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf}", cfen, &result) != 2)

    update_stats(&stats_original_labels, result);

    update_bucket(&bucket_original_labels, result);

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(with_board, bdata(bfen));

    CDESTROY(cfen)

    if (IS_BLACK(with_board->B_colour2move))
    {
      if (label_white2move)
        result = -result;

      ui64_t white_man_bb = mirror_bb(with_board->B_black_men_bb);
      ui64_t black_man_bb = mirror_bb(with_board->B_white_men_bb);
      ui64_t white_king_bb = mirror_bb(with_board->B_black_kings_bb);
      ui64_t black_king_bb = mirror_bb(with_board->B_white_kings_bb);

      with_board->B_white_men_bb = white_man_bb;
      with_board->B_black_men_bb = black_man_bb;
      with_board->B_white_kings_bb = white_king_bb;
      with_board->B_black_kings_bb = black_king_bb;

      with_board->B_colour2move = WHITE_ENUM;

      board2fen(with_board, bfen, FALSE);
    }

    int nwhite_men = BIT_COUNT(with_board->B_white_men_bb);

    int nblack_men = BIT_COUNT(with_board->B_black_men_bb);

    int nmen = nwhite_men + nblack_men;

    int nwhite_kings = BIT_COUNT(with_board->B_white_kings_bb);

    int nblack_kings = BIT_COUNT(with_board->B_black_kings_bb);

    int nkings = nwhite_kings + nblack_kings;

    int npieces = nmen + nkings;

    if ((npieces < arg_npieces_min) or (npieces > arg_npieces_max))
      continue;

    moves_list_t moves_list;

    gen_moves(with_board, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
      continue;

    int material_score = return_material_score(with_board);

    // if ((material_score != 0) and (fabs(result) < 1.0e-3)) continue;
    // if ((material_score < 0) and (result > 1.0e-3))
    //   continue;
    // if ((material_score > 0) and (result < -1.0e-3))
    //   continue;

    update_bucket(&bucket_material_score, material_score);

    update_bucket(&bucket_nwhite_men, nwhite_men);

    update_bucket(&bucket_nblack_men, nblack_men);

    update_bucket(&bucket_nwhite_kings, nwhite_kings);

    update_bucket(&bucket_nblack_kings, nblack_kings);

    update_bucket(&bucket_nmen, nmen);

    update_bucket(&bucket_nkings, nkings);

    {
      int delta_nmen = nwhite_men - nblack_men;
      int delta_nkings = nwhite_kings - nblack_kings;

      nmen = nwhite_men + nblack_men;

      double result_wtm = result;

      if (IS_BLACK(with_board->B_colour2move))
        result_wtm = -result_wtm;

      append_fbuffer_fmt(&fdata,
                         "%d,%d,%.6f\n",
                         delta_nmen,
                         delta_nkings,
                         result_wtm);
    }

    int prob = return_my_random(&fen2csv_random) % 100;

    if (prob <= 79)
    {
      append_fen(with_board, bfen, result, &ftraining, NULL);

      update_stats(&stats_training_labels, result);

      update_bucket(&bucket_training_labels, result);
    }
    else
    {
      append_fen(with_board, bfen, result, &fvalidation, &ffen);

      update_stats(&stats_validation_labels, result);

      update_bucket(&bucket_validation_labels, result);
    }
  }

  finalize_progress(&progress);

  destroy_progress(&progress);

  destroy_my_bstream(&my_bstream);

  flush_fbuffer(&ftraining, 0);
  flush_fbuffer(&fvalidation, 0);
  flush_fbuffer(&fdata, 0);

  BDESTROY(bfen)

  BDESTROY(bdata_name)

  my_stats_aggregate(&stats_original_labels, my_mpi_globals.MMG_slave_comm);

  my_stats_aggregate(&stats_training_labels, my_mpi_globals.MMG_slave_comm);

  my_stats_aggregate(&stats_validation_labels, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_original_labels,
                          my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_training_labels,
                          my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_validation_labels,
                          my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_material_score,
                          my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nwhite_men, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nblack_men, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nblack_kings, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nmen, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nkings, my_mpi_globals.MMG_slave_comm);

  mean_sigma(&stats_original_labels);
  printf_stats(&stats_original_labels);

  mean_sigma(&stats_validation_labels);
  printf_stats(&stats_validation_labels);

  mean_sigma(&stats_training_labels);
  printf_stats(&stats_training_labels);

  printf_bucket(&bucket_original_labels);

  printf_bucket(&bucket_training_labels);

  printf_bucket(&bucket_validation_labels);

  printf_bucket(&bucket_material_score);

  printf_bucket(&bucket_nwhite_men);

  printf_bucket(&bucket_nblack_men);

  printf_bucket(&bucket_nwhite_kings);

  printf_bucket(&bucket_nblack_kings);

  printf_bucket(&bucket_nmen);

  printf_bucket(&bucket_nkings);

  // write remaining

  flush_fbuffer(&ffen, 0);

  flush_fbuffer(&fvalidation, 0);

  flush_fbuffer(&ftraining, 0);

  BDESTROY(bsuffix)

  BDESTROY(btag)

  BDESTROY(barg_name)
}

void evaluate_material_network(const network_t *arg_with_network,
                               dense_network_state_t *arg_state,
                               int arg_nwhite_men,
                               int arg_nblack_men,
                               int arg_nwhite_kings,
                               int arg_nblack_kings,
                               int arg_inputs)
{
  const dense_network_shared_t *dense =
    &arg_with_network->N_shared_material_dense_network;

  int ilast = dense->DNS_nlayers - 1;

  HARDBUG(ilast < 0)

  int noutputs = dense->DNS_layers[ilast].LS_noutputs;

  HARDBUG((arg_nwhite_men < 0) or (arg_nblack_men < 0) or
          (arg_nwhite_kings < 0) or (arg_nblack_kings < 0))

  // Diagnostics need the intermediate inputs as well as the final outputs.
  if (!arg_inputs and (arg_with_network->N_material_cache != NULL) and
      (arg_nwhite_men <= NPIECES_MAX) and (arg_nblack_men <= NPIECES_MAX) and
      (arg_nwhite_kings <= NPIECES_MAX) and (arg_nblack_kings <= NPIECES_MAX))
  {
    size_t index = ((arg_nwhite_men * (NPIECES_MAX + 1) + arg_nblack_men) *
                      (NPIECES_MAX + 1) +
                    arg_nwhite_kings) *
                     (NPIECES_MAX + 1) +
                   arg_nblack_kings;
    memcpy(arg_state->DNS_layers[ilast].LT_outputs,
           arg_with_network->N_material_cache + index * noutputs,
           noutputs * sizeof(scaled_double_t));

    return;
  }

#ifdef USE_AVX2_INTRINSICS
  __m256 sum0, sum1, sum64[8];
#endif

  for (int iinput = 0;
       iinput < arg_with_network->N_shared_material_dense_network.DNS_ninputs;
       iinput++)
    arg_state->DNS_inputs[iinput] = 0;

  const layer_shared_t *with_current =
    arg_with_network->N_shared_material_dense_network.DNS_layers;

  layer_thread_t *with_current_thread = arg_state->DNS_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

#ifdef USE_AVX2_INTRINSICS
  if (arg_with_network->N_material_dense_network_16x8_kernel)
  {
    sum0 = _mm256_setzero_ps();
    sum1 = _mm256_setzero_ps();
  }

  if (arg_with_network->N_material_dense_network_64x16_kernel)
    for (int ichunk = 0; ichunk < 8; ichunk++)
      sum64[ichunk] = _mm256_setzero_ps();
#endif

  for (int imaterial = 0;
       imaterial < arg_with_network->N_shared_materials.MSS_nmaterials;
       imaterial++)
  {
    const material_shared_t *with_material_shared =
      arg_with_network->N_shared_materials.MSS_materials + imaterial;

    int istate = INVALID;

    if (with_material_shared->MS_id == MATERIAL_NWHITE_MEN_ID)
    {
      istate = arg_nwhite_men;
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_MEN_ID)
    {
      istate = arg_nblack_men;
    }
    else if ((with_material_shared->MS_id == MATERIAL_NWHITE_KINGS_ID) or
             (with_material_shared->MS_id == MATERIAL_NWHITE_KINGS20_ID))
    {
      istate = arg_nwhite_kings;

      if (istate >= with_material_shared->MS_nstates)
        istate = with_material_shared->MS_nstates - 1;
    }
    else if ((with_material_shared->MS_id == MATERIAL_NBLACK_KINGS_ID) or
             (with_material_shared->MS_id == MATERIAL_NBLACK_KINGS20_ID))
    {
      istate = arg_nblack_kings;

      if (istate >= with_material_shared->MS_nstates)
        istate = with_material_shared->MS_nstates - 1;
    }
    else if (with_material_shared->MS_id == MATERIAL_MAN_KING_ID)
    {
      int ncapped_white_kings = arg_nwhite_kings;
      int ncapped_black_kings = arg_nblack_kings;

      if (ncapped_white_kings > MATERIAL_KING_MAX)
        ncapped_white_kings = MATERIAL_KING_MAX;

      if (ncapped_black_kings > MATERIAL_KING_MAX)
        ncapped_black_kings = MATERIAL_KING_MAX;

      istate =
        (arg_nwhite_men * (MATERIAL_KING_MAX + 1) + ncapped_white_kings) *
          (NPIECES_MAX + 1) * (MATERIAL_KING_MAX + 1) +
        arg_nblack_men * (MATERIAL_KING_MAX + 1) + ncapped_black_kings;
    }

    HARDBUG((istate < 0) or (istate >= with_material_shared->MS_nstates))

    if (arg_with_network->N_material_architecture == NETWORK_ARCHITECTURE_ADD)
    {
#ifdef USE_AVX2_INTRINSICS
      if (arg_with_network->N_material_dense_network_16x8_kernel)
      {
        const float *weights = with_material_shared->MS_weights_nstatesxnembed +
                               (size_t)istate * with_material_shared->MS_nembed;

        // MPI shared allocations need not be 32-byte aligned.
        sum0 = _mm256_add_ps(sum0, _mm256_loadu_ps(weights));
        sum1 = _mm256_add_ps(sum1, _mm256_loadu_ps(weights + 8));
      }
      else if (arg_with_network->N_material_dense_network_64x16_kernel)
      {
        const float *weights = with_material_shared->MS_weights_nstatesxnembed +
                               (size_t)istate * with_material_shared->MS_nembed;

        for (int ichunk = 0; ichunk < 8; ichunk++)
          sum64[ichunk] =
            _mm256_add_ps(sum64[ichunk], _mm256_loadu_ps(weights + 8 * ichunk));
      }
      else
#endif
        vadd_ab2a(with_material_shared->MS_nembed,
                  arg_state->DNS_inputs + with_material_shared->MS_offset,
                  with_material_shared->MS_weights_nstatesxnembed +
                    (size_t)istate * with_material_shared->MS_nembed);
    }
    else if (arg_with_network->N_material_architecture ==
             NETWORK_ARCHITECTURE_CONCAT)
    {
      if (arg_inputs)
        vcopy_a2b(with_material_shared->MS_nembed,
                  with_material_shared->MS_weights_nstatesxnembed +
                    (size_t)istate * with_material_shared->MS_nembed,
                  arg_state->DNS_inputs + with_material_shared->MS_offset);

      vadd_ab2a(with_current->LS_noutputs,
                with_current_thread->LT_dot,
                with_material_shared->MS_sum_nstatesxnoutputs[istate]);
    }
    else
      FATAL("unknown with_network->N_architecture_maerial", EXIT_FAILURE)
  }
#ifdef USE_AVX2_INTRINSICS
  if (arg_with_network->N_material_dense_network_16x8_kernel)
  {
    _mm256_store_ps(arg_state->DNS_inputs, sum0);
    _mm256_store_ps(arg_state->DNS_inputs + 8, sum1);
  }
  else if (arg_with_network->N_material_dense_network_64x16_kernel)
  {
    for (int ichunk = 0; ichunk < 8; ichunk++)
      _mm256_store_ps(arg_state->DNS_inputs + 8 * ichunk, sum64[ichunk]);
  }
#endif

  (void)return_dense_network_score_scaled(
    &(arg_with_network->N_shared_material_dense_network),
    arg_state);
}

local void construct_material_cache(network_t *arg_network)
{
  arg_network->N_material_cache = NULL;

  if ((arg_network->N_network_id == INVALID) or
      (arg_network->N_material_architecture == NETWORK_ARCHITECTURE_NULL))
    return;

  dense_network_shared_t *dense = &arg_network->N_shared_material_dense_network;

  if (dense->DNS_nlayers == 0)
    return; // A dummy network has no loaded weights.

  int ilast = dense->DNS_nlayers - 1;
  int noutputs = dense->DNS_layers[ilast].LS_noutputs;
  size_t ncounts = NPIECES_MAX + 1;
  size_t nentries = ncounts * ncounts * ncounts * ncounts;
  void *memory;
  MPI_Win win;

  my_mpi_allocate_shared(my_mpi_globals.MMG_global_comm,
                         sizeof(scaled_double_t),
                         nentries * noutputs,
                         &memory,
                         &win);

  scaled_double_t *cache = memory;

  if (my_mpi_globals.MMG_is_master)
  {
    dense_network_state_t state = {0};

    construct_dense_network_state(dense, &state);

    size_t ientry = 0;
    // Fill every count tuple, including unreachable ones: no uninitialized
    // rows.
    for (int iwhite_man = 0; iwhite_man <= NPIECES_MAX; iwhite_man++)
      for (int iblack_man = 0; iblack_man <= NPIECES_MAX; iblack_man++)
        for (int iwhite_king = 0; iwhite_king <= NPIECES_MAX; iwhite_king++)
          for (int iblack_king = 0; iblack_king <= NPIECES_MAX; iblack_king++)
          {
            evaluate_material_network(arg_network,
                                      &state,
                                      iwhite_man,
                                      iblack_man,
                                      iwhite_king,
                                      iblack_king,
                                      TRUE);
            memcpy(cache + ientry * noutputs,
                   state.DNS_layers[ilast].LT_outputs,
                   noutputs * sizeof(scaled_double_t));
            ientry++;
          }

    for (int ilayer = 0; ilayer < dense->DNS_nlayers; ilayer++)
    {
      my_free(state.DNS_layers[ilayer].LT_dot);
      my_free(state.DNS_layers[ilayer].LT_sum);
      my_free(state.DNS_layers[ilayer].LT_outputs);
    }
  }
  my_mpi_sync_shared(my_mpi_globals.MMG_global_comm, win, "material_cache");
  MARK_ARRAY_READ_ONLY(cache, nentries * noutputs * sizeof(scaled_double_t));
  // Published only after initialization; search threads never write this table.
  arg_network->N_material_cache = cache;
  PRINTF("material cache network=%d entries=%zu outputs=%d bytes=%zu\n",
         arg_network->N_network_id,
         nentries,
         noutputs,
         nentries * noutputs * sizeof(scaled_double_t));
}

void init_networks(void)
{
  init_base64_table();
  init_base3_table();
  init_base3_mask_table();

  networks[NETWORK0_ID].N_network_id = INVALID;
  networks[NETWORK1_ID].N_network_id = INVALID;

  construct_network_shared(bdata(options.network0), NETWORK0_ID, TRUE);

  if (compat_strcasecmp(bdata(options.network1), "NULL") != 0)
    construct_network_shared(bdata(options.network1), NETWORK1_ID, TRUE);

  for (int inetwork = 0; inetwork < NETWORKS_MAX; inetwork++)
    construct_material_cache(networks + inetwork);
}

local double gelu(double arg_input)
{
  return (0.5 * arg_input * (1.0 + erf(arg_input / sqrt(2.0))));
}

#define SCALE 1000
#define XMAX 5
#define N 8

void test_networks(void)
{
  ALIGN64(float inputs[N]);
  ALIGN64(float outputs[N]);
  ALIGN64(double errors[N]);

  for (int ivalue = 0; ivalue < N; ivalue++)
    inputs[ivalue] = 0.0f;

  double maximum_error = 0.0;

  for (int isample = -XMAX * SCALE; isample <= XMAX * SCALE; ++isample)
  {
    float input = (float)isample / (float)SCALE;

    inputs[0] = input;

    activation_geluv2_scaled(N, inputs, outputs);

    double expected = gelu((double)input);

    errors[0] = fabs((double)outputs[0] - expected);

    if (errors[0] > maximum_error)
    {
      maximum_error = errors[0];

      for (int ivalue = N - 1; ivalue > 0; --ivalue)
      {
        inputs[ivalue] = inputs[ivalue - 1];
        outputs[ivalue] = outputs[ivalue - 1];
        errors[ivalue] = errors[ivalue - 1];
      }
    }
  }

  for (int ivalue = 1; ivalue < N; ivalue++)
  {
    PRINTF("i=%d a[i]=%.6f gelu(a[i])=%.6f b[i]=%.6f e[i]=%.6f\n",
           ivalue,
           inputs[ivalue],
           gelu((double)inputs[ivalue]),
           outputs[ivalue],
           errors[ivalue]);
  }
  PRINTF("ex=%.6f\n", maximum_error);
}
