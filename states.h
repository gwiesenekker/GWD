//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef StatesH
#define StatesH

typedef struct state
{
  //generic properties and methods

  int object_id;

  //specific properties

  cJSON *cjson_object;

  char string[MY_LINE_MAX];

  //specific methods

  pter_t printf_state;

  void (*set_state)(struct state *, char *);
  void (*set_event)(struct state *, char *);
  void (*set_date)(struct state *, char *);
  void (*set_white)(struct state *, char *);
  void (*set_black)(struct state *, char *);
  void (*set_result)(struct state *, char *);
  void (*set_starting_position)(struct state *, char *);
  void (*push_move)(struct state *, char *, char *);
  int (*pop_move)(struct state *);
  void (*set_depth)(struct state *, int);
  void (*set_time)(struct state *, int);
  void (*save)(struct state *, char *);
  void (*save2pdn)(struct state *, char *);

  char *(*get_state)(struct state *);
  char *(*get_event)(struct state *);
  char *(*get_date)(struct state *);
  char *(*get_white)(struct state *);
  char *(*get_black)(struct state *);
  char *(*get_result)(struct state *);
  char *(*get_starting_position)(struct state *);
  cJSON *(*get_moves)(struct state *);
  int (*get_depth)(struct state *);
  int (*get_time)(struct state *);
  void (*load)(struct state *, char *);
} state_t;

//alphabetical

//states.c

extern class_t *state_class;

void init_state_class(void);
void test_state_class(void);

#endif

