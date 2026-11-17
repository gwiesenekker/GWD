// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef StatesH
#define StatesH

typedef struct game_state
{
  cJSON *cjson_object;

  bstring bstate;

  void (*printf_game_state)(struct game_state *);
  void (*set_game_state)(struct game_state *, const char *);
  void (*set_event)(struct game_state *, const char *);
  void (*set_date)(struct game_state *, const char *);
  void (*set_white)(struct game_state *, const char *);
  void (*set_black)(struct game_state *, const char *);
  void (*set_result)(struct game_state *, const char *);
  void (*set_starting_position)(struct game_state *, const char *);
  void (*push_move)(struct game_state *, const char *, const char *);
  int (*pop_move)(struct game_state *);
  void (*clear_moves)(struct game_state *);
  void (*set_depth)(struct game_state *, int);
  void (*set_time)(struct game_state *, int);
  void (*save)(struct game_state *, const char *);
  void (*save2pdn)(struct game_state *, const char *);

  char *(*get_game_state)(struct game_state *);
  char *(*get_event)(const struct game_state *);
  char *(*get_date)(const struct game_state *);
  char *(*get_white)(const struct game_state *);
  char *(*get_black)(const struct game_state *);
  char *(*get_result)(const struct game_state *);
  char *(*get_starting_position)(const struct game_state *);
  cJSON *(*get_moves)(const struct game_state *);
  int (*get_depth)(const struct game_state *);
  int (*get_time)(const struct game_state *);
  void (*load)(struct game_state *, const char *);
} game_state_t;

typedef struct
{
  size_t S_size;
  int S_nstates;
  void **S_states;
  int S_icurrent;
  void *S_current;
} state_t;

void construct_game_state(game_state_t *);
void destroy_game_state(game_state_t *);

void *construct_state(state_t *, size_t, int);
void *push_state(state_t *);
void *pop_state(state_t *);
void *reset_state(state_t *);

void test_states(void);

#endif
