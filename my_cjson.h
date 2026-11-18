// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef My_cjsonH
#define My_cjsonH

#include "bstrlib.h"
#include "cJSON.h"

#define CJSON_PRAGMA_ID "pragma"
#define CJSON_PARAMETERS_ID "parameters"
#define CJSON_SHAPE_ID "shape"
#define CJSON_NETWORK2MATERIAL_SCORE_ID "network2material_score"
#define CJSON_MATERIALS_ID "materials"
#define CJSON_MATERIAL_NAME_ID "name"
#define CJSON_ARCHITECTURE_PATTERNS_ID "architecture_patterns"
#define CJSON_ARCHITECTURE_MATERIAL_ID "architecture_material"
#define CJSON_ACTIVATION_INPUTS_ID "activation_inputs"
#define CJSON_ACTIVATION_ID "activation"
#define CJSON_ACTIVATION_LAST_ID "activation_last"
#define CJSON_NMAN_MIN_ID "nman_min"
#define CJSON_NMAN_MAX_ID "nman_max"

#define CJSON_EVENT_ID "event"
#define CJSON_DATE_ID "date"
#define CJSON_WHITE_ID "white"
#define CJSON_BLACK_ID "black"
#define CJSON_RESULT_ID "result"
#define CJSON_STARTING_POSITION_ID "FEN"
#define CJSON_TIME_ID "time"
#define CJSON_DEPTH_ID "depth"

#define CJSON_MOVES_ID "moves"
#define CJSON_MOVE_STRING_ID "move_string"
#define CJSON_COMMENT_STRING_ID "comment_string"

#define CJSON_CSV_ID "csv"
#define CJSON_CSV_PATH_ID "path"
#define CJSON_CSV_COLUMNS_ID "columns"

#define CJSON_DIRECTORIES_ID "directories"
#define CJSON_DIRECTORY_NAME_ID "name"
#define CJSON_FILES_ID "files"
#define CJSON_FILE_NAME_ID "name"
#define CJSON_FILE_ROWS_ID "rows"
#define CJSON_FILE_COLS_ID "cols"
#define CJSON_FILE_DATA_ID "data"

typedef struct
{
  cJSON *MCJ_cjson;

  bstring MCJ_bstring;
} my_cjson_t;

void construct_my_cjson(my_cjson_t *arg_with_my_cjson);

void my_cjson_add_int(my_cjson_t *arg_with_my_cjson,
                      const char *arg_id,
                      int arg_int);
void my_cjson_set_int(my_cjson_t *arg_with_my_cjson,
                      const char *arg_id,
                      int arg_int);
int my_cjson_get_int(const my_cjson_t *arg_with_my_cjson, const char *arg_id);

void my_cjson_add_cstring(my_cjson_t *arg_with_my_cjson,
                          const char *arg_id,
                          const char *arg_cstring);
void my_cjson_set_cstring(my_cjson_t *arg_with_my_cjson,
                          const char *arg_id,
                          const char *arg_cstring);
char *my_cjson_get_cstring(const my_cjson_t *arg_with_my_cjson,
                           const char *arg_id);

char *my_cjson2cstring(my_cjson_t *arg_with_my_cjson);

void test_my_cjson(void);

#endif
