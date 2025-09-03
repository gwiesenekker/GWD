//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#undef USE_RECIPROCAL

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT

#define DOUBLE2SCALED(D) ((scaled_double_t) (D))
#define SCALED2DOUBLE(S) ((double) (S))

#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32

#define SCALED_DOUBLE_FACTOR 10000

#define FLOAT2SCALED(D)      (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2FLOAT(S)      ((float) (S) / SCALED_DOUBLE_FACTOR)

#define DOUBLE2SCALED(D)     (round((D) * SCALED_DOUBLE_FACTOR))
#define SCALED2DOUBLE(S)     ((double) (S) / SCALED_DOUBLE_FACTOR)

#else

#error unknown SCALED_DOUBLE_T

#endif

int load_network = TRUE;
network_shared_t network_shared;

//for binary fen

typedef i32_t bin_t;

local i8_t base64_table[256];

void init_base64_table(void)
{
  const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
  for (int i = 0; i < 256; i++) base64_table[i] = INVALID;

  for (int i = 0; base64_chars[i]; i++)
    base64_table[(unsigned char) base64_chars[i]] = i;
}

void decode_base64(size_t ns, unsigned char *s,
                   size_t nt, unsigned char *t)
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

  return(result);
}

local void read_1d_csv_base64(cJSON *arg_csv, int *arg_nrows,
  scaled_double_t **arg_cells)
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

  decode_base64(strlen(data), (unsigned char *) data,
                nrows * sizeof(float), (unsigned char *) cells_float);

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

local void read_2d_csv_base64(cJSON *arg_csv, int *arg_nrows, int *arg_ncols,
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

  MY_MALLOC_BY_TYPE(cells_float, float, nrows * ncols);

  decode_base64(strlen(data), (unsigned char *) data,
                nrows * ncols * sizeof(float),
                (unsigned char *) cells_float);

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

local scaled_double_t dot_scaled(int n, scaled_double_t *restrict a,
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
      __m256 va = _mm256_load_ps(a);
      __m256 vb = _mm256_load_ps(b);
  
      vs = _mm256_add_ps(vs, _mm256_mul_ps(va, vb));
      //vs = _mm256_fmadd_ps(va, vb, vs);

    }
  
    ALIGN64(float t[8]);
  
    _mm256_store_ps(t, vs);

    s = t[0] + t[1] + t[2] + t[3] + t[4] + t[5] + t[6] + t[7];
#else

  __m256i s64_lo = _mm256_setzero_si256();
  __m256i s64_hi = _mm256_setzero_si256();

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);
    __m256i tb = _mm256_load_si256((__m256i*)b);

    __m256i ta_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(ta));
    __m256i ta_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(ta, 1));

    __m256i tb_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(tb));
    __m256i tb_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(tb, 1));

    s64_lo = _mm256_add_epi64(s64_lo, _mm256_mul_epi32(ta_lo, tb_lo));
    s64_hi = _mm256_add_epi64(s64_hi, _mm256_mul_epi32(ta_hi, tb_hi));
  }

  ALIGN64(i64_t r64_lo[4]);
  ALIGN64(i64_t r64_hi[4]);

  _mm256_store_si256((__m256i*)r64_lo, s64_lo);
  _mm256_store_si256((__m256i*)r64_hi, s64_hi);

  i64_t s64 = r64_lo[0] + r64_lo[1] + r64_lo[2] + r64_lo[3] +
              r64_hi[0] + r64_hi[1] + r64_hi[2] + r64_hi[3];

  s = s64 / SCALED_DOUBLE_FACTOR;
#endif
  }
#endif

  END_BLOCK

  return(s);
}

void construct_network_shared(network_shared_t *self, int arg_verbose)
{
  network_shared_t *network = self;

  cJSON *networks_json;

  file2cjson(options.networks, &networks_json);

  //find the directory

  cJSON *directories = cJSON_GetObjectItem(networks_json,
                                           CJSON_DIRECTORIES_ID);
  
  HARDBUG(!cJSON_IsArray(directories))

  int found = FALSE;

  cJSON *directory;
  char *directory_name;

  cJSON_ArrayForEach(directory, directories)
  {
    cJSON *name_cjson = cJSON_GetObjectItem(directory,
                                            CJSON_DIRECTORY_NAME_ID);

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
      options.network_name, options.networks);

    FATAL("options.networks error", EXIT_FAILURE)
  }

  PRINTF("directory_name=%s\n", directory_name);

  cJSON *network2material_score_cjson =
    cJSON_GetObjectItem(directory, CJSON_NEURAL2MATERIAL_SCORE_ID);

  HARDBUG(!cJSON_IsNumber(network2material_score_cjson))

  network->NS_network2material_score =
   cJSON_GetNumberValue(network2material_score_cjson);

  if (arg_verbose)
    PRINTF("network->NS_network2material_score=%.2f\n",
           network->NS_network2material_score);

  cJSON *value_cjson = cJSON_GetObjectItem(directory, CJSON_SHAPE_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  char *svalue = cJSON_GetStringValue(value_cjson);

  network->NS_bshape = bfromcstr(svalue);

  if (arg_verbose) PRINTF("shape=%s\n", bdata(network->NS_bshape));
 
  //embedding

  value_cjson = cJSON_GetObjectItem(directory, CJSON_EMBEDDING_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "sum") == 0)
    network->NS_embedding = NETWORK_EMBEDDING_SUM;
  else if (compat_strcasecmp(svalue, "sum2") == 0)
    network->NS_embedding = NETWORK_EMBEDDING_SUM2;
  else if (compat_strcasecmp(svalue, "concat") == 0)
    network->NS_embedding = NETWORK_EMBEDDING_CONCAT;
  else
    FATAL("unknown embedding option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network->NS_embedding == NETWORK_EMBEDDING_SUM)
      PRINTF("embedding=SUM\n");
    else if (network->NS_embedding == NETWORK_EMBEDDING_SUM2)
      PRINTF("embedding=SUM2\n");
    else if (network->NS_embedding == NETWORK_EMBEDDING_CONCAT)
      PRINTF("embedding=CONCAT\n");
    else
      FATAL("unknown embedding option", EXIT_FAILURE)
  }

  //activation

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "relu6") == 0)
    network->NS_activation = NETWORK_ACTIVATION_RELU6;
  else if (compat_strcasecmp(svalue, "tanh") == 0)
    network->NS_activation = NETWORK_ACTIVATION_TANH;
  else if (compat_strcasecmp(svalue, "rsqrt") == 0)
    network->NS_activation = NETWORK_ACTIVATION_RSQRT;
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network->NS_activation == NETWORK_ACTIVATION_RELU6)
      PRINTF("activation=RELU6\n");
    else if (network->NS_activation == NETWORK_ACTIVATION_TANH)
      PRINTF("activation=TANH\n");
    else if (network->NS_activation == NETWORK_ACTIVATION_RSQRT)
      PRINTF("activation=RSQRT\n");
    else
      FATAL("unknown activation option", EXIT_FAILURE)
  }

  //activation_last

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_LAST_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "linear") == 0)
    network->NS_activation_last = NETWORK_ACTIVATION_LINEAR;
  else if (compat_strcasecmp(svalue, "sigmoid") == 0)
    network->NS_activation_last = NETWORK_ACTIVATION_SIGMOID;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network->NS_activation_last == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_last=LINEAR\n");
    else if (network->NS_activation_last == NETWORK_ACTIVATION_SIGMOID)
      PRINTF("activation_last=SIGMOID\n");
    else
      FATAL("unknown activation_last option", EXIT_FAILURE)
  }

  //nman_min

  network->NS_nman_min = 0;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MIN_ID);

  if (cJSON_IsNumber(value_cjson))   
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    network->NS_nman_min = ivalue;
  
    if (arg_verbose)
    {
      PRINTF("NS_nman_min=%d\n", network->NS_nman_min);
    }
  }

  //nman_max

  network->NS_nman_max = 40;

  value_cjson = cJSON_GetObjectItem(directory, CJSON_NMAN_MAX_ID);

  if (cJSON_IsNumber(value_cjson))   
  {
    int ivalue = cJSON_GetNumberValue(value_cjson);

    HARDBUG((ivalue < 0) or (ivalue > 40))

    network->NS_nman_max = ivalue;
  
    if (arg_verbose)
    {
      PRINTF("NS_nman_max=%d\n", network->NS_nman_max);
    }
  }

  HARDBUG(network->NS_nman_min > network->NS_nman_max)

  construct_patterns_shared(&(patterns_shared), bdata(network->NS_bshape));

PRINTF("load_network=%d\n", load_network);
  if (!load_network) return;

  network->NS_ninputs_patterns = 0;
  network->NS_ninputs_material = 0;
  network->NS_ninputs = 0;

  network->NS_nlayers = 0;
  network->NS_loaded = FALSE;

  network->NS_loaded = TRUE;

  //load the embedding vectors

  patterns_shared_t *with_patterns_shared = &patterns_shared;

  int iweight = 0;

  for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with = with_patterns_shared->PS_patterns + ipattern;

    BSTRING(bweights)

    bformata(bweights, "weights%d.csv", iweight);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    read_2d_csv_base64(file, &(with->PS_nstates), &(with->PS_nembed),
      &(with->PS_weights_nstatesxnembed), NULL);

    mark_pointer_read_only(with->PS_weights_nstatesxnembed[0]);

    if ((network->NS_embedding == NETWORK_EMBEDDING_SUM) or
        (network->NS_embedding == NETWORK_EMBEDDING_SUM2))
    {
      with->PS_offset = 0;

      if (network->NS_ninputs_patterns == 0)
      {
        network->NS_ninputs_patterns = with->PS_nembed; //first
 
        network->NS_ninputs = with->PS_nembed;
      }
      else
        HARDBUG(with->PS_nembed != network->NS_ninputs_patterns)
    }
    else
    {
      with->PS_offset = network->NS_ninputs;

      network->NS_ninputs += with->PS_nembed;
    }

    PRINTF("ipattern=%d with->PS_nstates=%d with->PS_nembed=%d"
           " with->PS_offset=%d\n",
           ipattern, with->PS_nstates, with->PS_nembed, with->PS_offset);

    HARDBUG(network->NS_ninputs >= NINPUTS_MAX)

    iweight++;
  }

  //load the material

  for (int imaterial = 0; imaterial < 4; imaterial++)
  {
    material_shared_t *with = &(network->NS_material[imaterial]);

    BSTRING(bweights)

    bformata(bweights, "weights%d.csv", iweight);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    HARDBUG(file == NULL)

    with->MS_nstates = 21;
    with->MS_nembed = INVALID;

    read_2d_csv_base64(file, &(with->MS_nstates), &(with->MS_nembed),
      &(with->MS_weights_nstatesxnembed),
      &(with->MS_weights_nembedxnstates));

    mark_pointer_read_only(with->MS_weights_nstatesxnembed[0]);
    mark_pointer_read_only(with->MS_weights_nembedxnstates[0]);

    if (network->NS_embedding == NETWORK_EMBEDDING_SUM)
    {
      with->MS_offset = 0;

      if (network->NS_ninputs_material == 0)
      {
        network->NS_ninputs_material = with->MS_nembed; //first

        HARDBUG(network->NS_ninputs_material != network->NS_ninputs_patterns)
      }
      else
        HARDBUG(with->MS_nembed != network->NS_ninputs_material)
    }
    else if (network->NS_embedding == NETWORK_EMBEDDING_SUM2)
    {
      with->MS_offset = network->NS_ninputs_patterns;

      if (network->NS_ninputs_material == 0)
      {
        network->NS_ninputs_material = with->MS_nembed; //first

        network->NS_ninputs += with->MS_nembed;
      }
      else
        HARDBUG(with->MS_nembed != network->NS_ninputs_material)
    }
    else 
    {
      with->MS_offset = network->NS_ninputs;

      network->NS_ninputs += with->MS_nembed;
    }

    PRINTF("with->MS_nstates=%d with->MS_nembed=%d with->MS_offset=%d\n",
           with->MS_nstates, with->MS_nembed, with->MS_offset);

    HARDBUG(network->NS_ninputs >= NINPUTS_MAX)

    iweight++;
  }

  PRINTF("network->NS_ninputs_patterns=%d\n", network->NS_ninputs_patterns);
  PRINTF("network->NS_ninputs_material=%d\n", network->NS_ninputs_material);
  PRINTF("network->NS_ninputs=%d\n", network->NS_ninputs);

  //read the dense network

  while(TRUE)
  {
    BSTRING(bweights)

    bformata(bweights, "weights%d.csv", iweight);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    if (file == NULL) break;

    HARDBUG(network->NS_nlayers >= NLAYERS_MAX)

    layer_shared_t *with = network->NS_layers + network->NS_nlayers;

    with->LS_ninputs = with->LS_noutputs = INVALID;

    read_2d_csv_base64(file, &(with->LS_ninputs), &(with->LS_noutputs),
      &(with->LS_weights_ninputsxnoutputs),
      &(with->LS_weights_noutputsxninputs));

    mark_pointer_read_only(with->LS_weights_ninputsxnoutputs[0]);
    mark_pointer_read_only(with->LS_weights_noutputsxninputs[0]);

    BSTRING(bbias)

    bformata(bbias, "bias%d.csv", iweight);

    file = open_cjson_file(directory, bdata(bbias));

    HARDBUG(file == NULL)

    BDESTROY(bbias)

    read_1d_csv_base64(file, &(with->LS_noutputs), &(with->LS_bias));

    mark_pointer_read_only(with->LS_bias);

    network->NS_nlayers++;
   
    iweight++;
  }

  if (arg_verbose) PRINTF("nlayers=%d\n", network->NS_nlayers);

  HARDBUG(network->NS_nlayers < 2)

  if (arg_verbose)
  {
    for (int ilayer = 0; ilayer < network->NS_nlayers; ilayer++)
    {
      layer_shared_t *with = network->NS_layers + ilayer;

      PRINTF("ilayer=%d (inputs, outputs)=(%d, %d)\n",
        ilayer, with->LS_ninputs, with->LS_noutputs);
    }
  }
 
  //check topology

  for (int ilayer = 0; ilayer < network->NS_nlayers - 1; ilayer++)
  {
    layer_shared_t *with_current = network->NS_layers + ilayer;
    layer_shared_t *with_next = network->NS_layers + ilayer + 1;
   
    HARDBUG(with_current->LS_noutputs != with_next->LS_ninputs)
  }

  if ((network->NS_embedding == NETWORK_EMBEDDING_SUM) or
      (network->NS_embedding == NETWORK_EMBEDDING_SUM2))
  {
    for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_patterns_shared->PS_patterns + ipattern;

      with_pattern_shared->PS_sum_nstatesxnoutputs = NULL;
    }

    for (int imaterial = 0; imaterial < 4; imaterial++)
    {
      material_shared_t *with_material_shared =
        &(network->NS_material[imaterial]);
  
      with_material_shared->MS_sum = NULL;
    }
  }
  else
  {
    //precompute layer0 contribution

    layer_shared_t *with_current = network->NS_layers;
  
    for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_patterns_shared->PS_patterns + ipattern;
  
      scaled_double_t *sum_nstatesxnoutputs;
  
      MY_MALLOC_BY_TYPE(sum_nstatesxnoutputs, scaled_double_t,
                        with_pattern_shared->PS_nstates *
                        with_current->LS_noutputs);
  
      MY_MALLOC_BY_TYPE(with_pattern_shared->PS_sum_nstatesxnoutputs,
                        scaled_double_t *, with_pattern_shared->PS_nstates);
  
      for (int istate = 0; istate < with_pattern_shared->PS_nstates; istate++)
      {
        with_pattern_shared->PS_sum_nstatesxnoutputs[istate] =
          sum_nstatesxnoutputs + istate * with_current->LS_noutputs;
  
        for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
        {
          with_pattern_shared->PS_sum_nstatesxnoutputs[istate][ioutput] = 
            dot_scaled(with_pattern_shared->PS_nembed,
                       with_pattern_shared->PS_weights_nstatesxnembed[istate],
                       with_current->LS_weights_noutputsxninputs[ioutput] +
                       with_pattern_shared->PS_offset);
        }
      }
  
      mark_pointer_read_only(sum_nstatesxnoutputs);
    }

    for (int imaterial = 0; imaterial < 4; imaterial++)
    {
      material_shared_t *with_material_shared =
        &(network->NS_material[imaterial]);
  
      scaled_double_t *sum;
  
      MY_MALLOC_BY_TYPE(sum, scaled_double_t,
                        with_material_shared->MS_nstates *
                        with_current->LS_noutputs);
  
      MY_MALLOC_BY_TYPE(with_material_shared->MS_sum, scaled_double_t *,
                        with_material_shared->MS_nstates);
  
      for (int istate = 0; istate < with_material_shared->MS_nstates; istate++)
      {
        with_material_shared->MS_sum[istate] =
          sum + istate * with_current->LS_noutputs;
  
        for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
        {
          with_material_shared->MS_sum[istate][ioutput] = 
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

void construct_network_thread(network_thread_t *self, int arg_verbose)
{
  network_thread_t *object = self;

  MY_MALLOC_BY_TYPE(object->NT_inputs, scaled_double_t,
                    network_shared.NS_ninputs);

  for (int ilayer = 0; ilayer < network_shared.NS_nlayers; ilayer++)
  {
    layer_shared_t *with = network_shared.NS_layers + ilayer;
    layer_thread_t *with_thread = object->NT_layers + ilayer;

    MY_MALLOC_BY_TYPE(with_thread->LT_dot, scaled_double_t, with->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_sum, scaled_double_t, with->LS_noutputs);
    MY_MALLOC_BY_TYPE(with_thread->LT_outputs, scaled_double_t,
                      with->LS_noutputs);
  }
}

local scaled_double_t dot_double(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  double s = 0;

  for (int i = 0; i < n; i++)
    s += SCALED2DOUBLE(a[i]) * SCALED2DOUBLE(b[i]);

  return(DOUBLE2SCALED(s));
}

void vcopy_ab(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
    for (int i = 0; i < n; i++) b[i] = a[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n ; a < z; a += 8, b += 8)
    {
      __m256 va = _mm256_loadu_ps(a);
  
      _mm256_store_ps(b, va);
    }
#else
    for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *)a);
  
      _mm256_store_si256((__m256i *)b, va);
    }
#endif
  }
#endif
}

void vadd_cab(int n, scaled_double_t *restrict c,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
    for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8, c += 8)
    {
      __m256 va = _mm256_load_ps(a);
      __m256 vb = _mm256_load_ps(b);
  
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

void vadd_aba(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
    for (int i = 0; i < n; i++) a[i] += b[i];
  }

#ifdef USE_AVX2_INTRINSICS
  else
  {
    SOFTBUG((n % 8) != 0)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (const scaled_double_t *z = a + n ; a < z; a += 8, b += 8)
    {
      const __m256 va = _mm256_load_ps(a);
      const __m256 vb = _mm256_load_ps(b);
  
      const __m256 vs = _mm256_add_ps(va, vb);
  
      _mm256_store_ps(a, vs);
    }
#else
    for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
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

local void activation_relu_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
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
      __m256 va = _mm256_load_ps(a);
  
      va = _mm256_max_ps(va, vz);
  
      _mm256_store_ps(b, va);
    }
#else
    __m256i zero = _mm256_setzero_si256();

    for (const int32_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256i va = _mm256_load_si256((__m256i *) a);

      va = _mm256_max_epi32(va, zero);

      _mm256_store_si256((__m256i *) b, va);
    }
#endif
  }
#endif
}

local void activation_relu_double(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
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
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  //for (int i = 0; i < n; i++) HARDBUG(a[i] > CLIP_VALUE)

#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    for (int i = 0; i < n; i++)
    {
      if (a[i] <= 0)
        b[i] = 0;
      else if (a[i] >= (scaled_double_t) CLIP_VALUE)
        b[i] = (scaled_double_t) CLIP_VALUE;
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
  
    const __m256 vc = _mm256_set1_ps((scaled_double_t) CLIP_VALUE);
  
    for (const scaled_double_t *z = a + n; a < z; a += 8, b += 8)
    {
      __m256 ta = _mm256_load_ps(a);
  
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
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
  {
#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= CLIP_VALUE)
      b[i] = (scaled_double_t) CLIP_VALUE;
    else
      b[i] = a[i];
#else
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= (SCALED_DOUBLE_FACTOR * CLIP_VALUE))
      b[i] = (scaled_double_t) (SCALED_DOUBLE_FACTOR * CLIP_VALUE);
    else
      b[i] = a[i];
#endif
  }
}

local void activation_tanh_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
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
      __m256 va = _mm256_load_ps(a);

      __m256 va2p = _mm256_add_ps(p, _mm256_mul_ps(va, va));

      __m256 rsqrt = _mm256_rsqrt_ps(va2p);

      //needed for Newton-Raphson

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      rsqrt = _mm256_mul_ps(rsqrt,
                _mm256_sub_ps(three_halfs,
                  _mm256_mul_ps(half, _mm256_mul_ps(va2p, rsqrt2))));

      //needed for correction

      rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      __m256 vq = _mm256_add_ps(one, _mm256_mul_ps(q, rsqrt2));

      _mm256_store_ps(b, _mm256_mul_ps(va,
                           _mm256_mul_ps(rsqrt, vq)));
    }
#else
    for (int i = 0; i < n; i++)
      b[i] = FLOAT2SCALED(tanhf(SCALED2FLOAT(a[i])));
#endif
  }
#endif
}

local void activation_tanh_double(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
    b[i] = DOUBLE2SCALED(tanh(SCALED2DOUBLE(a[i])));
}

local void activation_rsqrt_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS
  if (n < 8)
#endif
  {
    for (int i = 0; i < n; i++)
      b[i] = a[i]/sqrtf(1.0f + a[i] * a[i]);
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
      __m256 va = _mm256_load_ps(a);

      __m256 va2p1 = _mm256_add_ps(one, _mm256_mul_ps(va, va));

      __m256 rsqrt = _mm256_rsqrt_ps(va2p1);

      __m256 rsqrt2 = _mm256_mul_ps(rsqrt, rsqrt);

      rsqrt = _mm256_mul_ps(rsqrt,
                _mm256_sub_ps(three_halfs,
                  _mm256_mul_ps(half, _mm256_mul_ps(va2p1, rsqrt2))));

      _mm256_store_ps(b, _mm256_mul_ps(va, rsqrt));
    }
#else
    for (int i = 0; i < n; i++)
    {
      float af = SCALED2FLOAT(a[i]);
      b[i] = af/sqrtf(1.0f + af * af);
    }
#endif
  }
#endif
}

local void activation_rsqrt_double(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  for (int i = 0; i < n; i++)
    b[i] = DOUBLE2SCALED(a[i])/
                         sqrt(1.0 + SCALED2DOUBLE(a[i]) * SCALED2DOUBLE(a[i]));
}

local void activation_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  if (network_shared.NS_activation == NETWORK_ACTIVATION_RELU6)
  {
    activation_clipped_relu_scaled(n, a, b);
  }
  else if (network_shared.NS_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_scaled(n, a, b);
  }
  else if (network_shared.NS_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_scaled(n, a, b);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local void activation_double(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  if (network_shared.NS_activation == NETWORK_ACTIVATION_RELU6)
  {
    activation_clipped_relu_double(n, a, b);
  }
  else if (network_shared.NS_activation == NETWORK_ACTIVATION_TANH)
  {
    activation_tanh_double(n, a, b);
  }
  else if (network_shared.NS_activation == NETWORK_ACTIVATION_RSQRT)
  {
    activation_rsqrt_double(n, a, b);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)
}

local double return_sigmoid(double x)
{
  return(2.0 / (1.0 + exp(-x)) - 1.0);
}

int base3_index(const int32_t *embed, int nlinear)
{
  int result = 0;
  int power = 1;

  for (int ilinear = 0; ilinear < nlinear; ilinear++)
  {
    result += embed[ilinear] * power;

    power *= 3;
  }
  return(result);
}

//assumes inputs have been set

double return_network_score_scaled(network_thread_t *arg_network)
{
  BEGIN_BLOCK(__func__)

  layer_shared_t *with_current = network_shared.NS_layers;
  layer_thread_t *with_current_thread = arg_network->NT_layers;

  if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
      (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
  {
    ALIGN64(scaled_double_t activated_inputs[NINPUTS_MAX]);

    activation_scaled(network_shared.NS_ninputs,
                      arg_network->NT_inputs, activated_inputs);

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] =
        dot_scaled(with_current->LS_ninputs,
                   activated_inputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }
  }

  //first layer

  BEGIN_BLOCK("Ax+b-and-activation-first-layer")

  vadd_cab(with_current->LS_noutputs, with_current_thread->LT_sum,
           with_current_thread->LT_dot, with_current->LS_bias);
  
  BEGIN_BLOCK("Ax+b-and-activation-first-layer-activation")

  activation_scaled(with_current->LS_noutputs,
    with_current_thread->LT_sum, with_current_thread->LT_outputs);

  END_BLOCK

  END_BLOCK

  for (int ilayer = 1; ilayer < network_shared.NS_nlayers; ilayer++)
  {
    with_current = network_shared.NS_layers + ilayer;
    with_current_thread = arg_network->NT_layers + ilayer;

    layer_thread_t *with_previous_thread = arg_network->NT_layers + ilayer - 1;

    BEGIN_BLOCK("Ax+b-and-activation-other-layers")

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_dot[i] = 
        dot_scaled(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs[i]);
    }

    vadd_cab(with_current->LS_noutputs, with_current_thread->LT_sum,
             with_current_thread->LT_dot, with_current->LS_bias);

    if (ilayer < (network_shared.NS_nlayers - 1))
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-activation")

      activation_scaled(with_current->LS_noutputs,
        with_current_thread->LT_sum, with_current_thread->LT_outputs);

      END_BLOCK
    }
    else
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-last-layer")

      for (int i = 0; i < with_current->LS_noutputs; i++)
      {
        if (network_shared.NS_activation_last == NETWORK_ACTIVATION_LINEAR)
        {
          with_current_thread->LT_outputs[i] =
            with_current_thread->LT_sum[i];
        }
        else if (network_shared.NS_activation_last ==
                 NETWORK_ACTIVATION_SIGMOID)
        {
          with_current_thread->LT_outputs[i] =
            return_sigmoid(with_current_thread->LT_sum[i]);
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }

      END_BLOCK
    }

    END_BLOCK
  }
 
  END_BLOCK

  layer_thread_t *with_thread =
    arg_network->NT_layers + network_shared.NS_nlayers - 1;

  return(SCALED2DOUBLE(with_thread->LT_outputs[0]));
}

double return_network_score_double(network_thread_t *arg_network)
{
  layer_shared_t *with_current = network_shared.NS_layers;
  layer_thread_t *with_current_thread = arg_network->NT_layers;

  if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
      (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
  {
    scaled_double_t activated_inputs[NINPUTS_MAX];

    activation_double(network_shared.NS_ninputs,
                      arg_network->NT_inputs, activated_inputs);

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_sum[i] =
        dot_double(with_current->LS_ninputs,
                   activated_inputs,
                   with_current->LS_weights_noutputsxninputs[i]) +
        with_current->LS_bias[i];
    }
  }
  else
  {
    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_sum[i] =
        dot_double(with_current->LS_ninputs,
                   arg_network->NT_inputs,
                   with_current->LS_weights_noutputsxninputs[i]) +
        with_current->LS_bias[i];
    }
  }

  activation_double(with_current->LS_noutputs,
    with_current_thread->LT_sum, with_current_thread->LT_outputs);

  for (int ilayer = 1; ilayer < network_shared.NS_nlayers; ilayer++)
  {
    with_current = network_shared.NS_layers + ilayer;

    with_current_thread = arg_network->NT_layers + ilayer;

    layer_thread_t *with_previous_thread = arg_network->NT_layers + ilayer - 1;

    for (int i = 0; i < with_current->LS_noutputs; i++)
    {
      with_current_thread->LT_sum[i] =
        dot_double(with_current->LS_ninputs,
                   with_previous_thread->LT_outputs,
                   with_current->LS_weights_noutputsxninputs[i]) +
        with_current->LS_bias[i];
    }

    if (ilayer < (network_shared.NS_nlayers - 1))
    {
      activation_double(with_current->LS_noutputs,
        with_current_thread->LT_sum, with_current_thread->LT_outputs);
    }
    else
    {
      for (int i = 0; i < with_current->LS_noutputs; i++)
      {
        if (network_shared.NS_activation_last == NETWORK_ACTIVATION_LINEAR)
        {
          with_current_thread->LT_outputs[i] =
            with_current_thread->LT_sum[i];
        }
        else if (network_shared.NS_activation_last == NETWORK_ACTIVATION_SIGMOID)
        {
          with_current_thread->LT_outputs[i] =
            return_sigmoid(with_current_thread->LT_sum[i]);
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }
    }
  }

  layer_thread_t *with_thread =
    arg_network->NT_layers + network_shared.NS_nlayers - 1;

  return(SCALED2DOUBLE(with_thread->LT_outputs[0]));
}

//assumes board2pattern has been called

void board2network(board_t *object)
{
  check_board_patterns_thread(object, (char *) __FUNC__, TRUE);

  network_thread_t *network_thread = &(object->B_network_thread);

  patterns_thread_t *with_patterns_thread = &(network_thread->NT_patterns);

  for (int input = 0; input < network_shared.NS_ninputs; input++)
    network_thread->NT_inputs[input] = 0;

  layer_shared_t *with_current = network_shared.NS_layers;
  layer_thread_t *with_current_thread = network_thread->NT_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

  for (int ipattern = 0; ipattern < patterns_shared.PS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      patterns_shared.PS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    int istate = base3_index(with_pattern_thread->PT_embed,
                             with_pattern_shared->PS_nlinear);

    if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
        (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
    {
      vadd_aba(with_pattern_shared->PS_nembed,
               network_thread->NT_inputs + with_pattern_shared->PS_offset,
               with_pattern_shared->PS_weights_nstatesxnembed[istate]);
    }
    else if (network_shared.NS_embedding == NETWORK_EMBEDDING_CONCAT) 
    {
      vcopy_ab(with_pattern_shared->PS_nembed,
               with_pattern_shared->PS_weights_nstatesxnembed[istate],
               network_thread->NT_inputs + with_pattern_shared->PS_offset);

      vadd_aba(with_current->LS_noutputs, with_current_thread->LT_dot,
               with_pattern_shared->PS_sum_nstatesxnoutputs[istate]);
    }
    else  
      FATAL("unknown embedding", EXIT_FAILURE)

  }

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  for (int imaterial = 0; imaterial < 4; imaterial++)
  {
    int istate;

    if (imaterial == 0)
      istate = nwhite_man;
    else if (imaterial == 1)
      istate = nblack_man;
    else if (imaterial == 2)
      istate = nwhite_king;
    else
      istate = nblack_king;

    material_shared_t *with_material_shared =
      &(network_shared.NS_material[imaterial]);

    if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
        (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
    {
      vadd_aba(with_material_shared->MS_nembed,
               network_thread->NT_inputs + with_material_shared->MS_offset,
               with_material_shared->MS_weights_nstatesxnembed[istate]);
    }
    else if (network_shared.NS_embedding == NETWORK_EMBEDDING_CONCAT)
    {
      vcopy_ab(with_material_shared->MS_nembed,
               with_material_shared->MS_weights_nstatesxnembed[istate],
               network_thread->NT_inputs + with_material_shared->MS_offset);

      vadd_aba(with_current->LS_noutputs, with_current_thread->LT_dot,
               with_material_shared->MS_sum[istate]);
    }
    else
      FATAL("unknown embedding", EXIT_FAILURE)
  }
}

local void heap_sort_double(i64_t n, i64_t *p, double *a)
{
  i64_t i, j, k, t;

  if (n < 2) return;

  for (i = 0; i < n; i++) p[i] = i;

  k = n / 2;
  --n;

  while(TRUE)
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
    while(j <= n)
    {
      if ((j < n) and (a[p[j]] < a[p[j + 1]])) j++;
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

  for (i = 0; i < n; i++) HARDBUG(a[p[i]] > a[p[i + 1]])
}

local void update_mean_sigma(long long n, double x,
  double *xmax, double *mn, double *sn)
{
  double mnm1 = *mn;
  double snm1 = *sn;

  if (x > *xmax) *xmax = x;

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

    if ((arg_npositions > 0) and (nfen >= arg_npositions)) break;
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

    if (bchar(bline, 0) == '#') continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((ifen % my_mpi_globals.MMG_nslaves) !=
         my_mpi_globals.MMG_id_slave))
    {
      continue;
    }

    update_progress(&progress);

    nfen_mpi++;

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with, bdata(bfen));

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);
    int nblack_man = BIT_COUNT(with->B_black_man_bb);
    int nwhite_king = BIT_COUNT(with->B_white_king_bb);
    int nblack_king = BIT_COUNT(with->B_black_king_bb);

    int npieces = nwhite_man + nblack_man + nwhite_king + nblack_king;

    npieces_fen[ifen] = npieces;

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    //HARDBUG(fabs(result) >= 0.999999)

    //check_moves(with, &moves_list);

    int mat_score = return_material_score(with);

    score_scaled[ifen] =
      return_network_score_scaled(&(with->B_network_thread));

    score_double[ifen] =
      return_network_score_double(&(with->B_network_thread));

    if (IS_BLACK(with->B_colour2move)) mat_score = -mat_score;

    if (ifen <= 100)
      PRINTF("%s {%.6f} ms=%d ss=%.6f sd=%.6f r=%.6f\n",
        bdata(bfen), result, mat_score,
        score_scaled[ifen], score_double[ifen], result);

    delta_scaled[ifen] = fabs(score_scaled[ifen] - result);
    delta_double[ifen] = fabs(score_double[ifen] - result);

    mse_vs_draw_scaled +=
      score_scaled[ifen] * score_scaled[ifen];

    mse_vs_draw_double +=
      score_double[ifen] * score_double[ifen];

    mse_scaled += delta_scaled[ifen] * delta_scaled[ifen];

    mse_double += delta_double[ifen] * delta_double[ifen];

    double error = fabs(score_scaled[ifen] - score_double[ifen]);

    if (error > error_max_scaled_vs_double)
    {
      error_max_scaled_vs_double = error;

      PRINTF("%s {%.6f} ms=%d nss=%.6f nsd=%.6f r=%.6f\n",
        bdata(bfen), result, mat_score,
        score_scaled[ifen], score_double[ifen], result);

      PRINTF("error_max_scaled_vs_double=%.6f\n", error_max_scaled_vs_double);
    }

    if ((moves_list.ML_ncaptx == 0) and
        (with->B_white_king_bb == 0) and
        (with->B_black_king_bb == 0) and
        (result > 0.0) and
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
      int network2mat_score =
        round(score_scaled[ifen] * factor / nfactor);

      if (abs(network2mat_score - mat_score) > SCORE_MAN)
      {  
        if (nnetwork2mat_score_gt_score_man <= 100)
          PRINTF("network2mat_score=%d mat_score=%d\n",
                 network2mat_score, mat_score);
        nnetwork2mat_score_gt_score_man++;
      }
    }
  }

  finalize_progress(&progress);

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen_mpi=%lld\n", nfen_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  my_mpi_allreduce(MPI_IN_PLACE, npieces_fen, nfen,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, score_scaled, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, score_double, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, delta_scaled, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, delta_double, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &mse_vs_draw_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_vs_draw_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nnetwork2mat_score_gt_score_man, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &error_max_scaled_vs_double, 1,
    MPI_DOUBLE, MPI_MAX, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfactor, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &factor, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MMG_comm_slaves);

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
      if (npieces_fen[ifen] != ipiece) continue;

      ndelta++;

      update_mean_sigma(ndelta, delta_scaled[ifen],
        &delta_scaled_max, &delta_scaled_mean, &delta_scaled_sigma);

      update_mean_sigma(ndelta, delta_double[ifen],
        &delta_double_max, &delta_double_mean, &delta_double_sigma);
    }
    PRINTF("ipiece=%d nd=%lld ds_max=%.6f ds_mean=%.6f ds_sigma=%.6f"
           " dd_max=%.6f dd_mean=%.6f dd_sigma=%.6f\n",
     ipiece, ndelta,
     delta_scaled_max, delta_scaled_mean, sqrt(delta_scaled_sigma / ndelta),
     delta_double_max, delta_double_mean, sqrt(delta_double_sigma / ndelta));
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
      ipercentile, delta_scaled[pdelta_scaled[ifen]],
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

    if (bchar(bline, 0) == '#') continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((nfen % my_mpi_globals.MMG_nslaves) !=
         my_mpi_globals.MMG_id_slave))
    {
      continue;
    }

    nfen_mpi++;

#ifdef DEBUG
    if ((nfen_mpi % 1000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
    if (nfen_mpi == 10000) break;
#else
    if ((nfen_mpi % 100000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
#endif

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(sscanf(bdata(bline), "%s {%lf", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    fen2board(with, bdata(bfen));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.ML_nmoves <= 1) or (moves_list.ML_ncaptx > 0))
    {
      continue;
    }

    //HARDBUG(fabs(result) >= 0.999999)

    //check_moves(with, &moves_list);

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

  PRINTF("nfen_mpi=%lld nfen_skipped_mpi=%lld\n",
    nfen_mpi, nfen_skipped_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_skipped_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen_skipped=%lld\n", nfen_skipped_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfactor, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &factor, 1, MPI_DOUBLE,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);
  
 factor /= nfactor;

 double factor2man_score = SCORE_MAN / factor;

 PRINTF("nfactor=%lld factor=%.6f factor2man_score=%.6f\n",
   nfactor, factor, factor2man_score);
}

#define TAG "w2m-embed-patterns-wmbmwkbk"

local void append_fen(board_t *self, 
  bstring arg_bfen, double arg_result,
  fbuffer_t *arg_fcsv, fbuffer_t *arg_ffen)
{
  board_t *object = self;

  patterns_shared_t *with_patterns_shared = &patterns_shared;
  patterns_thread_t *with_patterns_thread =
     &(self->B_network_thread.NT_patterns);

  for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared = with_patterns_shared->PS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    int input = base3_index(with_pattern_thread->PT_embed,
                            with_pattern_shared->PS_nlinear);

    bin_t ibin = input;

    append_fbuffer_bin(arg_fcsv, &ibin, sizeof(bin_t));
  }

  //always W2M

  if (IS_BLACK(object->B_colour2move)) arg_result = -arg_result;

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

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

//PRINTF("material=%d\n", input);

  float result = arg_result;

  append_fbuffer_bin(arg_fcsv, &result, sizeof(float));

  append_fbuffer_bin(arg_fcsv, NULL, 0);

  if (arg_ffen != NULL)
    append_fbuffer_fmt(arg_ffen, "%s {%.6f}\n", bdata(arg_bfen), arg_result);
}

void fen2csv(char *arg_name, int arg_nman_min, int arg_nman_max,
  int arg_nkings_min, int arg_nkings_max)
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

  HARDBUG(bformata(btag, "%s-nmanmin%d-nmanmax%d"
                         "-nkingsmin%d-nkingsmax%d-%s",
                         TAG, arg_nman_min, arg_nman_max,
                         arg_nkings_min, arg_nkings_max,
                         bdata(network_shared.NS_bshape)) == BSTR_ERR)

  //training

  BSTRING(bsuffix)

  HARDBUG(bformata(bsuffix, "-%s-training.bin", bdata(btag)) == BSTR_ERR)

  fbuffer_t ftraining;

  construct_fbuffer(&ftraining, barg_name, bdata(bsuffix), TRUE);

  //validation

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s-validation.bin", bdata(btag)) == BSTR_ERR)

  fbuffer_t fvalidation;

  construct_fbuffer(&fvalidation, barg_name, bdata(bsuffix), TRUE);

  //fen

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s.fen", bdata(btag)) == BSTR_ERR)

  fbuffer_t ffen;

  construct_fbuffer(&ffen, barg_name, bdata(bsuffix), TRUE);

  //done

  PRINTF("training_name=%s\n", bdata(return_fbuffer_bname(&ftraining)));
  PRINTF("validation_name=%s\n", bdata(return_fbuffer_bname(&fvalidation)));
  PRINTF("fen_name=%s\n", bdata(return_fbuffer_bname(&ffen)));

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  i64_t nfen = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK) nfen++;

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen=%lld\n", nfen);

  i64_t nfen_mpi = 0;
  i64_t nfen_nman_mpi = 0;
  i64_t nfen_nkings_mpi = 0;
  i64_t nfen_ncapt_mpi = 0;
  i64_t nfen_append_mpi = 0;

  bucket_t bucket_material_score;

  construct_bucket(&bucket_material_score, "bucket_material_score",
                   SCORE_MAN, SCORE_LOST, SCORE_WON, BUCKET_LINEAR);

  bucket_t bucket_nwhite_man;

  construct_bucket(&bucket_nwhite_man, "bucket_nwhite_man",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  bucket_t bucket_nblack_man;

  construct_bucket(&bucket_nblack_man, "bucket_nblack_man",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  bucket_t bucket_nwhite_kings;

  construct_bucket(&bucket_nwhite_kings, "bucket_nwhite_kings",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  bucket_t bucket_nblack_kings;

  construct_bucket(&bucket_nblack_kings, "bucket_nblack_kings",
                   1, 0, NPIECES_MAX, BUCKET_LINEAR);

  bucket_t bucket_nman;

  construct_bucket(&bucket_nman, "bucket_nman",
                   1, 0, 2 * NPIECES_MAX, BUCKET_LINEAR);

  bucket_t bucket_nkings;

  construct_bucket(&bucket_nkings, "bucket_nkings",
                   1, 0, 2 * NPIECES_MAX, BUCKET_LINEAR);

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

    if (bchar(bline, 0) == '#') continue;

    if ((my_mpi_globals.MMG_nslaves > 0) and
        ((ifen % my_mpi_globals.MMG_nslaves) !=
         my_mpi_globals.MMG_id_slave)) continue;

    update_progress(&progress);

    nfen_mpi++;

    double result;

    CSTRING(cfen, blength(bline))

    //ARES convention

    HARDBUG(sscanf(bdata(bline), "%s {%lf}", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(with, bdata(bfen));

    if (IS_BLACK(with->B_colour2move)) result = -result;

    //END ARES CONVENTION

    CDESTROY(cfen)

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);

    int nblack_man = BIT_COUNT(with->B_black_man_bb);

    int nman = nwhite_man + nblack_man;

    if ((nman < arg_nman_min) or (nman > arg_nman_max))
    {   
      nfen_nman_mpi++;

      continue;
    }

    int nwhite_kings = BIT_COUNT(with->B_white_king_bb);

    int nblack_kings = BIT_COUNT(with->B_black_king_bb);

    int nkings = nwhite_kings + nblack_kings;

    if ((nkings < arg_nkings_min) or (nkings > arg_nkings_max))
    {   
      nfen_nkings_mpi++;

      continue;
    }

    moves_list_t moves_list;

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.ML_nmoves <= 1) or
        (moves_list.ML_ncaptx > 0))
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
      int nman = nwhite_man + nblack_man;

      double result_wtm = result;

      if (IS_BLACK(with->B_colour2move)) result_wtm = -result_wtm;

      append_fbuffer_fmt(&fdata, "%d,%d,%.6f\n",
        delta_nman, delta_nkings, result_wtm);
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

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_nman_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_nkings_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_ncapt_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_append_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MMG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen=%lld\n", nfen);
  PRINTF("nfen_mpi=%lld\n", nfen_mpi);
  PRINTF("nfen_nman_mpi=%lld\n", nfen_nman_mpi);
  PRINTF("nfen_nkings_mpi=%lld\n", nfen_nkings_mpi);
  PRINTF("nfen_ncapt_mpi=%lld\n", nfen_ncapt_mpi);
  PRINTF("nfen_append_mpi=%lld\n", nfen_append_mpi);

  my_mpi_bucket_aggregate(&bucket_material_score,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_man,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_man,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_kings,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nman,
    my_mpi_globals.MMG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nkings,
    my_mpi_globals.MMG_comm_slaves);

  printf_bucket(&bucket_material_score);

  printf_bucket(&bucket_nwhite_man);

  printf_bucket(&bucket_nblack_man);

  printf_bucket(&bucket_nwhite_kings);

  printf_bucket(&bucket_nblack_kings);

  printf_bucket(&bucket_nman);

  printf_bucket(&bucket_nkings);

  //write remaining

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

  construct_network_shared(&network_shared, TRUE);
}

