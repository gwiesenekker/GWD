//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#define NETWORKS_JSON "networks.json"

local cJSON *open_cjson_file(cJSON *directory, char *arg_name)
{
  cJSON *files = cJSON_GetObjectItem(directory, CJSON_FILES_ID);
  
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

local void read_1d_csv(cJSON *csv, int *arg_nrows,
  scaled_double_t **arg_cells, double **arg_cells_double)
{  
  *arg_cells = NULL;
  *arg_cells_double = NULL;

  cJSON *lines = cJSON_GetObjectItem(csv, CJSON_FILE_LINES_ID);

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

  MALLOC(cells, scaled_double_t, nrows)
  MALLOC(cells_double, double, nrows)

  int irow = 0;

  cJSON_ArrayForEach(line, lines)
  {
    char *string = cJSON_GetStringValue(line);

    HARDBUG(string == NULL)

    HARDBUG(irow >= nrows)

    double f;

    HARDBUG(sscanf(string, "%lf", &f) != 1)

    cells[irow] = DOUBLE2SCALED(f);

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

local void read_2d_csv(cJSON *csv, const int by_row,
  int *arg_nrows, int *arg_ncols,
  scaled_double_t ***arg_cells, scaled_double_t ***arg_cells_transposed,
  double ***arg_cells_double, double ***arg_cells_transposed_double)
{
  *arg_cells = NULL;
  *arg_cells_transposed = NULL;

  *arg_cells_double = NULL;
  *arg_cells_transposed_double = NULL;

  cJSON *lines = cJSON_GetObjectItem(csv, CJSON_FILE_LINES_ID);

  HARDBUG(!cJSON_IsArray(lines))

  int ncols = INVALID;
  int nrows = 0;

  cJSON *line;

  cJSON_ArrayForEach(line, lines)
  {
    HARDBUG(!cJSON_IsString(line))

    char string[MY_LINE_MAX];

    strcpy(string, cJSON_GetStringValue(line));

    int ntokens = 0;

    char *token = strtok(string, ",");

    while(token != NULL)
    {
      double f;

      HARDBUG(sscanf(token, "%lf", &f) != 1)
       
      ntokens++;

      token = strtok(NULL, ",");
    }

    HARDBUG(ntokens == 0)

    if (ncols == INVALID)
      ncols = ntokens;
    else
      HARDBUG(ntokens != ncols)

    nrows++;
  }

  HARDBUG(nrows == 0)

  scaled_double_t **cells;
  scaled_double_t **cells_transposed;

  double **cells_double;
  double **cells_transposed_double;

  if (by_row)
  {
    MALLOC(cells, scaled_double_t *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MALLOC(cells[irow], scaled_double_t, ncols)

    MALLOC(cells_transposed, scaled_double_t *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MALLOC(cells_transposed[icol], scaled_double_t, nrows)

    //

    MALLOC(cells_double, double *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MALLOC(cells_double[irow], double, ncols)

    MALLOC(cells_transposed_double, double *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MALLOC(cells_transposed_double[icol], double, nrows)
  }
  else
  {
    MALLOC(cells, scaled_double_t *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MALLOC(cells[icol], scaled_double_t, nrows)

    MALLOC(cells_transposed, scaled_double_t *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MALLOC(cells_transposed[irow], scaled_double_t, ncols)

    //

    MALLOC(cells_double, double *, ncols)

    for (int icol = 0; icol < ncols; icol++)
      MALLOC(cells_double[icol], double, nrows)

    MALLOC(cells_transposed_double, double *, nrows)

    for (int irow = 0; irow < nrows; irow++)
      MALLOC(cells_transposed_double[irow], double, ncols)
  }

  //now read csv

  int irow = 0;

  cJSON_ArrayForEach(line, lines)
  {
    HARDBUG(!cJSON_IsString(line))

    char string[MY_LINE_MAX];

    strcpy(string, cJSON_GetStringValue(line));

    HARDBUG(irow >= nrows)

    int icol = 0;

    char *token = strtok(string, ",");

    while(token != NULL)
    {
      double f;

      HARDBUG(sscanf(token, "%lf", &f) != 1)

      HARDBUG(icol >= ncols)

      if (by_row)
      {
        cells_transposed[icol][irow]= cells[irow][icol] = DOUBLE2SCALED(f);

        cells_transposed_double[icol][irow]= cells_double[irow][icol] = f;
      }
      else
      {
        cells_transposed[irow][icol] = cells[icol][irow] = DOUBLE2SCALED(f);

        cells_transposed_double[irow][icol] = cells_double[icol][irow] = f;
      }
       
      token = strtok(NULL, ",");

      icol++;
    }

    HARDBUG(icol != ncols)

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

local void print_input_map(int ninput_map, int input_map[], int as_board)
{
  if (as_board)
  {
    int i = 0;

    for (int row = 10; row >= 1; row--)
    {
      if ((row & 1) == 0) PRINTF("   ");
      for (int col = 1; col <= 5; col++)
      {
        i++;
        PRINTF("   %3d", input_map[map[i]]);
      }
      PRINTF("\n");
    }
  }
  else
  {
    for (int i = 0; i < ninput_map; i++)
      PRINTF("%d %d\n", i, input_map[i]);
  }
}

void load_neural(char *neural_name, neural_t *neural, int verbose)
{
  cJSON *networks_json;

  file2cjson(NETWORKS_JSON, &networks_json);

  //find the directory

  cJSON *directories = cJSON_GetObjectItem(networks_json,
                                           CJSON_DIRECTORIES_ID);
  
  HARDBUG(!cJSON_IsArray(directories))

  int found = FALSE;

  cJSON *directory;

  cJSON_ArrayForEach(directory, directories)
  {
    cJSON *name_cjson = cJSON_GetObjectItem(directory,
                                            CJSON_DIRECTORY_NAME_ID);

    char *directory_name = cJSON_GetStringValue(name_cjson);

    HARDBUG(directory_name == NULL)

    if (strcmp(directory_name, neural_name) == 0) 
    {
      found = TRUE;

      break;
    }
  }

  if (!found)
  {
    PRINTF("could not find name %s in %s\n",
      neural_name, NETWORKS_JSON);

    FATAL(NETWORKS_JSON " error", EXIT_FAILURE)
  }

  cJSON *neural2material_score_cjson =
    cJSON_GetObjectItem(directory, CJSON_NEURAL2MATERIAL_SCORE_ID);

  HARDBUG(!cJSON_IsNumber(neural2material_score_cjson))

  neural->neural2material_score =
   cJSON_GetNumberValue(neural2material_score_cjson);

  if (verbose)
    PRINTF("neural2material_score=%.2f\n", neural->neural2material_score);

  //input map

  cJSON *value_cjson =
    cJSON_GetObjectItem(directory, CJSON_INPUT_MAP_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  char *svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "v2") == 0)
    neural->neural_input_map = NEURAL_INPUT_MAP_V2;
  else if (my_strcasecmp(svalue, "v5") == 0)
    neural->neural_input_map = NEURAL_INPUT_MAP_V5;
  else if (my_strcasecmp(svalue, "v6") == 0)
    neural->neural_input_map = NEURAL_INPUT_MAP_V6;
  else
    FATAL("unknown input_map option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_input_map == NEURAL_INPUT_MAP_V2)
      PRINTF("input_map=V2\n");
    else if (neural->neural_input_map == NEURAL_INPUT_MAP_V5)
      PRINTF("input_map=V5\n");
    else if (neural->neural_input_map == NEURAL_INPUT_MAP_V6)
      PRINTF("input_map=V6\n");
    else
      FATAL("unknown input_map option", EXIT_FAILURE)
  }

  //activation

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_ACTIVATION_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "relu") == 0)
    neural->neural_activation = NEURAL_ACTIVATION_RELU;
  else if (my_strcasecmp(svalue, "clipped-relu") == 0)
    neural->neural_activation = NEURAL_ACTIVATION_CLIPPED_RELU;
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_activation == NEURAL_ACTIVATION_RELU)
      PRINTF("activation=RELU\n");
    else if (neural->neural_activation == NEURAL_ACTIVATION_CLIPPED_RELU)
      PRINTF("activation=CLIPPED_RELU\n");
    else
      FATAL("unknown activation option", EXIT_FAILURE)
  }

  //activation_last

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_ACTIVATION_LAST_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "linear") == 0)
    neural->neural_activation_last = NEURAL_ACTIVATION_LINEAR;
  else if (my_strcasecmp(svalue, "sigmoid") == 0)
    neural->neural_activation_last = NEURAL_ACTIVATION_SIGMOID;
  else
    FATAL("unknown activation_last option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_activation_last == NEURAL_ACTIVATION_LINEAR)
      PRINTF("activation_last=LINEAR\n");
    else if (neural->neural_activation_last == NEURAL_ACTIVATION_SIGMOID)
      PRINTF("activation_last=SIGMOID\n");
    else
      FATAL("unknown activation_last option", EXIT_FAILURE)
  }

  //label

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_LABEL_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "c2m") == 0)
    neural->neural_label = NEURAL_LABEL_C2M;
  else if (my_strcasecmp(svalue, "w2m") == 0)
    neural->neural_label = NEURAL_LABEL_W2M;
  else
    FATAL("unknown label option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_label == NEURAL_LABEL_C2M)
      PRINTF("label=C2M\n");
    else if (neural->neural_label == NEURAL_LABEL_W2M)
      PRINTF("label=W2M\n");
    else
      FATAL("unknown label option", EXIT_FAILURE)
  }

  //colour_bits

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_COLOUR_BITS_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "0") == 0)
    neural->neural_colour_bits = NEURAL_COLOUR_BITS_0;
  else
    FATAL("unknown colour_bits option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_colour_bits == NEURAL_COLOUR_BITS_0)
      PRINTF("colour_bits=0\n");
    else
      FATAL("unknown colour_bits option", EXIT_FAILURE)
  }

  //output

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_OUTPUT_ID);

  HARDBUG(!cJSON_IsString(value_cjson));

  svalue = cJSON_GetStringValue(value_cjson);

  if (my_strcasecmp(svalue, "w2m") == 0)
    neural->neural_output = NEURAL_OUTPUT_W2M;
  else if (my_strcasecmp(svalue, "c2m") == 0)
    neural->neural_output = NEURAL_OUTPUT_C2M;
  else
    FATAL("unknown output option", EXIT_FAILURE)

  if (verbose)
  {
    if (neural->neural_output == NEURAL_OUTPUT_W2M)
      PRINTF("output=W2M\n");
    else if (neural->neural_output == NEURAL_OUTPUT_C2M)
      PRINTF("output=C2M\n");
    else
      FATAL("unknown output option", EXIT_FAILURE)
  }

  //npieces_min

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_NPIECES_MIN_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson));

  int ivalue = round(cJSON_GetNumberValue(value_cjson));

  HARDBUG(ivalue < 0)

  neural->neural_npieces_min = ivalue;

  if (verbose)
  {
    PRINTF("npieces_min=%d\n", neural->neural_npieces_min);
  }

  //npieces_max

  value_cjson =
    cJSON_GetObjectItem(directory, CJSON_NPIECES_MAX_ID);

  HARDBUG(!cJSON_IsNumber(value_cjson));

  ivalue = round(cJSON_GetNumberValue(value_cjson));

  HARDBUG(ivalue > 40)

  neural->neural_npieces_max = ivalue;

  if (verbose)
  {
    PRINTF("npieces_max=%d\n", neural->neural_npieces_max);
  }

  //set input_map

  for (int i = 0; i < BOARD_MAX; i++)
    neural->white_man_input_map[i] = INVALID;
  for (int i = 0; i < BOARD_MAX; i++)
    neural->white_crown_input_map[i] = INVALID;
  for (int i = 0; i < BOARD_MAX; i++)
    neural->black_man_input_map[i] = INVALID;
  for (int i = 0; i < BOARD_MAX; i++)
    neural->black_crown_input_map[i] = INVALID;
  for (int i = 0; i < BOARD_MAX; i++)
    neural->empty_input_map[i] = INVALID;

  neural->c2m_input_map = INVALID;

  int ninputs = 0;

  if (neural->neural_input_map == NEURAL_INPUT_MAP_V2)
  {
    for (int i = 6; i <= 50; ++i)
      neural->white_man_input_map[map[i]] = ninputs++;
  
    for (int i = 1; i <= 45; ++i)
      neural->black_man_input_map[map[i]] = ninputs++;

    for (int i = 1; i <= 50; ++i)
      neural->white_crown_input_map[map[i]] = ninputs++;
  
    for (int i = 1; i <= 50; ++i)
      neural->black_crown_input_map[map[i]] = ninputs++;

    neural->c2m_input_map = ninputs++;
  } 
  else if (neural->neural_input_map == NEURAL_INPUT_MAP_V5)
  {
    for (int i = 1; i <= 50; ++i)
      neural->white_man_input_map[map[i]] = ninputs++;
  
    for (int i = 1; i <= 50; ++i)
      neural->black_man_input_map[map[i]] = ninputs++;

    for (int i = 1; i <= 50; ++i)
      neural->white_crown_input_map[map[i]] = ninputs++;
  
    for (int i = 1; i <= 50; ++i)
      neural->black_crown_input_map[map[i]] = ninputs++;

    for (int i = 1; i <= 50; ++i)
      neural->empty_input_map[map[i]] = ninputs++;

    neural->c2m_input_map = ninputs++;
  } 
  else if (neural->neural_input_map == NEURAL_INPUT_MAP_V6)
  {
    for (int i = 1; i <= 50; ++i)
      neural->white_man_input_map[map[i]] = ninputs++;
  
    for (int i = 1; i <= 50; ++i)
      neural->black_man_input_map[map[i]] = ninputs++;

    for (int i = 1; i <= 50; ++i)
      neural->empty_input_map[map[i]] = ninputs++;

    neural->c2m_input_map = ninputs++;
  } 
  else
    FATAL("unknown input_map option", EXIT_FAILURE)

  HARDBUG(ninputs >= NINPUTS_MAX)

  PRINTF("ninputs=%d\n", ninputs);

  int nleft = ninputs % 16;
 
  if (nleft > 0) ninputs += 16 - nleft;

  PRINTF("ninputs=%d\n", ninputs);
    
  PRINTF("white_man_input_map\n");
  print_input_map(BOARD_MAX, neural->white_man_input_map, TRUE);

  PRINTF("white_crown_input_map\n");
  print_input_map(BOARD_MAX, neural->white_crown_input_map, TRUE);

  PRINTF("black_man_input_map\n");
  print_input_map(BOARD_MAX, neural->black_man_input_map, TRUE);

  PRINTF("black_crown_input_map\n");
  print_input_map(BOARD_MAX, neural->black_crown_input_map, TRUE);

  PRINTF("empty_input_map\n");
  print_input_map(BOARD_MAX, neural->empty_input_map, TRUE);

  PRINTF("c2m_input_map=%d\n", neural->c2m_input_map);

  neural->neural_nlayers = 0;

  while(TRUE)
  {
    char weights[MY_LINE_MAX];

    snprintf(weights, MY_LINE_MAX, "weights%d.csv", neural->neural_nlayers);

    cJSON *file = open_cjson_file(directory, weights);

    if (file == NULL) break;

    HARDBUG(neural->neural_nlayers >= NLAYERS_MAX)

    layer_t *with = neural->neural_layers + neural->neural_nlayers;

    with->layer_ninputs = with->layer_noutputs = INVALID;

    read_2d_csv(file, FALSE,
      &(with->layer_ninputs), &(with->layer_noutputs),
      &(with->layer_weights), &(with->layer_weights_transposed),
      &(with->layer_weights_double), &(with->layer_weights_transposed_double));

    char bias[MY_LINE_MAX];

    snprintf(bias, MY_LINE_MAX, "bias%d.csv", neural->neural_nlayers);

    file = open_cjson_file(directory, bias);

    HARDBUG(file == NULL)

    read_1d_csv(file, &(with->layer_noutputs), &(with->layer_bias),
      &(with->layer_bias_double));

    neural->neural_nlayers++;
  }

  if (verbose) PRINTF("nlayers=%d\n", neural->neural_nlayers);

  HARDBUG(neural->neural_nlayers < 3)

  neural->nlayer0_sum = 0;

  MALLOC(neural->layer0_sum, scaled_double_t *, NODE_MAX)

  for (int inode = 0; inode < NODE_MAX; inode++)
  {
    layer_t *with = neural->neural_layers;

    MALLOC(neural->layer0_sum[inode], scaled_double_t,
           with->layer_noutputs)
  }

  //topology

  for (int ilayer = 0; ilayer < neural->neural_nlayers; ilayer++)
  {
    layer_t *with = neural->neural_layers + ilayer;

    if (verbose)
    {
      PRINTF("ilayer=%d (inputs, outputs)=(%d, %d)\n",
        ilayer, with->layer_ninputs, with->layer_noutputs);
    }

    if (ilayer == 0)
      with->layer_sum = neural->layer0_sum[0];
    else
      MALLOC(with->layer_sum, scaled_double_t, with->layer_noutputs);

    MALLOC(with->layer_outputs, scaled_double_t, with->layer_noutputs);
    MALLOC(with->layer_sum_double, double, with->layer_noutputs);
    MALLOC(with->layer_outputs_double, double, with->layer_noutputs);
  }
 
  //check topology

  for (int ilayer = 0; ilayer < neural->neural_nlayers - 1; ilayer++)
  {
    layer_t *with_current = neural->neural_layers + ilayer;
    layer_t *with_next = neural->neural_layers + ilayer + 1;
   
    HARDBUG(with_current->layer_noutputs != with_next->layer_ninputs)
  }
}

local scaled_double_t dot_scaled(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b, int scale)
{
  BEGIN_BLOCK(__FUNC__)

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  i64_t s = 0;
#else
  i32_t s = 0;
#endif

#ifdef USE_AVX2_INTRINSICS

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    const __m128i ta = _mm_load_si128((__m128i*)a);
    const __m128i tb = _mm_load_si128((__m128i*)b);
    __m128i ts = _mm_madd_epi16(ta, tb);

    i32_t result[4];

    _mm_store_si128((__m128i*)result, ts);

     s = result[0] + result[1] + result[2] + result[3];
  }
  else
  {
    SOFTBUG((n % 16) != 0)

    __m256i ts = _mm256_setzero_si256();
  
    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      const __m256i ta = _mm256_load_si256((__m256i*)a);
      const __m256i tb = _mm256_load_si256((__m256i*)b);
      ts = _mm256_add_epi32(ts, _mm256_madd_epi16(ta, tb));
    }

    i32_t result[8];

    _mm256_store_si256((__m256i*)result, ts);

    s = result[0] + result[1] + result[2] + result[3] +
        result[4] + result[5] + result[6] + result[7];
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  __m256i ts = _mm256_setzero_si256();

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);
    ts = _mm256_add_epi32(ts, _mm256_mullo_epi32(ta, tb));
  }

  i32_t result[8];

  _mm256_store_si256((__m256i*)result, ts);

  s = result[0] + result[1] + result[2] + result[3] +
      result[4] + result[5] + result[6] + result[7];

#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++)
    s += a[i] * b[i];
#else
#error unknown SCALED_DOUBLE_T
#endif
#else
  for (int i = 0; i < n; i++)
    s += a[i] * b[i];
#endif

  END_BLOCK

  //if (scale) s = round(SCALED2DOUBLE(s));
  //if (scale) s = (s + (SCALED_DOUBLE_FACTOR / 2)) / SCALED_DOUBLE_FACTOR;
  if (scale) s /= SCALED_DOUBLE_FACTOR;

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

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    __m128i ta = _mm_load_si128((__m128i*)a);
    _mm_store_si128((__m128i*)b, ta);
  }
  else
  {
    SOFTBUG((n % 16) != 0)

    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      __m256i ta = _mm256_load_si256((__m256i*)a);
      _mm256_store_si256((__m256i*)b, ta);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);
    _mm256_store_si256((__m256i*)b, ta);
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++) b[i] = a[i];
#else
#error unknown SCALED_DOUBLE_T
#endif

#else
  for (int i = 0; i < n; i++) b[i] = a[i];
#endif
}

local void vadd_aba(int n, scaled_double_t *restrict a,
  scaled_double_t *restrict b)
{
  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    const __m128i ta = _mm_load_si128((__m128i*)a);
    const __m128i tb = _mm_load_si128((__m128i*)b);
    __m128i ts = _mm_madd_epi16(ta, tb);
    _mm_store_si128((__m128i*)a, ts);
  }
  else
  {
    SOFTBUG((n % 16) != 0)

    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      const __m256i ta = _mm256_load_si256((__m256i*)a);
      const __m256i tb = _mm256_load_si256((__m256i*)b);
      const __m256i ts = _mm256_add_epi16(ta, tb);
      _mm256_store_si256((__m256i*)a, ts);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);
    const __m256i ts = _mm256_add_epi32(ta, tb);
    _mm256_store_si256((__m256i*)a, ts);
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++) a[i] += b[i];
#else
#error unknown SCALED_DOUBLE_T
#endif
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

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    const __m128i ta = _mm_load_si128((__m128i*)a);
    const __m128i tb = _mm_load_si128((__m128i*)b);
    __m128i ts = _mm_sub_epi16(ta, tb);
    _mm_store_si128((__m128i*)a, ts);
  }
  else
  {
    SOFTBUG((n % 16) != 0)

    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      const __m256i ta = _mm256_load_si256((__m256i*)a);
      const __m256i tb = _mm256_load_si256((__m256i*)b);
      const __m256i ts = _mm256_sub_epi16(ta, tb);
      _mm256_store_si256((__m256i*)a, ts);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    const __m256i ta = _mm256_load_si256((__m256i*)a);
    const __m256i tb = _mm256_load_si256((__m256i*)b);
    const __m256i ts = _mm256_sub_epi32(ta, tb);
    _mm256_store_si256((__m256i*)a, ts);
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++) a[i] -= b[i];
#else
#error unknown SCALED_DOUBLE_T
#endif
#else
  for (int i = 0; i < n; i++) a[i] -= b[i];
#endif

  END_BLOCK
}

local void vadd_acba(int n, scaled_double_t *restrict a,
  scaled_double_t c, scaled_double_t *restrict b)
{

  BEGIN_BLOCK(__FUNC__)

#ifdef USE_AVX2_INTRINSICS

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    __m128i tc = _mm_set1_epi16(c);

    const __m128i ta = _mm_load_si128((__m128i*)a);
    const __m128i tb = _mm_load_si128((__m128i*)b);

    __m128i tm = _mm_mullo_epi16(tb, tc);
    __m128i ts = _mm_madd_epi16(ta, tm);
    _mm_store_si128((__m128i*)a, ts);
  }
  else
  {
    SOFTBUG((n % 16) != 0)

    __m256i tc = _mm256_set1_epi16(c);

    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      const __m256i ta = _mm256_load_si256((__m256i*)a);
      const __m256i tb = _mm256_load_si256((__m256i*)b);

      const __m256i tm = _mm256_mullo_epi16(tb, tc);
      const __m256i ts = _mm256_add_epi16(ta, tm);
      _mm256_store_si256((__m256i*)a, ts);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
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
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++) a[i] += c * b[i];
#else
#error unknown SCALED_DOUBLE_T
#endif

#else
  for (int i = 0; i < n; i++) a[i] += c * b[i];
#endif

  END_BLOCK
}

local void activation_relu_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    __m128i zero = _mm_setzero_si128();
    __m128i ta = _mm_load_si128((__m128i*)a);
    ta = _mm_max_epi16(ta, zero);
    _mm_store_si128((__m128i*)b, ta);
  } 
  else
  {
    SOFTBUG((n % 16) != 0)

    __m256i zero = _mm256_setzero_si256();
  
    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      __m256i ta = _mm256_load_si256((__m256i*)a);
      ta = _mm256_max_epi16(ta, zero);
      _mm256_store_si256((__m256i*)b, ta);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  __m256i zero = _mm256_setzero_si256();

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);
    ta = _mm256_max_epi32(ta, zero);
    _mm256_store_si256((__m256i*)b, ta);
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0)
      b[i] = 0;
    else
      b[i] = a[i];
  }
#else
#error unknown SCALED_DOUBLE_T
#endif

#else
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0)
      b[i] = 0;
    else
      b[i] = a[i];
  }
#endif
}

local void activation_clipped_relu_scaled(int n,
  scaled_double_t *restrict a, scaled_double_t *restrict b)
{
#ifdef USE_AVX2_INTRINSICS

#if SCALED_DOUBLE_T == SCALED_DOUBLE_I16_T
  if (n == 8)
  {
    __m128i zero = _mm_setzero_si128();
    __m128i clip = _mm_set1_epi16(SCALED_DOUBLE_FACTOR);

    __m128i ta = _mm_load_si128((__m128i*)a);

    ta = _mm_max_epi16(ta, zero);
    ta = _mm_min_epi16(ta, clip);

    _mm_store_si128((__m128i*)b, ta);
  } 
  else
  {
    SOFTBUG((n % 16) != 0)

    __m256i zero = _mm256_setzero_si256();
    __m256i clip = _mm256_set1_epi16(SCALED_DOUBLE_FACTOR);
  
    for (const i16_t *z = a + n ; a < z; a += 16, b += 16)
    {
      __m256i ta = _mm256_load_si256((__m256i*)a);

      ta = _mm256_max_epi16(ta, zero);
      ta = _mm256_min_epi16(ta, clip);

      _mm256_store_si256((__m256i*)b, ta);
    }
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I32_T
  SOFTBUG((n % 8) != 0)

  __m256i zero = _mm256_setzero_si256();
  __m256i clip = _mm256_set1_epi32(SCALED_DOUBLE_FACTOR);

  for (const i32_t *z = a + n ; a < z; a += 8, b += 8)
  {
    __m256i ta = _mm256_load_si256((__m256i*)a);

    ta = _mm256_max_epi32(ta, zero);
    ta = _mm256_min_epi32(ta, clip);

    _mm256_store_si256((__m256i*)b, ta);
  }
#elif SCALED_DOUBLE_T == SCALED_DOUBLE_I64_T
  SOFTBUG((n % 4) != 0)

  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0)
      b[i] = 0;
    else if (a[i] >= SCALED_DOUBLE_FACTOR)
      b[i] = SCALED_DOUBLE_FACTOR;
    else
      b[i] = a[i];
  }
#else
#error unknown SCALED_DOUBLE_T
#endif

#else
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0)
      b[i] = 0;
    else if (a[i] >= SCALED_DOUBLE_FACTOR)
      b[i] = SCALED_DOUBLE_FACTOR;
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
  double *restrict a, double *restrict b)
{
  for (int i = 0; i < n; i++)
  {
    if (a[i] <= 0.0)
      b[i] = 0.0;
    else if (a[i] >= 1.0)
      b[i] = 1.0;
    else
      b[i] = a[i];
  }
}

local double return_sigmoid(double x)
{
  return(x);
  return(2.0 / (1.0 + exp(-x)) - 1.0);
}

double return_neural_score_scaled(neural_t *neural, int with_sigmoid,
  int skip_inputs)
{
  BEGIN_BLOCK(__func__)

  //first layer

  layer_t *with_current = neural->neural_layers;

  int ninputs = with_current->layer_ninputs;

  BEGIN_BLOCK("Ax+b-and-activation-first-layer")

  if (!skip_inputs)
  {
    for (int i = 0; i < with_current->layer_noutputs; i++)
    {
      with_current->layer_sum[i] = 
        dot_scaled(with_current->layer_ninputs,
                   neural->neural_inputs,
                   with_current->layer_weights[i], FALSE) +
        with_current->layer_bias[i];
    }

    double inputs_double[NINPUTS_MAX];

    for (int i = 0; i < ninputs; i++) inputs_double[i] =
      neural->neural_inputs[i];

    for (int i = 0; i < with_current->layer_noutputs; i++)
    {
      with_current->layer_sum_double[i] = 
        dot_double(with_current->layer_ninputs,
                   inputs_double,
                   with_current->layer_weights_double[i]) +
        with_current->layer_bias_double[i];

      with_current->layer_sum[i] = 
        DOUBLE2SCALED(with_current->layer_sum_double[i]);
    }
  }
  
  BEGIN_BLOCK("Ax+b-and-activation-first-layer-activation")

  if (neural->neural_activation == NEURAL_ACTIVATION_RELU)
  {
    activation_relu_scaled(with_current->layer_noutputs,
      with_current->layer_sum, with_current->layer_outputs);
  }
  else if (neural->neural_activation == NEURAL_ACTIVATION_CLIPPED_RELU)
  {
    activation_clipped_relu_scaled(with_current->layer_noutputs,
      with_current->layer_sum, with_current->layer_outputs);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  END_BLOCK

  END_BLOCK

  for (int ilayer = 1; ilayer < neural->neural_nlayers; ilayer++)
  {
    with_current = neural->neural_layers + ilayer;

    layer_t *with_previous = neural->neural_layers + ilayer - 1;

    BEGIN_BLOCK("Ax+b-and-activation-other-layers")

    for (int i = 0; i < with_current->layer_noutputs; i++)
    {
      with_current->layer_sum[i] = 
        dot_scaled(with_current->layer_ninputs,
                   with_previous->layer_outputs,
                   with_current->layer_weights[i], TRUE) +
        with_current->layer_bias[i];
    }

    if (ilayer < (neural->neural_nlayers - 1))
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-activation")

      if (neural->neural_activation == NEURAL_ACTIVATION_RELU)
      {
        activation_relu_scaled(with_current->layer_noutputs,
          with_current->layer_sum, with_current->layer_outputs);
      }
      else if (neural->neural_activation == NEURAL_ACTIVATION_CLIPPED_RELU)
      {
        activation_clipped_relu_scaled(with_current->layer_noutputs,
          with_current->layer_sum, with_current->layer_outputs);
      }
      else
        FATAL("unknown activation option", EXIT_FAILURE)

      END_BLOCK
    }
    else
    {
      BEGIN_BLOCK("Ax+b-and-activation-other-layers-last-layer")

      for (int i = 0; i < with_current->layer_noutputs; i++)
      {
        if (neural->neural_activation_last == NEURAL_ACTIVATION_LINEAR)
        {
          with_current->layer_outputs_double[i] =
            SCALED2DOUBLE(with_current->layer_sum[i]);
        }
        else if (neural->neural_activation_last == NEURAL_ACTIVATION_SIGMOID)
        {
          with_current->layer_outputs_double[i] =
            return_sigmoid(SCALED2DOUBLE(with_current->layer_sum[i]));
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }

      END_BLOCK
    }

    END_BLOCK
  }
 
  END_BLOCK

  layer_t *with = neural->neural_layers + neural->neural_nlayers - 1;

  if (with_sigmoid)
    return(with->layer_outputs_double[0]);
  else
    return(SCALED2DOUBLE(with->layer_sum[0]));
}

double return_neural_score_double(neural_t *neural, int with_sigmoid)
{
  double inputs_double[NINPUTS_MAX];

  layer_t *with_current = neural->neural_layers;

  int ninputs = with_current->layer_ninputs;

  for (int i = 0; i < ninputs; i++) inputs_double[i] = neural->neural_inputs[i];

  for (int i = 0; i < with_current->layer_noutputs; i++)
  {
    with_current->layer_sum_double[i] = 
      dot_double(with_current->layer_ninputs,
                 inputs_double,
                 with_current->layer_weights_double[i]) +
      with_current->layer_bias_double[i];
  }

  if (neural->neural_activation == NEURAL_ACTIVATION_RELU)
  {
    activation_relu_double(with_current->layer_noutputs,
      with_current->layer_sum_double, with_current->layer_outputs_double);
  }
  else if (neural->neural_activation == NEURAL_ACTIVATION_CLIPPED_RELU)
  {
    activation_clipped_relu_double(with_current->layer_noutputs,
      with_current->layer_sum_double, with_current->layer_outputs_double);
  }
  else
    FATAL("unknown activation option", EXIT_FAILURE)

  for (int ilayer = 1; ilayer < neural->neural_nlayers; ilayer++)
  {
    with_current = neural->neural_layers + ilayer;

    layer_t *with_previous = neural->neural_layers + ilayer - 1;

    for (int i = 0; i < with_current->layer_noutputs; i++)
    {
      with_current->layer_sum_double[i] = 
        dot_double(with_current->layer_ninputs,
                   with_previous->layer_outputs_double,
                   with_current->layer_weights_double[i]) +
        with_current->layer_bias_double[i];
    }

    if (ilayer < (neural->neural_nlayers - 1))
    {
      if (neural->neural_activation == NEURAL_ACTIVATION_RELU)
      {
        activation_relu_double(with_current->layer_noutputs,
          with_current->layer_sum_double, with_current->layer_outputs_double);
      }
      else if (neural->neural_activation == NEURAL_ACTIVATION_CLIPPED_RELU)
      {
        activation_clipped_relu_double(with_current->layer_noutputs,
          with_current->layer_sum_double, with_current->layer_outputs_double);
      }
      else
        FATAL("unknown activation option", EXIT_FAILURE)
    }
    else
    {
      for (int i = 0; i < with_current->layer_noutputs; i++)
      {
        if (neural->neural_activation_last == NEURAL_ACTIVATION_LINEAR)
        {
          with_current->layer_outputs_double[i] =
            with_current->layer_sum_double[i];
        }
        else if (neural->neural_activation_last == NEURAL_ACTIVATION_SIGMOID)
        {
          with_current->layer_outputs_double[i] =
            return_sigmoid(with_current->layer_sum_double[i]);
        }
        else
          FATAL("unknown activation_last option", EXIT_FAILURE)
      }
    }
  }

  layer_t *with = neural->neural_layers + neural->neural_nlayers - 1;

  if (with_sigmoid)
    return(with->layer_outputs_double[0]);
  else
    return(with->layer_sum_double[0]);
}

void copy_first_layer_sum(neural_t *neural)
{
  layer_t *with = neural->neural_layers;

  HARDBUG(with->layer_sum != neural->layer0_sum[neural->nlayer0_sum])

  neural->nlayer0_sum++;

  HARDBUG(neural->nlayer0_sum >= NODE_MAX)

  vcopy_ab(with->layer_noutputs, with->layer_sum,
           neural->layer0_sum[neural->nlayer0_sum]);

  with->layer_sum = neural->layer0_sum[neural->nlayer0_sum];
}

void restore_first_layer_sum(neural_t *neural)
{
  neural->nlayer0_sum--;

  HARDBUG(neural->nlayer0_sum < 0)

  layer_t *with = neural->neural_layers;

  with->layer_sum = neural->layer0_sum[neural->nlayer0_sum];
}

void check_first_layer_sum(neural_t *neural)
{
  layer_t *with_current = neural->neural_layers;

  for (int i = 0; i < with_current->layer_noutputs; i++)
  {
    scaled_double_t layer_sum = 
      dot_scaled(with_current->layer_ninputs,
                 neural->neural_inputs,
                 with_current->layer_weights[i], FALSE) +
      with_current->layer_bias[i];

    if (llabs(layer_sum - with_current->layer_sum[i]) > 10)
    {
      PRINTF("layer_sum=%lld with-current->layer_sum[i]=%lld\n",
       (i64_t) layer_sum, (i64_t) with_current->layer_sum[i]);

      FATAL("DEBUG with SCALED_DOUBLE_I64_T", EXIT_FAILURE)
    }
  }
}

void update_first_layer(neural_t *neural, int j, scaled_double_t v)
{
  if (options.material_only) return;

  layer_t *with_current = neural->neural_layers;

  if (v == 1)
  {
    vadd_aba(with_current->layer_noutputs,
      with_current->layer_sum,
      with_current->layer_weights_transposed[j]);
  }
  else if (v == -1)
  {
    vsub_aba(with_current->layer_noutputs,
      with_current->layer_sum,
      with_current->layer_weights_transposed[j]);
  }
  else 
  {
    vadd_acba(with_current->layer_noutputs,
      with_current->layer_sum, v,
      with_current->layer_weights_transposed[j]);
  }

  neural->neural_inputs[j] += v;
}

void board2neural(board_t *with, neural_t *neural, int debug)
{
  scaled_double_t inputs[NINPUTS_MAX];

  int ninputs = neural->neural_layers[0].layer_ninputs;

  HARDBUG(ninputs >= NINPUTS_MAX)

  if (debug)
  {
    for (int input = 0; input < ninputs; input++)
      inputs[input] = neural->neural_inputs[input];
  }

  for (int input = 0; input < ninputs; input++)
    neural->neural_inputs[input] = 0;

  ui64_t white_bb = with->board_white_man_bb | with->board_white_crown_bb;
  ui64_t black_bb = with->board_black_man_bb | with->board_black_crown_bb;

  ui64_t empty_bb = with->board_empty_bb & ~(white_bb | black_bb);

  for (int i = 1; i <= 50; ++i)
  {
    int isquare = map[i];
             
    if (with->board_white_man_bb & BITULL(isquare))
      neural->neural_inputs[neural->white_man_input_map[isquare]] = 1;

    if (with->board_black_man_bb & BITULL(isquare))
      neural->neural_inputs[neural->black_man_input_map[isquare]] = 1;

    if ((neural->neural_input_map == NEURAL_INPUT_MAP_V2) or
        (neural->neural_input_map == NEURAL_INPUT_MAP_V5))
    {
      if (with->board_white_crown_bb & BITULL(isquare))
        neural->neural_inputs[neural->white_crown_input_map[isquare]] = 1;

      if (with->board_black_crown_bb & BITULL(isquare))
        neural->neural_inputs[neural->black_crown_input_map[isquare]] = 1;
    }
    if ((neural->neural_input_map == NEURAL_INPUT_MAP_V5) or
        (neural->neural_input_map == NEURAL_INPUT_MAP_V6))
    {
      if (empty_bb & BITULL(isquare))
        neural->neural_inputs[neural->empty_input_map[isquare]] = 1;
    }
  }

  HARDBUG(neural->neural_colour_bits != NEURAL_COLOUR_BITS_0)

  neural->nlayer0_sum = 0;

  neural->neural_layers[0].layer_sum = neural->layer0_sum[0];

  if (debug)
  {
    int error = FALSE;

    for (int input = 0; input < ninputs; input++)
    {
      if (inputs[input] != neural->neural_inputs[input])
      {
        PRINTF("input=%d inputs[input]=%lld neural_inputs[input]=%lld\n",
          input,
          (i64_t) inputs[input],
          (i64_t) neural->neural_inputs[input]);

        error = TRUE;
      }
    } 
    if (error) FATAL("eh?", 1)
  }
}

local void heap_sort_double(i64_t n, double *a)
{
  i64_t i, j, k;
  double t;

  if (n < 2) return;
  k = n / 2;
  --n;

  while(TRUE)
  {
    if (k > 0)
    {
      t = a[--k];
    }
    else
    {
      t = a[n];
      a[n] = a[0];
      if (--n == 0)
      {
        a[0] = t;
        break;
      }
    }
    i = k;
    j = 2 * k + 1;
    while(j <= n)
    {
      if ((j < n) and (a[j] < a[j + 1])) j++;
      if (t < a[j])
      {  
        a[i] = a[j];
        i = j;
        j = 2 * j + 1;
      }
      else
      {
        j = n + 1;
      }
    }  
    a[i] = t;
  }
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

void fen2neural(char *arg_name)
{
#ifdef USE_OPENMPI
  HARDBUG(my_mpi_globals.MY_MPIG_nslaves < 1)

  int iboard = create_board(STDOUT, INVALID);
  board_t *with = return_with_board(iboard);

  char line[MY_LINE_MAX];

  //determine records

  FILE *fname;
  
  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  i64_t nfen = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL) nfen++;

  FCLOSE(fname)

  PRINTF("nfen=%lld\n", nfen);

  i64_t nfen_mpi = 0;

  i64_t nwon = 0;
  i64_t nlost = 0;

  i64_t nw2m_label_won_mat_score_white_won = 0;
  i64_t nw2m_label_won_mat_score_white_lost = 0;
  i64_t nw2m_label_lost_mat_score_white_won = 0;
  i64_t nw2m_label_lost_mat_score_white_lost = 0;

  i64_t nb2m_label_won_mat_score_white_won = 0;
  i64_t nb2m_label_won_mat_score_white_lost = 0;
  i64_t nb2m_label_lost_mat_score_white_won = 0;
  i64_t nb2m_label_lost_mat_score_white_lost = 0;

  i64_t nw2m_result_pos = 0;
  i64_t nw2m_result_neg = 0;
  i64_t nb2m_result_pos = 0;
  i64_t nb2m_result_neg = 0;

  i64_t nw2m_neural_score_scaled_pos = 0;
  i64_t nw2m_neural_score_scaled_neg = 0;
  i64_t nb2m_neural_score_scaled_pos = 0;
  i64_t nb2m_neural_score_scaled_neg = 0;

  i64_t nw2m_sigmoid_score_scaled_pos = 0;
  i64_t nw2m_sigmoid_score_scaled_neg = 0;
  i64_t nb2m_sigmoid_score_scaled_pos = 0;
  i64_t nb2m_sigmoid_score_scaled_neg = 0;

  i64_t nw2m_result_pos_sigmoid_score_scaled_neg = 0;
  i64_t nw2m_result_neg_sigmoid_score_scaled_pos = 0;

  i64_t nb2m_result_pos_sigmoid_score_scaled_neg = 0;
  i64_t nb2m_result_neg_sigmoid_score_scaled_pos = 0;

  i64_t *npieces_fen;

  MALLOC(npieces_fen, i64_t, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    npieces_fen[ifen] = 0;

  double *delta_scaled_fen, *delta_double_fen;

  MALLOC(delta_scaled_fen, double, nfen)
  MALLOC(delta_double_fen, double, nfen)

  for (i64_t ifen = 0; ifen < nfen; ifen++)
    delta_scaled_fen[ifen] = delta_double_fen[ifen] = 0.0;

  double mse_vs_draw_scaled = 0.0;
  double mse_vs_draw_double = 0.0;
  double mse_scaled = 0.0;
  double mse_double = 0.0;

  double error_max_scaled_vs_double = 0.0;

  i64_t nfactor = 0;
  double factor = 0.0;

  i64_t nneural2mat_score_gt_score_man = 0;

  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  i64_t ifen = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '#') continue;

    if ((ifen % my_mpi_globals.MY_MPIG_nslaves) !=
        my_mpi_globals.MY_MPIG_id_slave)
    {
      ifen++;
      continue;
    }

    nfen_mpi++;

#ifdef DEBUG
    if ((nfen_mpi % 1000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
    if (nfen_mpi == 10000) break;
#else
    if ((nfen_mpi % 100000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
#endif

    char fen[MY_LINE_MAX];
    double result;

    HARDBUG(sscanf(line, "%s {%lf", fen, &result) != 2)

    fen2board(fen, with->board_id);

    int nwhite_man = BIT_COUNT(with->board_white_man_bb);
    int nblack_man = BIT_COUNT(with->board_black_man_bb);
    int nwhite_crown = BIT_COUNT(with->board_white_crown_bb);
    int nblack_crown = BIT_COUNT(with->board_black_crown_bb);

    int npieces = nwhite_man + nblack_man + nwhite_crown + nblack_crown;

    if (((npieces > with->board_neural0.neural_npieces_max) or
         (npieces < with->board_neural0.neural_npieces_min)))
    {
      ifen++;
      continue;
    }

    npieces_fen[ifen] = npieces;

    moves_list_t moves_list;

    create_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.nmoves <= 1) or (moves_list.ncaptx > 0))
    {
      ifen++;

      continue;
    }

    //HARDBUG(fabs(result) >= 0.999999)

    //check_moves(with, &moves_list);

    int mat_score = (nwhite_man - nblack_man) * SCORE_MAN +
                    (nwhite_crown - nblack_crown) * SCORE_CROWN;

    if (result >= 0.9)
    {
      nwon++;

      if (IS_WHITE(with->board_colour2move))
      {
        if (mat_score >= SCORE_MAN) nw2m_label_won_mat_score_white_won++;
        if (mat_score <= -SCORE_MAN) nw2m_label_won_mat_score_white_lost++;
      }
      else
      {
        if (mat_score >= SCORE_MAN) nb2m_label_won_mat_score_white_won++;
        if (mat_score <= -SCORE_MAN) nb2m_label_won_mat_score_white_lost++;
      }
    } 
    else if (result <= -0.9)
    {
      nlost++;

      if (IS_WHITE(with->board_colour2move))
      {
        if (mat_score >= SCORE_MAN) nw2m_label_lost_mat_score_white_won++;
        if (mat_score <= -SCORE_MAN) nw2m_label_lost_mat_score_white_lost++;
      }
      else
      {
        if (mat_score >= SCORE_MAN) nb2m_label_lost_mat_score_white_won++;
        if (mat_score <= -SCORE_MAN) nb2m_label_lost_mat_score_white_lost++;
      }
    }

    double neural_score_scaled =
      return_neural_score_scaled(&(with->board_neural0), FALSE, FALSE);

    double neural_score_double =
      return_neural_score_double(&(with->board_neural0), FALSE);

    double sigmoid_score_scaled =
      return_neural_score_scaled(&(with->board_neural0), TRUE, FALSE);

    double sigmoid_score_double =
      return_neural_score_double(&(with->board_neural0), TRUE);

    if (IS_BLACK(with->board_colour2move))
    {
      mat_score = -mat_score;

      if (with->board_neural0.neural_label == NEURAL_LABEL_W2M)
        result = -result;

      if (with->board_neural0.neural_output == NEURAL_OUTPUT_W2M)
      {
        neural_score_scaled = -neural_score_scaled;
        neural_score_double = -neural_score_double;
        sigmoid_score_scaled = -sigmoid_score_scaled;
        sigmoid_score_double = -sigmoid_score_double;
      }
    }

    if (ifen <= 100)
      PRINTF("%s {%.6f} ms=%d nss=%.6f nsd=%.6f sss=%.6f ssd=%.6f r=%.6f\n",
        fen, result, mat_score,
        neural_score_scaled, neural_score_double,
        sigmoid_score_scaled, sigmoid_score_double, result);

    if (IS_WHITE(with->board_colour2move))
    {
      if (result > 0.0) 
        nw2m_result_pos++;
      if (result < 0.0)
        nw2m_result_neg++;

      if (neural_score_scaled > 0.0) 
        nw2m_neural_score_scaled_pos++;
      if (neural_score_scaled < 0.0)
        nw2m_neural_score_scaled_neg++;

      if (sigmoid_score_scaled > 0.0) 
        nw2m_sigmoid_score_scaled_pos++;
      if (sigmoid_score_scaled < 0.0)
        nw2m_sigmoid_score_scaled_neg++;

      if ((result > 0.0) and (sigmoid_score_scaled < 0.0))
        nw2m_result_pos_sigmoid_score_scaled_neg++;
      if ((result < 0.0) and (sigmoid_score_scaled > 0.0))
        nw2m_result_neg_sigmoid_score_scaled_pos++;
    } 
    else
    {
      if (result > 0.0) 
        nb2m_result_pos++;
      else if (result < 0.0)
        nb2m_result_neg++;

      if (neural_score_scaled > 0.0) 
        nb2m_neural_score_scaled_pos++;
      if (neural_score_scaled < 0.0)
        nb2m_neural_score_scaled_neg++;

      if (sigmoid_score_scaled > 0.0) 
        nb2m_sigmoid_score_scaled_pos++;
      if (sigmoid_score_scaled < 0.0)
        nb2m_sigmoid_score_scaled_neg++;

      if ((result > 0.0) and (sigmoid_score_scaled < 0.0))
        nb2m_result_pos_sigmoid_score_scaled_neg++;
      if ((result < 0.0) and (sigmoid_score_scaled > 0.0))
        nb2m_result_neg_sigmoid_score_scaled_pos++;
    }

    delta_scaled_fen[ifen] = fabs(sigmoid_score_scaled - result);
    delta_double_fen[ifen] = fabs(sigmoid_score_double - result);

    mse_vs_draw_scaled += sigmoid_score_scaled * sigmoid_score_scaled;
    mse_vs_draw_double += sigmoid_score_double * sigmoid_score_double;

    mse_scaled += delta_scaled_fen[ifen] * delta_scaled_fen[ifen];
    mse_double += delta_double_fen[ifen] * delta_double_fen[ifen];

    double error = fabs(neural_score_scaled - neural_score_double);

    if (error > error_max_scaled_vs_double)
    {
      error_max_scaled_vs_double = error;

      PRINTF("%s {%.6f} ms=%d nss=%.6f nsd=%.6f sss=%.6f ssd=%.6f r=%.6f\n",
        fen, result, mat_score,
        neural_score_scaled, neural_score_double,
        sigmoid_score_scaled, sigmoid_score_double, result);

      PRINTF("error_max_scaled_vs_double=%.6f\n", error_max_scaled_vs_double);
    }

    if ((moves_list.ncaptx == 0) and
        (with->board_white_crown_bb == 0) and
        (with->board_black_crown_bb == 0) and
        (result > 0.0) and
        (sigmoid_score_scaled >= result) and
        ((mat_score >= SCORE_MAN) and (mat_score <= (3 * SCORE_MAN))))
    {    
      ++nfactor;

      factor += mat_score / neural_score_scaled;

      if (nfactor <= 100)
        PRINTF("nfactor=%lld factor=%.6f\n", nfactor, factor / nfactor);
    }

    if (nfactor > 10)
    {
      int neural2mat_score = round(neural_score_scaled * factor / nfactor);

      if (abs(neural2mat_score - mat_score) > SCORE_MAN)
      {  
        if (nneural2mat_score_gt_score_man <= 100)
          PRINTF("neural2mat_score=%d mat_score=%d\n",
                 neural2mat_score, mat_score);
        nneural2mat_score_gt_score_man++;
      }
    }

    ifen++;
  }
  FCLOSE(fname)

  PRINTF("nfen_mpi=%lld\n", nfen_mpi);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  MPI_Allreduce(MPI_IN_PLACE, &nwon, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nlost, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nw2m_label_won_mat_score_white_won, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_label_won_mat_score_white_lost, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_label_lost_mat_score_white_won, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_label_lost_mat_score_white_lost, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nb2m_label_won_mat_score_white_won, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_label_won_mat_score_white_lost, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_label_lost_mat_score_white_won, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_label_lost_mat_score_white_lost, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nw2m_result_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_result_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_result_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_result_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nw2m_neural_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_neural_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_neural_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_neural_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nw2m_sigmoid_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_sigmoid_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_sigmoid_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_sigmoid_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nw2m_result_pos_sigmoid_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nw2m_result_neg_sigmoid_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_result_pos_sigmoid_score_scaled_neg, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &nb2m_result_neg_sigmoid_score_scaled_pos, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, npieces_fen, nfen,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, delta_scaled_fen, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, delta_double_fen, nfen,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &mse_vs_draw_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &mse_vs_draw_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &mse_scaled, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &mse_double, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nneural2mat_score_gt_score_man, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &error_max_scaled_vs_double, 1,
    MPI_DOUBLE, MPI_MAX, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nfactor, 1,
    MPI_LONG_LONG_INT, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &factor, 1,
    MPI_DOUBLE, MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  PRINTF("nwon=%lld\n"
         "nlost=%lld\n", 
         nwon,
         nlost);

  PRINTF("nw2m_label_won_mat_score_white_won=%lld\n"
         "nw2m_label_won_mat_score_white_lost=%lld\n"
         "nw2m_label_lost_mat_score_white_won=%lld\n"
         "nw2m_label_lost_mat_score_white_lost=%lld\n",
         nw2m_label_won_mat_score_white_won,
         nw2m_label_won_mat_score_white_lost,
         nw2m_label_lost_mat_score_white_won,
         nw2m_label_lost_mat_score_white_lost);

  PRINTF("nb2m_label_won_mat_score_white_won=%lld\n"
         "nb2m_label_won_mat_score_white_lost=%lld\n"
         "nb2m_label_lost_mat_score_white_won=%lld\n"
         "nb2m_label_lost_mat_score_white_lost=%lld\n",
         nb2m_label_won_mat_score_white_won,
         nb2m_label_won_mat_score_white_lost,
         nb2m_label_lost_mat_score_white_won,
         nb2m_label_lost_mat_score_white_lost);

  PRINTF("nw2m_result_pos=%lld\n"
         "nw2m_result_neg=%lld\n"
         "nb2m_result_pos=%lld\n"
         "nb2m_result_neg=%lld\n",
         nw2m_result_pos,
         nw2m_result_neg,
         nb2m_result_pos,
         nb2m_result_neg);

  PRINTF("nw2m_neural_score_scaled_pos=%lld\n"
         "nw2m_neural_score_scaled_neg=%lld\n"
         "nb2m_neural_score_scaled_pos=%lld\n"
         "nb2m_neural_score_scaled_neg=%lld\n",
         nw2m_neural_score_scaled_pos,
         nw2m_neural_score_scaled_neg,
         nb2m_neural_score_scaled_pos,
         nb2m_neural_score_scaled_neg);

  PRINTF("nw2m_sigmoid_score_scaled_pos=%lld\n"
         "nw2m_sigmoid_score_scaled_neg=%lld\n"
         "nb2m_sigmoid_score_scaled_pos=%lld\n"
         "nb2m_sigmoid_score_scaled_neg=%lld\n",
         nw2m_sigmoid_score_scaled_pos,
         nw2m_sigmoid_score_scaled_neg,
         nb2m_sigmoid_score_scaled_pos,
         nb2m_sigmoid_score_scaled_neg);

  PRINTF("nw2m_result_pos_sigmoid_score_scaled_neg=%lld (%.6f %%)\n"
         "nw2m_result_neg_sigmoid_score_scaled_pos=%lld (%.6f %%)\n" 
         "nb2m_result_pos_sigmoid_score_scaled_neg=%lld (%.6f %%)\n"
         "nb2m_result_neg_sigmoid_score_scaled_pos=%lld (%.6f %%)\n", 
         nw2m_result_pos_sigmoid_score_scaled_neg,
         nw2m_result_pos_sigmoid_score_scaled_neg * 100.0 / nw2m_result_pos,
         nw2m_result_neg_sigmoid_score_scaled_pos,
         nw2m_result_neg_sigmoid_score_scaled_pos * 100.0 / nw2m_result_neg,
         nb2m_result_pos_sigmoid_score_scaled_neg,
         nb2m_result_pos_sigmoid_score_scaled_neg * 100.0 / nb2m_result_pos,
         nb2m_result_neg_sigmoid_score_scaled_pos,
         nb2m_result_neg_sigmoid_score_scaled_pos * 100.0 / nb2m_result_neg);

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

    for (ifen = 0; ifen < nfen; ifen++)
    {
      if (npieces_fen[ifen] != ipiece) continue;

      ndelta++;

      update_mean_sigma(ndelta, delta_scaled_fen[ifen],
        &delta_scaled_max, &delta_scaled_mean, &delta_scaled_sigma);

      update_mean_sigma(ndelta, delta_double_fen[ifen],
        &delta_double_max, &delta_double_mean, &delta_double_sigma);
    }
    PRINTF("ipiece=%d nd=%lld ds_max=%.6f ds_mean=%.6f ds_sigma=%.6f"
           " dd_max=%.6f dd_mean=%.6f dd_sigma=%.6f\n",
     ipiece, ndelta,
     delta_scaled_max, delta_scaled_mean, sqrt(delta_scaled_sigma / ndelta),
     delta_double_max, delta_double_mean, sqrt(delta_double_sigma / ndelta));
  }

  heap_sort_double(nfen, delta_scaled_fen);

  heap_sort_double(nfen, delta_double_fen);

  PRINTF("ds_max=%.6f dd_max=%.6f\n",
    delta_scaled_fen[nfen - 1], delta_double_fen[nfen - 1]);

  PRINTF("delta percentiles\n");

  for (int ipercentile = 1; ipercentile <= 99; ++ipercentile)
  {
    ifen = round(ipercentile / 100.0 * nfen);

    HARDBUG(ifen >= nfen)

    PRINTF("ipercentile=%d delta_scaled=%.6f delta_double=%.6f\n",
      ipercentile, delta_scaled_fen[ifen], delta_double_fen[ifen]);
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

  PRINTF("nneural2mat_score_gt_score_man=%lld\n",
         nneural2mat_score_gt_score_man);

  i64_t ipercentile = round(90 / 100.0 * nfen);
  double percentile = delta_scaled_fen[ipercentile];

  PRINTF("percentile=%.6f\n", percentile);

  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  ifen = 0;
  nfen_mpi = 0;
  i64_t nfen_skipped_mpi = 0;

  nfactor = 0;
  factor = 0.0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '#') continue;

    if ((ifen % my_mpi_globals.MY_MPIG_nslaves) != my_mpi_globals.MY_MPIG_id_slave)
    {
      ifen++;
      continue;
    }

    nfen_mpi++;

#ifdef DEBUG
    if ((nfen_mpi % 1000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
    if (nfen_mpi == 10000) break;
#else
    if ((nfen_mpi % 100000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
#endif

    char fen[MY_LINE_MAX];
    double result;

    HARDBUG(sscanf(line, "%s {%lf", fen, &result) != 2)

    fen2board(fen, with->board_id);

    moves_list_t moves_list;

    create_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.nmoves <= 1) or (moves_list.ncaptx > 0))
    {
      ifen++;
      continue;
    }

    //HARDBUG(fabs(result) >= 0.999999)

    //check_moves(with, &moves_list);

    int nwhite_crown = BIT_COUNT(with->board_white_crown_bb);
    int nblack_crown = BIT_COUNT(with->board_black_crown_bb);

    if ((nwhite_crown > 0) or (nblack_crown > 0))
    {
      nfen_skipped_mpi++;

      ifen++;
       
      continue;
    }
    
    int nwhite_man = BIT_COUNT(with->board_white_man_bb);
    int nblack_man = BIT_COUNT(with->board_black_man_bb);

    int delta_man = abs(nwhite_man - nblack_man);

    if (delta_man != 1)
    {
      nfen_skipped_mpi++;

      ifen++;
       
      continue;
    }

    double sigmoid_score_scaled =
      return_neural_score_scaled(&(with->board_neural0), TRUE, FALSE);

    if (fabs(sigmoid_score_scaled - result) > percentile)
    {
      nfen_skipped_mpi++;

      ifen++;
       
      continue;
    }
 
    double neural_score_scaled =
      return_neural_score_scaled(&(with->board_neural0), FALSE, FALSE);

    nfactor++;
    factor += fabs(neural_score_scaled);

    ifen++;
  }
  FCLOSE(fname)

  PRINTF("nfen_mpi=%lld nfen_skipped_mpi=%lld\n",
    nfen_mpi, nfen_skipped_mpi);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_skipped_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen_skipped=%lld\n", nfen_skipped_mpi);

  MPI_Allreduce(MPI_IN_PLACE, &nfactor, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  MPI_Allreduce(MPI_IN_PLACE, &factor, 1, MPI_DOUBLE,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);
  
 factor /= nfactor;

 double factor2man_score = SCORE_MAN / factor;

 PRINTF("nfactor=%lld factor=%.6f factor2man_score=%.6f\n",
   nfactor, factor, factor2man_score);
#endif
}

#define DEPTH_AB 64

void fen2search(char *arg_name)
{
  options.time_limit = 0.5;
  options.time_ntrouble = 0;
  options.use_reductions = FALSE;

#ifdef USE_OPENMPI
  HARDBUG(my_mpi_globals.MY_MPIG_nslaves < 1)

  int iboard = create_board(STDOUT, INVALID);
  board_t *with = return_with_board(iboard);

  char line[MY_LINE_MAX];

  //determine records

  FILE *fname;
  
  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  i64_t nfen = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL) nfen++;

  FCLOSE(fname)

  PRINTF("nfen=%lld\n", nfen);

  FILE *ffen;

  if (my_mpi_globals.MY_MPIG_nslaves == 1)
  {
    char fen[MY_LINE_MAX];

    snprintf(fen, MY_LINE_MAX, "ab.fen");

    HARDBUG((ffen = fopen(fen, "w")) == NULL)
  }
  else
  {
    char fen[MY_LINE_MAX];

    snprintf(fen, MY_LINE_MAX, "ab.fen;%d;%d",
      my_mpi_globals.MY_MPIG_id_slave, my_mpi_globals.MY_MPIG_nslaves);

    HARDBUG((ffen = fopen(fen, "w")) == NULL)

    ui64_t s = randull(INVALID);

    PRINTF("seed=%llu\n", s);
  }

  i64_t nfen_mpi = 0;

  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  i64_t ifen = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '#') continue;

    if ((ifen % my_mpi_globals.MY_MPIG_nslaves) !=
        my_mpi_globals.MY_MPIG_id_slave)
    {
      ifen++;
      continue;
    }

    nfen_mpi++;

#ifdef DEBUG
    if ((nfen_mpi % 1000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
    if (nfen_mpi == 10000) break;
#else
    if ((nfen_mpi % 100000) == 0) PRINTF("nfen_mpi=%lld\n", nfen_mpi);
#endif

    char fen[MY_LINE_MAX];
    double result;

    HARDBUG(sscanf(line, "%s {%lf", fen, &result) != 2)

    fen2board(fen, with->board_id);

    moves_list_t moves_list;

    create_moves_list(&moves_list);

    gen_moves(with, &moves_list, FALSE);

    HARDBUG(moves_list.nmoves <= 1)
   
    HARDBUG(moves_list.ncaptx > 0)

    reset_my_timer(&(with->board_timer));

    search(with, &moves_list, 1, DEPTH_AB,
      SCORE_MINUS_INFINITY, FALSE);

    int best_score = with->board_search_best_score;

    double best_score_white = SCORE2TANH(best_score);

    if (IS_BLACK(with->board_colour2move))
      best_score_white = -best_score_white;
    
    HARDBUG(fprintf(ffen, "%s {%.6f} best_score=%d result=%.6f\n",
                          fen, best_score_white, best_score, result) < 0)

    HARDBUG(fflush(ffen) != 0)
  }
  FCLOSE(fname)
  FCLOSE(ffen)

  PRINTF("nfen_mpi=%lld\n", nfen_mpi);

  my_mpi_barrier("after pos", my_mpi_globals.MY_MPIG_comm_slaves, TRUE);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)
#endif
}

#define TAG "no-capt"

#define NPACK 7

local void return_pack(board_t *with, int colour_bit, int kind_bit,
  uint8_t pack[NPACK])
{
  for (int ipack = 0; ipack < NPACK; ipack++)
    pack[ipack] = 0;

  ui64_t white_bb = with->board_white_man_bb | with->board_white_crown_bb;
  ui64_t black_bb = with->board_black_man_bb | with->board_black_crown_bb;
  ui64_t empty_bb = with->board_empty_bb & ~(white_bb | black_bb);

  ui64_t colour_bb;

  if (IS_WHITE(colour_bit))
    colour_bb = white_bb;
  else
    colour_bb = black_bb;

  ui64_t kind_bb;

  if (IS_MAN(kind_bit))
    kind_bb = with->board_white_man_bb | with->board_black_man_bb;
  else
    kind_bb = with->board_white_crown_bb | with->board_black_crown_bb;

  int input = 0;

  for (int i = 1; i <= 50; ++i)
  { 
    int iboard = map[i];

    if (((colour_bit != 0) and (kind_bit != 0) and
         (colour_bb & BITULL(iboard)) and
         (kind_bb & BITULL(iboard))) or
        ((colour_bit == 0) and (kind_bit == 0) and
         (empty_bb & BITULL(iboard))))
    {
      int ipack = input / 8;

      HARDBUG(ipack >= NPACK)

      int ibit = 7 - input % 8;

      pack[ipack] |= (1 << ibit);
    }
    input++;
  }
}

#define NBUFFER_MAX 1000000

local void flush_buffer(bstring *bfeatures_name, bstring *bfeatures_buffer,
  bstring *blabels_name, bstring *blabels_buffer)
{
  int fd = my_lock_file(bdata(*bfeatures_name));

  HARDBUG(fd == -1)

  HARDBUG(my_write(fd, bdata(*bfeatures_buffer), blength(*bfeatures_buffer)) !=
                   blength(*bfeatures_buffer))

  my_unlock_file(fd);

  HARDBUG(bassigncstr(*bfeatures_buffer, "") == BSTR_ERR)

  fd = my_lock_file(bdata(*blabels_name));

  HARDBUG(fd == -1)

  HARDBUG(my_write(fd, bdata(*blabels_buffer), blength(*blabels_buffer)) !=
                   blength(*blabels_buffer))

  my_unlock_file(fd);

  HARDBUG(bassigncstr(*blabels_buffer, "") == BSTR_ERR)
}

local void append_pack(board_t *with, int *nbuffer,
  bstring *bfeatures_name, bstring *bfeatures_buffer,
  bstring *blabels_name, bstring *blabels_buffer,
  int nwhite_man, int nwhite_crown, int nblack_man, int nblack_crown,
  i64_t nfen, double result)
{
  if (*nbuffer >= NBUFFER_MAX)
  {
    flush_buffer(bfeatures_name, bfeatures_buffer,
                 blabels_name, blabels_buffer);

    *nbuffer = 0;
  }

  uint8_t pack[NPACK];

  return_pack(with, WHITE_BIT, MAN_BIT, pack);
 
  for (int ipack = 0; ipack < NPACK; ipack++)
    bformata(*bfeatures_buffer, "%3u,", pack[ipack]);

  return_pack(with, BLACK_BIT, MAN_BIT, pack);

  for (int ipack = 0; ipack < NPACK; ipack++)
    bformata(*bfeatures_buffer, "%3u,", pack[ipack]);

  return_pack(with, WHITE_BIT, CROWN_BIT, pack);

  for (int ipack = 0; ipack < NPACK; ipack++)
    bformata(*bfeatures_buffer, "%3u,", pack[ipack]);

  return_pack(with, BLACK_BIT, CROWN_BIT, pack);

  for (int ipack = 0; ipack < NPACK; ipack++)
    bformata(*bfeatures_buffer, "%3u,", pack[ipack]);

  return_pack(with, 0, 0, pack);

  for (int ipack = 0; ipack < NPACK; ipack++)
    bformata(*bfeatures_buffer, "%3u,", pack[ipack]);

  if (IS_WHITE(with->board_colour2move))
    bformata(*bfeatures_buffer, "%3u,", 1);
  else
    bformata(*bfeatures_buffer, "%3u,", 0);

  bformata(*bfeatures_buffer, "%3u,%3u,%3u,%3u,%lld\n",
    nwhite_man, nblack_man, nwhite_crown, nblack_crown, nfen);

  bformata(*blabels_buffer, "%.6f\n", result);

  (*nbuffer)++;
}

typedef struct
{
  int stage_min;
  int stage_max;

  bstring stage_training_features_name;
  bstring stage_training_labels_name;

  int stage_training_nbuffer;
  bstring stage_training_features_buffer;
  bstring stage_training_labels_buffer;

  bstring stage_validation_features_name;
  bstring stage_validation_labels_name;

  int stage_validation_nbuffer;
  bstring stage_validation_features_buffer;
  bstring stage_validation_labels_buffer;
} stage_t;

#define NSTAGES 2

local stage_t stages[NSTAGES] =
{
  {17, 32, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL},
  {7, 22, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL}
};

void fen2csv(char *arg_name)
{
#ifdef USE_OPENMPI
 
  FILE *fname;

  HARDBUG((fname = fopen(arg_name, "r")) == NULL)

  for (int istage = 0; istage < NSTAGES; istage++)
  {
    stage_t *stage = stages + istage;

    stage->stage_training_features_name =
      bformat("%s-%s-%d-%d-training-features.csv", arg_name, TAG,
              stage->stage_min, stage->stage_max);
    HARDBUG(stage->stage_training_features_name == NULL)

    stage->stage_training_labels_name =
      bformat("%s-%s-%d-%d-training-labels.csv", arg_name, TAG,
              stage->stage_min, stage->stage_max);
    HARDBUG(stage->stage_training_labels_name == NULL)

    stage->stage_training_nbuffer = 0;
    HARDBUG((stage->stage_training_features_buffer = bfromcstr("")) == NULL)
    HARDBUG((stage->stage_training_labels_buffer = bfromcstr("")) == NULL)

    stage->stage_validation_features_name =
      bformat("%s-%s-%d-%d-validation-features.csv", arg_name, TAG,
              stage->stage_min, stage->stage_max);
    HARDBUG(stage->stage_validation_features_name == NULL)

    stage->stage_validation_labels_name =
      bformat("%s-%s-%d-%d-validation-labels.csv", arg_name, TAG,
              stage->stage_min, stage->stage_max);
    HARDBUG(stage->stage_validation_labels_name == NULL)

    stage->stage_validation_nbuffer = 0;

    HARDBUG((stage->stage_validation_features_buffer = bfromcstr("")) == NULL)
    HARDBUG((stage->stage_validation_labels_buffer = bfromcstr("")) == NULL)

    PRINTF("istage=%d stage_training_features_name=%s\n",
      istage, bdata(stage->stage_training_features_name));
    PRINTF("istage=%d stage_training_labels_name=%s\n",
      istage, bdata(stage->stage_training_labels_name));
    PRINTF("istage=%d stage_validation_features_name=%s\n",
      istage, bdata(stage->stage_validation_features_name));
    PRINTF("istage=%d stage_validation_labels_name=%s\n",
      istage, bdata(stage->stage_validation_labels_name));

    remove(bdata(stage->stage_training_features_name));
    remove(bdata(stage->stage_training_labels_name));
    remove(bdata(stage->stage_validation_features_name));
    remove(bdata(stage->stage_validation_labels_name));
  }

  int iboard = create_board(STDOUT, INVALID);
  board_t *with = return_with_board(iboard);

  char line[MY_LINE_MAX];

  i64_t nfen = 0;
  i64_t nfen_mpi = 0;
  i64_t nfen_skipped_mpi = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '#') continue;

    if ((nfen % 1000000) == 0) PRINTF("nfen=%lld\n", nfen);
   
    if ((nfen++ % my_mpi_globals.MY_MPIG_nslaves) !=
        my_mpi_globals.MY_MPIG_id_slave) continue;

    nfen_mpi++;

    char fen[MY_LINE_MAX];
    double result;
    int iply, nply;

    if (sscanf(line, "%s {%lf}%d%d", fen, &result, &iply, &nply) == 4)
    {
      fen2board(fen, with->board_id);
    }
    else if (sscanf(line, "%s {%lf}", fen, &result) == 2)
    {
      fen2board(fen, with->board_id);
    }
    else
    {
      HARDBUG(sscanf(line, "%s%lf%d%d", fen, &result, &iply, &nply) != 4)

      string2board(fen, with->board_id);
    }

    moves_list_t moves_list;

    gen_moves(with, &moves_list, FALSE);

    if ((moves_list.nmoves <= 1) or
        (moves_list.ncaptx > 0))
    {
      nfen_skipped_mpi++;

      continue;
    }

    int nwhite_man = BIT_COUNT(with->board_white_man_bb);
    int nblack_man = BIT_COUNT(with->board_black_man_bb);
    int nwhite_crown = BIT_COUNT(with->board_white_crown_bb);
    int nblack_crown = BIT_COUNT(with->board_black_crown_bb);

    int npieces = nwhite_man + nblack_man + nwhite_crown + nblack_crown;

    int istage;

    for (istage = 0; istage < NSTAGES; istage++)
    {
      stage_t *stage = stages + istage;

      if ((stage->stage_min == 41) and (npieces <= stage->stage_max) and
          (nwhite_crown == 0) and (nblack_crown == 0)) break;

      if ((stage->stage_min == 42) and (npieces <= stage->stage_max) and
          ((nwhite_crown > 0) or (nblack_crown > 0))) break;

      if ((npieces >= stage->stage_min) and (npieces <= stage->stage_max))
        break;
    }

    if (istage >= NSTAGES)
    { 
      nfen_skipped_mpi++;

      continue;
    }

    HARDBUG(istage >= NSTAGES)

    stage_t *stage = stages + istage;

    //HARDBUG(fabs(result) >= 0.999999)

    //scale the result
    //result = (result * iply) / nply;

    int prob = randull(0) % 100;

    if (prob <= 79) 
    {
      append_pack(with, &(stage->stage_training_nbuffer),
        &(stage->stage_training_features_name),
        &(stage->stage_training_features_buffer),
        &(stage->stage_training_labels_name),
        &(stage->stage_training_labels_buffer),
        nwhite_man, nwhite_crown, nblack_man, nblack_crown,
        nfen, result);
    }
    else
    {
      append_pack(with, &(stage->stage_validation_nbuffer),
        &(stage->stage_validation_features_name),
        &(stage->stage_validation_features_buffer),
        &(stage->stage_validation_labels_name),
        &(stage->stage_validation_labels_buffer),
        nwhite_man, nwhite_crown, nblack_man, nblack_crown,
        nfen, result);
    }
  }
  FCLOSE(fname)

  PRINTF("nfen_mpi=%lld nfen_skipped_mpi=%lld\n",
    nfen_mpi, nfen_skipped_mpi);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  MPI_Allreduce(MPI_IN_PLACE, &nfen_skipped_mpi, 1, MPI_LONG_LONG_INT,
    MPI_SUM, my_mpi_globals.MY_MPIG_comm_slaves);

  HARDBUG(nfen_mpi != nfen)

  PRINTF("nfen=%lld nfen_skipped=%lld\n", nfen, nfen_skipped_mpi);

  //write remaining

  for (int istage = 0; istage < NSTAGES; istage++)
  {
    stage_t *stage = stages + istage;

    if (stage->stage_training_nbuffer > 0)
    {
      flush_buffer(&(stage->stage_training_features_name),
                   &(stage->stage_training_features_buffer),
                   &(stage->stage_training_labels_name),
                   &(stage->stage_training_labels_buffer));
    } 
    if (stage->stage_validation_nbuffer > 0)
    {
      flush_buffer(&(stage->stage_validation_features_name),
                   &(stage->stage_validation_features_buffer),
                   &(stage->stage_validation_labels_name),
                   &(stage->stage_validation_labels_buffer));
    }
  }
#endif
}

void test_neural(void)
{ 
}

#define NEGTB 6
#define NPOS  1000000

void egtb2neural(void)
{
  int iboard = create_board(STDOUT, INVALID);
  board_t *with = return_with_board(iboard);

  double *delta_white;
  double *delta_black;

  MALLOC(delta_white, double, NPOS)
  MALLOC(delta_black, double, NPOS)

  for (i64_t ipos = 0; ipos < NPOS; ipos++)
    delta_white[ipos] = delta_black[ipos] = 0.0;

  int delta_max = 0;

  i64_t nw2m_won_draw = 0;
  i64_t nw2m_lost_draw = 0;
  i64_t nb2m_won_draw = 0;
  i64_t nb2m_lost_draw = 0;

  for (i64_t ipos = 0; ipos < NPOS; ipos++)
  {
    if ((ipos % 10000) == 0) PRINTF("ipos=%lld\n", ipos);

    int nwhite = randull(0) % (NEGTB - 1) + 1;

    HARDBUG(nwhite >= NEGTB)

    int nblack = NEGTB - nwhite;

    HARDBUG(nblack <= 0)

    HARDBUG((nwhite + nblack) != NEGTB)

    char s[MY_LINE_MAX];

    if ((randull(0) % 2) == 0)
      s[0] = 'w';
    else
      s[0] = 'b';

    while(TRUE)
    {
      for (int i = 1; i <= 50; ++i) s[i] = *nn;
      for (int i = 1; i <= nwhite; ++i)
      {
        if ((randull(0) % 2) == 0)
        {
          int j;
          while(TRUE)
          {
            j = randull(0) % 50 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *wX;
        }
        else
        {
          int j;
          while(TRUE)
          {
            j = randull(0) % 45 + 6;
            if (s[j] == *nn) break;
          }
          s[j] = *wO;
        }
      }
  
      for (int i = 1; i <= nblack; ++i)
      {
        if ((randull(0) % 2) == 0)
        {
          int j;
          while(TRUE)
          {
            j = randull(0) % 50 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *bX;
        }
        else
        {
          int j;
          while(TRUE)
          {
            j = randull(0) % 45 + 1;
            if (s[j] == *nn) break;
          }
          s[j] = *bO;
        }
      }
      s[51] = '\0';

      string2board(s, iboard);
      if (!white_can_capture(with, TRUE) and
          !black_can_capture(with, TRUE)) break;
    }

    if (IS_WHITE(with->board_colour2move))
    {
      int mate = read_endgame(with, WHITE_BIT, FALSE);

      HARDBUG(mate == ENDGAME_UNKNOWN)

      int score = return_white_score(with);
      if (mate == INVALID)
      {
        if (score > delta_max)
        {
          delta_max = score;

          print_board(with->board_id);

          PRINTF("delta_max=%d\n", delta_max);

          delta_white[ipos] = score;
        }
      }
      else 
      {
        if ((mate > 0) and (score == 0)) nw2m_won_draw++;
        if ((mate <= 0) and (score == 0)) nw2m_lost_draw++;
      }
    }
    else 
    {
      int mate = read_endgame(with, BLACK_BIT, FALSE);

      HARDBUG(mate == ENDGAME_UNKNOWN)

      int score = return_black_score(with);
      if (mate == INVALID)
      {
        delta_black[ipos] = score;
      }
      else
      {
        if ((mate > 0) and (score == 0)) nb2m_won_draw++;
        if ((mate <= 0) and (score == 0)) nb2m_lost_draw++;
      }
    }
  }

  heap_sort_double(NPOS, delta_white);
  heap_sort_double(NPOS, delta_black);

  PRINTF("delta percentiles\n");

  for (int ipercentile = 1; ipercentile <= 99; ++ipercentile)
  {
    i64_t ipos = round(ipercentile / 100.0 * NPOS);

    PRINTF("ipercentile=%d delta_white=%.6f delta_black=%.6f\n",
      ipercentile, delta_white[ipos], delta_black[ipos]);
  }

  PRINTF("nw2m_won_draw=%lld\n", nw2m_won_draw);
  PRINTF("nw2m_lost_draw=%lld\n", nw2m_lost_draw);
  PRINTF("nb2m_won_draw=%lld\n", nb2m_won_draw);
  PRINTF("nb2m_lost_draw=%lld\n", nb2m_lost_draw);
}


