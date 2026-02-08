// SCU REVISION 8.1181 zo  8 feb 2026 10:04:41 CET
#include "globals.h"

// MPI shared memory does not guarantee 32-byte alignment

#define MM256_LOAD_PS _mm256_loadu_ps

#define NPIECES_PERCENTILE (INVALID)

#undef USE_RECIPROCAL
#define USE_ERF

#define APPEND_MATERIAL_COMBINED
#undef APPEND_MATERIAL_DELTA

#define TAG "w2m-embed-patterns-wmbmwkbk"

#ifdef APPEND_MATERIAL_COMBINED
#define TAG "w2m-embed-patterns-wmbmwkbk-combinedv2"
#endif

#ifdef APPEND_MATERIAL_DELTA
#define TAG "w2m-embed-patterns-wmbmwkbk-delta"
#endif

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT

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

network_global_t network_global;

// for binary fen

typedef i32_t bin_t;

local int load_network = TRUE;
local i8_t base64_table[256];

void init_base64_table(void)
{
  const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  for (int i = 0; i < 256; i++)
    base64_table[i] = INVALID;

  for (int i = 0; base64_chars[i]; i++)
    base64_table[(unsigned char)base64_chars[i]] = i;
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

local void read_2d_csv_base64(cJSON *arg_csv,
                              int *arg_nrows,
                              int *arg_ncols,
                              scaled_double_t ***arg_cells_nrowsxncols,
                              scaled_double_t ***arg_cells_ncolsxnrows)
{
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

  scaled_double_t *cells_nrowsxncols;

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
  cells_nrowsxncols = cells_float;
#else
  MY_MALLOC_BY_TYPE(cells_nrowsxncols, scaled_double_t, nrows * ncols);

  for (int ifloat = 0; ifloat < nrows * ncols; ifloat++)
    cells_nrowsxncols[ifloat] = FLOAT2SCALED(cells_float[ifloat]);

  my_free(cells_float);
#endif

  MY_MALLOC_BY_TYPE(*arg_cells_nrowsxncols, scaled_double_t *, nrows);

  for (int irow = 0; irow < nrows; irow++)
    (*arg_cells_nrowsxncols)[irow] = cells_nrowsxncols + irow * ncols;

  if (arg_cells_ncolsxnrows != NULL)
  {
    scaled_double_t *cells_ncolsxnrows;

    MY_MALLOC_BY_TYPE(cells_ncolsxnrows, scaled_double_t, ncols * nrows);

    MY_MALLOC_BY_TYPE(*arg_cells_ncolsxnrows, scaled_double_t *, ncols);

    for (int icol = 0; icol < ncols; icol++)
      (*arg_cells_ncolsxnrows)[icol] = cells_ncolsxnrows + icol * nrows;

    for (int irow = 0; irow < nrows; irow++)
    {
      for (int icol = 0; icol < ncols; icol++)
      {
        cells_ncolsxnrows[icol * nrows + irow] =
          cells_nrowsxncols[irow * ncols + icol];
      }
    }
  }

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
  if (n < 8)
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

      vs = _mm256_add_ps(vs, _mm256_mul_ps(va, vb));
      // vs = _mm256_fmadd_ps(va, vb, vs);
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

local void construct_dense_network_shared(dense_network_shared_t *self,
                                          cJSON *arg_directory,
                                          char *arg_prefix,
                                          int arg_verbose)
{
  dense_network_shared_t *object = self;

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

    if (file == NULL)
      continue;

    HARDBUG(object->DNS_nlayers >= NLAYERS_MAX)

    layer_shared_t *with = object->DNS_layers + object->DNS_nlayers;

    with->LS_ninputs = with->LS_noutputs = INVALID;

    read_2d_csv_base64(file,
                       &(with->LS_ninputs),
                       &(with->LS_noutputs),
                       &(with->LS_weights_ninputsxnoutputs),
                       &(with->LS_weights_noutputsxninputs));

    mark_pointer_read_only(with->LS_weights_ninputsxnoutputs[0]);
    mark_pointer_read_only(with->LS_weights_noutputsxninputs[0]);

    file = open_cjson_file(arg_directory, bdata(bbias));

    HARDBUG(file == NULL)

    read_1d_csv_base64(file, &(with->LS_noutputs), &(with->LS_bias));

    mark_pointer_read_only(with->LS_bias);

    object->DNS_nlayers++;

    BDESTROY(bbias)

    BDESTROY(bweights)
  }

  HARDBUG(object->DNS_nlayers < 1)

  if (arg_verbose)
  {
    PRINTF("nlayers=%d\n", object->DNS_nlayers);

    for (int ilayer = 0; ilayer < object->DNS_nlayers; ilayer++)
    {
      layer_shared_t *with = object->DNS_layers + ilayer;

      PRINTF("ilayer=%d (inputs, outputs)=(%d, %d)\n",
             ilayer,
             with->LS_ninputs,
             with->LS_noutputs);
    }
  }

  // check topology

  for (int ilayer = 0; ilayer < object->DNS_nlayers - 1; ilayer++)
  {
    layer_shared_t *with_current = object->DNS_layers + ilayer;
    layer_shared_t *with_next = object->DNS_layers + ilayer + 1;

    HARDBUG(with_current->LS_noutputs != with_next->LS_ninputs)
  }
}

void construct_network_shared(int arg_verbose)
{
  cJSON *networks_json;

  file2cjson(options.networks, &networks_json);

  // find the directory

  cJSON *directories = cJSON_GetObjectItem(networks_json, CJSON_DIRECTORIES_ID);

  HARDBUG(!cJSON_IsArray(directories))

  int found = FALSE;

  cJSON *directory;
  char *directory_name = "NULL";

  cJSON_ArrayForEach(directory, directories)
  {
    cJSON *name_cjson = cJSON_GetObjectItem(directory, CJSON_DIRECTORY_NAME_ID);

    directory_name = cJSON_GetStringValue(name_cjson);

    HARDBUG(directory_name == NULL)

    if (compat_strcasecmp(options.network_name, "NULL") == 0)
    {
      found = TRUE;

      strcpy(options.network_name, directory_name);

      break;
    }

    if (strcmp(directory_name, options.network_name) == 0)
    {
      found = TRUE;

      break;
    }
  }

  if (!found)
  {
    PRINTF("could not find name %s in %s\n",
           options.network_name,
           options.networks);

    FATAL("options.networks error", EXIT_FAILURE)
  }

  PRINTF("directory_name=%s\n", directory_name);

  // shape

  cJSON *value_cjson = cJSON_GetObjectItem(directory, CJSON_SHAPE_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  char *svalue = cJSON_GetStringValue(value_cjson);

  network_global.NG_bshape = bfromcstr(svalue);

  if (arg_verbose)
    PRINTF("shape=%s\n", bdata(network_global.NG_bshape));

  // network2material_score

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NETWORK2MATERIAL_SCORE_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson))

  network_global.NG_network2material_score = cJSON_GetNumberValue(value_cjson);

  if (arg_verbose)
    PRINTF("network_global.NG_network2material_score=%.2f\n",
           network_global.NG_network2material_score);

  // nmaterial

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMATERIAL_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson))

  materials_global.MSS_nmaterial = cJSON_GetNumberValue(value_cjson);

  HARDBUG((materials_global.MSS_nmaterial != 4) and
          (materials_global.MSS_nmaterial != 5) and
          (materials_global.MSS_nmaterial != 7))

  if (arg_verbose)
    PRINTF("materials_global.MSS_nmaterial=%d\n",
           materials_global.MSS_nmaterial);

  // architecture

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ARCHITECTURE_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "null") == 0)
    network_global.NG_architecture = NETWORK_ARCHITECTURE_NULL;
  else if (compat_strcasecmp(svalue, "add") == 0)
    network_global.NG_architecture = NETWORK_ARCHITECTURE_ADD;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    network_global.NG_architecture = NETWORK_ARCHITECTURE_CONCAT;
  else
    FATAL("unknown architecture option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network_global.NG_architecture == NETWORK_ARCHITECTURE_NULL)
      PRINTF("architecture=NULL\n");
    else if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
      PRINTF("architecture=ADD\n");
    else if (network_global.NG_architecture == NETWORK_ARCHITECTURE_CONCAT)
      PRINTF("architecture=CONCAT\n");
    else
      FATAL("unknown architecture option", EXIT_FAILURE)
  }

  // activation_inputs

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_INPUTS_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu6") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_RELU6;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_RSQRT;
  else if (compat_strcasecmp(svalue, "gelu") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_GELU;
  else if (compat_strcasecmp(svalue, "geluv2") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_GELUV2;
  else if (compat_strcasecmp(svalue, "linear") == 0)
    network_global.NG_activation_inputs = NETWORK_ACTIVATION_LINEAR;
  else
    FATAL("unknown activation_inputs option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_RELU6)
      PRINTF("activation_inputs=RELU6\n");
    else if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_TANH)
      PRINTF("activation_inputs=TANH\n");
    else if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation_inputs=RSQRT\n");
    else if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_GELU)
      PRINTF("activation_inputs=GELU\n");
    else if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_GELUV2)
      PRINTF("activation_inputs=GELUV2\n");
    else if (network_global.NG_activation_inputs == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_inputs=LINEAR\n");
    else
      FATAL("unknown activation_inputs option", EXIT_FAILURE)
  }

  // activation

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu6") == 0)
    network_global.NG_activation = NETWORK_ACTIVATION_RELU6;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    network_global.NG_activation = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    network_global.NG_activation = NETWORK_ACTIVATION_RSQRT;
  else if (compat_strcasecmp(svalue, "gelu") == 0)
    network_global.NG_activation = NETWORK_ACTIVATION_GELU;
  else if (compat_strcasecmp(svalue, "geluv2") == 0)
    network_global.NG_activation = NETWORK_ACTIVATION_GELUV2;
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network_global.NG_activation == NETWORK_ACTIVATION_RELU6)
      PRINTF("activation=RELU6\n");
    else if (network_global.NG_activation == NETWORK_ACTIVATION_TANH)
      PRINTF("activation=TANH\n");
    else if (network_global.NG_activation == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation=RSQRT\n");
    else if (network_global.NG_activation == NETWORK_ACTIVATION_GELU)
      PRINTF("activation=GELU\n");
    else if (network_global.NG_activation == NETWORK_ACTIVATION_GELUV2)
      PRINTF("activation=GELUV2\n");
    else
      FATAL("unknown activation option", EXIT_FAILURE)
  }

  // activation_last

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_LAST_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "linear") == 0)
    network_global.NG_activation_last = NETWORK_ACTIVATION_LINEAR;
  else if (compat_strcasecmp(svalue, "sigmoid") == 0)
    network_global.NG_activation_last = NETWORK_ACTIVATION_SIGMOID;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network_global.NG_activation_last == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_last=LINEAR\n");
    else if (network_global.NG_activation_last == NETWORK_ACTIVATION_SIGMOID)
      PRINTF("activation_last=SIGMOID\n");
    else
      FATAL("unknown activation_last option", EXIT_FAILURE)
  }

  // nman_min

  network_global.NG_nman_min = 0;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MIN_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    network_global.NG_nman_min = ivalue;

    if (arg_verbose)
    {
      PRINTF("NG_nman_min=%d\n", network_global.NG_nman_min);
    }
  }

  // nman_max

  network_global.NG_nman_max = 40;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MAX_ID);

  if (cJSON_IsNumber(value_cjson))
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    network_global.NG_nman_max = ivalue;

    if (arg_verbose)
    {
      PRINTF("NG_nman_max=%d\n", network_global.NG_nman_max);
    }
  }

  HARDBUG(network_global.NG_nman_min > network_global.NG_nman_max)

  construct_patterns_shared(&(patterns_shared),
                            bdata(network_global.NG_bshape));

  PRINTF("load_network=%d\n", load_network);

  if (!load_network)
    return;

  network_global.NG_pattern_dense_network_shared.DNS_ninputs = 0;
  network_global.NG_pattern_dense_network_shared.DNS_nlayers = 0;
  network_global.NG_pattern_dense_network_shared.DNS_activate_layer0 = FALSE;
  network_global.NG_pattern_dense_network_shared.DNS_compute_layer0 = TRUE;

  network_global.NG_material_dense_network_shared.DNS_ninputs = 0;
  network_global.NG_material_dense_network_shared.DNS_nlayers = 0;
  network_global.NG_material_dense_network_shared.DNS_activate_layer0 = FALSE;
  network_global.NG_material_dense_network_shared.DNS_compute_layer0 = TRUE;

  network_global.NG_dense_network_shared.DNS_ninputs = 0;
  network_global.NG_dense_network_shared.DNS_nlayers = 0;
  network_global.NG_dense_network_shared.DNS_activate_layer0 = FALSE;
  network_global.NG_dense_network_shared.DNS_compute_layer0 = TRUE;

  network_global.NG_loaded = TRUE;

  // load the embedding vectors

  for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with = patterns_shared.PSS_patterns + ipattern;

    BSTRING(bweights)

    bformata(bweights, "pattern_embedding%d_weights.csv", ipattern);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    read_2d_csv_base64(file,
                       &(with->PS_nstates),
                       &(with->PS_nembed),
                       &(with->PS_weights_nstatesxnembed),
                       NULL);

    mark_pointer_read_only(with->PS_weights_nstatesxnembed[0]);

    if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
    {
      with->PS_offset = 0;

      if (network_global.NG_pattern_dense_network_shared.DNS_ninputs == 0)
      {
        network_global.NG_pattern_dense_network_shared.DNS_ninputs =
          with->PS_nembed; // first
      }
      else
        HARDBUG(with->PS_nembed !=
                network_global.NG_pattern_dense_network_shared.DNS_ninputs)
    }
    else
    {
      with->PS_offset =
        network_global.NG_pattern_dense_network_shared.DNS_ninputs;

      network_global.NG_pattern_dense_network_shared.DNS_ninputs +=
        with->PS_nembed;
    }

    PRINTF("ipattern=%d with->PS_nstates=%d with->PS_nembed=%d"
           " with->PS_offset=%d\n",
           ipattern,
           with->PS_nstates,
           with->PS_nembed,
           with->PS_offset);

    HARDBUG(network_global.NG_pattern_dense_network_shared.DNS_ninputs >=
            NINPUTS_MAX)
  }

  // load the material

  for (int imaterial = 0; imaterial < materials_global.MSS_nmaterial;
       imaterial++)
  {
    material_shared_t *with = materials_global.MSS_materials + imaterial;

    BSTRING(bweights)

    bformata(bweights, "material_embedding%d_weights.csv", imaterial);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    if (materials_global.MSS_nmaterial == 4)
    {
      with->MG_nstates = 21;
    }
    else if (materials_global.MSS_nmaterial == 5)
    {
      if (imaterial < 4)
        with->MG_nstates = 21;
      else
        with->MG_nstates = 21 * 6 * 21 * 6;
    }
    else if (materials_global.MSS_nmaterial == 7)
    {
      if (imaterial < 6)
        with->MG_nstates = 21;
      else
        with->MG_nstates = 21 * 11;
    }
    else
      FATAL("network_global.NG_nmaterial out of range", EXIT_FAILURE)

    with->MG_nembed = INVALID;

    read_2d_csv_base64(file,
                       &(with->MG_nstates),
                       &(with->MG_nembed),
                       &(with->MG_weights_nstatesxnembed),
                       &(with->MG_weights_nembedxnstates));

    mark_pointer_read_only(with->MG_weights_nstatesxnembed[0]);
    mark_pointer_read_only(with->MG_weights_nembedxnstates[0]);

    if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
    {
      with->MG_offset = 0;

      if (network_global.NG_material_dense_network_shared.DNS_ninputs == 0)
      {
        network_global.NG_material_dense_network_shared.DNS_ninputs =
          with->MG_nembed; // first
      }
      else
        HARDBUG(with->MG_nembed !=
                network_global.NG_material_dense_network_shared.DNS_ninputs)
    }
    else
    {
      with->MG_offset =
        network_global.NG_material_dense_network_shared.DNS_ninputs;

      network_global.NG_material_dense_network_shared.DNS_ninputs +=
        with->MG_nembed;
    }

    PRINTF("with->MG_nstates=%d with->MG_nembed=%d with->MG_offset=%d\n",
           with->MG_nstates,
           with->MG_nembed,
           with->MG_offset);

    HARDBUG(network_global.NG_material_dense_network_shared.DNS_ninputs >=
            NINPUTS_MAX)
  }

  // read the dense networks

  construct_dense_network_shared(
    &(network_global.NG_pattern_dense_network_shared),
    directory,
    "pattern",
    TRUE);

  construct_dense_network_shared(
    &(network_global.NG_material_dense_network_shared),
    directory,
    "material",
    TRUE);

  // set expected inputs

  {
    dense_network_shared_t *with =
      &(network_global.NG_pattern_dense_network_shared);

    network_global.NG_dense_network_shared.DNS_ninputs =
      with->DNS_layers[with->DNS_nlayers - 1].LS_noutputs;

    with = &(network_global.NG_material_dense_network_shared);

    network_global.NG_dense_network_shared.DNS_ninputs +=
      with->DNS_layers[with->DNS_nlayers - 1].LS_noutputs;
  }

  PRINTF("network_global.NG_dense_network_shared.DNS_ninputs=%d\n",
         network_global.NG_dense_network_shared.DNS_ninputs);

  HARDBUG(network_global.NG_dense_network_shared.DNS_ninputs >= NINPUTS_MAX)

  construct_dense_network_shared(&(network_global.NG_dense_network_shared),
                                 directory,
                                 NULL,
                                 TRUE);

  if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
  {
    for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        patterns_shared.PSS_patterns + ipattern;

      with_pattern_shared->PS_sum_nstatesxnoutputs = NULL;
    }
    network_global.NG_pattern_dense_network_shared.DNS_activate_layer0 = TRUE;
    network_global.NG_pattern_dense_network_shared.DNS_compute_layer0 = TRUE;
  }
  else
  {
    // precompute layer0 contribution

    network_global.NG_pattern_dense_network_shared.DNS_activate_layer0 = FALSE;
    network_global.NG_pattern_dense_network_shared.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      network_global.NG_pattern_dense_network_shared.DNS_layers;

    for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        patterns_shared.PSS_patterns + ipattern;

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

        if (my_mpi_globals.MMG_id_global <= 0)
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

      // mark_pointer_read_only(sum_nstatesxnoutputs);
    }
  }

  if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
  {
    for (int imaterial = 0; imaterial < materials_global.MSS_nmaterial;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        materials_global.MSS_materials + imaterial;

      with_material_shared->MG_sum_nstatesxnoutputs = NULL;
    }

    network_global.NG_material_dense_network_shared.DNS_activate_layer0 = TRUE;
    network_global.NG_material_dense_network_shared.DNS_compute_layer0 = TRUE;
  }
  else
  {
    // precompute layer0 contribution

    network_global.NG_material_dense_network_shared.DNS_activate_layer0 = FALSE;
    network_global.NG_material_dense_network_shared.DNS_compute_layer0 = FALSE;

    layer_shared_t *with_current =
      network_global.NG_material_dense_network_shared.DNS_layers;

    for (int imaterial = 0; imaterial < materials_global.MSS_nmaterial;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        materials_global.MSS_materials + imaterial;

      scaled_double_t *sum;

      MY_MALLOC_BY_TYPE(sum,
                        scaled_double_t,
                        with_material_shared->MG_nstates *
                          with_current->LS_noutputs);

      MY_MALLOC_BY_TYPE(with_material_shared->MG_sum_nstatesxnoutputs,
                        scaled_double_t *,
                        with_material_shared->MG_nstates);

      for (int istate = 0; istate < with_material_shared->MG_nstates; istate++)
      {
        with_material_shared->MG_sum_nstatesxnoutputs[istate] =
          sum + istate * with_current->LS_noutputs;

        for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
        {
          with_material_shared->MG_sum_nstatesxnoutputs[istate][ioutput] =
            dot_scaled(with_material_shared->MG_nembed,
                       with_material_shared->MG_weights_nstatesxnembed[istate],
                       with_current->LS_weights_noutputsxninputs[ioutput] +
                         with_material_shared->MG_offset);
        }
      }

      mark_pointer_read_only(sum);
    }
  }
}

local void construct_dense_network_thread(
  dense_network_shared_t *with_dense_network_shared,
  dense_network_thread_t *with_dense_network_thread)
{
  for (int ilayer = 0; ilayer < with_dense_network_shared->DNS_nlayers;
       ilayer++)
  {
    layer_shared_t *with = with_dense_network_shared->DNS_layers + ilayer;

    layer_thread_t *with_thread =
      with_dense_network_thread->DNT_layers + ilayer;

    MY_MALLOC_BY_TYPE(with_thread->LT_dot, scaled_double_t, with->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_sum, scaled_double_t, with->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_outputs,
                      scaled_double_t,
                      with->LS_noutputs);
  }
}

void construct_network_thread(network_thread_t *self, int arg_verbose)
{
  network_thread_t *object = self;

  construct_dense_network_thread(
    &(network_global.NG_pattern_dense_network_shared),
    &(object->NT_pattern_dense_network_thread));

  construct_dense_network_thread(
    &(network_global.NG_material_dense_network_shared),
    &(object->NT_material_dense_network_thread));

  construct_dense_network_thread(&(network_global.NG_dense_network_shared),
                                 &(object->NT_dense_network_thread));
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
  if (n < 8)
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
  if (n < 8)
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
  if (n < 8)
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
  if (n < 8)
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
  if (n < 8)
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
  if (n < 8)
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

#define CLIP_VALUE 6

local void activation_clipped_relu_scaled(int n,
                                          scaled_double_t *restrict a,
                                          scaled_double_t *restrict b)
{
  // for (int i = 0; i < n; i++) HARDBUG(a[i] > CLIP_VALUE)

#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (int i = 0; i < n; i++)
    {
      if (a[i] <= 0)
        b[i] = 0;
      else if (a[i] >= (scaled_double_t)CLIP_VALUE)
        b[i] = (scaled_double_t)CLIP_VALUE;
      else
        b[i] = a[i];
    }
#else
    for (int i = 0; i < n; i++)
    {
      if (a[i] <= 0)
        b[i] = 0;
      else if (a[i] >= (SCALED_DOUBLE_FACTOR * CLIP_VALUE))
        b[i] = (SCALED_DOUBLE_FACTOR * CLIP_VALUE);
      else
        b[i] = a[i];
    }
#endif
  }
#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    const __m256 vz = _mm256_setzero_ps();

    const __m256 vc = _mm256_set1_ps((scaled_double_t)CLIP_VALUE);

    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 ta = MM256_LOAD_PS(a);

      ta = _mm256_max_ps(ta, vz);
      ta = _mm256_min_ps(ta, vc);

      _mm256_store_ps(b, ta);
    }
#else
    __m256i zero = _mm256_setzero_si256();
    __m256i clip = _mm256_set1_epi32(SCALED_DOUBLE_FACTOR * CLIP_VALUE);

    for (const int32_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);

      va = _mm256_max_epi32(va, zero);
      va = _mm256_min_epi32(va, clip);

      _mm256_store_si256((__m256i *)b, va);
    }
#endif
  }
#endif
}

local void activation_clipped_relu_double(int n,
                                          scaled_double_t *restrict a,
                                          scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= CLIP_VALUE)
      b[i] = (scaled_double_t)CLIP_VALUE;
    else
      b[i] = a[i];
#else
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= (SCALED_DOUBLE_FACTOR * CLIP_VALUE))
      b[i] = (scaled_double_t)(SCALED_DOUBLE_FACTOR * CLIP_VALUE);
    else
      b[i] = a[i];
#endif
  }
}

local void activation_tanh_scaled(int n,
                                  scaled_double_t *restrict a,
                                  scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
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
  if (n < 8)
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
  if ((n < 8) or TRUE)
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
  if (arg_activation == NETWORK_ACTIVATION_RELU6)
  {
    activation_clipped_relu_scaled(n, a, b);
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
  if (arg_activation == NETWORK_ACTIVATION_RELU6)
  {
    activation_clipped_relu_double(n, a, b);
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

int base3_index(const int32_t *embed, int nlinear)
{
  int result = 0;
  int power = 1;

  for (int ilinear = 0; ilinear < nlinear; ilinear++)
  {
    result += embed[ilinear] * power;

    power *= 3;
  }
  return (result);
}

// assumes inputs have been set

double
return_network_score_scaled(dense_network_shared_t *arg_dense_network_shared,
                            dense_network_thread_t *arg_dense_network_thread)
{
  BEGIN_BLOCK(__func__)

  layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_thread->DNT_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    ALIGN64(scaled_double_t activated_inputs[NINPUTS_MAX]);

    activation_scaled(network_global.NG_activation_inputs,
                      with_current->LS_ninputs,
                      arg_dense_network_thread->DNT_inputs,
                      activated_inputs);

    // preserve inputs for return_network_score_double

    HARDBUG(arg_dense_network_shared->DNS_compute_layer0 == FALSE)

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] =
        dot_scaled(with_current->LS_ninputs,
                   activated_inputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }
  }
  else
  {
    if (arg_dense_network_shared->DNS_compute_layer0)
    {
      for (int i = 0; i < with_current->LS_noutputs; i++)
      {
        with_current_thread->LT_dot[i] =
          dot_scaled(with_current->LS_ninputs,
                     arg_dense_network_thread->DNT_inputs,
                     with_current->LS_weights_noutputsxninputs[i]);
      }
    }
  }

  // first layer

  BEGIN_BLOCK("Ax+b-and-activation-first-layer")

  vadd_ab2c(with_current->LS_noutputs,
            with_current_thread->LT_dot,
            with_current->LS_bias,
            with_current_thread->LT_sum);

  BEGIN_BLOCK("Ax+b-and-activation-first-layer-activation")

  activation_scaled(network_global.NG_activation,
                    with_current->LS_noutputs,
                    with_current_thread->LT_sum,
                    with_current_thread->LT_outputs);

  END_BLOCK

  END_BLOCK

  for (int ilayer = 1; ilayer < arg_dense_network_shared->DNS_nlayers; ilayer++)
  {
    with_current = arg_dense_network_shared->DNS_layers + ilayer;
    with_current_thread = arg_dense_network_thread->DNT_layers + ilayer;

    layer_thread_t *with_previous_thread =
      arg_dense_network_thread->DNT_layers + ilayer - 1;

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

    if (with_current->LS_noutputs > 1)
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-activation")

      activation_scaled(network_global.NG_activation,
                        with_current->LS_noutputs,
                        with_current_thread->LT_sum,
                        with_current_thread->LT_outputs);

      END_BLOCK
    }
    else
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-last-layer")

      if (network_global.NG_activation_last == NETWORK_ACTIVATION_LINEAR)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (network_global.NG_activation_last == NETWORK_ACTIVATION_SIGMOID)
      {
        with_current_thread->LT_outputs[0] =
          return_sigmoid(with_current_thread->LT_sum[0]);
      }
      else
        FATAL("unknown activation_last option", EXIT_FAILURE)

      END_BLOCK
    }

    END_BLOCK
  }

  END_BLOCK

  layer_thread_t *with_thread = arg_dense_network_thread->DNT_layers +
                                arg_dense_network_shared->DNS_nlayers - 1;

  return (SCALED2DOUBLE(with_thread->LT_outputs[0]));
}

double
return_network_score_double(dense_network_shared_t *arg_dense_network_shared,
                            dense_network_thread_t *arg_dense_network_thread)
{
  layer_shared_t *with_current = arg_dense_network_shared->DNS_layers;
  layer_thread_t *with_current_thread = arg_dense_network_thread->DNT_layers;

  if (arg_dense_network_shared->DNS_activate_layer0)
  {
    scaled_double_t activated_inputs[NINPUTS_MAX];

    activation_double(network_global.NG_activation_inputs,
                      with_current->LS_ninputs,
                      arg_dense_network_thread->DNT_inputs,
                      activated_inputs);

    // preserve inputs for return_network_score_double

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
                   arg_dense_network_thread->DNT_inputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }
  }

  for (int i = 0; i < with_current->LS_noutputs; i++)
  {
    with_current_thread->LT_sum[i] =
      with_current_thread->LT_dot[i] + with_current->LS_bias[i];
  }

  activation_double(network_global.NG_activation,
                    with_current->LS_noutputs,
                    with_current_thread->LT_sum,
                    with_current_thread->LT_outputs);

  for (int ilayer = 1; ilayer < arg_dense_network_shared->DNS_nlayers; ilayer++)
  {
    with_current = arg_dense_network_shared->DNS_layers + ilayer;

    with_current_thread = arg_dense_network_thread->DNT_layers + ilayer;

    layer_thread_t *with_previous_thread =
      arg_dense_network_thread->DNT_layers + ilayer - 1;

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
      activation_double(network_global.NG_activation,
                        with_current->LS_noutputs,
                        with_current_thread->LT_sum,
                        with_current_thread->LT_outputs);
    }
    else
    {
      if (network_global.NG_activation_last == NETWORK_ACTIVATION_LINEAR)
      {
        with_current_thread->LT_outputs[0] = with_current_thread->LT_sum[0];
      }
      else if (network_global.NG_activation_last == NETWORK_ACTIVATION_SIGMOID)
      {
        with_current_thread->LT_outputs[0] =
          return_sigmoid(with_current_thread->LT_sum[0]);
      }
      else
        FATAL("unknown activation_last option", EXIT_FAILURE)
    }
  }

  layer_thread_t *with_thread = arg_dense_network_thread->DNT_layers +
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

void fen2network(char *arg_name, i64_t arg_npositions)
{
  board_t board;

  construct_board(&board, STDOUT);

  board_t *with = &board;

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

  i64_t nfen_mpi = 0;

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

  i64_t nfactor = 0;
  double factor = 0.0;

  i64_t nnetwork2mat_score_gt_score_man = 0;

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  i64_t ntodo = nfen;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = nfen / my_mpi_globals.MMG_nslaves + 1;

  progress_t progress;

  construct_progress(&progress, ntodo, 10);

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((ifen % my_mpi_globals.MMG_nslaves) != my_mpi_globals.MMG_id_slave))
    {
      continue;
    }

    update_progress(&progress);

    nfen_mpi++;

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with, bdata(bfen));

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);
    int nblack_man = BIT_COUNT(with->B_black_man_bb);
    int nwhite_king = BIT_COUNT(with->B_white_king_bb);
    int nblack_king = BIT_COUNT(with->B_black_king_bb);

    int npieces = nwhite_man + nblack_man + nwhite_king + nblack_king;

    if ((NPIECES_PERCENTILE != INVALID) and (npieces != NPIECES_PERCENTILE))
      continue;

    npieces_fen[ifen] = npieces;

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    // HARDBUG(fabs(result) >= 0.999999)

    // check_moves(with, &moves_list);

    int mat_score = return_material_score(with);

    return_score_from_board(with, score_scaled + ifen);

    score_double[ifen] = return_network_score_double(
      &(network_global.NG_dense_network_shared),
      &(with->B_network_thread.NT_dense_network_thread));

    if (IS_BLACK(with->B_colour2move))
      mat_score = -mat_score;

    if (ifen <= 100)
      PRINTF("%s {%.6f} ms=%d ss=%.6f sd=%.6f r=%.6f\n",
             bdata(bfen),
             result,
             mat_score,
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
             mat_score,
             score_scaled[ifen],
             score_double[ifen],
             result);

      PRINTF("error_max_scaled_vs_double=%.6f\n", error_max_scaled_vs_double);
    }

    if ((moves_list.ML_ncaptx == 0) and (with->B_white_king_bb == 0) and
        (with->B_black_king_bb == 0) and (result > 0.0) and
        (score_scaled[ifen] >= result) and
        ((mat_score >= SCORE_MAN) and (mat_score <= (3 * SCORE_MAN))))
    {
      ++nfactor;

      factor += mat_score / score_scaled[ifen];

      if (nfactor <= 100)
        PRINTF("nfactor=%lld factor=%.6f\n", nfactor, factor / nfactor);
    }

    if (nfactor > 10)
    {
      int network2mat_score = round(score_scaled[ifen] * factor / nfactor);

      if (abs(network2mat_score - mat_score) > SCORE_MAN)
      {
        if (nnetwork2mat_score_gt_score_man <= 100)
          PRINTF("network2mat_score=%d mat_score=%d\n",
                 network2mat_score,
                 mat_score);
        nnetwork2mat_score_gt_score_man++;
      }
    }
  }

  finalize_progress(&progress);

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen_mpi=%lld\n", nfen_mpi);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

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
                   &nnetwork2mat_score_gt_score_man,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &error_max_scaled_vs_double,
                   1,
                   MPI_DOUBLE,
                   MPI_MAX,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfactor,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &factor,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
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

  PRINTF("nfactor=%lld factor=%.6f\n", nfactor, factor / nfactor);

  PRINTF("nnetwork2mat_score_gt_score_man=%lld\n",
         nnetwork2mat_score_gt_score_man);

  i64_t ipercentile = round(90 / 100.0 * nfen);
  double percentile = delta_scaled[pdelta_scaled[ipercentile]];

  PRINTF("percentile=%.6f\n", percentile);

  construct_my_bstream(&my_bstream, arg_name);

  nfen_mpi = 0;
  i64_t nfen_skipped_mpi = 0;

  nfactor = 0;
  factor = 0.0;

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((nfen % my_mpi_globals.MMG_nslaves) != my_mpi_globals.MMG_id_slave))
    {
      continue;
    }

    nfen_mpi++;

#ifdef DEBUG
    if ((nfen_mpi % 1000) == 0)
      PRINTF("nfen_mpi=%lld\n", nfen_mpi);
    if (nfen_mpi == 10000)
      break;
#else
    if ((nfen_mpi % 100000) == 0)
      PRINTF("nfen_mpi=%lld\n", nfen_mpi);
#endif

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with, bdata(bfen));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    // HARDBUG(fabs(result) >= 0.999999)

    // check_moves(with, &moves_list);

    int nwhite_king = BIT_COUNT(with->B_white_king_bb);
    int nblack_king = BIT_COUNT(with->B_black_king_bb);

    if ((nwhite_king > 0) or (nblack_king > 0))
    {
      nfen_skipped_mpi++;

      continue;
    }

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);
    int nblack_man = BIT_COUNT(with->B_black_man_bb);

    int delta_man = abs(nwhite_man - nblack_man);

    if (delta_man != 1)
    {
      nfen_skipped_mpi++;

      continue;
    }

    if (fabs(score_scaled[ifen] - result) > percentile)
    {
      nfen_skipped_mpi++;

      continue;
    }

    nfactor++;
    factor += fabs(score_scaled[ifen]);
  }

  destroy_my_bstream(&my_bstream);

  BDESTROY(bfen)

  PRINTF("nfen_mpi=%lld nfen_skipped_mpi=%lld\n", nfen_mpi, nfen_skipped_mpi);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_skipped_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen_skipped=%lld\n", nfen_skipped_mpi);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfactor,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE,
                   &factor,
                   1,
                   MPI_DOUBLE,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  factor /= nfactor;

  double factor2man_score = SCORE_MAN / factor;

  PRINTF("nfactor=%lld factor=%.6f factor2man_score=%.6f\n",
         nfactor,
         factor,
         factor2man_score);
}

local void append_fen(board_t *self,
                      bstring arg_bfen,
                      double arg_result,
                      fbuffer_t *arg_fcsv,
                      fbuffer_t *arg_ffen)
{
  board_t *object = self;

  patterns_thread_t *with_patterns_thread =
    &(self->B_network_thread.NT_patterns);

  for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      patterns_shared.PSS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    int input = base3_index(with_pattern_thread->PT_embed,
                            with_pattern_shared->PS_nlinear);

    bin_t ibin = input;

    append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
  }

  // always W2M

  if (IS_BLACK(object->B_colour2move))
    arg_result = -arg_result;

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  int delta_man = nwhite_man - nblack_man;
  if (delta_man < -DELTA_MAN_MAX)
    delta_man = -DELTA_MAN_MAX;
  if (delta_man > DELTA_MAN_MAX)
    delta_man = DELTA_MAN_MAX;
  delta_man += DELTA_MAN_MAX;

  int delta_king = nwhite_king - nblack_king;
  if (delta_king < -DELTA_KING_MAX)
    delta_king = -DELTA_KING_MAX;
  if (delta_king > DELTA_KING_MAX)
    delta_king = DELTA_KING_MAX;
  delta_king += DELTA_KING_MAX;

  int input = nwhite_man;

  bin_t ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

  input = nblack_man;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

  input = nwhite_king;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

  input = nblack_king;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

#ifdef APPEND_MATERIAL_COMBINED
  int mwhite_king = nwhite_king;
  int mblack_king = nblack_king;

  if (mwhite_king > COMBINED_KING_MAX)
    mwhite_king = COMBINED_KING_MAX;
  if (mblack_king > COMBINED_KING_MAX)
    mblack_king = COMBINED_KING_MAX;

  input = (nwhite_man * (COMBINED_KING_MAX + 1) + mwhite_king) * 21 *
            (COMBINED_KING_MAX + 1) +
          nblack_man * (COMBINED_KING_MAX + 1) + mblack_king;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
#endif

#ifdef APPEND_MATERIAL_DELTA
  input = delta_man;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

  input = delta_king;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));

  input = delta_man * (2 * DELTA_KING_MAX + 1) + delta_king;

  ibin = input;

  append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
#endif

  // PRINTF("material=%d\n", input);

  float result = arg_result;

  append_fbuffer_bin(arg_fcsv, &result, sizeof(float));

  append_fbuffer_bin(arg_fcsv, NULL, 0);

  if (arg_ffen != NULL)
    append_fbuffer_fmt(arg_ffen, "%s {%.6f}\n", bdata(arg_bfen), arg_result);
}

void fen2csv(char *arg_name, int arg_npieces_min, int arg_npieces_max)
{
  my_random_t fen2csv_random;

  options.material_only = TRUE;

  construct_my_random(&fen2csv_random, 0);

  board_t board;

  construct_board(&board, STDOUT);

  board_t *with = &board;

  BSTRING(barg_name)

  HARDBUG(bassigncstr(barg_name, arg_name) == BSTR_ERR)

  BSTRING(btag)

  HARDBUG(bformata(btag,
                   "%s-npiecesmin%d-npiecesmax%d-%s",
                   TAG,
                   arg_npieces_min,
                   arg_npieces_max,
                   bdata(network_global.NG_bshape)) == BSTR_ERR)

  // training

  BSTRING(bsuffix)

  HARDBUG(bformata(bsuffix, "-%s-training.bin", bdata(btag)) == BSTR_ERR)

  fbuffer_t ftraining;

  construct_fbuffer(&ftraining, barg_name, bdata(bsuffix), TRUE);

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

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
    nfen++;

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen=%lld\n", nfen);

  i64_t nfen_mpi = 0;
  i64_t nfen_npieces_mpi = 0;
  i64_t nfen_ncapt_mpi = 0;
  i64_t nfen_append_mpi = 0;

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

  i64_t ntodo = nfen;

  if (my_mpi_globals.MMG_nslaves > 0)
    ntodo = nfen / my_mpi_globals.MMG_nslaves + 1;

  construct_progress(&progress, ntodo, 10);

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#')
      continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((ifen % my_mpi_globals.MMG_nslaves) != my_mpi_globals.MMG_id_slave))
      continue;

    update_progress(&progress);

    nfen_mpi++;

    double result;

    CSTRING(cfen, blength(bline))

    // ARES convention

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf}", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(with, bdata(bfen));

    if (IS_BLACK(with->B_colour2move))
      result = -result;

    // END ARES CONVENTION

    CDESTROY(cfen)

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);

    int nblack_man = BIT_COUNT(with->B_black_man_bb);

    int nman = nwhite_man + nblack_man;

    int nwhite_kings = BIT_COUNT(with->B_white_king_bb);

    int nblack_kings = BIT_COUNT(with->B_black_king_bb);

    int nkings = nwhite_kings + nblack_kings;

    int npieces = nman + nkings;

    if ((npieces < arg_npieces_min) or (npieces > arg_npieces_max))
    {
      nfen_npieces_mpi++;

      continue;
    }

    moves_list_t moves_list;

    gen_moves(with, &moves_list);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      nfen_ncapt_mpi++;

      continue;
    }

    nfen_append_mpi++;

    int material_score = return_material_score(with);

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

      if (IS_BLACK(with->B_colour2move))
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
      append_fen(with, bfen, result, &ftraining, NULL);
    }
    else
    {
      append_fen(with, bfen, result, &fvalidation, &ffen);
    }
  }

  finalize_progress(&progress);

  destroy_my_bstream(&my_bstream);

  flush_fbuffer(&ftraining, 0);
  flush_fbuffer(&fvalidation, 0);
  flush_fbuffer(&fdata, 0);

  BDESTROY(bfen)

  BDESTROY(bdata_name)

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_npieces_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_ncapt_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &nfen_append_mpi,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen=%lld\n", nfen);
  PRINTF("nfen_mpi=%lld\n", nfen_mpi);
  PRINTF("nfen_npieces_mpi=%lld\n", nfen_npieces_mpi);
  PRINTF("nfen_ncapt_mpi=%lld\n", nfen_ncapt_mpi);
  PRINTF("nfen_append_mpi=%lld\n", nfen_append_mpi);

  my_mpi_bucket_aggregate(&bucket_material_score,
                          my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_man, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_man, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_kings, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nman, my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nkings, my_mpi_globals.MMG_comm_slaves);

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

  construct_network_shared(TRUE);
}
