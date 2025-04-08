//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

local int ares = INVALID;

local void add2_64(int n, scaled_double_t *s,
  i64_t *a, i64_t *b)
{
  for (int i = 0; i < n; i++) s[i] = (a[i] + b[i]) / SCALED_DOUBLE_FACTOR;
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

local void read_1d_csv(cJSON *arg_csv, int *arg_nrows,
  scaled_double_t **arg_cells, double **arg_cells_double)
{  
  *arg_cells = NULL;
  *arg_cells_double = NULL;

  cJSON *lines = cJSON_GetObjectItem(arg_csv, CJSON_FILE_LINES_ID);

  HARDBUG(!cJSON_IsArray(lines))

  int nrows = 0;

  cJSON *line;

  cJSON_ArrayForEach(line, lines)
  {
    char *string = cJSON_GetStringValue(line);
  
    HARDBUG(string == NULL)

    double f;

    HARDBUG(sscanf(string, "%lf", &f) != 1)
       
    nrows++;
  }

  HARDBUG(nrows == 0)

  scaled_double_t *cells;
  double *cells_double;

  MY_MALLOC(cells, scaled_double_t, nrows)
  MY_MALLOC(cells_double, double, nrows)

  int irow = 0;

  cJSON_ArrayForEach(line, lines)
  {
    char *string = cJSON_GetStringValue(line);

    HARDBUG(string == NULL)

    HARDBUG(irow >= nrows)

    double f;

    HARDBUG(sscanf(string, "%lf", &f) != 1)

    cells_double[irow] = f;

    irow++;
  }

  HARDBUG(irow != nrows)

  if (*arg_nrows != INVALID)
    HARDBUG(nrows != *arg_nrows)
  else
    *arg_nrows = nrows;

  *arg_cells = cells;
  *arg_cells_double = cells_double;
}

local void read_2d_csv(cJSON *arg_csv, const int arg_by_row,
  int *arg_nrows, int *arg_ncols,
  scaled_double_t ***arg_cells, scaled_double_t ***arg_cells_transposed,
  double ***arg_cells_double, double ***arg_cells_transposed_double)
{
  *arg_cells = NULL;
  *arg_cells_transposed = NULL;

  *arg_cells_double = NULL;
  *arg_cells_transposed_double = NULL;

  cJSON *lines = cJSON_GetObjectItem(arg_csv, CJSON_FILE_LINES_ID);

  HARDBUG(!cJSON_IsArray(lines))

  int ncols = INVALID;
  int nrows = 0;

  cJSON *line;

  cJSON_ArrayForEach(line, lines)
  {
    HARDBUG(!cJSON_IsString(line))

    BSTRING(bline)

    HARDBUG(bassigncstr(bline, cJSON_GetStringValue(line)) == BSTR_ERR)

    struct bstrList *btokens;

    HARDBUG((btokens = bsplit(bline, ',')) == NULL)

    PUSH_LEAK(btokens)

    if (ncols == INVALID)
      ncols = btokens->qty;
    else
      HARDBUG(btokens->qty != ncols)

    for (int icol = 0; icol < ncols; icol++)
    {
      double f;

      HARDBUG(sscanf(bdata(btokens->entry[icol]), "%lf", &f) != 1)
    }

    nrows++;

    HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

    POP_LEAK(btokens)

    BDESTROY(bline)
  }

  HARDBUG(nrows == 0)

  scaled_double_t **cells;
  scaled_double_t **cells_transposed;

  double **cells_double;
  double **cells_transposed_double;

  if (arg_by_row)
  {
    MY_MALLOC(cells, scaled_double_t *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MY_MALLOC(cells[irow], scaled_double_t, ncols)

    MY_MALLOC(cells_transposed, scaled_double_t *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MY_MALLOC(cells_transposed[icol], scaled_double_t, nrows)

    //

    MY_MALLOC(cells_double, double *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MY_MALLOC(cells_double[irow], double, ncols)

    MY_MALLOC(cells_transposed_double, double *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MY_MALLOC(cells_transposed_double[icol], double, nrows)
  }
  else
  {
    MY_MALLOC(cells, scaled_double_t *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MY_MALLOC(cells[icol], scaled_double_t, nrows)

    MY_MALLOC(cells_transposed, scaled_double_t *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MY_MALLOC(cells_transposed[irow], scaled_double_t, ncols)

    //

    MY_MALLOC(cells_double, double *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MY_MALLOC(cells_double[icol], double, nrows)

    MY_MALLOC(cells_transposed_double, double *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MY_MALLOC(cells_transposed_double[irow], double, ncols)
  }

  //now read csv

  int irow = 0;

  cJSON_ArrayForEach(line, lines)
  {
    HARDBUG(!cJSON_IsString(line))

    BSTRING(bline)

    HARDBUG(bassigncstr(bline, cJSON_GetStringValue(line)) == BSTR_ERR)

    HARDBUG(irow >= nrows)

    struct bstrList *btokens;

    HARDBUG((btokens = bsplit(bline, ',')) == NULL)

    PUSH_LEAK(btokens)

    HARDBUG(btokens->qty != ncols)

    for (int icol = 0; icol < ncols; icol++)
    {
      double f;

      HARDBUG(sscanf(bdata(btokens->entry[icol]), "%lf", &f) != 1)

      if (arg_by_row)
      {
        cells_transposed_double[icol][irow]= cells_double[irow][icol] = f;
      }
      else
      {
        cells_transposed_double[irow][icol] = cells_double[icol][irow] = f;
      }
    }

    HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

    POP_LEAK(btokens)

    BDESTROY(bline)

    irow++;
  }

  HARDBUG(irow != nrows)

  if (*arg_nrows != INVALID)
    HARDBUG(nrows != *arg_nrows)
  else
    *arg_nrows = nrows;

  if (*arg_ncols != INVALID)
    HARDBUG(ncols != *arg_ncols)
  else
    *arg_ncols = ncols;

  *arg_cells = cells;
  *arg_cells_transposed = cells_transposed;

  *arg_cells_double = cells_double;
  *arg_cells_transposed_double = cells_transposed_double;
}

void construct_network(network_t *self, int arg_skip_load, int arg_verbose)
{
  network_t *network = self;

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

  network->N_network2material_score =
   cJSON_GetNumberValue(network2material_score_cjson);

  if (arg_verbose)
    PRINTF("N_network2material_score=%.2f\n", network->N_network2material_score);

  cJSON *value_cjson = cJSON_GetObjectItem(directory, CJSON_SHAPE_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  char *svalue = cJSON_GetStringValue(value_cjson);

  network->N_bshape = bfromcstr(svalue);

  if (arg_verbose) PRINTF("shape=%d\n", bdata(network->N_bshape));

  //king weight

  value_cjson = cJSON_GetObjectItem(directory, CJSON_KING_WEIGHT_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "ares") == 0)
    network->N__king_weight = NETWORK_KING_WEIGHT_ARES;
  else if (compat_strcasecmp(svalue, "gwd") == 0)
    network->N__king_weight = NETWORK_KING_WEIGHT_GWD;
  else
  {
    HARDBUG(sscanf(svalue, "%d", &(network->N__king_weight)) != 1)

    HARDBUG(network->N__king_weight < 1)
  }

  if (arg_verbose)
  {
    if (network->N__king_weight == NETWORK_KING_WEIGHT_ARES)
      PRINTF("king_weight=ARES\n");
    else if (network->N__king_weight == NETWORK_KING_WEIGHT_GWD)
      PRINTF("king_weight=GWD\n");
    else 
      PRINTF("king_weight=%d\n", network->N__king_weight);
  }


  //clip

  value_cjson = cJSON_GetObjectItem(directory, CJSON_CLIP_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson));

  int ivalue = cJSON_GetNumberValue(value_cjson);

  network->N_clip_value = ivalue;

  if (arg_verbose)
  {
    PRINTF("clip=%d\n", network->N_clip_value);
  }

  //activation_last

  value_cjson = cJSON_GetObjectItem(directory, CJSON_ACTIVATION_LAST_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (compat_strcasecmp(svalue, "linear") == 0)
    network->N_activation_last = NETWORK_ACTIVATION_LINEAR;
  else if (compat_strcasecmp(svalue, "sigmoid") == 0)
    network->N_activation_last = NETWORK_ACTIVATION_SIGMOID;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (arg_verbose)
  {
    if (network->N_activation_last == NETWORK_ACTIVATION_LINEAR)
      PRINTF("activation_last=LINEAR\n");
    else if (network->N_activation_last == NETWORK_ACTIVATION_SIGMOID)
      PRINTF("activation_last=SIGMOID\n");
    else
      FATAL("unknown activation_last option", EXIT_FAILURE)
  }

  MY_MALLOC(network->N_patterns, patterns_t, 1)

  network->N_ninputs =
    construct_patterns(network->N_patterns,
                       bdata(network->N_bshape));

  HARDBUG(network->N_ninputs >= NINPUTS_MAX)

  network->N_nwhite_king_input_map = network->N_ninputs++;
  network->N_nblack_king_input_map = network->N_ninputs++;
  
  HARDBUG(network->N_ninputs >= NINPUTS_MAX)

  PRINTF("N_nwhite_king_input_map=%d\n", network->N_nwhite_king_input_map);
  PRINTF("N_nblack_king_input_map=%d\n", network->N_nblack_king_input_map);

  PRINTF("network->N_ninputs=%d\n", network->N_ninputs);

  int nleft = network->N_ninputs % 16;
 
  if (nleft > 0) network->N_ninputs += 16 - nleft;

  PRINTF("network->N_ninputs=%d\n", network->N_ninputs);
    
  network->N_nlayers = 0;
  network->N_loaded = FALSE;

  network->N_nstate = 0;

  for (int istate = 0; istate < NODE_MAX; istate++)
  {
    MY_MALLOC(network->N_states[istate].NS_layer0_inputs,
              scaled_double_t, network->N_ninputs)

    //MY_MALLOC(network->N_states[istate].NS_layer0_sum,
    //          i32_t, with->L_noutputs)
  }

  network->N_inputs = network->N_states[0].NS_layer0_inputs;

  if (arg_skip_load) return;

  network->N_loaded = TRUE;

  while(TRUE)
  {
    BSTRING(bweights)

    bformata(bweights, "weights%d.csv", network->N_nlayers);

    cJSON *file = open_cjson_file(directory, bdata(bweights));

    BDESTROY(bweights)

    if (file == NULL) break;

    HARDBUG(network->N_nlayers >= NLAYERS_MAX)

    layer_t *with = network->N_layers + network->N_nlayers;

    with->L_ninputs = with->L_noutputs = INVALID;

    read_2d_csv(file, FALSE,
      &(with->L_ninputs), &(with->L_noutputs),
      &(with->L_weights), &(with->L_weights_transposed),
      &(with->L_weights_double), &(with->L_weights_transposed_double));

    BSTRING(bbias)

    bformata(bbias, "bias%d.csv", network->N_nlayers);

    file = open_cjson_file(directory, bdata(bbias));

    HARDBUG(file == NULL)

    BDESTROY(bbias)

    read_1d_csv(file, &(with->L_noutputs), &(with->L_bias),
      &(with->L_bias_double));

    //scale the weights and transposed weights

    MY_MALLOC(with->L_bias64, i64_t, with->L_noutputs);

    for (int ioutput = 0; ioutput < with->L_noutputs; ioutput++)
    {
      for (int input = 0; input < with->L_ninputs; input++)
      {
        with->L_weights_transposed[input][ioutput] =
          with->L_weights[ioutput][input] =
            DOUBLE2SCALED(with->L_weights_double[ioutput][input]);
      }

      with->L_bias[ioutput] =
        DOUBLE2SCALED(with->L_bias_double[ioutput]);

      with->L_bias64[ioutput] =
        with->L_bias[ioutput] * SCALED_DOUBLE_FACTOR;
    }

    mark_pointer_read_only(with->L_weights);
    mark_pointer_read_only(with->L_weights_double);

    for (int ioutput = 0; ioutput < with->L_noutputs; ioutput++)
    {
      mark_pointer_read_only(with->L_weights[ioutput]);
      mark_pointer_read_only(with->L_weights_double[ioutput]);
    }

    mark_pointer_read_only(with->L_weights_transposed);
    mark_pointer_read_only(with->L_weights_transposed_double);

    for (int input = 0; input < with->L_ninputs; input++)
    {
      mark_pointer_read_only(with->L_weights_transposed[input]);
      mark_pointer_read_only(with->L_weights_transposed_double[input]);
    }

    mark_pointer_read_only(with->L_bias);
    mark_pointer_read_only(with->L_bias_double);

    network->N_nlayers++;
  }

  if (arg_verbose) PRINTF("nlayers=%d\n", network->N_nlayers);

  HARDBUG(network->N_nlayers < 2)

  network->N_nstate = 0;

  for (int istate = 0; istate < NODE_MAX; istate++)
  {
    layer_t *with = network->N_layers;

    //MY_MALLOC(network->N_states[istate].NS_layer0_inputs,
    //          scaled_double_t, with->L_ninputs)

    MY_MALLOC(network->N_states[istate].NS_layer0_sum,
              i32_t, with->L_noutputs)
  }

  for (int ilayer = 0; ilayer < network->N_nlayers; ilayer++)
  {
    layer_t *with = network->N_layers + ilayer;

    if (arg_verbose)
    {
      PRINTF("ilayer=%d (inputs, outputs)=(%d, %d)\n",
        ilayer, with->L_ninputs, with->L_noutputs);
    }

    if (ilayer == 0)
    {
      //network->N_inputs = network->N_states[0].NS_layer0_inputs;

      with->L_sum = network->N_states[0].NS_layer0_sum;
    }
    else
    {
      MY_MALLOC(with->L_dot64, i64_t, with->L_noutputs);

      MY_MALLOC(with->L_sum, i32_t, with->L_noutputs);
    }

    MY_MALLOC(with->L_outputs, scaled_double_t, with->L_noutputs);
    MY_MALLOC(with->L_sum_double, double, with->L_noutputs);
    MY_MALLOC(with->L_outputs_double, double, with->L_noutputs);
  }
 
  //check topology

  for (int ilayer = 0; ilayer < network->N_nlayers - 1; ilayer++)
  {
    layer_t *with_current = network->N_layers + ilayer;
    layer_t *with_next = network->N_layers + ilayer + 1;
   
    HARDBUG(with_current->L_noutputs != with_next->L_ninputs)
  }
}

local i32_t dot_scaled(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

  i32_t s = 0;

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  __m256i ts = _mm256_setzero_si256();

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);

    ts = _mm256_add_epi32(ts, _mm256_mullo_epi32(ta, tb));
  }

  ALIGN64(i32_t r[8]);

  _mm256_store_si256((__m256i*)r, ts);

  s = r[0] + r[1] + r[2] + r[3] + r[4] + r[5] + r[6] + r[7];

#else

  for (int i = 0; i < n; i++)
    s += a[i] * b[i];

#endif

  END_BLOCK

  return(s);
}

local i64_t dot_scaled64(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

  i64_t s = 0;

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  __m256i s_lo = _mm256_setzero_si256();
  __m256i s_hi = _mm256_setzero_si256();

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);
    __m256i tb = _mm256_load_si256((__m256i*)b);

    __m256i ta_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(ta));
    __m256i ta_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(ta, 1));

    __m256i tb_lo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(tb));
    __m256i tb_hi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(tb, 1));

    s_lo = _mm256_add_epi64(s_lo, _mm256_mul_epi32(ta_lo, tb_lo));
    s_hi = _mm256_add_epi64(s_hi, _mm256_mul_epi32(ta_hi, tb_hi));
  }

  ALIGN64(i64_t result_lo[4]);
  ALIGN64(i64_t result_hi[4]);

  _mm256_store_si256((__m256i*)result_lo, s_lo);
  _mm256_store_si256((__m256i*)result_hi, s_hi);

  s = result_lo[0] + result_lo[1] + result_lo[2] + result_lo[3] +
      result_hi[0] + result_hi[1] + result_hi[2] + result_hi[3];

#else

  for (int i = 0; i < n; i++)
    s += a[i] * b[i];

#endif

  END_BLOCK

  return(s);
}

local double dot_double(int n, double *restrict a, double *restrict b)
{
  double s = 0;

  for (int i = 0; i < n; i++)
    s += a[i] * b[i];

  return(s);
}

local void vcopy_ab(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);

    _mm256_store_si256((__m256i*)b, ta);
  }

#else

  for (int i = 0; i < n; i++) b[i] = a[i];

#endif
}

local void vadd_aba(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);

    const __m256i ts = _mm256_add_epi32(ta, tb);

    _mm256_store_si256((__m256i*)a, ts);
  }

#else

  for (int i = 0; i < n; i++) a[i] += b[i];

#endif

  END_BLOCK
}

local void vsub_aba(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);

    const __m256i ts = _mm256_sub_epi32(ta, tb);

    _mm256_store_si256((__m256i*)a, ts);
  }

#else

  for (int i = 0; i < n; i++) a[i] -= b[i];

#endif

  END_BLOCK
}

local void vadd_acba(int n, i32_t *restrict a,
  scaled_double_t c, scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  __m256i tc = _mm256_set1_epi32(c);

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);

    const __m256i tm = _mm256_mullo_epi32(tb, tc);

    const __m256i ts = _mm256_add_epi32(ta, tm);

    _mm256_store_si256((__m256i*)a, ts);
  }

#else

  for (int i = 0; i < n; i++) a[i] += c * b[i];

#endif

  END_BLOCK
}

local void activation_relu_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
  FATAL("not implemented yet", EXIT_FAILURE)
}

local void activation_clipped_relu_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b,
  int arg_clip)
{
#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  __m256i zero = _mm256_setzero_si256();
  __m256i clip = _mm256_set1_epi32(SCALED_DOUBLE_FACTOR * arg_clip);

  for (const int32_t *z = a + n; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);

    ta = _mm256_max_epi32(ta, zero);
    ta = _mm256_min_epi32(ta, clip);

    _mm256_store_si256((__m256i*)b, ta);
  }

#else

  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0)
      b[i] = 0;
    else if (a[i] >= SCALED_DOUBLE_FACTOR * arg_clip)
      b[i] = SCALED_DOUBLE_FACTOR * arg_clip;
    else
      b[i] = a[i];
  }

#endif
}

local void activation_relu_double(int n,
  double *restrict a, double *restrict b)
{
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else
      b[i] = a[i];
  }
}

local void activation_clipped_relu_double(int n,
  double *restrict a, double *restrict b, int arg_clip)
{
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= arg_clip)
      b[i] = arg_clip;
    else
      b[i] = a[i];
  }
}

local double return_sigmoid(double x)
{
  return(2.0 / (1.0 + exp(-x)) - 1.0);
}

local void vscalea(int n, scaled_double_t *restrict a)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

  SOFTBUG((n % 8) != 0)

  __m256 fs = _mm256_set1_ps(1.0 / 1.0f);

  for (const i32_t *z = a + n; a < z; a += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i *)a);

    const __m256 fm = _mm256_mul_ps(_mm256_cvtepi32_ps(ta), fs);

    _mm256_store_si256((__m256i *)a, _mm256_cvtps_epi32(fm));
  }

#endif
  END_BLOCK
}

double return_network_score_scaled(network_t *arg_network, int arg_with_sigmoid,
  int arg_skip_inputs)
{
  BEGIN_BLOCK(__func__)

  //first layer

  layer_t *with_current = arg_network->N_layers;

  BEGIN_BLOCK("Ax+b-and-activation-first-layer")

  if (!arg_skip_inputs)
  {
    for (int i = 0; i < with_current->L_noutputs; i++)
    {
      with_current->L_sum[i] = 
        dot_scaled(with_current->L_ninputs,
                   arg_network->N_inputs,
                   with_current->L_weights[i]) +
        with_current->L_bias[i];
    }

    double inputs_double[NINPUTS_MAX];

    for (int i = 0; i < arg_network->N_ninputs; i++) inputs_double[i] =
      (double) arg_network->N_inputs[i];

    for (int i = 0; i < with_current->L_noutputs; i++)
    {
      with_current->L_sum_double[i] = 
        dot_double(with_current->L_ninputs,
                   inputs_double,
                   with_current->L_weights_double[i]) +
        with_current->L_bias_double[i];

      with_current->L_sum[i] = 
        DOUBLE2SCALED(with_current->L_sum_double[i]);
    }
  }
  
  BEGIN_BLOCK("Ax+b-and-activation-first-layer-activation")

  if (arg_network->N_clip_value == 0)
  {
    activation_relu_scaled(with_current->L_noutputs,
      with_current->L_sum, with_current->L_outputs);
  }
  else
  {
    activation_clipped_relu_scaled(with_current->L_noutputs,
      with_current->L_sum, with_current->L_outputs,
      arg_network->N_clip_value);
  }

  END_BLOCK

  END_BLOCK

  for (int ilayer = 1; ilayer < arg_network->N_nlayers; ilayer++)
  {
    with_current = arg_network->N_layers + ilayer;

    layer_t *with_previous = arg_network->N_layers + ilayer - 1;

    BEGIN_BLOCK("Ax+b-and-activation-other-layers")

    //if (ilayer == 1)
    //  vscalea(with_current->L_ninputs, with_previous->L_outputs);

    for (int i = 0; i < with_current->L_noutputs; i++)
    {
      with_current->L_dot64[i] = 
        dot_scaled64(with_current->L_ninputs,
                     with_previous->L_outputs,
                     with_current->L_weights[i]);
    }

    add2_64(with_current->L_noutputs, with_current->L_sum,
            with_current->L_dot64, with_current->L_bias64);

    if (ilayer < (arg_network->N_nlayers - 1))
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-activation")

      if (arg_network->N_clip_value == 0)
      {
        activation_relu_scaled(with_current->L_noutputs,
          with_current->L_sum, with_current->L_outputs);
      }
      else
      {
        activation_clipped_relu_scaled(with_current->L_noutputs,
          with_current->L_sum, with_current->L_outputs,
          arg_network->N_clip_value);
      }

      END_BLOCK
    }
    else
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-last-layer")

      for (int i = 0; i < with_current->L_noutputs; i++)
      {
        if (arg_network->N_activation_last == NETWORK_ACTIVATION_LINEAR)
        {
          with_current->L_outputs_double[i] =
            SCALED2DOUBLE(with_current->L_sum[i]);
        }
        else if (arg_network->N_activation_last == NETWORK_ACTIVATION_SIGMOID)
        {
          with_current->L_outputs_double[i] =
            return_sigmoid(SCALED2DOUBLE(with_current->L_sum[i]));
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }

      END_BLOCK
    }

    END_BLOCK
  }
 
  END_BLOCK

  layer_t *with = arg_network->N_layers + arg_network->N_nlayers - 1;

  return(with->L_outputs_double[0]);
}

double return_network_score_double(network_t *arg_network, int arg_with_sigmoid)
{
  double inputs_double[NINPUTS_MAX];

  layer_t *with_current = arg_network->N_layers;

  for (int i = 0; i < arg_network->N_ninputs; i++)
    inputs_double[i] = (double) arg_network->N_inputs[i];

  for (int i = 0; i < with_current->L_noutputs; i++)
  {
    with_current->L_sum_double[i] =
      dot_double(with_current->L_ninputs,
                 inputs_double,
                 with_current->L_weights_double[i]) +
      with_current->L_bias_double[i];
  }

  if (arg_network->N_clip_value == 0)
  {
    activation_relu_double(with_current->L_noutputs,
      with_current->L_sum_double, with_current->L_outputs_double);
  }
  else
  {
    activation_clipped_relu_double(with_current->L_noutputs,
      with_current->L_sum_double, with_current->L_outputs_double,
      arg_network->N_clip_value);
  }

  for (int ilayer = 1; ilayer < arg_network->N_nlayers; ilayer++)
  {
    with_current = arg_network->N_layers + ilayer;

    layer_t *with_previous = arg_network->N_layers + ilayer - 1;

    for (int i = 0; i < with_current->L_noutputs; i++)
    {
      with_current->L_sum_double[i] =
        dot_double(with_current->L_ninputs,
                   with_previous->L_outputs_double,
                   with_current->L_weights_double[i]) +
        with_current->L_bias_double[i];
    }

    if (ilayer < (arg_network->N_nlayers - 1))
    {
      if (arg_network->N_clip_value == 0)
      {
        activation_relu_double(with_current->L_noutputs,
          with_current->L_sum_double, with_current->L_outputs_double);
      }
      else
      {
        activation_clipped_relu_double(with_current->L_noutputs,
          with_current->L_sum_double, with_current->L_outputs_double,
          arg_network->N_clip_value);
      }
    }
    else
    {
      for (int i = 0; i < with_current->L_noutputs; i++)
      {
        if (arg_network->N_activation_last == NETWORK_ACTIVATION_LINEAR)
        {
          with_current->L_outputs_double[i] =
            with_current->L_sum_double[i];
        }
        else if (arg_network->N_activation_last == NETWORK_ACTIVATION_SIGMOID)
        {
          with_current->L_outputs_double[i] =
            return_sigmoid(with_current->L_sum_double[i]);
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }
    }
  }

  layer_t *with = arg_network->N_layers + arg_network->N_nlayers - 1;

  if (arg_with_sigmoid)
    return(with->L_outputs_double[0]);
  else
    return(with->L_sum_double[0]);
}

void push_network_state(network_t *arg_network)
{
  layer_t *with = arg_network->N_layers;

  HARDBUG(with->L_sum !=
          arg_network->N_states[arg_network->N_nstate].NS_layer0_sum)

  arg_network->N_nstate++;

  HARDBUG(arg_network->N_nstate >= NODE_MAX)

  network_state_t *state = arg_network->N_states + arg_network->N_nstate;

  vcopy_ab(with->L_ninputs, arg_network->N_inputs,
           state->NS_layer0_inputs);

  vcopy_ab(with->L_noutputs, with->L_sum, state->NS_layer0_sum);

  arg_network->N_inputs = state->NS_layer0_inputs;

  with->L_sum = state->NS_layer0_sum;
}

void pop_network_state(network_t *arg_network)
{
  arg_network->N_nstate--;

  HARDBUG(arg_network->N_nstate < 0)

  network_state_t *state = arg_network->N_states + arg_network->N_nstate;

  layer_t *with = arg_network->N_layers;

  arg_network->N_inputs = state->NS_layer0_inputs;

  with->L_sum = state->NS_layer0_sum;
}

void check_layer0(network_t *arg_network)
{
  layer_t *with_current = arg_network->N_layers;

  for (int i = 0; i < with_current->L_noutputs; i++)
  {
    i32_t L_sum = 
      dot_scaled(with_current->L_ninputs,
                 arg_network->N_inputs,
                 with_current->L_weights[i]) +
      with_current->L_bias[i];

    if (FALSE and (abs(L_sum - with_current->L_sum[i]) > 10))
    {
      PRINTF("L_sum=%d with-current->L_sum[i]=%d\n",
             L_sum, with_current->L_sum[i]);

      FATAL("POSSIBLE OVERFLOW?", EXIT_FAILURE)
    }
  }
}

void update_layer0(network_t *arg_network, int arg_j, scaled_double_t arg_v)
{
  if (options.material_only) return;

  layer_t *with_current = arg_network->N_layers;

  if (arg_v == 1)
  {
    vadd_aba(with_current->L_noutputs,
      with_current->L_sum,
      with_current->L_weights_transposed[arg_j]);
  }
  else if (arg_v == -1)
  {
    vsub_aba(with_current->L_noutputs,
      with_current->L_sum,
      with_current->L_weights_transposed[arg_j]);
  } 
  else
  {
    vadd_acba(with_current->L_noutputs,
      with_current->L_sum, arg_v,
      with_current->L_weights_transposed[arg_j]);
  }

  arg_network->N_inputs[arg_j] += arg_v;
}

void board2network(board_t *object, int arg_debug)
{
  scaled_double_t inputs[NINPUTS_MAX];

  network_t *network = &(object->B_network);

  HARDBUG(network->N_ninputs >= NINPUTS_MAX)

  if (arg_debug)
  {
    for (int input = 0; input < network->N_ninputs; input++)
      inputs[input] = network->N_inputs[input];
  }

  if (network->N_loaded)
  {
    network->N_nstate = 0;

    network->N_inputs =
      network->N_states[0].NS_layer0_inputs;
  
    network->N_layers[0].L_sum =
      network->N_states[0].NS_layer0_sum;
  
    for (int input = 0; input < network->N_ninputs; input++)
      network->N_inputs[input] = 0;
  }

  patterns_t *with_patterns = network->N_patterns;

  for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
  {
    pattern_t *with_pattern = with_patterns->patterns + ipattern;

    int *mask = object->B_pattern_mask->PM_mask + ipattern;

    int input = with_pattern->P_mask2inputs[*mask];

    if ((*mask & with_pattern->P_valid_mask) == with_pattern->P_valid_mask)
    {
      //valid or NINPUTS_MAX
   
      if (input != NINPUTS_MAX) network->N_inputs[input] = 1;
    }
    else
    {
      HARDBUG(input != INVALID)
    }
  }

  if (network->N__king_weight > 0)
  {
    int nwhite_king = BIT_COUNT(object->B_white_king_bb);
    int nblack_king = BIT_COUNT(object->B_black_king_bb);
  
    network->N_inputs[network->N_nwhite_king_input_map] =
      nwhite_king * network->N__king_weight;
    network->N_inputs[network->N_nblack_king_input_map] =
      nblack_king * network->N__king_weight;
  }
  else
  {
    network->N_inputs[network->N_nwhite_king_input_map] = 0;
    network->N_inputs[network->N_nblack_king_input_map] = 0;
  }

  if (arg_debug)
  {
    int error = FALSE;

    for (int input = 0; input < network->N_ninputs; input++)
    {
      if (inputs[input] != network->N_inputs[input])
      {
        PRINTF("input=%d inputs[input]=%d N_inputs[input]=%d\n",
          input, inputs[input], network->N_inputs[input]);

        error = TRUE;
      }
    } 
    if (error) FATAL("eh?", 1)
  }

  check_board_patterns(object, (char *) __FUNC__);
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
#ifdef USE_OPENMPI

  board_t board;

  construct_board(&board, STDOUT, FALSE);

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

  MY_MALLOC(npieces_fen, i64_t, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    npieces_fen[ifen] = 0;

  double *score_scaled, *score_double;

  MY_MALLOC(score_scaled, double, nfen)
  MY_MALLOC(score_double, double, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    score_scaled[ifen] = score_double[ifen] = 0.0;

  double *delta_scaled, *delta_double;

  MY_MALLOC(delta_scaled, double, nfen)
  MY_MALLOC(delta_double, double, nfen)

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

  for (i64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#') continue;

    if ((my_mpi_globals.MY_MPIG_nslaves > 0) and
        ((nfen % my_mpi_globals.MY_MPIG_nslaves) !=
         my_mpi_globals.MY_MPIG_id_slave))
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

    fen2board(with, bdata(bfen), TRUE);

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
      return_network_score_scaled(&(with->B_network), FALSE, FALSE);

    score_double[ifen] =
      return_network_score_double(&(with->B_network), FALSE);

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

  destroy_my_bstream(&my_bstream);

  PRINTF("nfen_mpi=%lld\n", nfen_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  my_mpi_allreduce(MPI_IN_PLACE, npieces_fen, nfen,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, score_scaled, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, score_double, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, delta_scaled, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, delta_double, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &mse_vs_draw_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_vs_draw_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &mse_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nnetwork2mat_score_gt_score_man, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &error_max_scaled_vs_double, 1,
    MPI_DOUBLE, MPI_MAX, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfactor, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &factor, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

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

  MY_MALLOC(pdelta_scaled, i64_t, nfen)

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

    if ((my_mpi_globals.MY_MPIG_nslaves > 0) and
        ((nfen % my_mpi_globals.MY_MPIG_nslaves) !=
         my_mpi_globals.MY_MPIG_id_slave))
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

    fen2board(with, bdata(bfen), TRUE);

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
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_skipped_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen_skipped=%lld\n", nfen_skipped_mpi);

  my_mpi_allreduce(MPI_IN_PLACE, &nfactor, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  my_mpi_allreduce(MPI_IN_PLACE, &factor, 1, MPI_DOUBLE,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  
 factor /= nfactor;

 double factor2man_score = SCORE_MAN / factor;

 PRINTF("nfactor=%lld factor=%.6f factor2man_score=%.6f\n",
   nfactor, factor, factor2man_score);
#endif
}

#define TAG "w2m-nocapt"

#define SEP ","

local void append_fen(board_t *self, int arg_king_weight,
  bstring arg_bfen, double arg_result,
  fbuffer_t *arg_fcsv, fbuffer_t *arg_ffen)
{
  board_t *object = self;

  patterns_t *with_patterns = object->B_network.N_patterns;

  for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
  {
    pattern_t *with_pattern = with_patterns->patterns + ipattern;

    int input =
      with_pattern->P_mask2inputs[object->B_pattern_mask->PM_mask[ipattern]];

    if ((object->B_pattern_mask->PM_mask[ipattern] &
         with_pattern->P_valid_mask) == with_pattern->P_valid_mask)
    {
      if (input == NINPUTS_MAX) input = INVALID;
    }
    else
    {
      HARDBUG(input != INVALID)
    }

    append_fbuffer(arg_fcsv, "%d" SEP, input);
  }

  //always W2M

  if (IS_BLACK(object->B_colour2move)) arg_result = -arg_result;

  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  append_fbuffer(arg_fcsv, "%3u" SEP "%3u" SEP "%.6f\n",
    nwhite_king * arg_king_weight, nblack_king * arg_king_weight, arg_result);

  if (arg_ffen != NULL) append_fbuffer(arg_ffen, "%s {%.6f}\n", bdata(arg_bfen), arg_result);
}

void fen2csv(char *arg_name, int arg_nman_max, int arg_nkings_max,
  int arg_king_weight)
{
#ifdef USE_OPENMPI

  ares = FALSE;
  if (strstr(arg_name, "v19") != NULL) ares = TRUE;
  PRINTF("ares=%d\n", ares);

  my_random_t fen2csv_random;

  options.material_only = TRUE;

  construct_my_random(&fen2csv_random, 0);

  board_t board;

  construct_board(&board, STDOUT, TRUE);

  board_t *with = &board;

  BSTRING(barg_name)

  HARDBUG(bassigncstr(barg_name, arg_name) == BSTR_ERR)

  //training

  BSTRING(bsuffix)

  HARDBUG(bformata(bsuffix, "-%s-nman%d-nkings%d-weight%d-%s-training.csv",
                   TAG, arg_nman_max, arg_nkings_max, arg_king_weight,
                   bdata(with->B_network.N_bshape)) == BSTR_ERR)

  fbuffer_t ftraining;

  construct_fbuffer(&ftraining, barg_name, bdata(bsuffix), TRUE);

  //validation

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s-nman%d-nkings%d-weight%d-%s-validation.csv",
                   TAG, arg_nman_max, arg_nkings_max, arg_king_weight,
                   bdata(with->B_network.N_bshape)) == BSTR_ERR)

  fbuffer_t fvalidation;

  construct_fbuffer(&fvalidation, barg_name, bdata(bsuffix), TRUE);

  //fen

  HARDBUG(bassigncstr(bsuffix, "") == BSTR_ERR)

  HARDBUG(bformata(bsuffix, "-%s-nman%d-nkings%d-weight%d-%s.fen",
                   TAG, arg_nman_max, arg_nkings_max, arg_king_weight,
                   bdata(with->B_network.N_bshape)) == BSTR_ERR)

  fbuffer_t ffen;

  construct_fbuffer(&ffen, barg_name, bdata(bsuffix), TRUE);

  //done

  PRINTF("training_name=%s\n", bdata(return_fbuffer_bname(&ftraining)));
  PRINTF("validation_name=%s\n", bdata(return_fbuffer_bname(&fvalidation)));
  PRINTF("fen_name=%s\n", bdata(return_fbuffer_bname(&ffen)));

  i64_t nfen = 0;
  i64_t nfen_mpi = 0;
  i64_t nfen_delta_mpi = 0;
  i64_t nfen_result_mpi = 0;
  i64_t nfen_nkings_mpi = 0;
  i64_t nfen_nman_mpi = 0;
  i64_t nfen_moves_capt_mpi = 0;

  bucket_t bucket_result_root;

  construct_bucket(&bucket_result_root, "bucket_result_root",
                   SCORE_MAN, SCORE_LOST, SCORE_WON, BUCKET_LINEAR);

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

  BSTRING(bdata_name);

  HARDBUG(bassigncstr(bdata_name, "/tmp5/gwies/data.csv") == BSTR_ERR)

  fbuffer_t fdata;

  construct_fbuffer(&fdata, bdata_name, NULL, TRUE);

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '#') continue;

    if ((nfen % 1000000) == 0) PRINTF("nfen=%lld\n", nfen);
   
    if ((my_mpi_globals.MY_MPIG_nslaves > 0) and
        ((nfen % my_mpi_globals.MY_MPIG_nslaves) !=
         my_mpi_globals.MY_MPIG_id_slave))
    {
      nfen++;

      continue;
    }

    nfen++;
    nfen_mpi++;

    double result, result_root, label;

    CSTRING(cfen, blength(bline))

    HARDBUG(ares == INVALID)
 
    if (ares)
    {
      HARDBUG(sscanf(bdata(bline), "%s {%lf}", cfen, &result) != 2)
    }
    else
    {
      if (sscanf(bdata(bline), "%s {%lf} %*s %lf %lf",
                 cfen, &result, &result_root, &label) != 4)
      {
        PRINTF("bline=%s\n", bdata(bline));
  
        FATAL("sscanf != 4", EXIT_FAILURE)
      }
    }

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(with, bdata(bfen), FALSE);

    CDESTROY(cfen)

    if (ares)
    {
      if (IS_BLACK(with->B_colour2move)) result = -result;

      result_root = result;
    }

    if (fabs(result - result_root) >= (SCORE_MAN / 4.0))
    {
      nfen_delta_mpi++;

      continue;
    }

    if (fabs(result) > 999)
    {
      nfen_result_mpi++;

      continue;
    }

    int nwhite_kings = BIT_COUNT(with->B_white_king_bb);

    int nblack_kings = BIT_COUNT(with->B_black_king_bb);

    if ((arg_nkings_max == INVALID) and
        ((nwhite_kings + nblack_kings) == 0))
    {
      nfen_nkings_mpi++;

      continue;
    }
 
    if ((arg_nkings_max != INVALID) and
        ((nwhite_kings > arg_nkings_max) or
         (nblack_kings > arg_nkings_max)))
    {
      nfen_nkings_mpi++;

      continue;
    }

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);

    int nblack_man = BIT_COUNT(with->B_black_man_bb);

    if ((nwhite_man + nblack_man) > arg_nman_max)
    {
      nfen_nman_mpi++;

      continue;
    }

    moves_list_t moves_list;

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.ML_nmoves <= 1) or
        (moves_list.ML_ncaptx > 0))
    {
      nfen_moves_capt_mpi++;

      continue;
    }

    update_bucket(&bucket_result_root, result_root);

    int material_score = return_material_score(with);

    update_bucket(&bucket_nwhite_man, nwhite_man);

    update_bucket(&bucket_nblack_man, nblack_man);

    update_bucket(&bucket_nwhite_kings, nwhite_kings);

    update_bucket(&bucket_nblack_kings, nblack_kings);

    update_bucket(&bucket_material_score, material_score);

    update_bucket(&bucket_nman, nwhite_man + nblack_man);

    if (!ares)
    {
      result = result_root;
      result *= 0.001;
    }

    {
      int delta_nman = nwhite_man - nblack_man;
      int delta_nkings = nwhite_kings - nblack_kings;
      int nman = nwhite_man + nblack_man;

      double result_wtm = result;

      if (IS_BLACK(with->B_colour2move)) result_wtm = -result_wtm;

      append_fbuffer(&fdata, "%d,%d,%d,%.6f\n",
        delta_nman, delta_nkings, nman, result_wtm);
    }

    int prob = return_my_random(&fen2csv_random) % 100;

    if (prob <= 79) 
    {
      append_fen(with, arg_king_weight, bfen, result, &ftraining, NULL);
    }
    else
    {
      append_fen(with, arg_king_weight, bfen, result, &fvalidation, &ffen);
    }
  }

  destroy_my_bstream(&my_bstream);

  flush_fbuffer(&fdata, 0);

  BDESTROY(bfen)

  BDESTROY(bdata_name)

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_delta_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_result_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_nkings_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_nman_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE, &nfen_moves_capt_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen=%lld\n", nfen);
  PRINTF("nfen_delta_mpi=%lld\n", nfen_delta_mpi);
  PRINTF("nfen_result_mpi=%lld\n", nfen_result_mpi);
  PRINTF("nfen_nkings_mpi=%lld\n", nfen_nkings_mpi);
  PRINTF("nfen_nman_mpi=%lld\n", nfen_nman_mpi);
  PRINTF("nfen_moves_capt_mpi=%lld\n", nfen_moves_capt_mpi);

  my_mpi_bucket_aggregate(&bucket_result_root,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_material_score,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_man,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_man,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nblack_kings,
    my_mpi_globals.MY_MPIG_comm_slaves);

  my_mpi_bucket_aggregate(&bucket_nman,
    my_mpi_globals.MY_MPIG_comm_slaves);

  printf_bucket(&bucket_result_root);

  printf_bucket(&bucket_material_score);

  printf_bucket(&bucket_nwhite_man);

  printf_bucket(&bucket_nblack_man);

  printf_bucket(&bucket_nwhite_kings);

  printf_bucket(&bucket_nblack_kings);

  printf_bucket(&bucket_nman);

  //write remaining

  flush_fbuffer(&ffen, 0);

  flush_fbuffer(&fvalidation, 0);

  flush_fbuffer(&ftraining, 0);

  BDESTROY(bsuffix)

  BDESTROY(barg_name)
#endif
}

void test_network(void)
{ 
}

