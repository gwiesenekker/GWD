// SCU REVISION 8.1256 za  7 nov 2026 11:28:24 CET
#include "globals.h"
#include <float.h>

_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
                 FLT_MIN_EXP == -125 && FLT_MAX_EXP == 128,
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

  for (int i = 0; i < 256; i++)
    base64_table[i] = INVALID;

  for (int i = 0; base64_chars[i]; i++)
    base64_table[(unsigned char)base64_chars[i]] = i;
}

local void init_base3_table(void)
{
  for (int wm = 0; wm < 256; wm++)
  {
    for (int bm = 0; bm < 256; bm++)
    {
      int index = 0;
      int power = 1;

      for (int k = 0; k < 8; k++)
      {
        int w = (wm >> k) & 1;
        int b = (bm >> k) & 1;

        int digit = w + 2 * b;

        index += digit * power;
        power *= 3;
      }

      base3_table[wm | (bm << 8)] = (uint16_t)index;
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

    for (int k = 0; k < BASE3_PATTERN_BITS; k++)
    {
      if ((mask >> k) & 1u)
      {
        index += power;
      }

      power *= 3u;
    }

    base3_mask_table[mask] = index;
  }
}

void decode_base64(size_t ns, unsigned char *s, size_t nt, unsigned char *t)
{
  ui32_t v = 0;
  int nb = -8;

  size_t j = 0;

  for (size_t i = 0; (i < ns) and (s[i] != '='); i++)
  {
    i8_t c = base64_table[(unsigned char)s[i]];

    HARDBUG(c == INVALID)

    v = (v << 6) | c;

    nb += 6;

    if (nb >= 0)
    {
      HARDBUG(j >= nt)

      t[j++] = (v >> nb) & 0xFF;

      nb -= 8;
    }
  }

  HARDBUG(j != nt)
}

local cJSON *open_cjson_file(cJSON *arg_directory, char *arg_name)
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

local void
read_1d_csv_base64(cJSON *arg_csv, int *arg_nrows, scaled_double_t **arg_cells)
{
  *arg_cells = NULL;

  cJSON *cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_ROWS_ID);

  HARDBUG(!cJSON_IsNumber(cjson_value))

  int nrows = round(cJSON_GetNumberValue(cjson_value));

  HARDBUG(nrows < 1)

  cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_DATA_ID);

  HARDBUG(!cJSON_IsString(cjson_value));

  char *data = cJSON_GetStringValue(cjson_value);

  float *cells_float;

  MY_MALLOC_BY_TYPE(cells_float, float, nrows)

  decode_base64(strlen(data),
                (unsigned char *)data,
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

local void
read_1d_csv_base64i32(cJSON *arg_csv, int *arg_nrows, i32_t **arg_cells)
{
  *arg_cells = NULL;

  cJSON *cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_ROWS_ID);

  HARDBUG(!cJSON_IsNumber(cjson_value))

  int nrows = round(cJSON_GetNumberValue(cjson_value));

  HARDBUG(nrows < 1)

  cjson_value = cJSON_GetObjectItem(arg_csv, CJSON_FILE_DATA_ID);

  HARDBUG(!cJSON_IsString(cjson_value));

  char *data = cJSON_GetStringValue(cjson_value);

  i32_t *cells;

  MY_MALLOC_BY_TYPE(cells, i32_t, nrows)

  decode_base64(strlen(data),
                (unsigned char *)data,
                nrows * sizeof(i32_t),
                (unsigned char *)cells);

  if (*arg_nrows != INVALID)
    HARDBUG(nrows != *arg_nrows)
  else
    *arg_nrows = nrows;

  *arg_cells = cells;
}

local void read_2d_csv_base64(cJSON *arg_csv,
                              int *arg_nrows,
                              int *arg_ncols,
                              scaled_double_t ***arg_cells_nrowsxncols,
                              scaled_double_t ***arg_cells_ncolsxnrows)
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

  HARDBUG(!cJSON_IsString(cjson_value));

  char *data = cJSON_GetStringValue(cjson_value);

  float *cells_float;

  MY_MALLOC_BY_TYPE(cells_float, float, nrows *ncols);

  decode_base64(strlen(data),
                (unsigned char *)data,
                nrows * ncols * sizeof(float),
                (unsigned char *)cells_float);

  if (arg_cells_nrowsxncols != NULL)
  {
    void *void_cells_nrowsxncols;

    MPI_Win win_cells_nrowsxncols;

    my_mpi_allocate_shared(my_mpi_globals.MMG_comm_global,
                           sizeof(scaled_double_t),
                           nrows * ncols,
                           &void_cells_nrowsxncols,
                           &win_cells_nrowsxncols);

    scaled_double_t *cells_nrowsxncols =
      (scaled_double_t *)void_cells_nrowsxncols;

    MY_MALLOC_BY_TYPE(*arg_cells_nrowsxncols, scaled_double_t *, nrows);

    for (int irow = 0; irow < nrows; irow++)
      (*arg_cells_nrowsxncols)[irow] = cells_nrowsxncols + irow * ncols;

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

    my_mpi_sync_shared(my_mpi_globals.MMG_comm_global,
                       win_cells_nrowsxncols,
                       "win_cells_nrowsxncols");

    MARK_ARRAY_READ_ONLY(cells_nrowsxncols, nrows * ncols);
  }

  if (arg_cells_ncolsxnrows != NULL)
  {
    scaled_double_t *cells_ncolsxnrows;

    void *void_cells_ncolsxnrows;

    MPI_Win win_cells_ncolsxnrows;

    my_mpi_allocate_shared(my_mpi_globals.MMG_comm_global,
                           sizeof(scaled_double_t),
                           ncols * nrows,
                           &void_cells_ncolsxnrows,
                           &win_cells_ncolsxnrows);

    cells_ncolsxnrows = (scaled_double_t *)void_cells_ncolsxnrows;

    MY_MALLOC_BY_TYPE(*arg_cells_ncolsxnrows, scaled_double_t *, ncols);

    for (int icol = 0; icol < ncols; icol++)
      (*arg_cells_ncolsxnrows)[icol] = cells_ncolsxnrows + icol * nrows;

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

    my_mpi_sync_shared(my_mpi_globals.MMG_comm_global,
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

local scaled_double_t dot_scaled(int n,
                                 scaled_double_t *restrict a,
                                 scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

  scaled_double_t s = 0;

#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      s += a[i] * b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    __m256 vs = _mm256_setzero_ps();

    for (const float *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);
      __m256 vb = MM256_LOAD_PS(b);

      vs = MM256_MADD_PS(va, vb, vs);
    }

    ALIGN64(float t[8]);

    _mm256_store_ps(t, vs);

    s = t[0] + t[1] + t[2] + t[3] + t[4] + t[5] + t[6] + t[7];
#else

    __m256i s64_lo = _mm256_setzero_si256();
    __m256i s64_hi = _mm256_setzero_si256();

    for (const i32_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256i ta = _mm256_load_si256((__m256i *)a);
      __m256i tb = _mm256_load_si256((__m256i *)b);

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

    s = s64 / SCALED_DOUBLE_FACTOR;
#endif
  }
#endif

  END_BLOCK

  return (s);
}

#if defined(USE_AVX2_INTRINSICS) && SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
local inline void dense_fixed_x8(const float *arg_inputs,
                                 scaled_double_t *const *arg_weights,
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
      for (int j = 0; j < 4; j++)
      {
        // Shared MPI allocations need not be 32-byte aligned.
        __m256 weights = _mm256_loadu_ps(arg_weights[ioutput + j] + 8 * ichunk);
        sums[j] = MM256_MADD_PS(inputs[ichunk], weights, sums[j]);
      }
    }

    for (int j = 0; j < 4; j++)
    {
      ALIGN64(float t[8]);
      _mm256_store_ps(t, sums[j]);
      arg_dot[ioutput + j] =
        t[0] + t[1] + t[2] + t[3] + t[4] + t[5] + t[6] + t[7];
    }
  }
}

local void dense_16x8(const float *arg_inputs,
                      scaled_double_t *const *arg_weights,
                      float *arg_dot)
{
  dense_fixed_x8(arg_inputs, arg_weights, arg_dot, 2);
}

local void dense_32x8(const float *arg_inputs,
                      scaled_double_t *const *arg_weights,
                      float *arg_dot)
{
  dense_fixed_x8(arg_inputs, arg_weights, arg_dot, 4);
}
#endif

local void dense_layer0_dot(dense_network_shared_t *arg_dense,
                            scaled_double_t *arg_inputs,
                            scaled_double_t *arg_dot)
{
  layer_shared_t *layer = arg_dense->DNS_layers;

#if defined(USE_AVX2_INTRINSICS) && SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
  if (arg_dense->DNS_16x8_kernel)
  {
    HARDBUG(layer->LS_ninputs != 16 || layer->LS_noutputs != 8)
    dense_16x8(arg_inputs, layer->LS_weights_noutputsxninputs, arg_dot);
    return;
  }
  if (arg_dense->DNS_32x8_kernel)
  {
    HARDBUG(layer->LS_ninputs != 32 || layer->LS_noutputs != 8)
    dense_32x8(arg_inputs, layer->LS_weights_noutputsxninputs, arg_dot);
    return;
  }
#endif

  for (int i = 0; i < layer->LS_noutputs; i++)
    arg_dot[i] = dot_scaled(layer->LS_ninputs,
                            arg_inputs,
                            layer->LS_weights_noutputsxninputs[i]);
}

local void construct_dense_network_shared(network_t *arg_network,
                                          cJSON *arg_directory,
                                          char *arg_prefix,
                                          int arg_verbose)
{
  dense_network_shared_t *with_dense_network_shared = NULL;

  if (compat_strcasecmp(arg_prefix, "pattern") == 0)
    with_dense_network_shared = &(arg_network->N_pattern_dense_network_shared);
  else if (compat_strcasecmp(arg_prefix, "material") == 0)
    with_dense_network_shared = &(arg_network->N_material_dense_network_shared);
  else if (compat_strcasecmp(arg_prefix, "final") == 0)
    with_dense_network_shared = &(arg_network->N_dense_network_shared);
  else
    FATAL("unknown arg_prefix", EXIT_FAILURE)

  HARDBUG(with_dense_network_shared == NULL)

  with_dense_network_shared->DNS_activation_inputs =
    arg_network->N_activation_inputs;
  with_dense_network_shared->DNS_activation = arg_network->N_activation;
  with_dense_network_shared->DNS_activation_last =
    arg_network->N_activation_last;

  if (arg_verbose)
    PRINTF("arg_prefix=%s\n", arg_prefix);

  for (int idense = 0; idense < 4; idense++)
  {
    BSTRING(bweights)

    BSTRING(bbias)

    if (arg_prefix == NULL)
    {
      bformata(bweights, "dense%d_weights.csv", idense);
      bformata(bbias, "dense%d_bias.csv", idense);
    }
    else
    {
      bformata(bweights, "%s_dense%d_weights.csv", arg_prefix, idense);
      bformata(bbias, "%s_dense%d_bias.csv", arg_prefix, idense);
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

local void construct_network_shared(char *arg_network_name,
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

  HARDBUG(!cJSON_IsString(value_cjson));

  char *svalue = cJSON_GetStringValue(value_cjson);

  with_network->N_bshape = bfromcstr(svalue);

  if (arg_verbose)
    PRINTF("shape=%s\n", bdata(with_network->N_bshape));

  construct_patterns_shared(&(with_network->N_patterns_shared),
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
    with_network->N_materials_shared.MSS_nmaterials = 0;
  else
    with_network->N_materials_shared.MSS_nmaterials =
      cJSON_GetArraySize(cjson_materials);

  HARDBUG(with_network->N_materials_shared.MSS_nmaterials > NMATERIAL_MAX)

  BSTRING(bmaterial_name)

  for (int imaterial = 0;
       imaterial < with_network->N_materials_shared.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *with_material_shared =
      with_network->N_materials_shared.MSS_materials + imaterial;

    cJSON *cjson_material = cJSON_GetArrayItem(cjson_materials, imaterial);

    HARDBUG(!cJSON_IsObject(cjson_material));

    cJSON *cjson_material_name =
      cJSON_GetObjectItem(cjson_material, CJSON_MATERIAL_NAME_ID);

    HARDBUG(!cJSON_IsString(cjson_material_name));

    const char *material_name = cJSON_GetStringValue(cjson_material_name);

    HARDBUG(material_name == NULL)

    HARDBUG((bmaterial_name = bfromcstr(material_name)) == NULL)

    if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_MAN_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_MAN_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_MAN_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_MAN_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_MAN_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_MAN_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_KING_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_KING_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_KING_ID;

      with_material_shared->MS_nstates = (MATERIAL_KING_MAX + 1);
      // with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_KING_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_KING_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_KING_ID;

      with_material_shared->MS_nstates = (MATERIAL_KING_MAX + 1);
      // with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NWHITE_KING20_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NWHITE_KING20_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NWHITE_KING20_ID;

      with_material_shared->MS_nstates = (NPIECES_MAX + 1);
    }
    else if (biseqcstr(bmaterial_name, MATERIAL_NBLACK_KING20_CNAME) == 1)
    {
      HARDBUG((with_material_shared->MS_bname =
                 bfromcstr(MATERIAL_NBLACK_KING20_CNAME)) == NULL)

      with_material_shared->MS_id = MATERIAL_NBLACK_KING20_ID;

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
    PRINTF("with_network->N_materials_shared.MSS_nmaterials=%d\n",
           with_network->N_materials_shared.MSS_nmaterials);

    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

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

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "add") == 0)
    with_network->N_architecture_patterns = NETWORK_ARCHITECTURE_ADD;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    with_network->N_architecture_patterns = NETWORK_ARCHITECTURE_CONCAT;
  else
    FATAL("unknown architecture_patterns option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD)
      PRINTF("architecture_patterns=ADD\n");
    else if (with_network->N_architecture_patterns ==
             NETWORK_ARCHITECTURE_CONCAT)
      PRINTF("architecture_patterns=CONCAT\n");
    else
      FATAL("unknown architecture_patterns option", EXIT_FAILURE)
  }

  // architecture material

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ARCHITECTURE_MATERIAL_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "NULL") == 0)
    with_network->N_architecture_material = NETWORK_ARCHITECTURE_NULL;
  else if (compat_strcasecmp(svalue, "add") == 0)
    with_network->N_architecture_material = NETWORK_ARCHITECTURE_ADD;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    with_network->N_architecture_material = NETWORK_ARCHITECTURE_CONCAT;
  else
    FATAL("unknown architecture_material option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_NULL)
      PRINTF("architecture_material=NULL\n");
    else if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD)
      PRINTF("architecture_material=ADD\n");
    else if (with_network->N_architecture_material ==
             NETWORK_ARCHITECTURE_CONCAT)
      PRINTF("architecture_material=CONCAT\n");
    else
      FATAL("unknown architecture_material option", EXIT_FAILURE)
  }

  // activation_inputs

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_INPUTS_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_RELU;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_RSQRT;
  else if (compat_strcasecmp(svalue, "gelu") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_GELU;
  else if (compat_strcasecmp(svalue, "geluv2") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_GELUV2;
  else if (compat_strcasecmp(svalue, "linear") == 0)
    with_network->N_activation_inputs = NETWORK_ACTIVATION_LINEAR;
  else
    FATAL("unknown activation_inputs option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (with_network->N_activation_inputs == NETWORK_ACTIVATION_RELU)
      PRINTF("activation_inputs=RELU\n");
    else if (with_network->N_activation_inputs == NETWORK_ACTIVATION_TANH)
      PRINTF("activation_inputs=TANH\n");
    else if (with_network->N_activation_inputs == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation_inputs=RSQRT\n");
    else if (with_network->N_activation_inputs == NETWORK_ACTIVATION_GELU)
      PRINTF("activation_inputs=GELU\n");
    else if (with_network->N_activation_inputs == NETWORK_ACTIVATION_GELUV2)
      PRINTF("activation_inputs=GELUV2\n");
    else if (with_network->N_activation_inputs == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_inputs=LINEAR\n");
    else
      FATAL("unknown activation_inputs option", EXIT_FAILURE)
  }

  // activation

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

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

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "linear") == 0)
    with_network->N_activation_last = NETWORK_ACTIVATION_LINEAR;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    with_network->N_activation_last = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "logits") == 0)
    with_network->N_activation_last = NETWORK_ACTIVATION_LOGITS;
  else if (compat_strcasecmp(svalue, "2heads") == 0)
    with_network->N_activation_last = NETWORK_ACTIVATION_2HEADS;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (arg_verbose)
  {
    PRINTF("with_network->N_activation_last=%s\n",
           activation_last2string(with_network->N_activation_last, TRUE));
  }

  // nman_min

  with_network->N_nman_min = 0;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MIN_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    with_network->N_nman_min = ivalue;

    if (arg_verbose)
    {
      PRINTF("N_nman_min=%d\n", with_network->N_nman_min);
    }
  }

  // nman_max

  with_network->N_nman_max = 40;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MAX_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    with_network->N_nman_max = ivalue;

    if (arg_verbose)
    {
      PRINTF("N_nman_max=%d\n", with_network->N_nman_max);
    }
  }

  HARDBUG(with_network->N_nman_min > with_network->N_nman_max)

  value_cjson = cJSON_GetObjectItem(directory, CJSON_PRAGMA_ID);

  if (cJSON_IsString(value_cjson))
  {
    svalue = cJSON_GetStringValue(value_cjson);

    PRINTF("pragma=%s\n", svalue);

    if (compat_strcasecmp(svalue, "dummy") == 0)
      return;
  }

  with_network->N_pattern_dense_network_shared.DNS_ninputs = 0;
  with_network->N_pattern_dense_network_shared.DNS_nlayers = 0;
  with_network->N_pattern_dense_network_shared.DNS_activate_layer0 = FALSE;
  with_network->N_pattern_dense_network_shared.DNS_compute_layer0 = TRUE;

  with_network->N_material_dense_network_shared.DNS_ninputs = 0;
  with_network->N_material_dense_network_shared.DNS_nlayers = 0;
  with_network->N_material_dense_network_shared.DNS_activate_layer0 = FALSE;
  with_network->N_material_dense_network_shared.DNS_compute_layer0 = TRUE;

  with_network->N_dense_network_shared.DNS_ninputs = 0;
  with_network->N_dense_network_shared.DNS_nlayers = 0;
  with_network->N_dense_network_shared.DNS_activate_layer0 = FALSE;
  with_network->N_dense_network_shared.DNS_compute_layer0 = TRUE;

  // load the embedding vectors for the patterns

  for (int ipattern = 0;
       ipattern < with_network->N_patterns_shared.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_patterns_shared.PSS_patterns + ipattern;

    BSTRING(bindex)

    bformata(bindex, "pattern_embedding%d_index.csv", ipattern);

    cJSON *file = open_cjson_file(directory, bdata(bindex));

    BDESTROY(bindex)

    MY_MALLOC_BY_TYPE(with_pattern_shared->PS_state_map,
                      int,
                      with_pattern_shared->PS_nstates);

    int nstate_map = INVALID;

    if (file == NULL)
    {
      // identity

      nstate_map = with_pattern_shared->PS_nstates;

      for (int istate = 0; istate < with_pattern_shared->PS_nstates; istate++)
        with_pattern_shared->PS_state_map[istate] = istate;
    }
    else
    {
      i32_t *state_map;

      read_1d_csv_base64i32(file, &nstate_map, &state_map);

      for (int istate = 0; istate < with_pattern_shared->PS_nstates; istate++)
        with_pattern_shared->PS_state_map[istate] = INVALID;

      for (int imap = 0; imap < nstate_map; imap++)
        with_pattern_shared->PS_state_map[state_map[imap]] = imap;
    }

    BSTRING(bweights)

    bformata(bweights, "pattern_embedding%d_weights.csv", ipattern);

    file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    read_2d_csv_base64(file,
                       &(nstate_map),
                       &(with_pattern_shared->PS_nembed),
                       &(with_pattern_shared->PS_weights_nstatesxnembed),
                       NULL);

    if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD)
    {
      with_pattern_shared->PS_offset = 0;

      if (with_network->N_pattern_dense_network_shared.DNS_ninputs == 0)
      {
        with_network->N_pattern_dense_network_shared.DNS_ninputs =
          with_pattern_shared->PS_nembed; // first
      }
      else
        HARDBUG(with_pattern_shared->PS_nembed !=
                with_network->N_pattern_dense_network_shared.DNS_ninputs)
    }
    else
    {
      with_pattern_shared->PS_offset =
        with_network->N_pattern_dense_network_shared.DNS_ninputs;

      with_network->N_pattern_dense_network_shared.DNS_ninputs +=
        with_pattern_shared->PS_nembed;
    }

    PRINTF("ipattern=%d with_pattern_shared->PS_nstates=%d "
           "with_pattern_shared->PS_nembed=%d"
           " with_pattern_shared->PS_offset=%d\n",
           ipattern,
           with_pattern_shared->PS_nstates,
           with_pattern_shared->PS_nembed,
           with_pattern_shared->PS_offset);

    HARDBUG(with_network->N_pattern_dense_network_shared.DNS_ninputs >=
            NINPUTS_MAX)
  }

  // load the material embedding vectors

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
  {
    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

      BSTRING(bweights)

      bformata(bweights, "material_embedding%d_weights.csv", imaterial);

      cJSON *file = open_cjson_file(directory, bdata(bweights));

      BDESTROY(bweights)

      HARDBUG(file == NULL)

      with_material_shared->MS_nembed = INVALID;

      read_2d_csv_base64(file,
                         &(with_material_shared->MS_nstates),
                         &(with_material_shared->MS_nembed),
                         &(with_material_shared->MS_weights_nstatesxnembed),
                         NULL);

      if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD)
      {
        with_material_shared->MS_offset = 0;

        if (with_network->N_material_dense_network_shared.DNS_ninputs == 0)
        {
          with_network->N_material_dense_network_shared.DNS_ninputs =
            with_material_shared->MS_nembed; // first
        }
        else
          HARDBUG(with_material_shared->MS_nembed !=
                  with_network->N_material_dense_network_shared.DNS_ninputs)
      }
      else
      {
        with_material_shared->MS_offset =
          with_network->N_material_dense_network_shared.DNS_ninputs;

        with_network->N_material_dense_network_shared.DNS_ninputs +=
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

      HARDBUG(with_network->N_material_dense_network_shared.DNS_ninputs >=
              NINPUTS_MAX)
    }
  }

  // read the dense networks

  construct_dense_network_shared(with_network, directory, "pattern", TRUE);

  // topology check

  HARDBUG(with_network->N_pattern_dense_network_shared.DNS_ninputs !=
          with_network->N_pattern_dense_network_shared.DNS_layers[0].LS_ninputs)

  // The specialized path sums 16-element embeddings for a 16x8 first layer.
  with_network->N_pattern_dense_network_16x8_kernel =
    (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD) and
    (with_network->N_pattern_dense_network_shared.DNS_layers[0].LS_ninputs ==
     16) and
    (with_network->N_pattern_dense_network_shared.DNS_layers[0].LS_noutputs ==
     8);

  with_network->N_pattern_dense_network_64x16_kernel =
    (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD) and
    (with_network->N_pattern_dense_network_shared.DNS_layers[0].LS_ninputs ==
     64) and
    (with_network->N_pattern_dense_network_shared.DNS_layers[0].LS_noutputs ==
     16);

  for (int ipattern = 0;
       ipattern < with_network->N_patterns_shared.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_patterns_shared.PSS_patterns + ipattern;

    if ((with_pattern_shared->PS_nembed != 16) or
        (with_pattern_shared->PS_offset != 0))
      with_network->N_pattern_dense_network_16x8_kernel = FALSE;
    if ((with_pattern_shared->PS_nembed != 64) or
        (with_pattern_shared->PS_offset != 0))
      with_network->N_pattern_dense_network_64x16_kernel = FALSE;
  }

  with_network->N_material_dense_network_16x8_kernel = FALSE;
  with_network->N_material_dense_network_64x16_kernel = FALSE;

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
  {
    construct_dense_network_shared(with_network, directory, "material", TRUE);

    // topology check

    HARDBUG(
      with_network->N_material_dense_network_shared.DNS_ninputs !=
      with_network->N_material_dense_network_shared.DNS_layers[0].LS_ninputs)

    with_network->N_material_dense_network_16x8_kernel =
      (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD) and
      (with_network->N_material_dense_network_shared.DNS_layers[0].LS_ninputs ==
       16) and
      (with_network->N_material_dense_network_shared.DNS_layers[0]
         .LS_noutputs == 8);

    with_network->N_material_dense_network_64x16_kernel =
      (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD) and
      (with_network->N_material_dense_network_shared.DNS_layers[0].LS_ninputs ==
       64) and
      (with_network->N_material_dense_network_shared.DNS_layers[0]
         .LS_noutputs == 16);

    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

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
      &(with_network->N_pattern_dense_network_shared);

    with_network->N_dense_network_shared.DNS_ninputs =
      with_pattern_dense_network_shared
        ->DNS_layers[with_pattern_dense_network_shared->DNS_nlayers - 1]
        .LS_noutputs;

    dense_network_shared_t *with_material_dense_network_shared =
      &(with_network->N_material_dense_network_shared);

    with_network->N_dense_network_shared.DNS_ninputs +=
      with_material_dense_network_shared
        ->DNS_layers[with_material_dense_network_shared->DNS_nlayers - 1]
        .LS_noutputs;
  }

  PRINTF("with_network->N_dense_network_shared.DNS_ninputs=%d\n",
         with_network->N_dense_network_shared.DNS_ninputs);

  HARDBUG(with_network->N_dense_network_shared.DNS_ninputs >= NINPUTS_MAX)

  construct_dense_network_shared(with_network, directory, "final", TRUE);

  if (with_network->N_dense_network_shared.DNS_ninputs !=
      with_network->N_dense_network_shared.DNS_layers[0].LS_ninputs)
  {
    PRINTF("with_network->N_dense_network_shared.DNS_ninputs=%d\n",
           with_network->N_dense_network_shared.DNS_ninputs);
    PRINTF("with_network->N_dense_network_shared.DNS_layers[0].LS_ninputs=%d\n",
           with_network->N_dense_network_shared.DNS_layers[0].LS_ninputs);

    FATAL("topology error", EXIT_FAILURE)
  }

  if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD)
  {
    for (int ipattern = 0;
         ipattern < with_network->N_patterns_shared.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_patterns_shared.PSS_patterns + ipattern;

      with_pattern_shared->PS_sum_nstatesxnoutputs = NULL;
    }
    with_network->N_pattern_dense_network_shared.DNS_activate_layer0 = TRUE;
    with_network->N_pattern_dense_network_shared.DNS_compute_layer0 = TRUE;
  }
  else if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_CONCAT)
  {
    // precompute layer0 contribution

    with_network->N_pattern_dense_network_shared.DNS_activate_layer0 = FALSE;
    with_network->N_pattern_dense_network_shared.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      with_network->N_pattern_dense_network_shared.DNS_layers;

    for (int ipattern = 0;
         ipattern < with_network->N_patterns_shared.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_patterns_shared.PSS_patterns + ipattern;

      void *void_sum_nstatesxnoutputs;

      MPI_Win win_sum_nstatesxnoutputs;

      my_mpi_allocate_shared(my_mpi_globals.MMG_comm_global,
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
                         with_pattern_shared->PS_weights_nstatesxnembed[istate],
                         with_current->LS_weights_noutputsxninputs[ioutput] +
                           with_pattern_shared->PS_offset);
          }
        }
      }

      my_mpi_sync_shared(my_mpi_globals.MMG_comm_global,
                         win_sum_nstatesxnoutputs,
                         "sum_nstatesxnoutputs");

      MARK_ARRAY_READ_ONLY(sum_nstatesxnoutputs,
                           with_pattern_shared->PS_nstates *
                             with_current->LS_noutputs);
    }
  }
  else
    FATAL("unknown with_network->N_architecur_pattern", EXIT_FAILURE)

  if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD)
  {
    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

      with_material_shared->MS_sum_nstatesxnoutputs = NULL;
    }

    with_network->N_material_dense_network_shared.DNS_activate_layer0 = TRUE;
    with_network->N_material_dense_network_shared.DNS_compute_layer0 = TRUE;
  }
  else if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_CONCAT)
  {
    // precompute layer0 contribution

    with_network->N_material_dense_network_shared.DNS_activate_layer0 = FALSE;
    with_network->N_material_dense_network_shared.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      with_network->N_material_dense_network_shared.DNS_layers;

    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

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
                       with_material_shared->MS_weights_nstatesxnembed[istate],
                       with_current->LS_weights_noutputsxninputs[ioutput] +
                         with_material_shared->MS_offset);
        }
      }

      mark_pointer_read_only(sum);
    }
  }
}

local void
construct_dense_network_state(dense_network_shared_t *with_dense_network_shared,
                              dense_network_state_t *with_dense_network_state)
{
  for (int ilayer = 0; ilayer < with_dense_network_shared->DNS_nlayers;
       ilayer++)
  {
    layer_shared_t *with_layer_shared =
      with_dense_network_shared->DNS_layers + ilayer;

    layer_thread_t *with_thread = with_dense_network_state->DNS_layers + ilayer;

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
    &(with_network->N_pattern_dense_network_shared),
    &(arg_with_network_state->NS_pattern_dense_network_state));

  construct_dense_network_state(
    &(with_network->N_material_dense_network_shared),
    &(arg_with_network_state->NS_material_dense_network_state));

  construct_dense_network_state(
    &(with_network->N_dense_network_shared),
    &(arg_with_network_state->NS_dense_network_state));
}

local scaled_double_t dot_double(int n,
                                 scaled_double_t *restrict a,
                                 scaled_double_t *restrict b)
{
  double s = 0;

  for (int i = 0; i < n; i++)
    s += SCALED2DOUBLE(a[i]) * SCALED2DOUBLE(b[i]);

  return (DOUBLE2SCALED(s));
}

void vcopy_a2b(int n, scaled_double_t *restrict a, scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      b[i] = a[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);

      _mm256_store_ps(b, va);
    }
#else
    for (const i32_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);

      _mm256_store_si256((__m256i *)b, va);
    }
#endif
  }
#endif
}

local void vadd_ab2c(int n,
                     scaled_double_t *restrict a,
                     scaled_double_t *restrict b,
                     scaled_double_t *restrict c)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      c[i] = a[i] + b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8, c += 8)
    {
      __m256 va = MM256_LOAD_PS(a);
      __m256 vb = MM256_LOAD_PS(b);

      __m256 vc = _mm256_add_ps(va, vb);

      _mm256_store_ps(c, vc);
    }
#else
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8, c += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);
      __m256i vb = _mm256_load_si256((__m256i *)b);

      __m256i vc = _mm256_add_epi32(va, vb);

      _mm256_store_si256((__m256i *)c, vc);
    }
#endif
  }
#endif

  END_BLOCK
}

void vmul_ab2c(int n,
               scaled_double_t *restrict a,
               scaled_double_t *restrict b,
               scaled_double_t *restrict c)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      c[i] = a[i] * b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8, c += 8)
    {
      __m256 va = MM256_LOAD_PS(a);
      __m256 vb = MM256_LOAD_PS(b);

      __m256 vc = _mm256_mul_ps(va, vb);

      _mm256_store_ps(c, vc);
    }
#else
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8, c += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);
      __m256i vb = _mm256_load_si256((__m256i *)b);

      __m256i vc = _mm256_mullo_epi32(va, vb);

      _mm256_store_si256((__m256i *)c, vc);
    }
#endif
  }
#endif

  END_BLOCK
}

void vadd_ab2a(int n, scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      a[i] += b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      const __m256 va = MM256_LOAD_PS(a);
      const __m256 vb = MM256_LOAD_PS(b);

      const __m256 vs = _mm256_add_ps(va, vb);

      _mm256_store_ps(a, vs);
    }
#else
    for (const i32_t *z = a + n; a < z; a += 8, b += 8)
    {
      const __m256i va = _mm256_load_si256((__m256i *)a);
      const __m256i vb = _mm256_load_si256((__m256i *)b);

      const __m256i vs = _mm256_add_epi32(va, vb);

      _mm256_store_si256((__m256i *)a, vs);
    }
#endif
  }
#endif

  END_BLOCK
}

void vmul_ab2a(int n, scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      a[i] *= b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      const __m256 va = MM256_LOAD_PS(a);
      const __m256 vb = MM256_LOAD_PS(b);

      const __m256 vs = _mm256_mul_ps(va, vb);

      _mm256_store_ps(a, vs);
    }
#else
    for (const i32_t *z = a + n; a < z; a += 8, b += 8)
    {
      const __m256i va = _mm256_load_si256((__m256i *)a);
      const __m256i vb = _mm256_load_si256((__m256i *)b);

      const __m256i vs = _mm256_mullo_epi32(va, vb);

      _mm256_store_si256((__m256i *)a, vs);
    }
#endif
  }
#endif

  END_BLOCK
}

local void activation_relu_scaled(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
    {
      if (a[i] <= 0)
        b[i] = 0;
      else
        b[i] = a[i];
    }
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 vz = _mm256_setzero_ps();

    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);

      va = _mm256_max_ps(va, vz);

      _mm256_store_ps(b, va);
    }
#else
    __m256i zero = _mm256_setzero_si256();

    for (const int32_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);

      va = _mm256_max_epi32(va, zero);

      _mm256_store_si256((__m256i *)b, va);
    }
#endif
  }
#endif
}

local void activation_relu_double(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else
      b[i] = a[i];
  }
}

local void activation_tanh_scaled(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      b[i] = tanhf(a[i]);
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

      // needed for correction

      rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      __m256 vq = _mm256_add_ps(one, _mm256_mul_ps(q, rsqrt2));

      _mm256_store_ps(b, _mm256_mul_ps(va, _mm256_mul_ps(rsqrt, vq)));
    }
#else
    for (int i = 0; i < n; i++)
      b[i] = FLOAT2SCALED(tanhf(SCALED2FLOAT(a[i])));
#endif
  }
#endif
}

local void activation_tanh_double(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
    b[i] = DOUBLE2SCALED(tanh(SCALED2DOUBLE(a[i])));
}

local void activation_rsqrt_scaled(int n,
                                   scaled_double_t *restrict a,
                                   scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    for (int i = 0; i < n; i++)
      b[i] = a[i] / sqrtf(1.0f + a[i] * a[i]);
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 three_halfs = _mm256_set1_ps(1.5f);

    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);

      __m256 va2p1 = _mm256_add_ps(one, _mm256_mul_ps(va, va));

      __m256 rsqrt = _mm256_rsqrt_ps(va2p1);

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      rsqrt = _mm256_mul_ps(
        rsqrt,
        _mm256_sub_ps(three_halfs,
                      _mm256_mul_ps(half, _mm256_mul_ps(va2p1, rsqrt2))));

      _mm256_store_ps(b, _mm256_mul_ps(va, rsqrt));
    }
#else
    for (int i = 0; i < n; i++)
    {
      float af = SCALED2FLOAT(a[i]);
      b[i] = af / sqrtf(1.0f + af * af);
    }
#endif
  }
#endif
}

local void activation_rsqrt_double(int n,
                                   scaled_double_t *restrict a,
                                   scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
    b[i] = DOUBLE2SCALED(a[i]) /
           sqrt(1.0 + SCALED2DOUBLE(a[i]) * SCALED2DOUBLE(a[i]));
}

local void activation_gelu_scaled(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
#ifdef USE_ERF
  const float c = 0.707107f;
#else
  const float k = 0.797884f;
  const float c = 0.044715f;
#endif

  for (int i = 0; i < n; i++)
  {
    float af = a[i];

#ifdef USE_ERF
    b[i] = 0.5f * af * (1.0f + erff(c * af));
#else
    float af2 = af * af;

    b[i] = 0.5f * af * (1.0f + tanhf(k * (af + c * af * af2)));
#endif
  }
#else
#error not implemented
#endif
}

local void activation_gelu_double(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
#ifdef USE_ERF
  const float c = 0.707107f;
#else
  const double k = 0.797884;
  const double c = 0.044715;
#endif

  for (int i = 0; i < n; i++)
  {
    double ad = a[i];
#ifdef USE_ERF
    b[i] = 0.5f * ad * (1.0f + erf(c * ad));
#else
    double ad2 = ad * ad;

    b[i] = DOUBLE2SCALED(0.5 * ad * (1.0 + tanhf(k * (ad + c * ad * ad2))));
#endif
  }
}

local void activation_geluv2_scaled(int n,
                                    scaled_double_t *restrict a,
                                    scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if ((n < 8) or ((n % 8) != 0))
#endif
  {
    const float p = 0.544790f;

    for (int i = 0; i < n; i++)
    {
      float af = a[i];
      float af2 = af * af;

      b[i] = 0.5f * af * (1.0f + af / sqrtf(p + af2));
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

    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);

      __m256 vy = _mm256_mul_ps(half, va);

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

      _mm256_store_ps(b, vg);
    }
#else
    const __m256 p = _mm256_set1_ps(0.544790f);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 one = _mm256_set1_ps(1.0f);

    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 va = MM256_LOAD_PS(a);

      __m256 va2 = _mm256_mul_ps(va, va);

      __m256 vsqrt = _mm256_sqrt_ps(_mm256_add_ps(p, va2));

      __m256 rsqrt = _mm256_add_ps(one, _mm256_div_ps(va, vsqrt));

      __m256 vg = _mm256_mul_ps(_mm256_mul_ps(half, va), rsqrt);

      _mm256_store_ps(b, vg);
    }
#endif
#else
#error not implemented
#endif
  }
#endif
}

local void activation_geluv2_double(int n,
                                    scaled_double_t *restrict a,
                                    scaled_double_t *restrict b)
{
  const double p = 0.544790;

  for (int i = 0; i < n; i++)
  {
    double ad = a[i];
    double ad2 = ad * ad;

    b[i] = DOUBLE2SCALED(0.5 * ad * (1.0 + ad / sqrt(p + ad2)));
  }
}

local void activation_scaled(int arg_activation,
                             int n,
                             scaled_double_t *restrict a,
                             scaled_double_t *restrict b)
{
  if (arg_activation == NETWORK_ACTIVATION_RELU)
  {
    activation_relu_scaled(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_scaled(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_scaled(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELU)
  {
    activation_gelu_scaled(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELUV2)
  {
    activation_geluv2_scaled(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_LINEAR)
  {
    vcopy_a2b(n, a, b);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local void activation_double(int arg_activation,
                             int n,
                             scaled_double_t *restrict a,
                             scaled_double_t *restrict b)
{
  if (arg_activation == NETWORK_ACTIVATION_RELU)
  {
    activation_relu_double(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_double(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_double(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELU)
  {
    activation_gelu_double(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_GELUV2)
  {
    activation_geluv2_double(n, a, b);
  }
  else if (arg_activation == NETWORK_ACTIVATION_LINEAR)
  {
    vcopy_a2b(n, a, b);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local double return_sigmoid(double x) { return (2.0 / (1.0 + exp(-x)) - 1.0); }

int base3_index_v2(board_t *arg_board, ui64_t arg_pattern_bb)
{
  BEGIN_BLOCK(__func__)

  int result = 0;
  int power = 1;

  ui64_t white_man_bb = arg_board->B_white_man_bb;
  ui64_t black_man_bb = arg_board->B_black_man_bb;

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

int base3_index_v3(board_t *arg_board, ui64_t arg_pattern_bb)
{
  uint64_t wm = _pext_u64(arg_board->B_white_man_bb, arg_pattern_bb);
  uint64_t bm = _pext_u64(arg_board->B_black_man_bb, arg_pattern_bb);

  return base3_table[wm | (bm << 8)];
}

int base3_index_v4(board_t *arg_board, ui64_t arg_pattern_bb)
{
  // HARDBUG(BIT_COUNT(arg_pattern_bb) > BASE3_PATTERN_BITS)

  uint32_t wm = (uint32_t)_pext_u64(arg_board->B_white_man_bb, arg_pattern_bb);
  uint32_t bm = (uint32_t)_pext_u64(arg_board->B_black_man_bb, arg_pattern_bb);

  HARDBUG((wm & bm) != 0)

  return (int)(base3_mask_table[wm] + 2u * base3_mask_table[bm]);
}

// assumes inputs have been set

double return_dense_network_score_scaled(
  dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state)
{
  BEGIN_BLOCK(__func__)

  layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_state->DNS_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    ALIGN64(scaled_double_t activated_inputs[NINPUTS_MAX]);

    activation_scaled(arg_dense_network_shared->DNS_activation_inputs,
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

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] =
        dot_scaled(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs[i]);
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

      if (arg_dense_network_shared->DNS_activation_last ==
          NETWORK_ACTIVATION_LINEAR)
      {
        HARDBUG(with_current->LS_noutputs != 1)

        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
               NETWORK_ACTIVATION_TANH)
      {
        HARDBUG(with_current->LS_noutputs != 1)

        with_current_thread->LT_outputs[0] =
          tanhf(with_current_thread->LT_sum[0]);
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
               NETWORK_ACTIVATION_LOGITS)
      {
        HARDBUG(with_current->LS_noutputs != 3)

        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
        with_current_thread->LT_outputs[1] = with_current_thread->LT_sum[1];
        with_current_thread->LT_outputs[2] = with_current_thread->LT_sum[2];
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
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

    return (logits2 - logits0) * (pwin + ploss);
  }
  else
  {
    return (0.0);
  }
}

double return_dense_network_score_double(
  dense_network_shared_t *arg_dense_network_shared,
  dense_network_state_t *arg_dense_network_state)
{
  layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_state->DNS_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    scaled_double_t activated_inputs[NINPUTS_MAX];

    activation_double(arg_dense_network_shared->DNS_activation_inputs,
                      with_current->LS_ninputs,
                      arg_dense_network_state->DNS_inputs,
                      activated_inputs);

    // preserve inputs for return_dense_network_score_double

    // always compute layer0

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] =
        dot_double(with_current->LS_ninputs,
                   activated_inputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }
  }
  else
  {
    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] =
        dot_double(with_current->LS_ninputs,
                   arg_dense_network_state->DNS_inputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }
  }

  for (int i = 0; i < with_current->LS_noutputs; i++)
  {
    with_current_thread->LT_sum[i] =
      with_current_thread->LT_dot[i] + with_current->LS_bias[i];
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

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_sum[i] =
        dot_double(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs[i]) +
        with_current->LS_bias[i];
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
      if (arg_dense_network_shared->DNS_activation_last ==
          NETWORK_ACTIVATION_LINEAR)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
               NETWORK_ACTIVATION_TANH)
      {
        with_current_thread->LT_outputs[0] =
          tanh(with_current_thread->LT_sum[0]);
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
               NETWORK_ACTIVATION_LOGITS)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (arg_dense_network_shared->DNS_activation_last ==
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

local void heap_sort_double(i64_t n, i64_t *p, double *a)
{
  i64_t i, j, k, t;

  if (n < 2)
    return;

  for (i = 0; i < n; i++)
    p[i] = i;

  k = n / 2;
  --n;

  while (TRUE)
  {
    if (k > 0)
    {
      t = p[--k];
    }
    else
    {
      t = p[n];
      p[n] = p[0];
      if (--n == 0)
      {
        p[0] = t;
        break;
      }
    }
    i = k;
    j = 2 * k + 1;
    while (j <= n)
    {
      if ((j < n) and (a[p[j]] < a[p[j + 1]]))
        j++;
      if (a[t] < a[p[j]])
      {
        p[i] = p[j];
        i = j;
        j = 2 * j + 1;
      }
      else
      {
        j = n + 1;
      }
    }
    p[i] = t;
  }

  for (i = 0; i < n; i++)
    HARDBUG(a[p[i]] > a[p[i + 1]])
}

local void
update_mean_sigma(long long n, double x, double *xmax, double *mn, double *sn)
{
  double mnm1 = *mn;
  double snm1 = *sn;

  if (x > *xmax)
    *xmax = x;

  *mn = mnm1 + (x - mnm1) / n;

  *sn = snm1 + (x - mnm1) * (x - *mn);
}

static inline uint64_t mirror_bb(uint64_t bb)
{
  ui64_t result;

  result = (bb >> 6);

  result = BIT_REVERSE(result) >> (64 - 54);

  result = (result << 6);

  ui64_t debug = 0;

  while (bb)
  {
    int ifield = BIT_CTZ(bb);

    bb &= ~BITULL(ifield);

    debug |= 1ULL << (65 - ifield);
  }

  HARDBUG(debug != result)

  return (result);
}

void fen2network(char *arg_name, i64_t arg_npositions)
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

    int nwhite_man = BIT_COUNT(with_board->B_white_man_bb);
    int nblack_man = BIT_COUNT(with_board->B_black_man_bb);
    int nwhite_king = BIT_COUNT(with_board->B_white_king_bb);
    int nblack_king = BIT_COUNT(with_board->B_black_king_bb);

    int npieces = nwhite_man + nblack_man + nwhite_king + nblack_king;

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
      &(with_network->N_dense_network_shared),
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
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   score_scaled,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   score_double,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   delta_scaled,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   delta_double,
                   nfen,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_vs_draw_scaled,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_vs_draw_double,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_scaled,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &mse_double,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &error_max_scaled_vs_double,
                   1,
                   MPI_DOUBLE,
                   MPI_MAX,
                   my_mpi_globals.MMG_comm_slaves);

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

    int nwhite_king = BIT_COUNT(with_board->B_white_king_bb);
    int nblack_king = BIT_COUNT(with_board->B_black_king_bb);

    if ((nwhite_king > 0) or (nblack_king > 0))
      continue;

    int nwhite_man = BIT_COUNT(with_board->B_white_man_bb);
    int nblack_man = BIT_COUNT(with_board->B_black_man_bb);

    if ((nwhite_man == 0) or (nblack_man == 0))
      continue;

    int delta_man = nwhite_man - nblack_man;
    if (delta_man < 0)
      continue;

    // if (fabs(score_scaled[ifen] - result) > percentile)
    //   continue;

    nfactor[delta_man]++;
    factor[delta_man] += fabs(score_scaled[ifen]);
    score[delta_man] += ABS_SCORE_T(return_score_from_board(with_board, NULL));
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
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &factor,
                   NPIECES_MAX + 1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &score,
                   NPIECES_MAX + 1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

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

local void append_fen(board_t *arg_with_board,
                      bstring arg_bfen,
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

  int nwhite_man = BIT_COUNT(arg_with_board->B_white_man_bb);
  int nblack_man = BIT_COUNT(arg_with_board->B_black_man_bb);
  int nwhite_king = BIT_COUNT(arg_with_board->B_white_king_bb);
  int nblack_king = BIT_COUNT(arg_with_board->B_black_king_bb);

  network_t *with_network = networks + arg_with_board->B_network_id;

  for (int ipattern = 0;
       ipattern < with_network->N_patterns_shared.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_patterns_shared.PSS_patterns + ipattern;

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
       imaterial < with_network->N_materials_shared.MSS_nmaterials;
       imaterial++)
  {
    material_shared_t *with_material_shared =
      with_network->N_materials_shared.MSS_materials + imaterial;

    if (with_material_shared->MS_id == MATERIAL_NWHITE_MAN_ID)
    {
      bin_t ibin = nwhite_man;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_MAN_ID)
    {
      bin_t ibin = nblack_man;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NWHITE_KING_ID)
    {
      int mwhite_king = nwhite_king;

      if (mwhite_king > MATERIAL_KING_MAX)
        mwhite_king = MATERIAL_KING_MAX;

      bin_t ibin = mwhite_king;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_KING_ID)
    {
      int mblack_king = nblack_king;

      if (mblack_king > MATERIAL_KING_MAX)
        mblack_king = MATERIAL_KING_MAX;

      bin_t ibin = mblack_king;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NWHITE_KING20_ID)
    {
      bin_t ibin = nwhite_king;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_NBLACK_KING20_ID)
    {
      bin_t ibin = nblack_king;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else if (with_material_shared->MS_id == MATERIAL_MAN_KING_ID)
    {
      int mwhite_king = nwhite_king;

      if (mwhite_king > MATERIAL_KING_MAX)
        mwhite_king = MATERIAL_KING_MAX;

      int mblack_king = nblack_king;

      if (mblack_king > MATERIAL_KING_MAX)
        mblack_king = MATERIAL_KING_MAX;

      bin_t ibin = (nwhite_man * (MATERIAL_KING_MAX + 1) + mwhite_king) *
                     (NPIECES_MAX + 1) * (MATERIAL_KING_MAX + 1) +
                   nblack_man * (MATERIAL_KING_MAX + 1) + mblack_king;

      append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
    }
    else
      FATAL("unknown with_material_shared->MS_id", EXIT_FAILURE)
  }

  if (with_network->N_activation_last == NETWORK_ACTIVATION_LINEAR)
  {
    float result = arg_result;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));
  }
  else if (with_network->N_activation_last == NETWORK_ACTIVATION_LOGITS)
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
  else if (with_network->N_activation_last == NETWORK_ACTIVATION_2HEADS)
  {
    float result = arg_result;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));

    result = (float)return_material_score(arg_with_board) / 1000000.0f;

    append_fbuffer_bin(arg_fcsv, &result, sizeof(float));
  }
  else
    FATAL("unknown with_network->N_activation_last", EXIT_FAILURE)

  append_fbuffer_bin(arg_fcsv, NULL, 0);

  if (arg_ffen != NULL)
    append_fbuffer_fmt(arg_ffen, "%s {%.6f}\n", bdata(arg_bfen), arg_result);
}

void fen2csv(char *arg_name, int arg_npieces_min, int arg_npieces_max)
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
             with_network->N_materials_shared.MSS_nmaterials,
             activation_last2string(with_network->N_activation_last, TRUE),
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
         ipattern < with_network->N_patterns_shared.PSS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_network->N_patterns_shared.PSS_patterns + ipattern;

      int nstates = with_pattern_shared->PS_nstates;

      PRINTF("ipattern=%d nstates=%d\n", ipattern, nstates);

      bin_t ibin = nstates;

      append_fbuffer_bin(&ftraining, &ibin, sizeof(bin_t));
    }

    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

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
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_fen2csv_barrier,
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

  bucket_t bucket_nwhite_man;

  construct_bucket(&bucket_nwhite_man,
                   "bucket_nwhite_man",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nblack_man;

  construct_bucket(&bucket_nblack_man,
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

  bucket_t bucket_nman;

  construct_bucket(&bucket_nman,
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

  BSTRING(bdata_name);

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

      ui64_t white_man_bb = mirror_bb(with_board->B_black_man_bb);
      ui64_t black_man_bb = mirror_bb(with_board->B_white_man_bb);
      ui64_t white_king_bb = mirror_bb(with_board->B_black_king_bb);
      ui64_t black_king_bb = mirror_bb(with_board->B_white_king_bb);

      with_board->B_white_man_bb = white_man_bb;
      with_board->B_black_man_bb = black_man_bb;
      with_board->B_white_king_bb = white_king_bb;
      with_board->B_black_king_bb = black_king_bb;

      with_board->B_colour2move = WHITE_ENUM;

      board2fen(with_board, bfen, FALSE);
    }

    int nwhite_man = BIT_COUNT(with_board->B_white_man_bb);

    int nblack_man = BIT_COUNT(with_board->B_black_man_bb);

    int nman = nwhite_man + nblack_man;

    int nwhite_kings = BIT_COUNT(with_board->B_white_king_bb);

    int nblack_kings = BIT_COUNT(with_board->B_black_king_bb);

    int nkings = nwhite_kings + nblack_kings;

    int npieces = nman + nkings;

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

    update_bucket(&bucket_nwhite_man, nwhite_man);

    update_bucket(&bucket_nblack_man, nblack_man);

    update_bucket(&bucket_nwhite_kings, nwhite_kings);

    update_bucket(&bucket_nblack_kings, nblack_kings);

    update_bucket(&bucket_nman, nman);

    update_bucket(&bucket_nkings, nkings);

    {
      int delta_nman = nwhite_man - nblack_man;
      int delta_nkings = nwhite_kings - nblack_kings;
      nman = nwhite_man + nblack_man;

      double result_wtm = result;

      if (IS_BLACK(with_board->B_colour2move))
        result_wtm = -result_wtm;

      append_fbuffer_fmt(&fdata,
                         "%d,%d,%.6f\n",
                         delta_nman,
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

  my_stats_aggregate(&stats_original_labels, my_mpi_globals.MMG_comm_slaves);

  my_stats_aggregate(&stats_training_labels, my_mpi_globals.MMG_comm_slaves);

  my_stats_aggregate(&stats_validation_labels, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_original_labels,
                          my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_training_labels,
                          my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_validation_labels,
                          my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_material_score,
                          my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_man, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_man, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_kings, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nman, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nkings, my_mpi_globals.MMG_comm_slaves);

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

  printf_bucket(&bucket_nwhite_man);

  printf_bucket(&bucket_nblack_man);

  printf_bucket(&bucket_nwhite_kings);

  printf_bucket(&bucket_nblack_kings);

  printf_bucket(&bucket_nman);

  printf_bucket(&bucket_nkings);

  // write remaining

  flush_fbuffer(&ffen, 0);

  flush_fbuffer(&fvalidation, 0);

  flush_fbuffer(&ftraining, 0);

  BDESTROY(bsuffix)

  BDESTROY(btag)

  BDESTROY(barg_name)
}

void init_networks(void)
{
  init_base64_table();
  init_base3_table();
  init_base3_mask_table();

  networks[NETWORK0_ID].N_network_id = INVALID;
  networks[NETWORK1_ID].N_network_id = INVALID;

  construct_network_shared(options.network0, NETWORK0_ID, TRUE);

  if (compat_strcasecmp(options.network1, "NULL") != 0)
    construct_network_shared(options.network1, NETWORK1_ID, TRUE);
}

local double gelu(double x) { return (0.5 * x * (1.0 + erf(x / sqrt(2.0)))); }

#define SCALE 1000
#define XMAX 5
#define N 8

void test_networks(void)
{
  ALIGN64(float a[N]);
  ALIGN64(float b[N]);
  ALIGN64(double e[N]);

  for (int i = 0; i < N; i++)
    a[i] = 0.0f;

  double ex = 0.0;

  for (int ix = -XMAX * SCALE; ix <= XMAX * SCALE; ++ix)
  {
    float x = (float)ix / (float)SCALE;

    a[0] = x;

    activation_geluv2_scaled(N, a, b);

    double g = gelu((double)x);

    e[0] = fabs((double)b[0] - g);

    if (e[0] > ex)
    {
      ex = e[0];

      for (int i = N - 1; i > 0; --i)
      {
        a[i] = a[i - 1];
        b[i] = b[i - 1];
        e[i] = e[i - 1];
      }
    }
  }

  for (int i = 1; i < N; i++)
  {
    PRINTF("i=%d a[i]=%.6f gelu(a[i])=%.6f b[i]=%.6f e[i]=%.6f\n",
           i,
           a[i],
           gelu((double)a[i]),
           b[i],
           e[i]);
  }
  PRINTF("ex=%.6f\n", ex);
}
